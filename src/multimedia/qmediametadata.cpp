// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qmediametadata.h"
#include <QtCore/qcoreapplication.h>
#include <qvariant.h>
#include <qobject.h>
#include <qdatetime.h>
#include <qmediaformat.h>
#include <qsize.h>
#include <qurl.h>
#include <qimage.h>

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
    \row \li Author \li The authors of the media. \li QStringList
    \row \li Comment \li A user comment about the media. \li QString
    \row \li Description \li A description of the media.  \li QString
    \row \li Genre \li The genre of the media.  \li QStringList
    \row \li Date \li The date of the media. \li QDateTime.
    \row \li Language \li The language of media. \li QLocale::Language

    \row \li Publisher \li The publisher of the media.  \li QString
    \row \li Copyright \li The media's copyright notice.  \li QString
    \row \li Url \li A Url pointing to the origin of the media.  \li QUrl

    \header \li {3,1}
    Media attributes
    \row \li MediaType \li The type of the media (audio, video, etc).  \li QString
    \row \li FileFormat \li The file format of the media.  \li QMediaFormat::FileFormat
    \row \li Duration \li The duration in millseconds of the media.  \li qint64

    \header \li {3,1}
    Audio attributes
    \row \li AudioBitRate \li The bit rate of the media's audio stream in bits per second.  \li int
    \row \li AudioCodec \li The codec of the media's audio stream.  \li QMediaFormat::AudioCodec

    \header \li {3,1}
    Video attributes
    \row \li VideoFrameRate \li The frame rate of the media's video stream. \li qreal
    \row \li VideoBitRate \li The bit rate of the media's video stream in bits per second.  \li int
    \row \li VideoCodec \li The codec of the media's video stream.  \li QMediaFormat::VideoCodec

    \header \li {3,1}
    Music attributes
    \row \li AlbumTitle \li The title of the album the media belongs to.  \li QString
    \row \li AlbumArtist \li The principal artist of the album the media belongs to.  \li QString
    \row \li ContributingArtist \li The artists contributing to the media.  \li QStringList
    \row \li TrackNumber \li The track number of the media.  \li int
    \row \li Composer \li The composer of the media.  \li QStringList
    \row \li LeadPerformer \li The lead performer in the media.  \li QStringList

    \row \li ThumbnailImage \li An embedded thumbnail image.  \li QImage
    \row \li CoverArtImage \li An embedded cover art image. \li  QImage

    \header \li {3,1}
    Image and video attributes
    \row \li Orientation \li The rotation angle of an image or video. \li int
    \row \li Resolution \li The dimensions of an image or video. \li QSize

    \endtable
*/


/*!
    Returns the meta type used to store data for Key \a key.
*/
QMetaType QMediaMetaData::keyType(Key key)
{
    switch (key) {
    case Title:
    case Comment:
    case Description:
    case Publisher:
    case Copyright:
    case MediaType:
    case AlbumTitle:
    case AlbumArtist:
        return QMetaType::fromType<QString>();
    case Genre:
    case Author:
    case ContributingArtist:
    case Composer:
    case LeadPerformer:
        return QMetaType::fromType<QStringList>();

    case Date:
        return QMetaType::fromType<QDateTime>();

    case Language:
        return QMetaType::fromType<QLocale::Language>();
    case Url:
        return QMetaType::fromType<QUrl>();

    case Duration:
        return QMetaType::fromType<qint64>();
    case FileFormat:
        return QMetaType::fromType<QMediaFormat::FileFormat>();

    case AudioBitRate:
    case VideoBitRate:
    case TrackNumber:
    case Orientation:
        return QMetaType::fromType<int>();
    case AudioCodec:
        return QMetaType::fromType<QMediaFormat::AudioCodec>();
    case VideoCodec:
        return QMetaType::fromType<QMediaFormat::VideoCodec>();
    case VideoFrameRate:
        return QMetaType::fromType<qreal>();


    case ThumbnailImage:
    case CoverArtImage:
        return QMetaType::fromType<QImage>();

    case Resolution:
        return QMetaType::fromType<QSize>();
    default:
        return QMetaType::fromType<void>();
    }
}

/*!
    \qmlvaluetype mediaMetaData
    \ingroup qmlvaluetypes
    \inqmlmodule QtMultimedia
    \since 6.2
    //! \instantiates QMediaMetaData
    \brief Provides meta-data for media files.
    \ingroup multimedia_qml
    \ingroup multimedia_audio_qml
    \ingroup multimedia_video_qml

    Meta-data is supplementary data describing media.
    See QMediaMetaData for available meta data attributes.
*/

