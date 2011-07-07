/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Mobility Components.
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
    { QtMultimediaKit::Year, "year" },
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
    { QtMultimediaKit::Resolution, "resolution" },
    { QtMultimediaKit::PixelAspectRatio, "pixel-aspect-ratio" },

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

QVariant QGstreamerMetaDataProvider::metaData(QtMultimediaKit::MetaData key) const
{
    static const int count = sizeof(qt_gstreamerMetaDataKeys) / sizeof(QGstreamerMetaDataKeyLookup);

    for (int i = 0; i < count; ++i) {
        if (qt_gstreamerMetaDataKeys[i].key == key) {
            return m_session->tags().value(QByteArray(qt_gstreamerMetaDataKeys[i].token));
        }
    }
    return QVariant();
}

QList<QtMultimediaKit::MetaData> QGstreamerMetaDataProvider::availableMetaData() const
{
    static QMap<QByteArray, QtMultimediaKit::MetaData> keysMap;
    if (keysMap.isEmpty()) {
        const int count = sizeof(qt_gstreamerMetaDataKeys) / sizeof(QGstreamerMetaDataKeyLookup);
        for (int i = 0; i < count; ++i) {
            keysMap[QByteArray(qt_gstreamerMetaDataKeys[i].token)] = qt_gstreamerMetaDataKeys[i].key;
        }
    }

    QList<QtMultimediaKit::MetaData> res;
    foreach (const QByteArray &key, m_session->tags().keys()) {
        QtMultimediaKit::MetaData tag = keysMap.value(key, QtMultimediaKit::MetaData(-1));
        if (tag != -1)
            res.append(tag);
    }

    return res;
}

QVariant QGstreamerMetaDataProvider::extendedMetaData(const QString &key) const
{
    return m_session->tags().value(key.toLatin1());
}

QStringList QGstreamerMetaDataProvider::availableExtendedMetaData() const
{
    QStringList res;
    foreach (const QByteArray &key, m_session->tags().keys())
        res.append(QString(key));

    return res;
}

void QGstreamerMetaDataProvider::updateTags()
{
    emit metaDataChanged();
}
