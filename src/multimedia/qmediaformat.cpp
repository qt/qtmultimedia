// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qmediaformat.h"
#include "private/qplatformmediaintegration_p.h"
#include "private/qplatformmediaformatinfo_p.h"
#include "private/qmultimedia_enum_to_string_converter_p.h"
#if QT_CONFIG(mimetype)
#include <QtCore/qmimedatabase.h>
#endif

QT_BEGIN_NAMESPACE

// clang-format off

struct DescriptionRole{};

QT_MM_MAKE_STRING_RESOLVER(QMediaFormat::FileFormat, QtMultimediaPrivate::EnumName,
                           (QMediaFormat::UnspecifiedFormat, "Unspecified")
                           (QMediaFormat::WMV, "WMV")
                           (QMediaFormat::AVI, "AVI")
                           (QMediaFormat::Matroska, "Matroska")
                           (QMediaFormat::MPEG4, "MPEG-4")
                           (QMediaFormat::Ogg, "Ogg")
                           (QMediaFormat::QuickTime, "QuickTime")
                           (QMediaFormat::WebM, "WebM")
                           // Audio Formats
                           (QMediaFormat::Mpeg4Audio, "MPEG-4 Audio")
                           (QMediaFormat::AAC, "AAC")
                           (QMediaFormat::WMA, "WMA")
                           (QMediaFormat::MP3, "MP3")
                           (QMediaFormat::FLAC, "FLAC")
                           (QMediaFormat::Wave, "Wave")
                          );

QT_MM_MAKE_STRING_RESOLVER(QMediaFormat::FileFormat, DescriptionRole,
                           (QMediaFormat::UnspecifiedFormat, "Unspecified File Format")
                           (QMediaFormat::WMV, "Windows Media Video (WMV)")
                           (QMediaFormat::AVI, "Audio Video Interleave (AVI)")
                           (QMediaFormat::Matroska, "Matroska Multimedia Container")
                           (QMediaFormat::MPEG4, "MPEG-4 Video Container")
                           (QMediaFormat::Ogg, "Ogg")
                           (QMediaFormat::QuickTime, "QuickTime Container")
                           (QMediaFormat::WebM, "WebM")
                           // Audio Formats
                           (QMediaFormat::Mpeg4Audio, "MPEG-4 Audio")
                           (QMediaFormat::AAC, "Advanced Audio Coding (AAC)")
                           (QMediaFormat::WMA, "Windows Media Audio (WMA)")
                           (QMediaFormat::MP3, "MP3")
                           (QMediaFormat::FLAC, "Free Lossless Audio Codec (FLAC)")
                           (QMediaFormat::Wave, "Wave Audio File Format (WAVE)")
                          );

QT_MM_MAKE_STRING_RESOLVER(QMediaFormat::AudioCodec, QtMultimediaPrivate::EnumName,
                           (QMediaFormat::AudioCodec::Unspecified, "Unspecified")
                           (QMediaFormat::AudioCodec::MP3, "MP3")
                           (QMediaFormat::AudioCodec::AAC, "AAC")
                           (QMediaFormat::AudioCodec::AC3, "AC3")
                           (QMediaFormat::AudioCodec::EAC3, "EAC3")
                           (QMediaFormat::AudioCodec::FLAC, "FLAC")
                           (QMediaFormat::AudioCodec::DolbyTrueHD, "DolbyTrueHD")
                           (QMediaFormat::AudioCodec::Opus, "Opus")
                           (QMediaFormat::AudioCodec::Vorbis, "Vorbis")
                           (QMediaFormat::AudioCodec::Wave, "Wave")
                           (QMediaFormat::AudioCodec::WMA, "WMA")
                           (QMediaFormat::AudioCodec::ALAC, "ALAC")
                          );

