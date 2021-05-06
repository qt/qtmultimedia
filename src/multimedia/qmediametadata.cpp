/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "qmediametadata.h"
#include <qvariant.h>
#include <qobject.h>
#include <qdatetime.h>
#include <qmediaformat.h>
#include <qsize.h>

QT_BEGIN_NAMESPACE

/*!
    \class QMediaMetaData
    \inmodule QtMultimedia

    \brief Provides meta-data for media files.

    \note Not all identifiers are supported on all platforms.

    \table 60%
    \header \li {3,1}
    Common attributes
    \header \li Value \li Description \li Type
    \row \li Title \li The title of the media.  \li QString
    \row \li SubTitle \li The sub-title of the media. \li QString
    \row \li Author \li The authors of the media. \li QStringList
    \row \li Comment \li A user comment about the media. \li QString
    \row \li Description \li A description of the media.  \li QString
    \row \li Category \li The category of the media.  \li QStringList
    \row \li Genre \li The genre of the media.  \li QStringList
    \row \li Date \li The date of the media. \li QDate.
    \row \li UserRating \li A user rating of the media. \li int [0..100]
    \row \li Keywords \li A list of keywords describing the media.  \li QStringList
    \row \li Language \li The language of media, as an ISO 639-2 code. \li QString

    \row \li Publisher \li The publisher of the media.  \li QString
    \row \li Copyright \li The media's copyright notice.  \li QString
    \row \li ParentalRating \li The parental rating of the media.  \li QString
    \row \li RatingOrganization \li The organization responsible for the parental rating of the media.
    \li QString

    \header \li {3,1}
    Media attributes
    \row \li Size \li The size in bytes of the media. \li qint64
    \row \li MediaType \li The type of the media (audio, video, etc).  \li QString
    \row \li FileFormat \li The file format of the media.  \li QMediaFormat::FileFormat
    \row \li Duration \li The duration in millseconds of the media.  \li qint64

    \header \li {3,1}
    Audio attributes
    \row \li AudioBitRate \li The bit rate of the media's audio stream in bits per second.  \li int
    \row \li AudioCodec \li The codec of the media's audio stream.  \li QMediaForma::AudioCodec
    \row \li AverageLevel \li The average volume level of the media.  \li int
    \row \li ChannelCount \li The number of channels in the media's audio stream. \li int
    \row \li PeakValue \li The peak volume of the media's audio stream. \li int
    \row \li SampleRate \li The sample rate of the media's audio stream in hertz. \li int

    \header \li {3,1}
    Music attributes
    \row \li AlbumTitle \li The title of the album the media belongs to.  \li QString
    \row \li AlbumArtist \li The principal artist of the album the media belongs to.  \li QString
    \row \li ContributingArtist \li The artists contributing to the media.  \li QStringList
    \row \li Composer \li The composer of the media.  \li QStringList
    \row \li Conductor \li The conductor of the media. \li QString
    \row \li Lyrics \li The lyrics to the media. \li QString
    \row \li Mood \li The mood of the media.  \li QString
    \row \li TrackNumber \li The track number of the media.  \li int
    \row \li TrackCount \li The number of tracks on the album containing the media.  \li int

    \row \li CoverArtUrlSmall \li The URL of a small cover art image. \li  QUrl
    \row \li CoverArtUrlLarge \li The URL of a large cover art image. \li  QUrl
    \row \li CoverArtImage \li An embedded cover art image. \li  QImage

    \header \li {3,1}
    Image and video attributes
    \row \li Resolution \li The dimensions of an image or video. \li QSize
    \row \li Orientation \li Orientation of an image or video. \li int (degrees)

    \header \li {3,1}
    Video attributes
    \row \li VideoFrameRate \li The frame rate of the media's video stream. \li qreal
    \row \li VideoBitRate \li The bit rate of the media's video stream in bits per second.  \li int
    \row \li VideoCodec \li The codec of the media's video stream.  \li QMediaFormat::VideoCodec

    \row \li PosterUrl \li The URL of a poster image. \li QUrl
    \row \li PosterImage \li An embedded poster image. \li QImage

    \header \li {3,1}
    Movie attributes
    \row \li ChapterNumber \li The chapter number of the media.  \li int
    \row \li Director \li The director of the media.  \li QString
    \row \li LeadPerformer \li The lead performer in the media.  \li QStringList
    \row \li Writer \li The writer of the media.  \li QStringList

    \header \li {3,1}
    Photo attributes.
    \row \li CameraManufacturer \li The manufacturer of the camera used to capture the media.  \li QString
    \row \li CameraModel \li The model of the camera used to capture the media.  \li QString
    \row \li Event \li The event during which the media was captured.  \li QString
    \row \li Subject \li The subject of the media.  \li QString
    \row \li ExposureTime \li Exposure time, given in seconds.  \li qreal
    \row \li FNumber \li The F Number.  \li int
    \row \li ExposureProgram
        \li The class of the program used by the camera to set exposure when the picture is taken.  \li QString
    \row \li ISOSpeedRatings
        \li Indicates the ISO Speed and ISO Latitude of the camera or input device as specified in ISO 12232. \li qreal
    \row \li ExposureBiasValue
        \li The exposure bias.
        The unit is the APEX (Additive System of Photographic Exposure) setting.  \li qreal
    \row \li DateTimeOriginal \li The date and time when the original image data was generated. \li QDateTime
    \row \li DateTimeDigitized \li The date and time when the image was stored as digital data.  \li QDateTime
    \row \li SubjectDistance \li The distance to the subject, given in meters. \li qreal
    \row \li LightSource
        \li The kind of light source. \li QString
    \row \li Flash
        \li Status of flash when the image was shot. \li QCameraExposure::FlashMode
    \row \li FocalLength
        \li The actual focal length of the lens, in mm. \li qreal
    \row \li ExposureMode
        \li Indicates the exposure mode set when the image was shot. \li QCameraExposure::ExposureMode
    \row \li WhiteBalance
        \li Indicates the white balance mode set when the image was shot. \li QCameraImageProcessing::WhiteBalanceMode
    \row \li DigitalZoomRatio
        \li Indicates the digital zoom ratio when the image was shot. \li qreal
    \row \li FocalLengthIn35mmFilm
        \li Indicates the equivalent focal length assuming a 35mm film camera, in mm. \li qreal
    \row \li SceneCaptureType
        \li Indicates the type of scene that was shot.
        It can also be used to record the mode in which the image was shot. \li QString
    \row \li GainControl
        \li Indicates the degree of overall image gain adjustment. \li qreal
    \row \li Contrast
        \li Indicates the direction of contrast processing applied by the camera when the image was shot. \li qreal
    \row \li Saturation
        \li Indicates the direction of saturation processing applied by the camera when the image was shot. \li qreal
    \row \li Sharpness
        \li Indicates the direction of sharpness processing applied by the camera when the image was shot. \li qreal
    \row \li DeviceSettingDescription
        \li Exif tag, indicates information on the picture-taking conditions of a particular camera model. \li QString

    \row \li GPSLatitude
        \li Latitude value of the geographical position (decimal degrees).
        A positive latitude indicates the Northern Hemisphere,
        and a negative latitude indicates the Southern Hemisphere. \li double
    \row \li GPSLongitude
        \li Longitude value of the geographical position (decimal degrees).
        A positive longitude indicates the Eastern Hemisphere,
        and a negative longitude indicates the Western Hemisphere. \li double
    \row \li GPSAltitude
        \li The value of altitude in meters above sea level. \li double
    \row \li GPSTimeStamp
        \li Time stamp of GPS data. \li QDateTime
    \row \li GPSSatellites
        \li GPS satellites used for measurements. \li QString
    \row \li GPSStatus
        \li Status of GPS receiver at image creation time. \li QString
    \row \li GPSDOP
        \li Degree of precision for GPS data. \li qreal
    \row \li GPSSpeed
        \li Speed of GPS receiver movement in kilometers per hour. \li qreal
    \row \li GPSTrack
        \li Direction of GPS receiver movement.
        The range of values is [0.0, 360),
        with 0 direction pointing on either true or magnetic north,
        depending on GPSTrackRef. \li qreal
    \row \li GPSTrackRef
        \li Reference for movement direction. \li QChar.
        'T' means true direction and 'M' is magnetic direction.
    \row \li GPSImgDirection
        \li Direction of image when captured. \li qreal
        The range of values is [0.0, 360).
    \row \li GPSImgDirectionRef
        \li Reference for image direction. \li QChar.
        'T' means true direction and 'M' is magnetic direction.
    \row \li GPSMapDatum
        \li Geodetic survey data used by the GPS receiver. \li QString
    \row \li GPSProcessingMethod
        \li The name of the method used for location finding. \li QString
    \row \li GPSAreaInformation
        \li The name of the GPS area. \li QString

    \row \li ThumbnailImage \li An embedded thumbnail image.  \li QImage
    \endtable
*/

