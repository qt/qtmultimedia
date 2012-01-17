/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include "camerabinmetadata.h"

#include <gst/gst.h>
#include <gst/gstversion.h>

struct QGstreamerMetaDataKeyLookup
{
    QtMultimedia::MetaData key;
    const char *token;
};

static const QGstreamerMetaDataKeyLookup qt_gstreamerMetaDataKeys[] =
{
    { QtMultimedia::Title, GST_TAG_TITLE },
    //{ QtMultimedia::SubTitle, 0 },
    //{ QtMultimedia::Author, 0 },
    { QtMultimedia::Comment, GST_TAG_COMMENT },
    { QtMultimedia::Description, GST_TAG_DESCRIPTION },
    //{ QtMultimedia::Category, 0 },
    { QtMultimedia::Genre, GST_TAG_GENRE },
    //{ QtMultimedia::Year, 0 },
    //{ QtMultimedia::UserRating, 0 },

    { QtMultimedia::Language, GST_TAG_LANGUAGE_CODE },

    { QtMultimedia::Publisher, GST_TAG_ORGANIZATION },
    { QtMultimedia::Copyright, GST_TAG_COPYRIGHT },
    //{ QtMultimedia::ParentalRating, 0 },
    //{ QtMultimedia::RatingOrganization, 0 },

    // Media
    //{ QtMultimedia::Size, 0 },
    //{ QtMultimedia::MediaType, 0 },
    { QtMultimedia::Duration, GST_TAG_DURATION },

    // Audio
    { QtMultimedia::AudioBitRate, GST_TAG_BITRATE },
    { QtMultimedia::AudioCodec, GST_TAG_AUDIO_CODEC },
    //{ QtMultimedia::ChannelCount, 0 },
    //{ QtMultimedia::SampleRate, 0 },

    // Music
    { QtMultimedia::AlbumTitle, GST_TAG_ALBUM },
    { QtMultimedia::AlbumArtist,  GST_TAG_ARTIST},
    { QtMultimedia::ContributingArtist, GST_TAG_PERFORMER },
#if (GST_VERSION_MAJOR >= 0) && (GST_VERSION_MINOR >= 10) && (GST_VERSION_MICRO >= 19)
    { QtMultimedia::Composer, GST_TAG_COMPOSER },
#endif
    //{ QtMultimedia::Conductor, 0 },
    //{ QtMultimedia::Lyrics, 0 },
    //{ QtMultimedia::Mood, 0 },
    { QtMultimedia::TrackNumber, GST_TAG_TRACK_NUMBER },

    //{ QtMultimedia::CoverArtUrlSmall, 0 },
    //{ QtMultimedia::CoverArtUrlLarge, 0 },

    // Image/Video
    //{ QtMultimedia::Resolution, 0 },
    //{ QtMultimedia::PixelAspectRatio, 0 },

    // Video
    //{ QtMultimedia::VideoFrameRate, 0 },
    //{ QtMultimedia::VideoBitRate, 0 },
    { QtMultimedia::VideoCodec, GST_TAG_VIDEO_CODEC },

    //{ QtMultimedia::PosterUrl, 0 },

    // Movie
    //{ QtMultimedia::ChapterNumber, 0 },
    //{ QtMultimedia::Director, 0 },
    { QtMultimedia::LeadPerformer, GST_TAG_PERFORMER },
    //{ QtMultimedia::Writer, 0 },

    // Photos
    //{ QtMultimedia::CameraManufacturer, 0 },
    //{ QtMultimedia::CameraModel, 0 },
    //{ QtMultimedia::Event, 0 },
    //{ QtMultimedia::Subject, 0 }
};

CameraBinMetaData::CameraBinMetaData(QObject *parent)
    :QMetaDataWriterControl(parent)
{
}

QVariant CameraBinMetaData::metaData(QtMultimedia::MetaData key) const
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

void CameraBinMetaData::setMetaData(QtMultimedia::MetaData key, const QVariant &value)
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

QList<QtMultimedia::MetaData> CameraBinMetaData::availableMetaData() const
{
    static QMap<QByteArray, QtMultimedia::MetaData> keysMap;
    if (keysMap.isEmpty()) {
        const int count = sizeof(qt_gstreamerMetaDataKeys) / sizeof(QGstreamerMetaDataKeyLookup);
        for (int i = 0; i < count; ++i) {
            keysMap[QByteArray(qt_gstreamerMetaDataKeys[i].token)] = qt_gstreamerMetaDataKeys[i].key;
        }
    }

    QList<QtMultimedia::MetaData> res;
    foreach (const QByteArray &key, m_values.keys()) {
        QtMultimedia::MetaData tag = keysMap.value(key, QtMultimedia::MetaData(-1));
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
