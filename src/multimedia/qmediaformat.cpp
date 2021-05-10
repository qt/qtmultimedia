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

/*!
    Returns a list of container formats that are supported for \a mode.

    The list is constrained by the chosen audio and video codec and will only match file
    formats that can be created with those codecs.

    To get all supported file formats, run this query on a default constructed QMediaFormat.
 */
QList<QMediaFormat::FileFormat> QMediaFormat::supportedFileFormats(QMediaFormat::ConversionMode m)
{
    return QPlatformMediaIntegration::instance()->formatInfo()->supportedFileFormats(*this, m);
}

/*!
    Returns a list of video codecs that are supported for \a mode.

    The list is constrained by the chosen file format and audio codec and will only return
    the video codecs that can be used with those settings.

    To get all supported video codecs, run this query on a default constructed QMediaFormat.
 */
QList<QMediaFormat::VideoCodec> QMediaFormat::supportedVideoCodecs(QMediaFormat::ConversionMode m)
{
    return QPlatformMediaIntegration::instance()->formatInfo()->supportedVideoCodecs(*this, m);
}

/*!
    Returns a list of audio codecs that are supported for \a mode.

    The list is constrained by the chosen file format and video codec and will only return
    the audio codecs that can be used with those settings.

    To get all supported audio codecs, run this query on a default constructed QMediaFormat.
 */
QList<QMediaFormat::AudioCodec> QMediaFormat::supportedAudioCodecs(QMediaFormat::ConversionMode m)
{
    return QPlatformMediaIntegration::instance()->formatInfo()->supportedAudioCodecs(*this, m);
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

QT_END_NAMESPACE