/*
    Some potential attributes to add if we can properly support them.
    Might require that we add EXIF support to Qt Multimedia

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
        \li Status of flash when the image was shot. \li QCamera::FlashMode
    \row \li FocalLength
        \li The actual focal length of the lens, in mm. \li qreal
    \row \li ExposureMode
        \li Indicates the exposure mode set when the image was shot. \li QCamera::ExposureMode
    \row \li WhiteBalance
        \li Indicates the white balance mode set when the image was shot. \li QCamera::WhiteBalanceMode
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

    \endtable
*/

/*!
    \enum QMediaMetaData::Key

    The following meta data keys can be used:

    \value Title Media title
    \value Author Media author
    \value Comment Comment
    \value Description Brief desripttion
    \value Genre Genre the media belongs to
    \value Date Creation date
    \value Language Media language
    \value Publisher Media publisher info.
    \value Copyright Media copyright info.
    \value Url Publisher's website URL
    \value Duration Media playback duration
    \value MediaType Type of the media
    \value FileFormat File format
    \value AudioBitRate
    \value AudioCodec
    \value VideoBitRate
    \value VideoCodec
    \value VideoFrameRate
    \value AlbumTitle Album's title
    \value AlbumArtist Artist's info.
    \value ContributingArtist
    \value TrackNumber
    \value Composer Media composer's info.
    \value LeadPerformer
    \value ThumbnailImage Media thumbnail image
    \value CoverArtImage Media cover art
    \value Orientation
    \value Resolution
*/

/*!
    \variable QMediaMetaData::NumMetaData
    \internal
*/

/*!
    \qmlmethod variant QtMultimedia::mediaMetaData::value(Key key)

    Returns the meta data value for Key \a key, or a null QVariant if no
    meta-data for the key is available.
*/

/*!
    \fn QVariant QMediaMetaData::value(QMediaMetaData::Key key) const

    Returns the meta data value for Key \a key, or a null QVariant if no
    meta data for the key is available.
*/

/*!
    \qmlmethod bool QtMultimedia::mediaMetaData::isEmpty()
    Returns \c true if the meta data contains no items: otherwise returns \c{false}.
*/

/*!
    \fn bool QMediaMetaData::isEmpty() const
    Returns \c true if the meta data contains no items: otherwise returns \c{false}.
*/

/*!
    \qmlmethod void QtMultimedia::mediaMetaData::clear()
    Removes all data from the MediaMetaData object.
*/

/*!
    \fn void QMediaMetaData::clear()
    Removes all data from the meta data object.
*/

/*!
    \qmlmethod void QtMultimedia::mediaMetaData::insert(Key k, variant value)
    Inserts a \a value into a Key: \a{k}.
*/

/*!
    \fn void QMediaMetaData::insert(QMediaMetaData::Key k, const QVariant &value)
    Inserts a \a value into a Key: \a{k}.
*/
/*!
    \qmlmethod void QtMultimedia::mediaMetaData::remove(Key k)
    Removes meta data from a Key: \a{k}.
*/

/*!
    \fn void QMediaMetaData::remove(QMediaMetaData::Key k)
    Removes meta data from a Key: \a{k}.
*/

/*!
    \qmlmethod list<Key> QtMultimedia::mediaMetaData::keys()
    Returns a list of MediaMetaData.Keys.
*/

/*!
    \fn QMediaMetaData::keys() const
    Returns a QList of QMediaMetaData::Keys.
*/