//QMetaType QMediaMetaData::typeForKey(QMediaMetaData::Key k)
//{

//}

/*!
    \fn QVariant QMediaMetaData::value(QMediaMetaData::Key k) const
*/

/*!
    \fn void QMediaMetaData::insert(QMediaMetaData::Key k, const QVariant &value)
*/

QString QMediaMetaData::stringValue(QMediaMetaData::Key k) const
{
    QVariant value = data.value(k);
    if (value.isNull())
        return QString();

    switch (k) {
    // string based or convertible to string
    case Title:
    case Author:
    case Comment:
    case Description:
    case Genre:
    case Language:
    case Publisher:
    case Copyright:
    case Date:
    case Url:
    case MediaType:
    case AudioBitRate:
    case VideoBitRate:
    case VideoFrameRate:
    case AlbumTitle:
    case AlbumArtist:
    case ContributingArtist:
    case TrackNumber:
    case Composer:
    case Orientation:
    case LeadPerformer:
        return value.toString();
    case Duration: {
        QTime time = QTime::fromMSecsSinceStartOfDay(value.toInt());
        return time.toString();
    }
    case FileFormat:
        return QMediaFormat::fileFormatName(value.value<QMediaFormat::FileFormat>());
    case AudioCodec:
        return QMediaFormat::audioCodecName(value.value<QMediaFormat::AudioCodec>());
    case VideoCodec:
        return QMediaFormat::videoCodecName(value.value<QMediaFormat::VideoCodec>());
    case Resolution: {
        QSize size = value.toSize();
        return QString::fromUtf8("%1 x %2").arg(size.width()).arg(size.height());
    }
    case ThumbnailImage:
    case CoverArtImage:
        break;
    }
    return QString();
}

