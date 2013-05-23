/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qt7backend.h"
#include "qt7playermetadata.h"
#include "qt7playersession.h"
#include <QtCore/qvarlengtharray.h>
#include <QtMultimedia/qmediametadata.h>

#import <QTKit/QTMovie.h>

#ifdef QUICKTIME_C_API_AVAILABLE
    #include <QuickTime/QuickTime.h>
    #undef check // avoid name clash;
#endif

QT_USE_NAMESPACE

QT7PlayerMetaDataControl::QT7PlayerMetaDataControl(QT7PlayerSession *session, QObject *parent)
    :QMetaDataReaderControl(parent), m_session(session)
{
}

QT7PlayerMetaDataControl::~QT7PlayerMetaDataControl()
{
}

bool QT7PlayerMetaDataControl::isMetaDataAvailable() const
{
    return !m_tags.isEmpty();
}

bool QT7PlayerMetaDataControl::isWritable() const
{
    return false;
}

QVariant QT7PlayerMetaDataControl::metaData(const QString &key) const
{
    return m_tags.value(key);
}

QStringList QT7PlayerMetaDataControl::availableMetaData() const
{
    return m_tags.keys();
}

#ifdef QUICKTIME_C_API_AVAILABLE

static QString stripCopyRightSymbol(const QString &key)
{
    return key.right(key.length()-1);
}

static QString convertQuickTimeKeyToUserKey(const QString &key)
{
    if (key == QLatin1String("com.apple.quicktime.displayname"))
        return QLatin1String("nam");
    else if (key == QLatin1String("com.apple.quicktime.album"))
        return QLatin1String("alb");
    else if (key == QLatin1String("com.apple.quicktime.artist"))
        return QLatin1String("ART");
    else
        return QLatin1String("???");
}

static OSStatus readMetaValue(QTMetaDataRef metaDataRef, QTMetaDataItem item, QTPropertyClass propClass,
                              QTPropertyID id, QTPropertyValuePtr *value, ByteCount *size)
{
    QTPropertyValueType type;
    ByteCount propSize;
    UInt32 propFlags;
    OSStatus err = QTMetaDataGetItemPropertyInfo(metaDataRef, item, propClass, id, &type, &propSize, &propFlags);

    if (err == noErr) {
        *value = malloc(propSize);
        if (*value != 0) {
            err = QTMetaDataGetItemProperty(metaDataRef, item, propClass, id, propSize, *value, size);

            if (err == noErr && (type == 'code' || type == 'itsk' || type == 'itlk')) {
                // convert from native endian to big endian
                OSTypePtr pType = (OSTypePtr)*value;
                *pType = EndianU32_NtoB(*pType);
            }
        }
        else
            return -1;
    }

    return err;
}

static UInt32 getMetaType(QTMetaDataRef metaDataRef, QTMetaDataItem item)
{
    QTPropertyValuePtr value = 0;
    ByteCount ignore = 0;
    OSStatus err = readMetaValue(
            metaDataRef, item, kPropertyClass_MetaDataItem, kQTMetaDataItemPropertyID_DataType, &value, &ignore);

    if (err == noErr) {
        UInt32 type = *((UInt32 *) value);
        if (value)
            free(value);
        return type;
    }

    return 0;
}

static QString cFStringToQString(CFStringRef str)
{
    if(!str)
        return QString();
    CFIndex length = CFStringGetLength(str);
    const UniChar *chars = CFStringGetCharactersPtr(str);
    if (chars)
        return QString(reinterpret_cast<const QChar *>(chars), length);

    QVarLengthArray<UniChar> buffer(length);
    CFStringGetCharacters(str, CFRangeMake(0, length), buffer.data());
    return QString(reinterpret_cast<const QChar *>(buffer.constData()), length);
}


