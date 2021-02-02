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

#include "qdarwinformatsinfo_p.h"
#include <AVFoundation/AVFoundation.h>
#include <qdebug.h>

QT_BEGIN_NAMESPACE

static struct {
    const char *name;
    QMediaFormat::FileFormat value;
} mediaContainerMap[] = {
    { "video/x-ms-asf", QMediaFormat::ASF },
    { "video/avi", QMediaFormat::AVI },
    { "video/x-matroska", QMediaFormat::Matroska },
    { "video/mp4", QMediaFormat::MPEG4 },
    { "video/quicktime", QMediaFormat::QuickTime },
    { "video/ogg", QMediaFormat::Ogg },
    { "audio/mp3", QMediaFormat::MP3 },
    { nullptr, QMediaFormat::UnspecifiedFormat }
};

static struct {
    const char *name;
    QMediaFormat::VideoCodec value;
} videoCodecMap[] = {
    // See CMVideoCodecType for the four character code names of codecs
    { "video/mp4; codecs=\"mp1v\"", QMediaFormat::VideoCodec::MPEG1 },
    { "video/mp4; codecs=\"mp2v\"", QMediaFormat::VideoCodec::MPEG2 },
    { "video/mp4; codecs=\"mp4v\"", QMediaFormat::VideoCodec::MPEG4 },
    { "video/mp4; codecs=\"avc1\"", QMediaFormat::VideoCodec::H264 },
    { "video/mp4; codecs=\"hvc1\"", QMediaFormat::VideoCodec::H265 },
    { "video/mp4; codecs=\"vp09\"", QMediaFormat::VideoCodec::VP9 },
    { "video/mp4; codecs=\"av01\"", QMediaFormat::VideoCodec::AV1 }, // ### ????
    { "video/mp4; codecs=\"jpeg\"", QMediaFormat::VideoCodec::MotionJPEG },
    { nullptr, QMediaFormat::VideoCodec::Unspecified }
};

static struct {
    const char *name;
    QMediaFormat::AudioCodec value;
} audioCodecMap[] = {
    // AudioFile.h
    { "audio/mp3", QMediaFormat::AudioCodec::MP3 },
    { "audio/aac", QMediaFormat::AudioCodec::AAC },
    { "video/mp4; codecs=\"ac-3\"", QMediaFormat::AudioCodec::AC3 },
    { "video/mp4; codecs=\"ec-3\"", QMediaFormat::AudioCodec::EAC3 },
    { "audio/flac", QMediaFormat::AudioCodec::FLAC },
    { nullptr, QMediaFormat::AudioCodec::Unspecified },
};

QDarwinFormatInfo::QDarwinFormatInfo()
{
    auto avtypes = [AVURLAsset audiovisualMIMETypes];
    for (AVFileType filetype in avtypes) {
        auto *m = mediaContainerMap;
        while (m->name) {
            if (!strcmp(filetype.UTF8String, m->name)) {
                m_decodableMediaContainers.append(m->value);
                break;
            }
            ++m;
        }
    }

    {
        auto *m = videoCodecMap;
        while (m->name) {
            if ([AVURLAsset isPlayableExtendedMIMEType:[NSString stringWithUTF8String:m->name]])
                m_decodableVideoCodecs << m->value;
            ++m;
        }
    }
    {
        auto *m = audioCodecMap;
        while (m->name) {
            qDebug() << "audio" << m->name << [AVURLAsset isPlayableExtendedMIMEType:[NSString stringWithUTF8String:m->name]];
            if ([AVURLAsset isPlayableExtendedMIMEType:[NSString stringWithUTF8String:m->name]])
                m_decodableAudioCodecs << m->value;
            ++m;
        }
    }
    // Audio format seems to be symmetric
    m_encodableAudioCodecs = m_decodableAudioCodecs;

    // ### Haven't seen a good way to figure this out.
    // seems AVFoundation only supports those for encoding
    m_encodableMediaContainers << QMediaFormat::MPEG4 << QMediaFormat::QuickTime;
    // AVCaptureVideoDataOutput.availableVideoCodecTypes does not mention H265 even though it is supported
    m_encodableVideoCodecs << QMediaFormat::VideoCodec::H264 << QMediaFormat::VideoCodec::H265 << QMediaFormat::VideoCodec::MotionJPEG;
}

QDarwinFormatInfo::~QDarwinFormatInfo()
{
}

QList<QMediaFormat::FileFormat> QDarwinFormatInfo::decodableMediaContainers() const
{
    return m_decodableMediaContainers;
}

QList<QMediaFormat::AudioCodec> QDarwinFormatInfo::decodableAudioCodecs() const
{
    return m_decodableAudioCodecs;
}

QList<QMediaFormat::VideoCodec> QDarwinFormatInfo::decodableVideoCodecs() const
{
    return m_decodableVideoCodecs;
}

QList<QMediaFormat::FileFormat> QDarwinFormatInfo::encodableMediaContainers() const
{
    return m_encodableMediaContainers;
}

QList<QMediaFormat::AudioCodec> QDarwinFormatInfo::encodableAudioCodecs() const
{
    return m_encodableAudioCodecs;
}

QList<QMediaFormat::VideoCodec> QDarwinFormatInfo::encodableVideoCodecs() const
{
    return m_encodableVideoCodecs;
}

int QDarwinFormatInfo::audioFormatForCodec(QMediaFormat::AudioCodec codec)
{
    int codecId = kAudioFormatMPEG4AAC;
    switch (codec) {
    case QMediaFormat::AudioCodec::Unspecified:
    case QMediaFormat::AudioCodec::DolbyTrueHD:
    case QMediaFormat::AudioCodec::Opus:
    case QMediaFormat::AudioCodec::Vorbis:
    case QMediaFormat::AudioCodec::Wave:
    case QMediaFormat::AudioCodec::WindowsMediaAudio:
        // Unsupported, shouldn't happen. Fall back to AAC
    case QMediaFormat::AudioCodec::AAC:
        codecId = kAudioFormatMPEG4AAC;
        break;
    case QMediaFormat::AudioCodec::MP3:
        codecId = kAudioFormatMPEGLayer3;
        break;
    case QMediaFormat::AudioCodec::AC3:
        codecId = kAudioFormatAC3;
        break;
    case QMediaFormat::AudioCodec::EAC3:
        codecId = kAudioFormatEnhancedAC3;
        break;
    case QMediaFormat::AudioCodec::FLAC:
        codecId = kAudioFormatFLAC;
        break;
    }
    return codecId;
}

NSString *QDarwinFormatInfo::videoFormatForCodec(QMediaFormat::VideoCodec codec)
{
    const char *c = "hvc1"; // fallback is H265
    switch (codec) {
    case QMediaFormat::VideoCodec::Unspecified:
    case QMediaFormat::VideoCodec::VP8:
    case QMediaFormat::VideoCodec::H265:
    case QMediaFormat::VideoCodec::AV1:
    case QMediaFormat::VideoCodec::Theora:
        break;

    case QMediaFormat::VideoCodec::MPEG1:
        c = "mp1v";
        break;
    case QMediaFormat::VideoCodec::MPEG2:
        c = "mp2v";
        break;
    case QMediaFormat::VideoCodec::MPEG4:
        c = "mp4v";
        break;
    case QMediaFormat::VideoCodec::H264:
        c = "avc1";
        break;
    case QMediaFormat::VideoCodec::VP9:
        c = "vp09";
        break;
    case QMediaFormat::VideoCodec::MotionJPEG:
        c = "jpeg";
    }
    return [NSString stringWithUTF8String:c];
}
QT_END_NAMESPACE