QT_MM_MAKE_STRING_RESOLVER(QMediaFormat::AudioCodec, DescriptionRole,
                           (QMediaFormat::AudioCodec::Unspecified, "Unspecified Audio Codec")
                           (QMediaFormat::AudioCodec::MP3, "MP3")
                           (QMediaFormat::AudioCodec::AAC, "Advanced Audio Coding (AAC)")
                           (QMediaFormat::AudioCodec::AC3, "Dolby Digital (AC3)")
                           (QMediaFormat::AudioCodec::EAC3, "Dolby Digital Plus (E-AC3)")
                           (QMediaFormat::AudioCodec::FLAC, "Free Lossless Audio Codec (FLAC)")
                           (QMediaFormat::AudioCodec::DolbyTrueHD, "Dolby True HD")
                           (QMediaFormat::AudioCodec::Opus, "Opus")
                           (QMediaFormat::AudioCodec::Vorbis, "Vorbis")
                           (QMediaFormat::AudioCodec::Wave, "Wave")
                           (QMediaFormat::AudioCodec::WMA, "Windows Media Audio (WMA)")
                           (QMediaFormat::AudioCodec::ALAC, "Apple Lossless Audio Codec (ALAC)")
                          );


QT_MM_MAKE_STRING_RESOLVER(QMediaFormat::VideoCodec, QtMultimediaPrivate::EnumName,
                           (QMediaFormat::VideoCodec::Unspecified, "Unspecified")
                           (QMediaFormat::VideoCodec::MPEG1, "MPEG1")
                           (QMediaFormat::VideoCodec::MPEG2, "MPEG2")
                           (QMediaFormat::VideoCodec::MPEG4, "MPEG4")
                           (QMediaFormat::VideoCodec::H264, "H264")
                           (QMediaFormat::VideoCodec::H265, "H265")
                           (QMediaFormat::VideoCodec::VP8, "VP8")
                           (QMediaFormat::VideoCodec::VP9, "VP9")
                           (QMediaFormat::VideoCodec::AV1, "AV1")
                           (QMediaFormat::VideoCodec::Theora, "Theora")
                           (QMediaFormat::VideoCodec::WMV, "WMV")
                           (QMediaFormat::VideoCodec::MotionJPEG, "MotionJPEG")
                          );

QT_MM_MAKE_STRING_RESOLVER(QMediaFormat::VideoCodec, DescriptionRole,
                           (QMediaFormat::VideoCodec::Unspecified, "Unspecified Video Codec")
                           (QMediaFormat::VideoCodec::MPEG1, "MPEG-1 Video")
                           (QMediaFormat::VideoCodec::MPEG2, "MPEG-2 Video")
                           (QMediaFormat::VideoCodec::MPEG4, "MPEG-4 Video")
                           (QMediaFormat::VideoCodec::H264, "H.264")
                           (QMediaFormat::VideoCodec::H265, "H.265")
                           (QMediaFormat::VideoCodec::VP8, "VP8")
                           (QMediaFormat::VideoCodec::VP9, "VP9")
                           (QMediaFormat::VideoCodec::AV1, "AV1")
                           (QMediaFormat::VideoCodec::Theora, "Theora")
                           (QMediaFormat::VideoCodec::WMV, "Windows Media Video (WMV)")
                           (QMediaFormat::VideoCodec::MotionJPEG, "MotionJPEG")
                          );
// clang-format on

/*!
    \class QMediaFormat
    \ingroup multimedia
    \inmodule QtMultimedia
    \brief Describes an encoding format for a multimedia file or stream.
    \since 6.2

    QMediaFormat describes an encoding format for a multimedia file or stream.

    You can check whether a certain media format can be used for encoding
    or decoding using QMediaFormat.
*/

/*!
    \qmlvaluetype mediaFormat
    \ingroup qmlvaluetypes
    \since 6.2
    //! \nativetype QMediaFormat
    \brief MediaFormat describes the format of a media file.
    \inqmlmodule QtMultimedia
    \ingroup multimedia_qml

    The MediaFormat type describes the format of a media file. It contains
    three properties that describe the file type and the audio and video codecs
    that are being used.

    MediaFormat can be used to specify the type of file that should be created
    by a MediaRecorder. The snippet below shows an example that sets up the
    recorder to create an mpeg4 video with AAC encoded audio and H265 video:

    \qml
    CaptureSession {
        ... // setup inputs
        MediaRecorder {
            mediaFormat {
                fileFormat: MediaFormat.MPEG4
                audioCodec: MediaFormat.AudioCodec.AAC
                videoCodec: MediaFormat.VideoCodec.H265
            }
        }
    }
    \endqml

    If the specified mediaFormat is not supported, the MediaRecorder will automatically try
    to find the best possible replacement format and use that instead.

    \sa MediaRecorder, CaptureSession
*/

