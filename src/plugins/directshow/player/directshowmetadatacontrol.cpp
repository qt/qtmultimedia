/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
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
    { QtMultimedia::MetaData::Title, L"Title" },
    { QtMultimedia::MetaData::SubTitle, L"WM/SubTitle" },
    { QtMultimedia::MetaData::Author, L"Author" },
    { QtMultimedia::MetaData::Comment, L"Comment" },
    { QtMultimedia::MetaData::Description, L"Description" },
    { QtMultimedia::MetaData::Category, L"WM/Category" },
    { QtMultimedia::MetaData::Genre, L"WM/Genre" },
    //{ QtMultimedia::MetaData::Date, 0 },
    { QtMultimedia::MetaData::Year, L"WM/Year" },
    { QtMultimedia::MetaData::UserRating, L"UserRating" },
    //{ QtMultimedia::MetaData::MetaDatawords, 0 },
    { QtMultimedia::MetaData::Language, L"Language" },
    { QtMultimedia::MetaData::Publisher, L"WM/Publisher" },
    { QtMultimedia::MetaData::Copyright, L"Copyright" },
    { QtMultimedia::MetaData::ParentalRating, L"ParentalRating" },
    //{ QtMultimedia::MetaData::RatingOrganisation, L"RatingOrganisation" },

    // Media
    { QtMultimedia::MetaData::Size, L"FileSize" },
    { QtMultimedia::MetaData::MediaType, L"MediaType" },
    { QtMultimedia::MetaData::Duration, L"Duration" },

    // Audio
    { QtMultimedia::MetaData::AudioBitRate, L"AudioBitRate" },
    { QtMultimedia::MetaData::AudioCodec, L"AudioCodec" },
    { QtMultimedia::MetaData::ChannelCount, L"ChannelCount" },
    { QtMultimedia::MetaData::SampleRate, L"Frequency" },

    // Music
    { QtMultimedia::MetaData::AlbumTitle, L"WM/AlbumTitle" },
    { QtMultimedia::MetaData::AlbumArtist, L"WM/AlbumArtist" },
    { QtMultimedia::MetaData::ContributingArtist, L"Author" },
    { QtMultimedia::MetaData::Composer, L"WM/Composer" },
    { QtMultimedia::MetaData::Conductor, L"WM/Conductor" },
    { QtMultimedia::MetaData::Lyrics, L"WM/Lyrics" },
    { QtMultimedia::MetaData::Mood, L"WM/Mood" },
    { QtMultimedia::MetaData::TrackNumber, L"WM/TrackNumber" },
    //{ QtMultimedia::MetaData::TrackCount, 0 },
    //{ QtMultimedia::MetaData::CoverArtUriSmall, 0 },
    //{ QtMultimedia::MetaData::CoverArtUriLarge, 0 },

    // Image/Video
    //{ QtMultimedia::MetaData::Resolution, 0 },
    //{ QtMultimedia::MetaData::PixelAspectRatio, 0 },

    // Video
    //{ QtMultimedia::MetaData::FrameRate, 0 },
    { QtMultimedia::MetaData::VideoBitRate, L"VideoBitRate" },
    { QtMultimedia::MetaData::VideoCodec, L"VideoCodec" },

    //{ QtMultimedia::MetaData::PosterUri, 0 },

    // Movie
    { QtMultimedia::MetaData::ChapterNumber, L"ChapterNumber" },
    { QtMultimedia::MetaData::Director, L"WM/Director" },
    { QtMultimedia::MetaData::LeadPerformer, L"LeadPerformer" },
    { QtMultimedia::MetaData::Writer, L"WM/Writer" },

    // Photos
    { QtMultimedia::MetaData::CameraManufacturer, L"CameraManufacturer" },
    { QtMultimedia::MetaData::CameraModel, L"CameraModel" },
    { QtMultimedia::MetaData::Event, L"Event" },
    { QtMultimedia::MetaData::Subject, L"Subject" }
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

        if (key == QtMultimedia::MetaData::Author)
            m_content->get_AuthorName(&string);
        else if (key == QtMultimedia::MetaData::Title)
            m_content->get_Title(&string);
        else if (key == QtMultimedia::MetaData::ParentalRating)
            m_content->get_Rating(&string);
        else if (key == QtMultimedia::MetaData::Description)
            m_content->get_Description(&string);
        else if (key == QtMultimedia::MetaData::Copyright)
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
