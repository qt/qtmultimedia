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

#ifndef QMEDIAFORMAT_H
#define QMEDIAFORMAT_H

#include <QtCore/qsharedpointer.h>
#include <QtMultimedia/qtmultimediaglobal.h>

QT_BEGIN_NAMESPACE

class QMimeType;
class QMediaFormat;
class QMediaEncoderSettings;
class QMediaFormatPrivate;

QT_DECLARE_QESDP_SPECIALIZATION_DTOR_WITH_EXPORT(QMediaFormatPrivate, Q_MULTIMEDIA_EXPORT)

class Q_MULTIMEDIA_EXPORT QMediaFormat
{
    Q_GADGET
    Q_PROPERTY(FileFormat fileFormat READ fileFormat WRITE setFileFormat)
    Q_PROPERTY(AudioCodec audioCodec READ audioCodec WRITE setAudioCodec)
    Q_PROPERTY(VideoCodec videoCodec READ videoCodec WRITE setVideoCodec)
    Q_ENUMS(FileFormat)
    Q_ENUMS(AudioCodec)
    Q_ENUMS(VideoCodec)
public:
    enum FileFormat {
        UnspecifiedFormat = -1,
        // Video Formats
        ASF,
        AVI,
        Matroska,
        MPEG4,
        Ogg,
        QuickTime,
        WebM,
        // Audio Formats
        AAC,
        FLAC,
        MP3,
        Mpeg4Audio,
        ALAC,
        Wave,
        LastFileFormat = Wave
    };

    enum class AudioCodec {
        Unspecified = -1,
        MP3,
        AAC,
        AC3,
        EAC3,
        FLAC,
        DolbyTrueHD,
        Opus,
        Vorbis,
        Wave,
        ALAC,
        LastAudioCodec = ALAC
    };

    enum class VideoCodec {
        Unspecified = -1,
        MPEG1,
        MPEG2,
        MPEG4,
        H264,
        H265,
        VP8,
        VP9,
        AV1,
        Theora,
        MotionJPEG,
        LastVideoCodec = MotionJPEG
    };

    enum ConversionMode {
        Encode,
        Decode
    };

    enum ResolveFlags
    {
        NoFlags,
        RequiresVideo
    };

    QMediaFormat(FileFormat format = UnspecifiedFormat);
    ~QMediaFormat();
    QMediaFormat(const QMediaFormat &other) noexcept;
    QMediaFormat &operator=(const QMediaFormat &other) noexcept;

    QMediaFormat(QMediaFormat &&other) noexcept = default;
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_PURE_SWAP(QMediaFormat)
    void swap(QMediaFormat &other) noexcept
    { qSwap(d, other.d); }

    FileFormat fileFormat() const { return fmt; }
    void setFileFormat(FileFormat f) { fmt = f; }

    void setVideoCodec(VideoCodec codec) { video = codec; }
    VideoCodec videoCodec() const { return video; }

    void setAudioCodec(AudioCodec codec) { audio = codec; }
    AudioCodec audioCodec() const { return audio; }

    bool isSupported(ConversionMode mode) const;

    QMimeType mimeType() const;

    QList<QMediaFormat::FileFormat> supportedFileFormats(ConversionMode m);
    QList<QMediaFormat::VideoCodec> supportedVideoCodecs(ConversionMode m);
    QList<QMediaFormat::AudioCodec> supportedAudioCodecs(ConversionMode m);

    static QString fileFormatName(QMediaFormat::FileFormat c);
    static QString audioCodecName(QMediaFormat::AudioCodec c);
    static QString videoCodecName(QMediaFormat::VideoCodec c);

    static QString fileFormatDescription(QMediaFormat::FileFormat c);
    static QString audioCodecDescription(QMediaFormat::AudioCodec c);
    static QString videoCodecDescription(QMediaFormat::VideoCodec c);

    bool operator==(const QMediaFormat &other) const;
    bool operator!=(const QMediaFormat &other) const
    { return !operator==(other); }

    void resolveForEncoding(ResolveFlags flags);

protected:
    friend class QMediaFormatPrivate;
    FileFormat fmt;
    AudioCodec audio = AudioCodec::Unspecified;
    VideoCodec video = VideoCodec::Unspecified;
    QExplicitlySharedDataPointer<QMediaFormatPrivate> d;
};

QT_END_NAMESPACE

#endif