namespace {

const char *mimeTypeForFormat[QMediaFormat::LastFileFormat + 2] =
{
    "",
    "video/x-ms-wmv",
    "video/x-msvideo",
    "video/x-matroska",
    "video/mp4",
    "video/ogg",
    "video/quicktime",
    "video/webm",
    // Audio Formats
    "audio/mp4",
    "audio/aac",
    "audio/x-ms-wma",
    "audio/mpeg",
    "audio/flac",
    "audio/wav"
};

constexpr QMediaFormat::FileFormat videoFormatPriorityList[] =
{
    QMediaFormat::MPEG4,
    QMediaFormat::QuickTime,
    QMediaFormat::AVI,
    QMediaFormat::WebM,
    QMediaFormat::WMV,
    QMediaFormat::Matroska,
    QMediaFormat::Ogg,
    QMediaFormat::UnspecifiedFormat
};

constexpr QMediaFormat::FileFormat audioFormatPriorityList[] =
{
    QMediaFormat::Mpeg4Audio,
    QMediaFormat::MP3,
    QMediaFormat::WMA,
    QMediaFormat::FLAC,
    QMediaFormat::Wave,
    QMediaFormat::UnspecifiedFormat
};

constexpr QMediaFormat::AudioCodec audioPriorityList[] =
{
    QMediaFormat::AudioCodec::AAC,
    QMediaFormat::AudioCodec::MP3,
    QMediaFormat::AudioCodec::AC3,
    QMediaFormat::AudioCodec::Opus,
    QMediaFormat::AudioCodec::EAC3,
    QMediaFormat::AudioCodec::DolbyTrueHD,
    QMediaFormat::AudioCodec::WMA,
    QMediaFormat::AudioCodec::FLAC,
    QMediaFormat::AudioCodec::Vorbis,
    QMediaFormat::AudioCodec::Wave,
    QMediaFormat::AudioCodec::Unspecified
};

constexpr QMediaFormat::VideoCodec videoPriorityList[] =
{
    QMediaFormat::VideoCodec::H265,
    QMediaFormat::VideoCodec::VP9,
    QMediaFormat::VideoCodec::H264,
    QMediaFormat::VideoCodec::AV1,
    QMediaFormat::VideoCodec::VP8,
    QMediaFormat::VideoCodec::WMV,
    QMediaFormat::VideoCodec::Theora,
    QMediaFormat::VideoCodec::MPEG4,
    QMediaFormat::VideoCodec::MPEG2,
    QMediaFormat::VideoCodec::MPEG1,
    QMediaFormat::VideoCodec::MotionJPEG,
};

}

class QMediaFormatPrivate : public QSharedData
{};

QT_DEFINE_QESDP_SPECIALIZATION_DTOR(QMediaFormatPrivate);

/*! \enum QMediaFormat::FileFormat

    Describes the container format used in a multimedia file or stream.

    \value WMA
        \l {Windows Media Audio}
    \value AAC
        \l{Advanced Audio Coding}
    \value Matroska
        \l{Matroska (MKV)}
    \value WMV
        \l{Windows Media Video}
    \value MP3
        \l{MPEG-1 Audio Layer III or MPEG-2 Audio Layer III}
    \value Wave
        \l{Waveform Audio File Format}
    \value Ogg
        \l{Ogg}
    \value MPEG4
        \l{MPEG-4}
    \value AVI
        \l{Audio Video Interleave}
    \value QuickTime
        \l{QuickTime}
    \value WebM
        \l{WebM}
    \value Mpeg4Audio
        \l{MPEG-4 Part 3 or MPEG-4 Audio (formally ISO/IEC 14496-3)}
    \value FLAC
        \l{Free Lossless Audio Codec}
    \value UnspecifiedFormat
        The format is unspecified.

    \omitvalue LastFileFormat
*/

