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

#include "camerabinmetadata.h"

#include <QtMultimedia/qmediametadata.h>

#include <gst/gst.h>
#include <gst/gstversion.h>

#include <QDebug>

QT_BEGIN_NAMESPACE

struct QGstreamerMetaDataKeyLookup
{
    QString key;
    const char *token;
    QVariant::Type type;
};

static QVariant fromGStreamerOrientation(const QVariant &value)
{
    // Note gstreamer tokens either describe the counter clockwise rotation of the
    // image or the clockwise transform to apply to correct the image.  The orientation
    // value returned is the clockwise rotation of the image.
    const QString token = value.toString();
    if (token == QStringLiteral("rotate-90"))
        return 270;
    else if (token == QStringLiteral("rotate-180"))
        return 180;
    else if (token == QStringLiteral("rotate-270"))
        return 90;
    else
        return 0;
}

static QVariant toGStreamerOrientation(const QVariant &value)
{
    switch (value.toInt()) {
    case 90:
        return QStringLiteral("rotate-270");
    case 180:
        return QStringLiteral("rotate-180");
    case 270:
        return QStringLiteral("rotate-90");
    default:
        return QStringLiteral("rotate-0");
    }
}

static const QGstreamerMetaDataKeyLookup qt_gstreamerMetaDataKeys[] =
{
    { QMediaMetaData::Title, GST_TAG_TITLE, QVariant::String },
    //{ QMediaMetaData::SubTitle, 0, QVariant::String },
    //{ QMediaMetaData::Author, 0, QVariant::String },
    { QMediaMetaData::Comment, GST_TAG_COMMENT, QVariant::String },
    { QMediaMetaData::Date, GST_TAG_DATE_TIME, QVariant::DateTime },
    { QMediaMetaData::Description, GST_TAG_DESCRIPTION, QVariant::String },
    //{ QMediaMetaData::Category, 0, QVariant::String },
    { QMediaMetaData::Genre, GST_TAG_GENRE, QVariant::String },
    //{ QMediaMetaData::Year, 0, QVariant::Int },
    //{ QMediaMetaData::UserRating, , QVariant::Int },

    { QMediaMetaData::Language, GST_TAG_LANGUAGE_CODE, QVariant::String },

    { QMediaMetaData::Publisher, GST_TAG_ORGANIZATION, QVariant::String },
    { QMediaMetaData::Copyright, GST_TAG_COPYRIGHT, QVariant::String },
    //{ QMediaMetaData::ParentalRating, 0, QVariant::String },
    //{ QMediaMetaData::RatingOrganisation, 0, QVariant::String },

    // Media
    //{ QMediaMetaData::Size, 0, QVariant::Int },
    //{ QMediaMetaData::MediaType, 0, QVariant::String },
    { QMediaMetaData::Duration, GST_TAG_DURATION, QVariant::Int },

    // Audio
    { QMediaMetaData::AudioBitRate, GST_TAG_BITRATE, QVariant::Int },
    { QMediaMetaData::AudioCodec, GST_TAG_AUDIO_CODEC, QVariant::String },
    //{ QMediaMetaData::ChannelCount, 0, QVariant::Int },
    //{ QMediaMetaData::SampleRate, 0, QVariant::Int },

    // Music
    { QMediaMetaData::AlbumTitle, GST_TAG_ALBUM, QVariant::String },
    { QMediaMetaData::AlbumArtist,  GST_TAG_ARTIST, QVariant::String},
    { QMediaMetaData::ContributingArtist, GST_TAG_PERFORMER, QVariant::String },
#if (GST_VERSION_MAJOR >= 0) && (GST_VERSION_MINOR >= 10) && (GST_VERSION_MICRO >= 19)
    { QMediaMetaData::Composer, GST_TAG_COMPOSER, QVariant::String },
#endif
    //{ QMediaMetaData::Conductor, 0, QVariant::String },
    //{ QMediaMetaData::Lyrics, 0, QVariant::String },
    //{ QMediaMetaData::Mood, 0, QVariant::String },
    { QMediaMetaData::TrackNumber, GST_TAG_TRACK_NUMBER, QVariant::Int },

    //{ QMediaMetaData::CoverArtUrlSmall, 0, QVariant::String },
    //{ QMediaMetaData::CoverArtUrlLarge, 0, QVariant::String },

    // Image/Video
    //{ QMediaMetaData::Resolution, 0, QVariant::Size },
    //{ QMediaMetaData::PixelAspectRatio, 0, QVariant::Size },

    // Video
    //{ QMediaMetaData::VideoFrameRate, 0, QVariant::String },
    //{ QMediaMetaData::VideoBitRate, 0, QVariant::Double },
    { QMediaMetaData::VideoCodec, GST_TAG_VIDEO_CODEC, QVariant::String },

    //{ QMediaMetaData::PosterUrl, 0, QVariant::String },

    // Movie
    //{ QMediaMetaData::ChapterNumber, 0, QVariant::Int },
    //{ QMediaMetaData::Director, 0, QVariant::String },
    { QMediaMetaData::LeadPerformer, GST_TAG_PERFORMER, QVariant::String },
    //{ QMediaMetaData::Writer, 0, QVariant::String },

#if (GST_VERSION_MAJOR >= 0) && (GST_VERSION_MINOR >= 10) && (GST_VERSION_MICRO >= 30)
    // Photos
    { QMediaMetaData::CameraManufacturer, GST_TAG_DEVICE_MANUFACTURER, QVariant::String },
    { QMediaMetaData::CameraModel, GST_TAG_DEVICE_MODEL, QVariant::String },
    //{ QMediaMetaData::Event, 0, QVariant::String },
    //{ QMediaMetaData::Subject, 0, QVariant::String },

    { QMediaMetaData::Orientation, GST_TAG_IMAGE_ORIENTATION, QVariant::String },

    // GPS
    { QMediaMetaData::GPSLatitude, GST_TAG_GEO_LOCATION_LATITUDE, QVariant::Double },
    { QMediaMetaData::GPSLongitude, GST_TAG_GEO_LOCATION_LONGITUDE, QVariant::Double },
    { QMediaMetaData::GPSAltitude, GST_TAG_GEO_LOCATION_ELEVATION, QVariant::Double },
    { QMediaMetaData::GPSTrack, GST_TAG_GEO_LOCATION_MOVEMENT_DIRECTION, QVariant::Double },
    { QMediaMetaData::GPSSpeed, GST_TAG_GEO_LOCATION_MOVEMENT_SPEED, QVariant::Double },
    { QMediaMetaData::GPSImgDirection, GST_TAG_GEO_LOCATION_CAPTURE_DIRECTION, QVariant::Double }
#endif
};

