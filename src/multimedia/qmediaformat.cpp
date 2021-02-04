/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "qmediaformat.h"
#include "private/qmediaplatformintegration_p.h"
#include "private/qmediaplatformformatinfo_p.h"
#include <QtCore/qmimedatabase.h>

QT_BEGIN_NAMESPACE

namespace {

// info from https://en.wikipedia.org/wiki/Comparison_of_video_container_formats
constexpr bool audioSupportMatrix[QMediaFormat::FileFormat::LastFileFormat + 1][(int)QMediaFormat::AudioCodec::LastAudioCodec + 1] =
{
    //  MP3,   AAC,   AC3, EAC3,   FLAC,  DTHD,  Opus,Vorbis,  Wave, ALAC
        // Container formats (Audio and Video)
    {  true,  true,  true,  true,  true, false,  true, false, false,  true }, // ASF
    {  true,  true,  true, false,  true, false,  true, false, false,  true }, // AVI,
    {  true,  true,  true,  true,  true,  true,  true,  true, false,  true }, // Matroska,
    {  true,  true,  true,  true,  true,  true,  true, false, false,  true }, // MPEG4,
    { false, false, false, false,  true, false,  true,  true, false, false }, // Ogg,
    {  true,  true,  true,  true, false, false, false, false, false,  true }, // QuickTime,
    { false, false, false, false,  true, false,  true,  true, false, false }, // WebM,
        // Audio Formats
    { false,  true, false, false, false, false, false, false, false, false }, // AAC,
    { false, false, false, false,  true, false, false, false, false, false }, // FLAC,
    {  true, false, false, false, false, false, false, false, false, false }, // Mpeg3,
    {  true,  true,  true,  true,  true,  true,  true, false, false,  true }, // Mpeg4Audio,
    { false, false, false, false, false, false, false, false, false,  true }, // ALAC,
    { false, false, false, false, false, false, false, false,  true, false }, // Wave,
};

inline bool formatSupportsCodec(QMediaFormat::FileFormat format, QMediaFormat::AudioCodec codec)
{
    return audioSupportMatrix[format][(int)codec];
}

// info from https://en.wikipedia.org/wiki/Comparison_of_video_container_formats
constexpr bool videoSupportMatrix[QMediaFormat::FileFormat::LastFileFormat + 1][(int)QMediaFormat::VideoCodec::LastVideoCodec + 1] =
{
    //MPEG1, MPEG2, MPEG4,  H264,  H265,   VP8,   VP9,   AV1,Theora, MotionJPEG,
        // Container formats (Audio and Video)
    {  true,  true,  true,  true,  true, false, false, false, false,false }, // ASF
    {  true,  true,  true,  true,  true,  true,  true, false,  true,  true }, // AVI,
    {  true,  true,  true,  true,  true,  true,  true,  true,  true, false }, // Matroska,
    {  true,  true,  true,  true,  true,  true,  true,  true,  true,  true }, // MPEG4,
    { false,  true,  true,  true,  true, false, false, false, false,  true }, // Ogg,
    { false, false, false, false, false, false, false, false,  true, false }, // QuickTime,
    { false, false, false, false, false,  true,  true,  true, false, false }, // WebM,
        // Audio Formats
    { false, false, false, false, false, false, false, false, false, false }, // AAC,
    { false, false, false, false, false, false, false, false, false, false }, // FLAC,
    { false, false, false, false, false, false, false, false, false, false }, // Mpeg3,
    { false, false, false, false, false, false, false, false, false, false }, // Mpeg4Audio,
    { false, false, false, false, false, false, false, false, false, false }, // ALAC,
    { false, false, false, false, false, false, false, false, false, false }, // Wave,
};

inline bool formatSupportsCodec(QMediaFormat::FileFormat format, QMediaFormat::VideoCodec codec)
{
    return videoSupportMatrix[format][(int)codec];
}


constexpr QMediaFormat::FileFormat videoFormatPriorityList[] =
{
    QMediaFormat::MPEG4,
    QMediaFormat::QuickTime,
    QMediaFormat::AVI,
    QMediaFormat::WebM,
    QMediaFormat::ASF,
    QMediaFormat::Matroska,
    QMediaFormat::Ogg,
    QMediaFormat::UnspecifiedFormat
};

constexpr QMediaFormat::FileFormat audioFormatPriorityList[] =
{
    QMediaFormat::AAC,
    QMediaFormat::MP3,
    QMediaFormat::Mpeg4Audio,
    QMediaFormat::FLAC,
    QMediaFormat::ALAC,
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
    QMediaFormat::VideoCodec::Theora,
    QMediaFormat::VideoCodec::MPEG4,
    QMediaFormat::VideoCodec::MPEG2,
    QMediaFormat::VideoCodec::MPEG1,
    QMediaFormat::VideoCodec::MotionJPEG,
};

const char *mimeTypeForFormat[QMediaFormat::LastFileFormat + 2] =
{
    "",
    "video/x-ms-asf",
    "video/x-msvideo",
    "video/x-matroska",
    "video/mp4",
    "video/ogg",
    "video/quicktime",
    "video/webm",
    // Audio Formats
    "audio/aac",
    "audio/flac",
    "audio/mpeg",
    "audio/mp4",
    "audio/alac",
    "audio/wave",
};


}