/*! \qmlproperty enumeration QtMultimedia::mediaFormat::fileFormat

    Describes the container format used in a multimedia file or stream.
    It can take one of the following values:

    \table
    \header \li Property value
            \li Description
    \row \li MediaFormat.WMA
        \li \l{Windows Media Audio}
    \row \li MediaFormat.AAC
        \li \l{Advanced Audio Coding}
    \row \li MediaFormat.Matroska
        \li \l{Matroska (MKV)}
    \row \li MediaFormat.WMV
        \li \l{Windows Media Video}
    \row \li MediaFormat.MP3
        \li \l{MPEG-1 Audio Layer III or MPEG-2 Audio Layer III}
    \row \li MediaFormat.Wave
        \li \l{Waveform Audio File Format}
    \row \li MediaFormat.Ogg
        \li \l{Ogg}
    \row \li MediaFormat.MPEG4
        \li \l{MPEG-4}
    \row \li MediaFormat.AVI
        \li \l{Audio Video Interleave}
    \row \li MediaFormat.QuickTime
        \li \l{QuickTime}
    \row \li MediaFormat.WebM
        \li \l{WebM}
    \row \li MediaFormat.Mpeg4Audio
        \li \l{MPEG-4 Part 3 or MPEG-4 Audio (formally ISO/IEC 14496-3)}
    \row \li MediaFormat.FLAC
        \li \l{Free Lossless Audio Codec}
    \row \li MediaFormat.UnspecifiedFormat
        \li The format is unspecified.
    \endtable
*/

/*! \enum QMediaFormat::AudioCodec

    Describes the audio codec used in multimedia file or stream.

    \value WMA
        \l {Windows Media Audio}
    \value AC3
        \l {Dolby Digital}
    \value AAC
        \l{Advanced Audio Coding}
    \value ALAC
        \l{Apple Lossless Audio Codec}
    \value DolbyTrueHD
        \l{Dolby TrueHD}
    \value EAC3
        \l {Dolby Digital Plus (EAC3)}
    \value MP3
        \l{MPEG-1 Audio Layer III or MPEG-2 Audio Layer III}
    \value Wave
        \l{Waveform Audio File Format}
    \value Vorbis
        \l{Ogg Vorbis}
    \value FLAC
        \l{Free Lossless Audio Codec}
    \value Opus
        \l{Opus Audio Format}
    \value Unspecified
        Unspecified codec

    \omitvalue LastAudioCodec
*/

/*! \qmlproperty enumeration QtMultimedia::mediaFormat::audioCodec

    Describes the audio codec used in multimedia file or stream.
    It can take one of the following values:

    \table
    \header \li Property value
            \li Description
    \row \li MediaFormat.WMA
        \li \l {Windows Media Audio}
    \row \li MediaFormat.AC3
        \li \l {Dolby Digital}
    \row \li MediaFormat.AAC
        \li \l{Advanced Audio Coding}
    \row \li MediaFormat.ALAC
        \li \l{Apple Lossless Audio Codec}
    \row \li MediaFormat.DolbyTrueHD
        \li \l{Dolby TrueHD}
    \row \li MediaFormat.EAC3
        \li \l {Dolby Digital Plus (EAC3)}
    \row \li MediaFormat.MP3
        \li \l{MPEG-1 Audio Layer III or MPEG-2 Audio Layer III}
    \row \li MediaFormat.Wave
        \li \l{Waveform Audio File Format}
    \row \li MediaFormat.Vorbis
        \li \l{Ogg Vorbis}
    \row \li MediaFormat.FLAC
        \li \l{Free Lossless Audio Codec}
    \row \li MediaFormat.Opus
        \li \l{Opus Audio Format}
    \row \li MediaFormat.Unspecified
        \li Unspecified codec
    \endtable
*/

