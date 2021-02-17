/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QMEDIAMETADATA_H
#define QMEDIAMETADATA_H

#include <QtCore/qpair.h>
#include <QtCore/qvariant.h>
#include <QtCore/qstring.h>
#include <QtCore/qhash.h>
#include <QtMultimedia/qtmultimediaglobal.h>

QT_BEGIN_NAMESPACE

// Class forward declaration required for QDoc bug
class QString;

class Q_MULTIMEDIA_EXPORT QMediaMetaData {
public:
    enum Key {
        Title,
        Author,
        Comment,
        Description,
        Genre,
        Year,
        Date,

        Language,
        Publisher,
        Copyright,
        Url,

        Duration,

        MediaType,
        FileFormat,
        AudioBitRate,
        AudioCodec,
        VideoBitRate,
        VideoCodec,
        VideoFrameRate,

        AlbumTitle,
        AlbumArtist,
        ContributingArtist,
        TrackNumber,
        Composer,

        ThumbnailImage,
        CoverArtImage,
        Orientation,

        Resolution,
        LeadPerformer
    };

    static constexpr int NumMetaData = LeadPerformer + 1;

//    QMetaType typeForKey(Key k);
    QVariant value(Key k) const;
    void insert(Key k, const QVariant &value);
    void remove(Key k) { data.remove(k); }
    QList<Key> keys() const { return data.keys(); }

    QVariant &operator[](Key k) { return data[k]; }
    void clear() { data.clear(); }

    bool isEmpty() const { return data.isEmpty(); }
    QString stringValue(Key k) const;

    static QString metaDataKeyToString(Key k);

protected:
    friend bool operator==(const QMediaMetaData &a, const QMediaMetaData &b)
    { return a.data == b.data; }
    friend bool operator!=(const QMediaMetaData &a, const QMediaMetaData &b)
    { return a.data != b.data; }

    QHash<Key, QVariant> data;

#ifdef Q_QDOC
    // ### For reference right now, remove
    // QDoc does not like macros, so try to keep this in sync :)
    QString Title;
    QString SubTitle;
    QString Author;
    QString Comment;
    QString Description;
    QString Category;
    QString Genre;
    QString Year;
    QString Date;
    QString UserRating;
    QString Keywords;
    QString Language;
    QString Publisher;
    QString Copyright;
    QString ParentalRating;
    QString RatingOrganization;

    // Media
    QString Size;
    QString MediaType;
    QString Duration;

    // Audio
    QString AudioBitRate;
    QString AudioCodec;
    QString AverageLevel;
    QString ChannelCount;
    QString PeakValue;
    QString SampleRate;

    // Music
    QString AlbumTitle;
    QString AlbumArtist;
    QString ContributingArtist;
    QString Composer;
    QString Conductor;
    QString Lyrics;
    QString Mood;
    QString TrackNumber;
    QString TrackCount;

    QString CoverArtUrlSmall;
    QString CoverArtUrlLarge;

    // Image/Video
    QString Resolution;
    QString PixelAspectRatio;

    // Video
    QString VideoFrameRate;
    QString VideoBitRate;
    QString VideoCodec;

    QString PosterUrl;

    // Movie
    QString ChapterNumber;
    QString Director;
    QString LeadPerformer;
    QString Writer;

    // Photos
    QString CameraManufacturer;
    QString CameraModel;
    QString Event;
    QString Subject;
    QString Orientation;
    QString ExposureTime;
    QString FNumber;
    QString ExposureProgram;
    QString ISOSpeedRatings;
    QString ExposureBiasValue;
    QString DateTimeOriginal;
    QString DateTimeDigitized;
    QString SubjectDistance;
    QString LightSource;
    QString Flash;
    QString FocalLength;
    QString ExposureMode;
    QString WhiteBalance;
    QString DigitalZoomRatio;
    QString FocalLengthIn35mmFilm;
    QString SceneCaptureType;
    QString GainControl;
    QString Contrast;
    QString Saturation;
    QString Sharpness;
    QString DeviceSettingDescription;

    // Location
    QString GPSLatitude;
    QString GPSLongitude;
    QString GPSAltitude;
    QString GPSTimeStamp;
    QString GPSSatellites;
    QString GPSStatus;
    QString GPSDOP;
    QString GPSSpeed;
    QString GPSTrack;
    QString GPSTrackRef;
    QString GPSImgDirection;
    QString GPSImgDirectionRef;
    QString GPSMapDatum;
    QString GPSProcessingMethod;
    QString GPSAreaInformation;

    QString PosterImage;
    QString CoverArtImage;
    QString ThumbnailImage;
#endif
};

QT_END_NAMESPACE

#endif // QMEDIAMETADATA_H