CameraBinMetaData::CameraBinMetaData(QObject *parent)
    :QMetaDataWriterControl(parent)
{
}

QVariant CameraBinMetaData::metaData(const QString &key) const
{
    if (key == QMediaMetaData::Orientation) {
        return fromGStreamerOrientation(m_values.value(QByteArray(GST_TAG_IMAGE_ORIENTATION)));
    } else if (key == QMediaMetaData::GPSSpeed) {
        const double metersPerSec = m_values.value(QByteArray(GST_TAG_GEO_LOCATION_MOVEMENT_SPEED)).toDouble();
        return (metersPerSec * 3600) / 1000;
    }

    static const int count = sizeof(qt_gstreamerMetaDataKeys) / sizeof(QGstreamerMetaDataKeyLookup);

    for (int i = 0; i < count; ++i) {
        if (qt_gstreamerMetaDataKeys[i].key == key) {
            const char *name = qt_gstreamerMetaDataKeys[i].token;

            return m_values.value(QByteArray::fromRawData(name, qstrlen(name)));
        }
    }
    return QVariant();
}

void CameraBinMetaData::setMetaData(const QString &key, const QVariant &value)
{
    QVariant correctedValue = value;
    if (value.isValid()) {
        if (key == QMediaMetaData::Orientation) {
            correctedValue = toGStreamerOrientation(value);
        } else if (key == QMediaMetaData::GPSSpeed) {
            // kilometers per hour to meters per second.
            correctedValue = (value.toDouble() * 1000) / 3600;
        }
    }

    static const int count = sizeof(qt_gstreamerMetaDataKeys) / sizeof(QGstreamerMetaDataKeyLookup);

    for (int i = 0; i < count; ++i) {
        if (qt_gstreamerMetaDataKeys[i].key == key) {
            const char *name = qt_gstreamerMetaDataKeys[i].token;

            if (correctedValue.isValid()) {
                correctedValue.convert(qt_gstreamerMetaDataKeys[i].type);
                m_values.insert(QByteArray::fromRawData(name, qstrlen(name)), correctedValue);
            } else {
                m_values.remove(QByteArray::fromRawData(name, qstrlen(name)));
            }

            emit QMetaDataWriterControl::metaDataChanged();
            emit metaDataChanged(m_values);

            return;
        }
    }
}

QStringList CameraBinMetaData::availableMetaData() const
{
    static QMap<QByteArray, QString> keysMap;
    if (keysMap.isEmpty()) {
        const int count = sizeof(qt_gstreamerMetaDataKeys) / sizeof(QGstreamerMetaDataKeyLookup);
        for (int i = 0; i < count; ++i) {
            keysMap[QByteArray(qt_gstreamerMetaDataKeys[i].token)] = qt_gstreamerMetaDataKeys[i].key;
        }
    }

    QStringList res;
    foreach (const QByteArray &key, m_values.keys()) {
        QString tag = keysMap.value(key);
        if (!tag.isEmpty())
            res.append(tag);
    }

    return res;
}

QT_END_NAMESPACE