void QMediaFormatPrivate::resolveForEncoding(QMediaFormat *f, bool audioOnly)
{
    auto supportedFormats = QMediaEncoderInfo::supportedFileFormats();
    auto supportedAudioCodecs = QMediaEncoderInfo::supportedAudioCodecs();
    auto supportedVideoCodecs = QMediaEncoderInfo::supportedVideoCodecs();

    auto bestSupportedFileFormat = [&](QMediaFormat::AudioCodec audio = QMediaFormat::AudioCodec::Unspecified,
                                       QMediaFormat::VideoCodec video = QMediaFormat::VideoCodec::Unspecified)
    {
        auto *list = audioOnly ? audioFormatPriorityList : videoFormatPriorityList;
        while (*list != QMediaFormat::UnspecifiedFormat) {
            if (supportedFormats.contains(*list)) {
                if ((audio == QMediaFormat::AudioCodec::Unspecified || formatSupportsCodec(f->fmt, audio)) &&
                    (video == QMediaFormat::VideoCodec::Unspecified || formatSupportsCodec(f->fmt, video)))
                    break;
            }
            ++list;
        }
        return *list;
    };

    // reset non supported formats and codecs
    if (!supportedFormats.contains(f->fmt))
        f->fmt = QMediaFormat::UnspecifiedFormat;
    if (!supportedAudioCodecs.contains(f->audio))
        f->audio = QMediaFormat::AudioCodec::Unspecified;
    if (audioOnly || !supportedVideoCodecs.contains(f->video))
        f->video = QMediaFormat::VideoCodec::Unspecified;

    // try finding a file format that is supported
    if (f->fmt == QMediaFormat::UnspecifiedFormat)
        f->fmt = bestSupportedFileFormat(f->audio, f->video);
    // try without the audio codec
    if (f->fmt == QMediaFormat::UnspecifiedFormat)
        f->fmt = bestSupportedFileFormat(QMediaFormat::AudioCodec::Unspecified, f->video);
    // try without the video codec
    if (f->fmt == QMediaFormat::UnspecifiedFormat)
        f->fmt = bestSupportedFileFormat(f->audio);
    // give me a format that's supported
    if (f->fmt == QMediaFormat::UnspecifiedFormat)
        f->fmt = bestSupportedFileFormat();
    // still nothing? Give up
    if (f->fmt == QMediaFormat::UnspecifiedFormat)
        return;

    // now that we have a format, reset codecs that won't work
    if (f->audio != QMediaFormat::AudioCodec::Unspecified) {
        if (!formatSupportsCodec(f->fmt, f->audio) || !supportedAudioCodecs.contains(f->audio))
            f->audio = QMediaFormat::AudioCodec::Unspecified;
    }
    if (f->video != QMediaFormat::VideoCodec::Unspecified) {
        if (!formatSupportsCodec(f->fmt, f->video) || !supportedVideoCodecs.contains(f->video))
            f->video = QMediaFormat::VideoCodec::Unspecified;
    }

    // find a matching audio format
    if (f->audio == QMediaFormat::AudioCodec::Unspecified) {
        auto *list = audioPriorityList;
        while (*list != QMediaFormat::AudioCodec::Unspecified) {
            if (supportedAudioCodecs.contains(*list) && formatSupportsCodec(f->fmt, *list))
                break;
            ++list;
        }
        f->audio = *list;
    }

    // find a matching video format
    if (!audioOnly && f->video == QMediaFormat::VideoCodec::Unspecified) {
        auto *list = videoPriorityList;
        while (*list != QMediaFormat::VideoCodec::Unspecified) {
            if (supportedVideoCodecs.contains(*list) && formatSupportsCodec(f->fmt, *list))
                break;
            ++list;
        }
        f->video = *list;
    }
}