QString QMediaMetaData::metaDataKeyToString(QMediaMetaData::Key k)
{
    switch (k) {
        case QMediaMetaData::Title:
            return (QObject::tr("Title"));
        case QMediaMetaData::Author:
            return (QObject::tr("Author"));
        case QMediaMetaData::Comment:
            return (QObject::tr("Comment"));
        case QMediaMetaData::Description:
            return (QObject::tr("Description"));
        case QMediaMetaData::Genre:
            return (QObject::tr("Genre"));
        case QMediaMetaData::Date:
            return (QObject::tr("Date"));
        case QMediaMetaData::Language:
            return (QObject::tr("Language"));
        case QMediaMetaData::Publisher:
            return (QObject::tr("Publisher"));
        case QMediaMetaData::Copyright:
            return (QObject::tr("Copyright"));
        case QMediaMetaData::Url:
            return (QObject::tr("Url"));
        case QMediaMetaData::Duration:
            return (QObject::tr("Duration"));
        case QMediaMetaData::MediaType:
            return (QObject::tr("Media type"));
        case QMediaMetaData::FileFormat:
            return (QObject::tr("Container Format"));
        case QMediaMetaData::AudioBitRate:
            return (QObject::tr("Audio bit rate"));
        case QMediaMetaData::AudioCodec:
            return (QObject::tr("Audio codec"));
        case QMediaMetaData::VideoBitRate:
            return (QObject::tr("Video bit rate"));
        case QMediaMetaData::VideoCodec:
            return (QObject::tr("Video codec"));
        case QMediaMetaData::VideoFrameRate:
            return (QObject::tr("Video frame rate"));
        case QMediaMetaData::AlbumTitle:
            return (QObject::tr("Album title"));
        case QMediaMetaData::AlbumArtist:
            return (QObject::tr("Album artist"));
        case QMediaMetaData::ContributingArtist:
            return (QObject::tr("Contributing artist"));
        case QMediaMetaData::TrackNumber:
            return (QObject::tr("Track number"));
        case QMediaMetaData::Composer:
            return (QObject::tr("Composer"));
        case QMediaMetaData::ThumbnailImage:
            return (QObject::tr("Thumbnail image"));
        case QMediaMetaData::CoverArtImage:
            return (QObject::tr("Cover art image"));
        case QMediaMetaData::Orientation:
            return (QObject::tr("Orientation"));
        case QMediaMetaData::Resolution:
            return (QObject::tr("Resolution"));
        case QMediaMetaData::LeadPerformer:
            return (QObject::tr("Lead performer"));
    }
    return QString();
}


QT_END_NAMESPACE