/*! \enum QMediaFormat::VideoCodec

    Describes the video coded used in multimedia file or stream.

    \value VP8
        \l{VP8}
    \value MPEG2
        \l{MPEG-2}
    \value MPEG1
        \l{MPEG-1}
    \value WMV
        \l{Windows Media Video}
    \value H265
        \l{High Efficiency Video Coding (HEVC)}
    \value H264
        \l{Advanced Video Coding}
    \value MPEG4
        \l{MPEG-4}
    \value AV1
        \l{AOMedia Video 1}
    \value MotionJPEG
        \l{MotionJPEG}
    \value VP9
        \l{VP9}
    \value Theora
        \l{Theora}
    \value Unspecified
        Video codec not specified

    \omitvalue LastVideoCodec
*/

/*! \qmlproperty enumeration QtMultimedia::mediaFormat::videoCodec

    Describes the video codec used in multimedia file or stream.
    It can take one of the following values:

    \table
    \header \li Property value
            \li Description
    \row \li MediaFormat.VP8
        \li \l{VP8}
    \row \li MediaFormat.MPEG2
        \li \l{MPEG-2}
    \row \li MediaFormat.MPEG1
        \li \l{MPEG-1}
    \row \li MediaFormat.WMV
        \li \l{Windows Media Video}
    \row \li MediaFormat.H265
        \li \l{High Efficiency Video Coding (HEVC)}
    \row \li MediaFormat.H264
        \li \l{Advanced Video Coding}
    \row \li MediaFormat.MPEG4
        \li \l{MPEG-4}
    \row \li MediaFormat.AV1
        \li \l{AOMedia Video 1}
    \row \li MediaFormat.MotionJPEG
        \li \l{MotionJPEG}
    \row \li MediaFormat.VP9
        \li \l{VP9}
    \row \li MediaFormat.Theora
        \li \l{Theora}
    \row \li MediaFormat.Unspecified
        \li Video codec not specified
    \endtable
*/

// these are non inline to make a possible future addition of a d pointer binary compatible

/*!
    Constructs a QMediaFormat object for \a format.
*/
QMediaFormat::QMediaFormat(FileFormat format)
    : fmt(format)
{
}

/*!
    Destroys the QMediaFormat object.
*/
QMediaFormat::~QMediaFormat() = default;

/*!
    Constructs a QMediaFormat object by copying from \a other.
*/
QMediaFormat::QMediaFormat(const QMediaFormat &other) noexcept = default;

/*!
    \fn void QMediaFormat::swap(QMediaFormat &other) noexcept

    Swaps the media format with \a other.
*/
/*!
    Copies \a other into this QMediaFormat object.
*/
QMediaFormat &QMediaFormat::operator=(const QMediaFormat &other) noexcept = default;

/*! \fn QMediaFormat::QMediaFormat(QMediaFormat &&other)

    Constructs a QMediaFormat objects by moving from \a other.
*/

/*! \fn QMediaFormat &QMediaFormat::operator=(QMediaFormat &&other)

    Moves \a other into this QMediaFormat objects.
*/

// Properties
/*! \property QMediaFormat::fileFormat

    \brief The file (container) format of the media.

    \sa QMediaFormat::FileFormat
*/

/*! \property QMediaFormat::audioCodec

    \brief The audio codec of the media.

    \sa QMediaFormat::AudioCodec
*/

/*! \property QMediaFormat::videoCodec

    \brief The video codec of the media.

    \sa QMediaFormat::VideoCodec
*/

/*! \fn void QMediaFormat::setVideoCodec(VideoCodec codec)

    Sets the video codec to \a codec.

    \sa videoCodec(), QMediaFormat::VideoCodec
*/

/*! \fn QMediaFormat::VideoCodec QMediaFormat::videoCodec() const

    Returns the video codec used in this format.

    \sa setVideoCodec(), QMediaFormat::VideoCodec
*/

/*! \fn void QMediaFormat::setAudioCodec(AudioCodec codec)

    Sets the audio codec to \a codec.

    \sa audioCodec(), QMediaFormat::AudioCodec
*/

/*! \fn QMediaFormat::AudioCodec QMediaFormat::audioCodec() const

    Returns the audio codec used in this format.

    \sa setAudioCodec(), QMediaFormat::AudioCodec
*/

/*!
    Returns \c true if Qt Multimedia can encode or decode this format,
    depending on \a mode.
*/