/*! \enum QMediaFormat::FileFormat

    Describes the container format used in a multimedia file or stream.
*/

/*! \enum QMediaFormat::AudioCodec

    Describes the audio coded used in multimedia file or stream.
*/

/*! \enum QMediaFormat::AudioCodec

    Describes the video coded used in multimedia file or stream.
*/

/*! \class QMediaFormat

    Describes an encoding format for a multimedia file or stream.

    You can check whether a certain QMediaFormat can be used for encoding
    or decoding using QMediaDecoderInfo or QMediaEncoderInfo.
*/

// these are non inline to make a possible future addition of a d pointer binary compatible
QMediaFormat::QMediaFormat(FileFormat format)
    : fmt(format)
{
    Q_UNUSED(d);
}

QMediaFormat::~QMediaFormat() = default;
QMediaFormat::QMediaFormat(const QMediaFormat &other) = default;
QMediaFormat &QMediaFormat::operator=(const QMediaFormat &other) = default;


/*! \fn void QMediaFormat::setMediaContainer(QMediaFormat::FileFormat container)

    Sets the container to \a container.

    \sa mediaContainer(), QMediaFormat::FileFormat
*/

/*! \fn QMediaFormat::FileFormat QMediaFormat::mediaContainer() const

    Returns the container used in this format.

    \sa setMediaContainer(), QMediaFormat::FileFormat
*/

/*! \fn void QMediaFormat::setVideoCodec(VideoCodec codec)

    Sets the video codec to \a codec.

    \sa videoCodec(), QMediaFormat::VideoCodec
*/
bool QMediaFormat::setVideoCodec(VideoCodec codec)
{
    if (!videoSupportMatrix[fmt][(int)codec])
        return false;
    video = codec;
    return true;
}

/*! \fn QMediaFormat::VideoCodec QMediaFormat::videoCodec() const

    Returns the video codec used in this format.

    \sa setVideoCodec(), QMediaFormat::VideoCodec
*/

/*! \fn void QMediaFormat::setAudioCodec(AudioCodec codec)

    Sets the audio codec to \a codec.

    \sa audioCodec(), QMediaFormat::AudioCodec
*/
bool QMediaFormat::setAudioCodec(QMediaFormat::AudioCodec codec)
{
    if (!audioSupportMatrix[fmt][(int)codec])
        return false;
    audio = codec;
    return true;
}

/*! \fn QMediaFormat::AudioCodec QMediaFormat::audioCodec() const

    Returns the audio codec used in this format.

    \sa setAudioCodec(), QMediaFormat::AudioCodec
*/

/*! \fn bool QMediaFormat::isValid() const

    Returns true if the format is valid.
*/

