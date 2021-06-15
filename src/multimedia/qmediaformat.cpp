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
#include "private/qplatformmediaintegration_p.h"
#include "private/qplatformmediaformatinfo_p.h"
#include <QtCore/qmimedatabase.h>

QT_BEGIN_NAMESPACE

namespace {

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
    "audio/wav",
};

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

}

class QMediaFormatPrivate : public QSharedData
{};

QT_DEFINE_QESDP_SPECIALIZATION_DTOR(QMediaFormatPrivate);

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
    or decoding using QMediaDecoderInfo or QMediaRecorderInfo.
*/

// these are non inline to make a possible future addition of a d pointer binary compatible

/*!
    Constucts a QMediaFormat object for \a format.
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
    Copies \a other into this QMediaFormat object.
*/
QMediaFormat &QMediaFormat::operator=(const QMediaFormat &other) noexcept = default;

/*! \fn QMediaFormat::QMediaFormat(QMediaFormat &&other)

    Constructs a QMediaFormat objects by moving from \a other.
*/

/*! \fn QMediaFormat &QMediaFormat::operator=(QMediaFormat &&other)

    Moves \a other into this QMediaFormat objects.
*/

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

/*! \fn bool QMediaFormat::isValid() const

    Returns true if the format is valid.
*/

/*!
    Returns true if Qt Multimedia can encode or decode this format,
    depending on \a mode.

    \sa QMediaDecoderInfo
 */
bool QMediaFormat::isSupported(ConversionMode mode) const
{
    return QPlatformMediaIntegration::instance()->formatInfo()->isSupported(*this, mode);
}

/*!
    Returns the mimetype for the file format used in this media format.

    \sa format(), setFormat()
 */
QMimeType QMediaFormat::mimeType() const
{
    return QMimeDatabase().mimeTypeForName(QString::fromLatin1(mimeTypeForFormat[fmt + 1]));
}

static QPlatformMediaFormatInfo *formatInfo()
{
    QPlatformMediaFormatInfo *result = nullptr;
    if (auto *pi = QPlatformMediaIntegration::instance())
        result = pi->formatInfo();
    return result;
}

/*!
    Returns a list of container formats that are supported for \a mode.

    The list is constrained by the chosen audio and video codec and will only match file
    formats that can be created with those codecs.

    To get all supported file formats, run this query on a default constructed QMediaFormat.
 */
QList<QMediaFormat::FileFormat> QMediaFormat::supportedFileFormats(QMediaFormat::ConversionMode m)
{
    auto *fi = formatInfo();
    return fi != nullptr ? fi->supportedFileFormats(*this, m) : QList<QMediaFormat::FileFormat>{};
}

/*!
    Returns a list of video codecs that are supported for \a mode.

    The list is constrained by the chosen file format and audio codec and will only return
    the video codecs that can be used with those settings.

    To get all supported video codecs, run this query on a default constructed QMediaFormat.
 */
QList<QMediaFormat::VideoCodec> QMediaFormat::supportedVideoCodecs(QMediaFormat::ConversionMode m)
{
    auto *fi = formatInfo();
    return fi != nullptr ? fi->supportedVideoCodecs(*this, m) : QList<QMediaFormat::VideoCodec>{};
}

/*!
    Returns a list of audio codecs that are supported for \a mode.

    The list is constrained by the chosen file format and video codec and will only return
    the audio codecs that can be used with those settings.

    To get all supported audio codecs, run this query on a default constructed QMediaFormat.
 */
QList<QMediaFormat::AudioCodec> QMediaFormat::supportedAudioCodecs(QMediaFormat::ConversionMode m)
{
    auto *fi = formatInfo();
    return fi != nullptr ? fi->supportedAudioCodecs(*this, m) : QList<QMediaFormat::AudioCodec>{};
}

QString QMediaFormat::fileFormatName(QMediaFormat::FileFormat c)
{
    constexpr const char *descriptions[QMediaFormat::LastFileFormat + 2] = {
        "Unspecified",
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

bool QMediaFormat::operator==(const QMediaFormat &other) const
{
    Q_ASSERT(!d);
    return fmt == other.fmt &&
            audio == other.audio &&
           video == other.video;
}

/*!
    Resolves the format to a format that is supported by QMediaRecorder.

    This method tries to find the best possible match for unspecified settings.
    Settings that are not supported by the encoder will be modified to the closest
    match that is supported.
 */
void QMediaFormat::resolveForEncoding(ResolveFlags flags)
{
    const bool requiresVideo = (flags & ResolveFlags::RequiresVideo) != 0;

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
    if ((flags == NoFlags) || !supportedVideoCodecs.contains(video))
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
    if (fmt == QMediaFormat::UnspecifiedFormat)
        return;

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

QT_END_NAMESPACE