bool QMediaFormat::isSupported(ConversionMode mode) const
{
    return QPlatformMediaIntegration::instance()->formatInfo()->isSupported(*this, mode);
}

/*!
    Returns the \l{MIME type} for the file format used in this media format.
*/

#if QT_CONFIG(mimetype)
QMimeType QMediaFormat::mimeType() const
{
    return QMimeDatabase().mimeTypeForName(QString::fromLatin1(mimeTypeForFormat[fmt + 1]));
}
#endif

/*!
    \enum QMediaFormat::ConversionMode

    In many cases, systems have asymmetric capabilities and can often decode more formats
    or codecs than can be encoded. This enum describes the requested conversion mode to
    be used when checking whether a certain file format or codec is supported.

    \value Encode
        Used to check whether a certain file format or codec can be encoded.
    \value Decode
        Used to check whether a certain file format or codec can be decoded.

    \sa supportedFileFormats, supportedAudioCodecs, supportedVideoCodecs
*/

/*!
    \qmlmethod list<FileFormat> QtMultimedia::mediaFormat::supportedFileFormats(conversionMode)
    Returns a list of file formats for the audio and video
    codec indicated by \a{conversionMode}.

    To get all supported file formats, run this query on a default constructed MediaFormat. To
    get a list of file formats supporting a specific combination of an audio and video codec,
    you can set the audioCodec and videoCodec properties before running this query.

    \sa QMediaFormat::ConversionMode
*/

/*!
    Returns a list of file formats for the audio and video
    codec indicated by \a{m}.

    To get all supported file formats, run this query on a default constructed
    QMediaFormat.

    \sa QMediaFormat::ConversionMode
*/
QList<QMediaFormat::FileFormat> QMediaFormat::supportedFileFormats(QMediaFormat::ConversionMode m)
{
    return QPlatformMediaIntegration::instance()->formatInfo()->supportedFileFormats(*this, m);
}

/*!
    \qmlmethod list<VideoCodec> QtMultimedia::mediaFormat::supportedVideoCodecs(conversionMode)
    Returns a list of video codecs for the chosen file format and
    audio codec (\a conversionMode).

    To get all supported video codecs, run this query on a default constructed MediaFormat. To
    get a list of supported video codecs for a specific combination of a file format and an audio
    codec, you can set the fileFormat and audioCodec properties before running this query.

    \sa QMediaFormat::ConversionMode
*/

/*!
    Returns a list of video codecs for the chosen file format and
    audio codec (\a m).

    To get all supported video codecs, run this query on a default constructed
    MediaFormat.

    \sa QMediaFormat::ConversionMode
*/
QList<QMediaFormat::VideoCodec> QMediaFormat::supportedVideoCodecs(QMediaFormat::ConversionMode m)
{
    return QPlatformMediaIntegration::instance()->formatInfo()->supportedVideoCodecs(*this, m);
}

/*!
    \qmlmethod list<AudioCodec> QtMultimedia::mediaFormat::supportedAudioFormats(conversionMode)
    Returns a list of audio codecs for the chosen file format and
    video codec (\a conversionMode).

    To get all supported audio codecs, run this query on a default constructed MediaFormat. To get
    a list of supported audio codecs for a specific combination of a file format and a video codec,
    you can set the fileFormat and videoCodec properties before running this query.

    \sa QMediaFormat::ConversionMode
*/

/*!
    Returns a list of audio codecs for the chosen file format and
    video codec (\a m).

    To get all supported audio codecs, run this query on a default constructed
    QMediaFormat.

    \sa QMediaFormat::ConversionMode
*/
QList<QMediaFormat::AudioCodec> QMediaFormat::supportedAudioCodecs(QMediaFormat::ConversionMode m)
{
    return QPlatformMediaIntegration::instance()->formatInfo()->supportedAudioCodecs(*this, m);
}

/*!
    \qmlmethod string QtMultimedia::mediaFormat::fileFormatName(fileFormat)
    Returns a string based name for \a fileFormat.
*/

