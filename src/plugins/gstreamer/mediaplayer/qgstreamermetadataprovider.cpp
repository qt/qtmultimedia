/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qgstreamermetadataprovider.h"
#include "qgstreamerplayersession.h"
#include <QDebug>

#include <gst/gstversion.h>

QT_BEGIN_NAMESPACE

struct QGstreamerMetaDataKeyLookup
{
    QString key;
    const char *token;
};

static const QGstreamerMetaDataKeyLookup qt_gstreamerMetaDataKeys[] =
{
    { QtMultimedia::MetaData::Title, GST_TAG_TITLE },
    //{ QtMultimedia::MetaData::SubTitle, 0 },
    //{ QtMultimedia::MetaData::Author, 0 },
    { QtMultimedia::MetaData::Comment, GST_TAG_COMMENT },
    { QtMultimedia::MetaData::Description, GST_TAG_DESCRIPTION },
    //{ QtMultimedia::MetaData::Category, 0 },
    { QtMultimedia::MetaData::Genre, GST_TAG_GENRE },
    { QtMultimedia::MetaData::Year, "year" },
    //{ QtMultimedia::MetaData::UserRating, 0 },

    { QtMultimedia::MetaData::Language, GST_TAG_LANGUAGE_CODE },

    { QtMultimedia::MetaData::Publisher, GST_TAG_ORGANIZATION },
    { QtMultimedia::MetaData::Copyright, GST_TAG_COPYRIGHT },
    //{ QtMultimedia::MetaData::ParentalRating, 0 },
    //{ QtMultimedia::MetaData::RatingOrganisation, 0 },

    // Media
    //{ QtMultimedia::MetaData::Size, 0 },
    //{ QtMultimedia::MetaData::MediaType, 0 },
    { QtMultimedia::MetaData::Duration, GST_TAG_DURATION },

    // Audio
    { QtMultimedia::MetaData::AudioBitRate, GST_TAG_BITRATE },
    { QtMultimedia::MetaData::AudioCodec, GST_TAG_AUDIO_CODEC },
    //{ QtMultimedia::MetaData::ChannelCount, 0 },
    //{ QtMultimedia::MetaData::SampleRate, 0 },

    // Music
    { QtMultimedia::MetaData::AlbumTitle, GST_TAG_ALBUM },
    { QtMultimedia::MetaData::AlbumArtist,  GST_TAG_ARTIST},
    { QtMultimedia::MetaData::ContributingArtist, GST_TAG_PERFORMER },
#if (GST_VERSION_MAJOR >= 0) && (GST_VERSION_MINOR >= 10) && (GST_VERSION_MICRO >= 19)
    { QtMultimedia::MetaData::Composer, GST_TAG_COMPOSER },
#endif
    //{ QtMultimedia::MetaData::Conductor, 0 },
    //{ QtMultimedia::MetaData::Lyrics, 0 },
    //{ QtMultimedia::MetaData::Mood, 0 },
    { QtMultimedia::MetaData::TrackNumber, GST_TAG_TRACK_NUMBER },

    //{ QtMultimedia::MetaData::CoverArtUrlSmall, 0 },
    //{ QtMultimedia::MetaData::CoverArtUrlLarge, 0 },

    // Image/Video
    { QtMultimedia::MetaData::Resolution, "resolution" },
    { QtMultimedia::MetaData::PixelAspectRatio, "pixel-aspect-ratio" },

    // Video
    //{ QtMultimedia::MetaData::VideoFrameRate, 0 },
    //{ QtMultimedia::MetaData::VideoBitRate, 0 },
    { QtMultimedia::MetaData::VideoCodec, GST_TAG_VIDEO_CODEC },

    //{ QtMultimedia::MetaData::PosterUrl, 0 },

    // Movie
    //{ QtMultimedia::MetaData::ChapterNumber, 0 },
    //{ QtMultimedia::MetaData::Director, 0 },
    { QtMultimedia::MetaData::LeadPerformer, GST_TAG_PERFORMER },
    //{ QtMultimedia::MetaData::Writer, 0 },

    // Photos
    //{ QtMultimedia::MetaData::CameraManufacturer, 0 },
    //{ QtMultimedia::MetaData::CameraModel, 0 },
    //{ QtMultimedia::MetaData::Event, 0 },
    //{ QtMultimedia::MetaData::Subject, 0 }
};

QGstreamerMetaDataProvider::QGstreamerMetaDataProvider(QGstreamerPlayerSession *session, QObject *parent)
    :QMetaDataReaderControl(parent), m_session(session)
{
    connect(m_session, SIGNAL(tagsChanged()), SLOT(updateTags()));
}

QGstreamerMetaDataProvider::~QGstreamerMetaDataProvider()
{
}

bool QGstreamerMetaDataProvider::isMetaDataAvailable() const
{
    return !m_session->tags().isEmpty();
}

bool QGstreamerMetaDataProvider::isWritable() const
{
    return false;
}

QVariant QGstreamerMetaDataProvider::metaData(const QString &key) const
{
    static const int count = sizeof(qt_gstreamerMetaDataKeys) / sizeof(QGstreamerMetaDataKeyLookup);

    for (int i = 0; i < count; ++i) {
        if (qt_gstreamerMetaDataKeys[i].key == key) {
            return m_session->tags().value(QByteArray(qt_gstreamerMetaDataKeys[i].token));
        }
    }
    return QVariant();
}

QStringList QGstreamerMetaDataProvider::availableMetaData() const
{
    static QMap<QByteArray, QString> keysMap;
    if (keysMap.isEmpty()) {
        const int count = sizeof(qt_gstreamerMetaDataKeys) / sizeof(QGstreamerMetaDataKeyLookup);
        for (int i = 0; i < count; ++i) {
            keysMap[QByteArray(qt_gstreamerMetaDataKeys[i].token)] = qt_gstreamerMetaDataKeys[i].key;
        }
    }

    QStringList res;
    foreach (const QByteArray &key, m_session->tags().keys()) {
        QString tag = keysMap.value(key);
        if (!tag.isEmpty())
            res.append(tag);
    }

    return res;
}

void QGstreamerMetaDataProvider::updateTags()
{
    emit metaDataChanged();
}

QT_END_NAMESPACE