/*!
    \qmlmethod string QtMultimedia::mediaMetaData::stringValue(Key key)
    Returns the meta data for key \a key as a QString.

    This is mainly meant to simplify presenting the meta data to a user.
*/
/*!
    Returns the meta data for key \a key as a QString.

    This is mainly meant to simplify presenting the meta data to a user.
*/
QString QMediaMetaData::stringValue(QMediaMetaData::Key key) const
{
    QVariant value = data.value(key);
    if (value.isNull())
        return QString();

    switch (key) {
    // string based or convertible to string
    case Title:
    case Author:
    case Comment:
    case Description:
    case Genre:
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
    case Language: {
        auto l = value.value<QLocale::Language>();
        return QLocale::languageToString(l);
    }
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
/*!
    \qmlmethod string QtMultimedia::mediaMetaData::metaDataKeyToString(Key key)
    returns a string representation of \a key that can be used when presenting
    meta data to users.
*/

/*!
    returns a string representation of \a key that can be used when presenting
    meta data to users.
*/
QString QMediaMetaData::metaDataKeyToString(QMediaMetaData::Key key)
{
    switch (key) {
        case QMediaMetaData::Title:
            return (QCoreApplication::translate("QMediaMetaData", "Title"));
        case QMediaMetaData::Author:
            return (QCoreApplication::translate("QMediaMetaData", "Author"));
        case QMediaMetaData::Comment:
            return (QCoreApplication::translate("QMediaMetaData", "Comment"));
        case QMediaMetaData::Description:
            return (QCoreApplication::translate("QMediaMetaData", "Description"));
        case QMediaMetaData::Genre:
            return (QCoreApplication::translate("QMediaMetaData", "Genre"));
        case QMediaMetaData::Date:
            return (QCoreApplication::translate("QMediaMetaData", "Date"));
        case QMediaMetaData::Language:
            return (QCoreApplication::translate("QMediaMetaData", "Language"));
        case QMediaMetaData::Publisher:
            return (QCoreApplication::translate("QMediaMetaData", "Publisher"));
        case QMediaMetaData::Copyright:
            return (QCoreApplication::translate("QMediaMetaData", "Copyright"));
        case QMediaMetaData::Url:
            return (QCoreApplication::translate("QMediaMetaData", "Url"));
        case QMediaMetaData::Duration:
            return (QCoreApplication::translate("QMediaMetaData", "Duration"));
        case QMediaMetaData::MediaType:
            return (QCoreApplication::translate("QMediaMetaData", "Media type"));
        case QMediaMetaData::FileFormat:
            return (QCoreApplication::translate("QMediaMetaData", "Container Format"));
        case QMediaMetaData::AudioBitRate:
            return (QCoreApplication::translate("QMediaMetaData", "Audio bit rate"));
        case QMediaMetaData::AudioCodec:
            return (QCoreApplication::translate("QMediaMetaData", "Audio codec"));
        case QMediaMetaData::VideoBitRate:
            return (QCoreApplication::translate("QMediaMetaData", "Video bit rate"));
        case QMediaMetaData::VideoCodec:
            return (QCoreApplication::translate("QMediaMetaData", "Video codec"));
        case QMediaMetaData::VideoFrameRate:
            return (QCoreApplication::translate("QMediaMetaData", "Video frame rate"));
        case QMediaMetaData::AlbumTitle:
            return (QCoreApplication::translate("QMediaMetaData", "Album title"));
        case QMediaMetaData::AlbumArtist:
            return (QCoreApplication::translate("QMediaMetaData", "Album artist"));
        case QMediaMetaData::ContributingArtist:
            return (QCoreApplication::translate("QMediaMetaData", "Contributing artist"));
        case QMediaMetaData::TrackNumber:
            return (QCoreApplication::translate("QMediaMetaData", "Track number"));
        case QMediaMetaData::Composer:
            return (QCoreApplication::translate("QMediaMetaData", "Composer"));
        case QMediaMetaData::ThumbnailImage:
            return (QCoreApplication::translate("QMediaMetaData", "Thumbnail image"));
        case QMediaMetaData::CoverArtImage:
            return (QCoreApplication::translate("QMediaMetaData", "Cover art image"));
        case QMediaMetaData::Orientation:
            return (QCoreApplication::translate("QMediaMetaData", "Orientation"));
        case QMediaMetaData::Resolution:
            return (QCoreApplication::translate("QMediaMetaData", "Resolution"));
        case QMediaMetaData::LeadPerformer:
            return (QCoreApplication::translate("QMediaMetaData", "Lead performer"));
    }
    return QString();
}

// operator documentation
/*!
\fn QVariant &QMediaMetaData ::operator[](QMediaMetaData::Key k)
    Returns data stored at the Key \a{k}.
    \code
        QMediaMetaData rockBallad1;
        rockBalad[QMediaMetaData::Genre]="Rock"
    \endcode
*/

/*!
\fn bool QMediaMetaData::operator==(const QMediaMetaData &a, const QMediaMetaData &b)
    Compares two meta data objects \a a and \a b, and returns
    \c true if they are identical or \c false if they differ.
*/

/*!
\fn bool QMediaMetaData::operator!=(const QMediaMetaData &a, const QMediaMetaData &b)
    Compares two meta data objects \a a and \a b, and returns
    \c false if they are identical or \c true if they differ.
*/

/*!
    \variable QMediaMetaData::data
    \brief the meta data.
    \note this is a \c protected member of its class.
*/

QT_END_NAMESPACE

#include "moc_qmediametadata.cpp"