/*!
    Returns a string based name for \a fileFormat.
*/
QString QMediaFormat::fileFormatName(QMediaFormat::FileFormat fileFormat)
{
    return QtMultimediaPrivate::StringResolver<QMediaFormat::FileFormat>::toQString(fileFormat)
            .value_or(QStringLiteral("Unknown File Format"));
}

/*!
    \qmlmethod string QtMultimedia::mediaFormat::audioCodecName(codec)
    Returns a string based name for \a codec.
*/

/*!
    Returns a string based name for \a codec.
*/
QString QMediaFormat::audioCodecName(QMediaFormat::AudioCodec codec)
{
    return QtMultimediaPrivate::StringResolver<QMediaFormat::AudioCodec>::toQString(codec).value_or(
            QStringLiteral("Unknown Audio Codec"));
}

/*!
    \qmlmethod string QtMultimedia::mediaFormat::videoCodecName(codec)
    Returns a string based name for \a codec.
*/

/*!
    Returns a string based name for \a codec.
*/
QString QMediaFormat::videoCodecName(QMediaFormat::VideoCodec codec)
{
    return QtMultimediaPrivate::StringResolver<QMediaFormat::VideoCodec>::toQString(codec).value_or(
            QStringLiteral("Unknown Video Codec"));
}

/*!
    \qmlmethod string QtMultimedia::mediaFormat::fileFormatDescription(fileFormat)
    Returns a description for \a fileFormat.
*/

/*!
    Returns a description for \a fileFormat.
*/
QString QMediaFormat::fileFormatDescription(QMediaFormat::FileFormat fileFormat)
{
    return QtMultimediaPrivate::StringResolver<QMediaFormat::FileFormat,
                                               DescriptionRole>::toQString(fileFormat)
            .value_or(QStringLiteral("Unknown File Format"));
}

/*!
    \qmlmethod string QtMultimedia::mediaFormat::audioCodecDescription(codec)
    Returns a description for \a codec.
*/

/*!
    Returns a description for \a codec.
*/
QString QMediaFormat::audioCodecDescription(QMediaFormat::AudioCodec codec)
{
    return QtMultimediaPrivate::StringResolver<QMediaFormat::AudioCodec,
                                               DescriptionRole>::toQString(codec)
            .value_or(QStringLiteral("Unknown Audio Codec"));
}

/*!
    \qmlmethod string QtMultimedia::mediaFormat::videoCodecDescription(codec)
    Returns a description for \a codec.
*/

/*!
    Returns a description for \a codec.
*/
QString QMediaFormat::videoCodecDescription(QMediaFormat::VideoCodec codec)
{
    return QtMultimediaPrivate::StringResolver<QMediaFormat::VideoCodec,
                                               DescriptionRole>::toQString(codec)
            .value_or(QStringLiteral("Unknown Video Codec"));
}

/*!
    \fn bool QMediaFormat::operator!=(const QMediaFormat &other) const

    Returns \c true if \a other is not equal to the current media format,
    otherwise returns \c false.
*/

/*!
    Returns \c true if \a other is equal to the current media format, otherwise
    returns \c false.
*/
bool QMediaFormat::operator==(const QMediaFormat &other) const
{
    Q_ASSERT(!d);
    return fmt == other.fmt &&
            audio == other.audio &&
           video == other.video;
}

/*!
    \enum QMediaFormat::ResolveFlags

    Describes the requirements for resolving a suitable format for
    QMediaRecorder.

    \value NoFlags
           No requirements
    \value RequiresVideo
           A video codec is required

    \sa resolveForEncoding()
*/

