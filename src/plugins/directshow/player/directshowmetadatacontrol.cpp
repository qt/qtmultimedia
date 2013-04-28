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

#include <dshow.h>
#include <initguid.h>
#include <qnetwork.h>

#include "directshowmetadatacontrol.h"
#include "directshowplayerservice.h"

#include <QtMultimedia/qmediametadata.h>
#include <QtCore/qcoreapplication.h>

#ifndef QT_NO_WMSDK
namespace
{
    struct QWMMetaDataKeyLookup
    {
        QString key;
        const wchar_t *token;
    };
}

static const QWMMetaDataKeyLookup qt_wmMetaDataKeys[] =
{
    { QMediaMetaData::Title, L"Title" },
    { QMediaMetaData::SubTitle, L"WM/SubTitle" },
    { QMediaMetaData::Author, L"Author" },
    { QMediaMetaData::Comment, L"Comment" },
    { QMediaMetaData::Description, L"Description" },
    { QMediaMetaData::Category, L"WM/Category" },
    { QMediaMetaData::Genre, L"WM/Genre" },
    //{ QMediaMetaData::Date, 0 },
    { QMediaMetaData::Year, L"WM/Year" },
    { QMediaMetaData::UserRating, L"UserRating" },
    //{ QMediaMetaData::MetaDatawords, 0 },
    { QMediaMetaData::Language, L"Language" },
    { QMediaMetaData::Publisher, L"WM/Publisher" },
    { QMediaMetaData::Copyright, L"Copyright" },
    { QMediaMetaData::ParentalRating, L"ParentalRating" },
    //{ QMediaMetaData::RatingOrganisation, L"RatingOrganisation" },

    // Media
    { QMediaMetaData::Size, L"FileSize" },
    { QMediaMetaData::MediaType, L"MediaType" },
    { QMediaMetaData::Duration, L"Duration" },

    // Audio
    { QMediaMetaData::AudioBitRate, L"AudioBitRate" },
    { QMediaMetaData::AudioCodec, L"AudioCodec" },
    { QMediaMetaData::ChannelCount, L"ChannelCount" },
    { QMediaMetaData::SampleRate, L"Frequency" },

    // Music
    { QMediaMetaData::AlbumTitle, L"WM/AlbumTitle" },
    { QMediaMetaData::AlbumArtist, L"WM/AlbumArtist" },
    { QMediaMetaData::ContributingArtist, L"Author" },
    { QMediaMetaData::Composer, L"WM/Composer" },
    { QMediaMetaData::Conductor, L"WM/Conductor" },
    { QMediaMetaData::Lyrics, L"WM/Lyrics" },
    { QMediaMetaData::Mood, L"WM/Mood" },
    { QMediaMetaData::TrackNumber, L"WM/TrackNumber" },
    //{ QMediaMetaData::TrackCount, 0 },
    //{ QMediaMetaData::CoverArtUriSmall, 0 },
    //{ QMediaMetaData::CoverArtUriLarge, 0 },

    // Image/Video
    //{ QMediaMetaData::Resolution, 0 },
    //{ QMediaMetaData::PixelAspectRatio, 0 },

    // Video
    //{ QMediaMetaData::FrameRate, 0 },
    { QMediaMetaData::VideoBitRate, L"VideoBitRate" },
    { QMediaMetaData::VideoCodec, L"VideoCodec" },

    //{ QMediaMetaData::PosterUri, 0 },

    // Movie
    { QMediaMetaData::ChapterNumber, L"ChapterNumber" },
    { QMediaMetaData::Director, L"WM/Director" },
    { QMediaMetaData::LeadPerformer, L"LeadPerformer" },
    { QMediaMetaData::Writer, L"WM/Writer" },

    // Photos
    { QMediaMetaData::CameraManufacturer, L"CameraManufacturer" },
    { QMediaMetaData::CameraModel, L"CameraModel" },
    { QMediaMetaData::Event, L"Event" },
    { QMediaMetaData::Subject, L"Subject" }
};

