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

#include "camerabinmetadata.h"

#include <gst/gst.h>
#include <gst/gstversion.h>

struct QGstreamerMetaDataKeyLookup
{
    QtMultimediaKit::MetaData key;
    const char *token;
};

static const QGstreamerMetaDataKeyLookup qt_gstreamerMetaDataKeys[] =
{
    { QtMultimediaKit::Title, GST_TAG_TITLE },
    //{ QtMultimediaKit::SubTitle, 0 },
    //{ QtMultimediaKit::Author, 0 },
    { QtMultimediaKit::Comment, GST_TAG_COMMENT },
    { QtMultimediaKit::Description, GST_TAG_DESCRIPTION },
    //{ QtMultimediaKit::Category, 0 },
    { QtMultimediaKit::Genre, GST_TAG_GENRE },
    //{ QtMultimediaKit::Year, 0 },
    //{ QtMultimediaKit::UserRating, 0 },

    { QtMultimediaKit::Language, GST_TAG_LANGUAGE_CODE },

    { QtMultimediaKit::Publisher, GST_TAG_ORGANIZATION },
    { QtMultimediaKit::Copyright, GST_TAG_COPYRIGHT },
    //{ QtMultimediaKit::ParentalRating, 0 },
    //{ QtMultimediaKit::RatingOrganisation, 0 },

    // Media
    //{ QtMultimediaKit::Size, 0 },
    //{ QtMultimediaKit::MediaType, 0 },
    { QtMultimediaKit::Duration, GST_TAG_DURATION },

    // Audio
    { QtMultimediaKit::AudioBitRate, GST_TAG_BITRATE },
    { QtMultimediaKit::AudioCodec, GST_TAG_AUDIO_CODEC },
    //{ QtMultimediaKit::ChannelCount, 0 },
    //{ QtMultimediaKit::SampleRate, 0 },

    // Music
    { QtMultimediaKit::AlbumTitle, GST_TAG_ALBUM },
    { QtMultimediaKit::AlbumArtist,  GST_TAG_ARTIST},
    { QtMultimediaKit::ContributingArtist, GST_TAG_PERFORMER },
#if (GST_VERSION_MAJOR >= 0) && (GST_VERSION_MINOR >= 10) && (GST_VERSION_MICRO >= 19)
    { QtMultimediaKit::Composer, GST_TAG_COMPOSER },
#endif
    //{ QtMultimediaKit::Conductor, 0 },
    //{ QtMultimediaKit::Lyrics, 0 },
    //{ QtMultimediaKit::Mood, 0 },
    { QtMultimediaKit::TrackNumber, GST_TAG_TRACK_NUMBER },

    //{ QtMultimediaKit::CoverArtUrlSmall, 0 },
    //{ QtMultimediaKit::CoverArtUrlLarge, 0 },

    // Image/Video
    //{ QtMultimediaKit::Resolution, 0 },
    //{ QtMultimediaKit::PixelAspectRatio, 0 },

    // Video
    //{ QtMultimediaKit::VideoFrameRate, 0 },
    //{ QtMultimediaKit::VideoBitRate, 0 },
    { QtMultimediaKit::VideoCodec, GST_TAG_VIDEO_CODEC },

    //{ QtMultimediaKit::PosterUrl, 0 },

    // Movie
    //{ QtMultimediaKit::ChapterNumber, 0 },
    //{ QtMultimediaKit::Director, 0 },
    { QtMultimediaKit::LeadPerformer, GST_TAG_PERFORMER },
    //{ QtMultimediaKit::Writer, 0 },

    // Photos
    //{ QtMultimediaKit::CameraManufacturer, 0 },
    //{ QtMultimediaKit::CameraModel, 0 },
    //{ QtMultimediaKit::Event, 0 },
    //{ QtMultimediaKit::Subject, 0 }
};

CameraBinMetaData::CameraBinMetaData(QObject *parent)
    :QMetaDataWriterControl(parent)
{
}

QVariant CameraBinMetaData::metaData(QtMultimediaKit::MetaData key) const
{
    static const int count = sizeof(qt_gstreamerMetaDataKeys) / sizeof(QGstreamerMetaDataKeyLookup);

    for (int i = 0; i < count; ++i) {
        if (qt_gstreamerMetaDataKeys[i].key == key) {
            const char *name = qt_gstreamerMetaDataKeys[i].token;

            return m_values.value(QByteArray::fromRawData(name, qstrlen(name)));
        }
    }
    return QVariant();
}

void CameraBinMetaData::setMetaData(QtMultimediaKit::MetaData key, const QVariant &value)
{
    static const int count = sizeof(qt_gstreamerMetaDataKeys) / sizeof(QGstreamerMetaDataKeyLookup);

    for (int i = 0; i < count; ++i) {
        if (qt_gstreamerMetaDataKeys[i].key == key) {
            const char *name = qt_gstreamerMetaDataKeys[i].token;

            m_values.insert(QByteArray::fromRawData(name, qstrlen(name)), value);

            emit QMetaDataWriterControl::metaDataChanged();
            emit metaDataChanged(m_values);

            return;
        }
    }
}

QList<QtMultimediaKit::MetaData> CameraBinMetaData::availableMetaData() const
{
    static QMap<QByteArray, QtMultimediaKit::MetaData> keysMap;
    if (keysMap.isEmpty()) {
        const int count = sizeof(qt_gstreamerMetaDataKeys) / sizeof(QGstreamerMetaDataKeyLookup);
        for (int i = 0; i < count; ++i) {
            keysMap[QByteArray(qt_gstreamerMetaDataKeys[i].token)] = qt_gstreamerMetaDataKeys[i].key;
        }
    }

    QList<QtMultimediaKit::MetaData> res;
    foreach (const QByteArray &key, m_values.keys()) {
        QtMultimediaKit::MetaData tag = keysMap.value(key, QtMultimediaKit::MetaData(-1));
        if (tag != -1)
            res.append(tag);
    }

    return res;
}

QVariant CameraBinMetaData::extendedMetaData(QString const &name) const
{
    return m_values.value(name.toLatin1());
}

void CameraBinMetaData::setExtendedMetaData(QString const &name, QVariant const &value)
{
    m_values.insert(name.toLatin1(), value);
    emit QMetaDataWriterControl::metaDataChanged();
    emit metaDataChanged(m_values);
}

QStringList CameraBinMetaData::availableExtendedMetaData() const
{
    QStringList res;
    foreach (const QByteArray &key, m_values.keys())
        res.append(QString(key));

    return res;
}