/*!
    Returns true if Qt Multimedia can decode this format.

    \sa QMediaDecoderInfo
 */
bool QMediaFormat::canDecode() const
{
    if (!QMediaDecoderInfo::supportedFileFormats().contains(fmt))
        return false;
    if (audio == QMediaFormat::AudioCodec::Unspecified && video == QMediaFormat::VideoCodec::Unspecified)
        return false;
    if (audio != QMediaFormat::AudioCodec::Unspecified) {
        if (!QMediaDecoderInfo::supportedAudioCodecs().contains(audio))
            return false;
    }
    if (video != QMediaFormat::VideoCodec::Unspecified) {
        if (!QMediaDecoderInfo::supportedVideoCodecs().contains(video))
            return false;
    }
    return true;
}

/*!
    Returns true if Qt Multimedia can encode this format.

    \sa QMediaEncoderInfo
*/
bool QMediaFormat::canEncode() const
{
    if (!QMediaEncoderInfo::supportedFileFormats().contains(fmt))
        return false;
    if (audio == QMediaFormat::AudioCodec::Unspecified && video == QMediaFormat::VideoCodec::Unspecified)
        return false;
    if (audio != QMediaFormat::AudioCodec::Unspecified) {
        if (!QMediaEncoderInfo::supportedAudioCodecs().contains(audio))
            return false;
    }
    if (video != QMediaFormat::VideoCodec::Unspecified) {
        if (!QMediaEncoderInfo::supportedVideoCodecs().contains(video))
            return false;
    }
    return true;
}

/*!
    Returns true if is is an audio-only file format.
 */
bool QMediaFormat::isAudioFormat() const
{
    return fmt >= AAC;
}

/*!
    Returns the mimetype for the file format used in this media format.

    \sa format(), setFormat()
 */
QMimeType QMediaFormat::mimeType() const
{
    return QMimeDatabase().mimeTypeForName(QString::fromLatin1(mimeTypeForFormat[fmt + 1]));
}

QString QMediaFormat::fileFormatName(QMediaFormat::FileFormat c)
{
    constexpr const char *descriptions[QMediaFormat::LastFileFormat + 2] = {
        "Unspecified File Format"
        "ASF",
        "AVI",
        "Matroska",
        "MPEG-4",
        "Ogg",
        "QuickTime",
        "WebM",
        // Audio Formats
        "AAC",
        "FLAC",
        "MP3",
        "MPEG-4 Audio",
        "ALAC",
        "Wave"
    };
    return QString::fromUtf8(descriptions[int(c) + 1]);
}

QString QMediaFormat::audioCodecName(QMediaFormat::AudioCodec c)
{
    constexpr const char *descriptions[] = {
        "Invalid",
        "MP3",
        "AAC",
        "AC3",
        "EAC3",
        "FLAC",
        "DolbyTrueHD",
        "Opus",
        "Vorbis",
        "Wave",
        "ALAC",
    };
    return QString::fromUtf8(descriptions[int(c) + 1]);
}

QString QMediaFormat::videoCodecName(QMediaFormat::VideoCodec c)
{
    constexpr const char *descriptions[] = {
        "Invalid",
        "MPEG1",
        "MPEG2",
        "MPEG4",
        "H264",
        "H265",
        "VP8",
        "VP9",
        "AV1",
        "Theora",
        "MotionJPEG"
    };
    return QString::fromUtf8(descriptions[int(c) + 1]);
}

QString QMediaFormat::fileFormatDescription(QMediaFormat::FileFormat c)
{
    constexpr const char *descriptions[QMediaFormat::LastFileFormat + 2] = {
        "Unspecified File Format",
        "Windows Media Format",
        "Audio Video Interleave",
        "Matroska Multimedia Container",
        "MPEG-4 Video Container",
        "Ogg",
        "QuickTime Container",
        "WebM",
        // Audio Formats
        "Advanced Audio Codec (AAC)",
        "Free Lossless Audio Codec (FLAC)",
        "MP3",
        "MPEG-4 Audio Container",
        "Apple Lossless Audio Codec (ALAC)",
        "Wave File"
    };
    return QString::fromUtf8(descriptions[int(c) + 1]);
}