static QVariant getValue(IWMHeaderInfo *header, const wchar_t *key)
{
    WORD streamNumber = 0;
    WMT_ATTR_DATATYPE type = WMT_TYPE_DWORD;
    WORD size = 0;

    if (header->GetAttributeByName(&streamNumber, key, &type, 0, &size) == S_OK) {
        switch (type) {
        case WMT_TYPE_DWORD:
            if (size == sizeof(DWORD)) {
                DWORD word;
                if (header->GetAttributeByName(
                        &streamNumber,
                        key,
                        &type,
                        reinterpret_cast<BYTE *>(&word),
                        &size) == S_OK) {
                    return int(word);
                }
            }
            break;
        case WMT_TYPE_STRING:
            {
                QString string;
                string.resize(size / 2 - 1);

                if (header->GetAttributeByName(
                        &streamNumber,
                        key,
                        &type,
                        reinterpret_cast<BYTE *>(const_cast<ushort *>(string.utf16())),
                        &size) == S_OK) {
                    return string;
                }
            }
            break;
        case WMT_TYPE_BINARY:
            {
                QByteArray bytes;
                bytes.resize(size);
                if (header->GetAttributeByName(
                        &streamNumber,
                        key,
                        &type,
                        reinterpret_cast<BYTE *>(bytes.data()),
                        &size) == S_OK) {
                    return bytes;
                }
            }
            break;
        case WMT_TYPE_BOOL:
            if (size == sizeof(DWORD)) {
                DWORD word;
                if (header->GetAttributeByName(
                        &streamNumber,
                        key,
                        &type,
                        reinterpret_cast<BYTE *>(&word),
                        &size) == S_OK) {
                    return bool(word);
                }
            }
            break;
        case WMT_TYPE_QWORD:
            if (size == sizeof(QWORD)) {
                QWORD word;
                if (header->GetAttributeByName(
                        &streamNumber,
                        key,
                        &type,
                        reinterpret_cast<BYTE *>(&word),
                        &size) == S_OK) {
                    return qint64(word);
                }
            }
            break;
        case WMT_TYPE_WORD:
            if (size == sizeof(WORD)){
                WORD word;
                if (header->GetAttributeByName(
                        &streamNumber,
                        key, 
                        &type,
                        reinterpret_cast<BYTE *>(&word),
                        &size) == S_OK) {
                    return short(word);
                }
            }
            break;
        case WMT_TYPE_GUID:
            if (size == 16) {
            }
            break;
        default:
            break;
        }
    }
    return QVariant();
}
#endif

DirectShowMetaDataControl::DirectShowMetaDataControl(QObject *parent)
    : QMetaDataReaderControl(parent)
    , m_content(0)
#ifndef QT_NO_WMSDK
    , m_headerInfo(0)
#endif
{
}

DirectShowMetaDataControl::~DirectShowMetaDataControl()
{
}

bool DirectShowMetaDataControl::isMetaDataAvailable() const
{
#ifndef QT_NO_WMSDK
    return m_content || m_headerInfo;
#else
    return m_content;
#endif
}

QVariant DirectShowMetaDataControl::metaData(const QString &key) const
{
    QVariant value;

#ifndef QT_NO_WMSDK
    if (m_headerInfo) {
        static const int  count = sizeof(qt_wmMetaDataKeys) / sizeof(QWMMetaDataKeyLookup);
        for (int i = 0; i < count; ++i) {
            if (qt_wmMetaDataKeys[i].key == key) {
                value = getValue(m_headerInfo, qt_wmMetaDataKeys[i].token);
                break;
            }
        }
    }  else if (m_content) {
#else
    if (m_content) {
#endif
        BSTR string = 0;

        if (key == QMediaMetaData::Author)
            m_content->get_AuthorName(&string);
        else if (key == QMediaMetaData::Title)
            m_content->get_Title(&string);
        else if (key == QMediaMetaData::ParentalRating)
            m_content->get_Rating(&string);
        else if (key == QMediaMetaData::Description)
            m_content->get_Description(&string);
        else if (key == QMediaMetaData::Copyright)
            m_content->get_Copyright(&string);

        if (string) {
            value = QString::fromUtf16(reinterpret_cast<ushort *>(string), ::SysStringLen(string));

            ::SysFreeString(string);
        }
    }
    return value;
}

QStringList DirectShowMetaDataControl::availableMetaData() const
{
    return QStringList();
}

void DirectShowMetaDataControl::updateGraph(IFilterGraph2 *graph, IBaseFilter *source)
{
    if (m_content)
        m_content->Release();

    if (!graph || graph->QueryInterface(
            IID_IAMMediaContent, reinterpret_cast<void **>(&m_content)) != S_OK) {
        m_content = 0;
    }

#ifdef QT_NO_WMSDK
    Q_UNUSED(source);
#else
    if (m_headerInfo)
        m_headerInfo->Release();

    m_headerInfo = com_cast<IWMHeaderInfo>(source, IID_IWMHeaderInfo);
#endif
    // DirectShowMediaPlayerService holds a lock at this point so defer emitting signals to a later
    // time.
    QCoreApplication::postEvent(this, new QEvent(QEvent::Type(MetaDataChanged)));
}

void DirectShowMetaDataControl::customEvent(QEvent *event)
{
    if (event->type() == QEvent::Type(MetaDataChanged)) {
        event->accept();

        emit metaDataChanged();
#ifndef QT_NO_WMSDK
        emit metaDataAvailableChanged(m_content || m_headerInfo);
#else
        emit metaDataAvailableChanged(m_content);
#endif
    } else {
        QMetaDataReaderControl::customEvent(event);
    }
}
