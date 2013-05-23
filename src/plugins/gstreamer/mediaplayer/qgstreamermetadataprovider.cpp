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

#include "qgstreamermetadataprovider.h"
#include "qgstreamerplayersession.h"
#include <QDebug>
#include <QtMultimedia/qmediametadata.h>

#include <gst/gstversion.h>

QT_BEGIN_NAMESPACE

struct QGstreamerMetaDataKeyLookup
{
    QString key;
    const char *token;
};

static const QGstreamerMetaDataKeyLookup qt_gstreamerMetaDataKeys[] =
{
    { QMediaMetaData::Title, GST_TAG_TITLE },
    //{ QMediaMetaData::SubTitle, 0 },
    //{ QMediaMetaData::Author, 0 },
    { QMediaMetaData::Comment, GST_TAG_COMMENT },
    { QMediaMetaData::Description, GST_TAG_DESCRIPTION },
    //{ QMediaMetaData::Category, 0 },
    { QMediaMetaData::Genre, GST_TAG_GENRE },
    { QMediaMetaData::Year, "year" },
    //{ QMediaMetaData::UserRating, 0 },

    { QMediaMetaData::Language, GST_TAG_LANGUAGE_CODE },

    { QMediaMetaData::Publisher, GST_TAG_ORGANIZATION },
    { QMediaMetaData::Copyright, GST_TAG_COPYRIGHT },
    //{ QMediaMetaData::ParentalRating, 0 },
    //{ QMediaMetaData::RatingOrganisation, 0 },

    // Media
    //{ QMediaMetaData::Size, 0 },
    //{ QMediaMetaData::MediaType, 0 },
    { QMediaMetaData::Duration, GST_TAG_DURATION },

    // Audio
    { QMediaMetaData::AudioBitRate, GST_TAG_BITRATE },
    { QMediaMetaData::AudioCodec, GST_TAG_AUDIO_CODEC },
    //{ QMediaMetaData::ChannelCount, 0 },
    //{ QMediaMetaData::SampleRate, 0 },

    // Music
    { QMediaMetaData::AlbumTitle, GST_TAG_ALBUM },
    { QMediaMetaData::AlbumArtist,  GST_TAG_ARTIST},
    { QMediaMetaData::ContributingArtist, GST_TAG_PERFORMER },
#if (GST_VERSION_MAJOR >= 0) && (GST_VERSION_MINOR >= 10) && (GST_VERSION_MICRO >= 19)
    { QMediaMetaData::Composer, GST_TAG_COMPOSER },
#endif
    //{ QMediaMetaData::Conductor, 0 },
    //{ QMediaMetaData::Lyrics, 0 },
    //{ QMediaMetaData::Mood, 0 },
    { QMediaMetaData::TrackNumber, GST_TAG_TRACK_NUMBER },

    //{ QMediaMetaData::CoverArtUrlSmall, 0 },
    //{ QMediaMetaData::CoverArtUrlLarge, 0 },

    // Image/Video
    { QMediaMetaData::Resolution, "resolution" },
    { QMediaMetaData::PixelAspectRatio, "pixel-aspect-ratio" },

    // Video
    //{ QMediaMetaData::VideoFrameRate, 0 },
    //{ QMediaMetaData::VideoBitRate, 0 },
    { QMediaMetaData::VideoCodec, GST_TAG_VIDEO_CODEC },

    //{ QMediaMetaData::PosterUrl, 0 },

    // Movie
    //{ QMediaMetaData::ChapterNumber, 0 },
    //{ QMediaMetaData::Director, 0 },
    { QMediaMetaData::LeadPerformer, GST_TAG_PERFORMER },
    //{ QMediaMetaData::Writer, 0 },

    // Photos
    //{ QMediaMetaData::CameraManufacturer, 0 },
    //{ QMediaMetaData::CameraModel, 0 },
    //{ QMediaMetaData::Event, 0 },
    //{ QMediaMetaData::Subject, 0 }
};

QGstreamerMetaDataProvider::QGstreamerMetaDataProvider(QGstreamerPlayerSession *session, QObject *parent)
    :QMetaDataReaderControl(parent), m_session(session)
{
    connect(m_session, SIGNAL(tagsChanged()), SLOT(updateTags()));

    const int count = sizeof(qt_gstreamerMetaDataKeys) / sizeof(QGstreamerMetaDataKeyLookup);
    for (int i = 0; i < count; ++i) {
        m_keysMap[QByteArray(qt_gstreamerMetaDataKeys[i].token)] = qt_gstreamerMetaDataKeys[i].key;
    }
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
    return m_tags.value(key);
}

QStringList QGstreamerMetaDataProvider::availableMetaData() const
{
    return m_tags.keys();
}

void QGstreamerMetaDataProvider::updateTags()
{
    QVariantMap oldTags = m_tags;
    m_tags.clear();

    QSet<QString> allTags = QSet<QString>::fromList(m_tags.keys());

    QMapIterator<QByteArray ,QVariant> i(m_session->tags());
    while (i.hasNext()) {
         i.next();
         //use gstreamer native keys for elements not in m_keysMap
         QString key = m_keysMap.value(i.key(), i.key());
         m_tags[key] = i.value();
         allTags.insert(key);
    }

    bool changed = false;
    foreach (const QString &key, allTags) {
        const QVariant value = m_tags.value(key);
        if (value != oldTags.value(key)) {
            changed = true;
            emit metaDataChanged(key, value);
        }
    }

    if (changed)
        emit metaDataChanged();
}

QT_END_NAMESPACE