static QString getMetaValue(QTMetaDataRef metaDataRef, QTMetaDataItem item, SInt32 id)
{
    QTPropertyValuePtr value = 0;
    ByteCount size = 0;
    OSStatus err = readMetaValue(metaDataRef, item, kPropertyClass_MetaDataItem, id, &value, &size);
    QString string;

    if (err == noErr) {
        UInt32 dataType = getMetaType(metaDataRef, item);
        switch (dataType){
        case kQTMetaDataTypeUTF8:
        case kQTMetaDataTypeMacEncodedText:
            string = cFStringToQString(CFStringCreateWithBytes(0, (UInt8*)value, size, kCFStringEncodingUTF8, false));
            break;
        case kQTMetaDataTypeUTF16BE:
            string = cFStringToQString(CFStringCreateWithBytes(0, (UInt8*)value, size, kCFStringEncodingUTF16BE, false));
            break;
        default:
            break;
        }

        if (value)
            free(value);
    }

    return string;
}


static void readFormattedData(QTMetaDataRef metaDataRef, OSType format, QMultiMap<QString, QString> &result)
{
    QTMetaDataItem item = kQTMetaDataItemUninitialized;
    OSStatus err = QTMetaDataGetNextItem(metaDataRef, format, item, kQTMetaDataKeyFormatWildcard, 0, 0, &item);
    while (err == noErr){
        QString key = getMetaValue(metaDataRef, item, kQTMetaDataItemPropertyID_Key);
        if (format == kQTMetaDataStorageFormatQuickTime)
            key = convertQuickTimeKeyToUserKey(key);
        else
            key = stripCopyRightSymbol(key);

        if (!result.contains(key)){
            QString val = getMetaValue(metaDataRef, item, kQTMetaDataItemPropertyID_Value);
            result.insert(key, val);
        }
        err = QTMetaDataGetNextItem(metaDataRef, format, item, kQTMetaDataKeyFormatWildcard, 0, 0, &item);
    }
}
#endif


void QT7PlayerMetaDataControl::updateTags()
{
    bool wasEmpty = m_tags.isEmpty();
    m_tags.clear();

    QTMovie *movie = (QTMovie*)m_session->movie();

    if (movie) {
        QMultiMap<QString, QString> metaMap;

#ifdef QUICKTIME_C_API_AVAILABLE
        QTMetaDataRef metaDataRef;
        OSStatus err = QTCopyMovieMetaData([movie quickTimeMovie], &metaDataRef);
        if (err == noErr) {
            readFormattedData(metaDataRef, kQTMetaDataStorageFormatUserData, metaMap);
            readFormattedData(metaDataRef, kQTMetaDataStorageFormatQuickTime, metaMap);
            readFormattedData(metaDataRef, kQTMetaDataStorageFormatiTunes, metaMap);
        }
#else
        AutoReleasePool pool;
        NSString *name = [movie attributeForKey:@"QTMovieDisplayNameAttribute"];
        metaMap.insert(QLatin1String("nam"), QString::fromUtf8([name UTF8String]));
#endif // QUICKTIME_C_API_AVAILABLE

        m_tags.insert(QMediaMetaData::AlbumArtist, metaMap.value(QLatin1String("ART")));
        m_tags.insert(QMediaMetaData::AlbumTitle, metaMap.value(QLatin1String("alb")));
        m_tags.insert(QMediaMetaData::Title, metaMap.value(QLatin1String("nam")));
        m_tags.insert(QMediaMetaData::Date, metaMap.value(QLatin1String("day")));
        m_tags.insert(QMediaMetaData::Genre, metaMap.value(QLatin1String("gnre")));
        m_tags.insert(QMediaMetaData::TrackNumber, metaMap.value(QLatin1String("trk")));
        m_tags.insert(QMediaMetaData::Description, metaMap.value(QLatin1String("des")));
    }

    if (!wasEmpty || !m_tags.isEmpty())
        Q_EMIT metaDataChanged();
}

#include "moc_qt7playermetadata.cpp"