/*!
    Resolves the format, based on \a flags, to a format that is supported by
    QMediaRecorder.

    This method tries to find the best possible match for unspecified settings.
    Settings that are not supported by the recorder will be modified to the closest
    match that is supported.

    When resolving, priority is given in the following order:
    \list 1
    \li File format
    \li Video codec
    \li Audio codec
    \endlist
*/
void QMediaFormat::resolveForEncoding(ResolveFlags flags)
{
    const bool requiresVideo = (flags & ResolveFlags::RequiresVideo) != 0;

    if (!requiresVideo)
        video = VideoCodec::Unspecified;

    // need to adjust the format. Priority is given first to file format, then video codec, then audio codec

    QMediaFormat nullFormat;
    auto supportedFormats = nullFormat.supportedFileFormats(QMediaFormat::Encode);
    auto supportedAudioCodecs = nullFormat.supportedAudioCodecs(QMediaFormat::Encode);
    auto supportedVideoCodecs = nullFormat.supportedVideoCodecs(QMediaFormat::Encode);

    auto bestSupportedFileFormat = [&](QMediaFormat::AudioCodec audio = QMediaFormat::AudioCodec::Unspecified,
                                       QMediaFormat::VideoCodec video = QMediaFormat::VideoCodec::Unspecified)
    {
        QMediaFormat f;
        f.setAudioCodec(audio);
        f.setVideoCodec(video);
        auto supportedFormats = f.supportedFileFormats(QMediaFormat::Encode);
        auto *list = (flags == NoFlags) ? audioFormatPriorityList : videoFormatPriorityList;
        while (*list != QMediaFormat::UnspecifiedFormat) {
            if (supportedFormats.contains(*list))
                break;
            ++list;
        }
        return *list;
    };

    // reset format if it does not support video when video is required
    if (requiresVideo && this->supportedVideoCodecs(QMediaFormat::Encode).isEmpty())
        fmt = QMediaFormat::UnspecifiedFormat;

    // reset non supported formats and codecs
    if (!supportedFormats.contains(fmt))
        fmt = QMediaFormat::UnspecifiedFormat;
    if (!supportedAudioCodecs.contains(audio))
        audio = QMediaFormat::AudioCodec::Unspecified;
    if (!requiresVideo || !supportedVideoCodecs.contains(video))
        video = QMediaFormat::VideoCodec::Unspecified;

    if (requiresVideo) {
        // try finding a file format that is supported
        if (fmt == QMediaFormat::UnspecifiedFormat)
            fmt = bestSupportedFileFormat(audio, video);
        // try without the audio codec
        if (fmt == QMediaFormat::UnspecifiedFormat)
            fmt = bestSupportedFileFormat(QMediaFormat::AudioCodec::Unspecified, video);
    }
    // try without the video codec
    if (fmt == QMediaFormat::UnspecifiedFormat)
        fmt = bestSupportedFileFormat(audio);
    // give me a format that's supported
    if (fmt == QMediaFormat::UnspecifiedFormat)
        fmt = bestSupportedFileFormat();
    // still nothing? Give up
    if (fmt == QMediaFormat::UnspecifiedFormat) {
        *this = {};
        return;
    }

    // find a working video codec
    if (requiresVideo) {
        // reset the audio codec, so that we won't throw away the video codec
        // if it is supported (choosing the specified video codec has higher
        // priority than the specified audio codec)
        auto a = audio;
        audio = QMediaFormat::AudioCodec::Unspecified;
        auto videoCodecs = this->supportedVideoCodecs(QMediaFormat::Encode);
        if (!videoCodecs.contains(video)) {
            // not supported, try to find a replacement
            auto *list = videoPriorityList;
            while (*list != QMediaFormat::VideoCodec::Unspecified) {
                if (videoCodecs.contains(*list))
                    break;
                ++list;
            }
            video = *list;
        }
        audio = a;
    } else {
        video = QMediaFormat::VideoCodec::Unspecified;
    }

    // and a working audio codec
    auto audioCodecs = this->supportedAudioCodecs(QMediaFormat::Encode);
    if (!audioCodecs.contains(audio)) {
        auto *list = audioPriorityList;
        while (*list != QMediaFormat::AudioCodec::Unspecified) {
            if (audioCodecs.contains(*list))
                break;
            ++list;
        }
        audio = *list;
    }
}

/*!
    \variable QMediaFormat::audio
    \internal
*/
/*!
    \variable QMediaFormat::d
    \internal
*/
/*!
    \variable QMediaFormat::video
    \internal
*/
/*!
    \variable QMediaFormat::fmt
    \internal
*/
QT_END_NAMESPACE

#include "moc_qmediaformat.cpp"
