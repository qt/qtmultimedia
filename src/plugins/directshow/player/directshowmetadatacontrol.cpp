/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Mobility Components.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
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
        QtMultimediaKit::MetaData key;
        const wchar_t *token;
    };
}

static const QWMMetaDataKeyLookup qt_wmMetaDataKeys[] =
{
    { QtMultimediaKit::Title, L"Title" },
    { QtMultimediaKit::SubTitle, L"WM/SubTitle" },
    { QtMultimediaKit::Author, L"Author" },
    { QtMultimediaKit::Comment, L"Comment" },
    { QtMultimediaKit::Description, L"Description" },
    { QtMultimediaKit::Category, L"WM/Category" },
    { QtMultimediaKit::Genre, L"WM/Genre" },
    //{ QtMultimediaKit::Date, 0 },
    { QtMultimediaKit::Year, L"WM/Year" },
    { QtMultimediaKit::UserRating, L"UserRating" },
    //{ QtMultimediaKit::MetaDatawords, 0 },
    { QtMultimediaKit::Language, L"Language" },
    { QtMultimediaKit::Publisher, L"WM/Publisher" },
    { QtMultimediaKit::Copyright, L"Copyright" },
    { QtMultimediaKit::ParentalRating, L"ParentalRating" },
    { QtMultimediaKit::RatingOrganisation, L"RatingOrganisation" },

    // Media
    { QtMultimediaKit::Size, L"FileSize" },
    { QtMultimediaKit::MediaType, L"MediaType" },
    { QtMultimediaKit::Duration, L"Duration" },

    // Audio
    { QtMultimediaKit::AudioBitRate, L"AudioBitRate" },
    { QtMultimediaKit::AudioCodec, L"AudioCodec" },
    { QtMultimediaKit::ChannelCount, L"ChannelCount" },
    { QtMultimediaKit::SampleRate, L"Frequency" },

    // Music
    { QtMultimediaKit::AlbumTitle, L"WM/AlbumTitle" },
    { QtMultimediaKit::AlbumArtist, L"WM/AlbumArtist" },
    { QtMultimediaKit::ContributingArtist, L"Author" },
    { QtMultimediaKit::Composer, L"WM/Composer" },
    { QtMultimediaKit::Conductor, L"WM/Conductor" },
    { QtMultimediaKit::Lyrics, L"WM/Lyrics" },
    { QtMultimediaKit::Mood, L"WM/Mood" },
    { QtMultimediaKit::TrackNumber, L"WM/TrackNumber" },
    //{ QtMultimediaKit::TrackCount, 0 },
    //{ QtMultimediaKit::CoverArtUriSmall, 0 },
    //{ QtMultimediaKit::CoverArtUriLarge, 0 },

    // Image/Video
    //{ QtMultimediaKit::Resolution, 0 },
    //{ QtMultimediaKit::PixelAspectRatio, 0 },

    // Video
    //{ QtMultimediaKit::FrameRate, 0 },
    { QtMultimediaKit::VideoBitRate, L"VideoBitRate" },
    { QtMultimediaKit::VideoCodec, L"VideoCodec" },

    //{ QtMultimediaKit::PosterUri, 0 },

    // Movie
    { QtMultimediaKit::ChapterNumber, L"ChapterNumber" },
    { QtMultimediaKit::Director, L"WM/Director" },
    { QtMultimediaKit::LeadPerformer, L"LeadPerformer" },
    { QtMultimediaKit::Writer, L"WM/Writer" },

    // Photos
    { QtMultimediaKit::CameraManufacturer, L"CameraManufacturer" },
    { QtMultimediaKit::CameraModel, L"CameraModel" },
    { QtMultimediaKit::Event, L"Event" },
    { QtMultimediaKit::Subject, L"Subject" }
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

QVariant DirectShowMetaDataControl::metaData(QtMultimediaKit::MetaData key) const
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

        switch (key) {
        case QtMultimediaKit::Author:
            m_content->get_AuthorName(&string);
            break;
        case QtMultimediaKit::Title:
            m_content->get_Title(&string);
            break;
        case QtMultimediaKit::ParentalRating:
            m_content->get_Rating(&string);
            break;
        case QtMultimediaKit::Description:
            m_content->get_Description(&string);
            break;
        case QtMultimediaKit::Copyright:
            m_content->get_Copyright(&string);
            break;
        default:
            break;
        }

        if (string) {
            value = QString::fromUtf16(reinterpret_cast<ushort *>(string), ::SysStringLen(string));

            ::SysFreeString(string);
        }
    }
    return value;
}

QList<QtMultimediaKit::MetaData> DirectShowMetaDataControl::availableMetaData() const
{
    return QList<QtMultimediaKit::MetaData>();
}

QVariant DirectShowMetaDataControl::extendedMetaData(const QString &) const
{
    return QVariant();
}

QStringList DirectShowMetaDataControl::availableExtendedMetaData() const
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