QString QMediaFormat::audioCodecDescription(QMediaFormat::AudioCodec c)
{
    constexpr const char *descriptions[] = {
        "Unspecified Audio Codec",
        "MP3",
        "Advanced Audio Codec (AAC)",
        "Dolby Digital (AC3)",
        "Dolby Digital Plus (E-AC3)",
        "Free Lossless Audio Codec (FLAC)",
        "Dolby True HD",
        "Opus",
        "Vorbis",
        "Wave",
        "Apple Lossless Audio Codec (ALAC)",
    };
    return QString::fromUtf8(descriptions[int(c) + 1]);
}

QString QMediaFormat::videoCodecDescription(QMediaFormat::VideoCodec c)
{
    constexpr const char *descriptions[] = {
        "Unspecified Video Codec",
        "MPEG-1 Video",
        "MPEG-2 Video",
        "MPEG-4 Video",
        "H.264",
        "H.265",
        "VP8",
        "VP9",
        "AV1",
        "Theora",
        "MotionJPEG"
    };
    return QString::fromUtf8(descriptions[int(c) + 1]);
}

/*! \class QMediaDecoderInfo

    QMediaDecoderInfo describes the media formats supported for decoding
    on the current platform.

    Qt Multimedia might be able to decode formats that are not listed
    in the QMediaFormat::FileFormat, QMediaFormat::AudioCodec and QMediaFormat::VideoCodec enums.

    \sa QMediaFormat::canDecode()
*/

/*!
    Returns a list of container formats that are supported for decoding by
    Qt Multimedia.

    This does not imply that Qt can successfully decode the media file or
    stream, as the audio or video codec used within the container might not
    be supported.
 */
QList<QMediaFormat::FileFormat> QMediaDecoderInfo::supportedFileFormats()
{
    return QMediaPlatformIntegration::instance()->formatInfo()->decodableMediaContainers();
}

/*!
    Returns a list of video codecs that are supported for decoding by
    Qt Multimedia.
 */
QList<QMediaFormat::VideoCodec> QMediaDecoderInfo::supportedVideoCodecs()
{
    return QMediaPlatformIntegration::instance()->formatInfo()->decodableVideoCodecs();
}

/*!
    Returns a list of audio codecs that are supported for decoding by
    Qt Multimedia.
 */
QList<QMediaFormat::AudioCodec> QMediaDecoderInfo::supportedAudioCodecs()
{
    return QMediaPlatformIntegration::instance()->formatInfo()->decodableAudioCodecs();
}

/*! \class QMediaEncodecInfo

    QMediaEncoderInfo describes the media formats supported for
    encoding on the current platform.

    \sa QMediaFormat::canEncode()
*/

/*!
    Returns a list of container formats that are supported for encoding by
    Qt Multimedia.
 */
QList<QMediaFormat::FileFormat> QMediaEncoderInfo::supportedFileFormats()
{
    return QMediaPlatformIntegration::instance()->formatInfo()->encodableMediaContainers();
}

/*!
    Returns a list of video codecs that are supported for encoding by
    Qt Multimedia.
 */
QList<QMediaFormat::VideoCodec> QMediaEncoderInfo::supportedVideoCodecs()
{
    return QMediaPlatformIntegration::instance()->formatInfo()->encodableVideoCodecs();
}

/*!
    Returns a list of audio codecs that are supported for encoding by
    Qt Multimedia.
 */
QList<QMediaFormat::AudioCodec> QMediaEncoderInfo::supportedAudioCodecs()
{
    return QMediaPlatformIntegration::instance()->formatInfo()->encodableAudioCodecs();
}

QT_END_NAMESPACE
