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

#include "qwindowsmultimediautils_p.h"

#include <mfapi.h>
#include <mfidl.h>

QT_BEGIN_NAMESPACE

QVideoFrameFormat::PixelFormat QWindowsMultimediaUtils::pixelFormatFromMediaSubtype(const GUID &subtype)
{
    if (subtype == MFVideoFormat_ARGB32)
        return QVideoFrameFormat::Format_ARGB32;
    if (subtype == MFVideoFormat_RGB32)
        return QVideoFrameFormat::Format_RGB32;
    if (subtype == MFVideoFormat_AYUV)
        return QVideoFrameFormat::Format_AYUV444;
    if (subtype == MFVideoFormat_I420)
        return QVideoFrameFormat::Format_YUV420P;
    if (subtype == MFVideoFormat_UYVY)
        return QVideoFrameFormat::Format_UYVY;
    if (subtype == MFVideoFormat_YV12)
        return QVideoFrameFormat::Format_YV12;
    if (subtype == MFVideoFormat_NV12)
        return QVideoFrameFormat::Format_NV12;
    if (subtype == MFVideoFormat_YUY2)
        return QVideoFrameFormat::Format_YUYV;
    if (subtype == MFVideoFormat_P010)
        return QVideoFrameFormat::Format_P010;
    if (subtype == MFVideoFormat_P016)
        return QVideoFrameFormat::Format_P016;
    if (subtype == MFVideoFormat_L8)
        return QVideoFrameFormat::Format_Y8;
    if (subtype == MFVideoFormat_L16)
        return QVideoFrameFormat::Format_Y16;

    return QVideoFrameFormat::Format_Invalid;
}

GUID QWindowsMultimediaUtils::videoFormatForCodec(QMediaFormat::VideoCodec codec)
{
    switch (codec) {
    case QMediaFormat::VideoCodec::MPEG1:
        return MFVideoFormat_MPG1;
    case QMediaFormat::VideoCodec::MPEG2:
        return MFVideoFormat_MPEG2;
    case QMediaFormat::VideoCodec::MPEG4:
        return MFVideoFormat_MP4V;
    case QMediaFormat::VideoCodec::H264:
        return MFVideoFormat_H264;
    case QMediaFormat::VideoCodec::H265:
        return MFVideoFormat_H265;
    case QMediaFormat::VideoCodec::VP8:
        return MFVideoFormat_VP80;
    case QMediaFormat::VideoCodec::VP9:
        return MFVideoFormat_VP90;
    case QMediaFormat::VideoCodec::AV1:
        return MFVideoFormat_AV1;
    case QMediaFormat::VideoCodec::MotionJPEG:
        return MFVideoFormat_MJPG;
    }
    // Use H264 as the default video codec
    return MFVideoFormat_H264;
}

GUID QWindowsMultimediaUtils::audioFormatForCodec(QMediaFormat::AudioCodec codec)
{
    switch (codec) {
    case QMediaFormat::AudioCodec::MP3:
        return MFAudioFormat_MP3;
    case QMediaFormat::AudioCodec::AAC:
        return MFAudioFormat_AAC;
    case QMediaFormat::AudioCodec::ALAC:
        return MFAudioFormat_ALAC;
    case QMediaFormat::AudioCodec::FLAC:
        return MFAudioFormat_FLAC;
    case QMediaFormat::AudioCodec::Vorbis:
        return MFAudioFormat_Vorbis;
    case QMediaFormat::AudioCodec::Wave:
        return MFAudioFormat_PCM;
    case QMediaFormat::AudioCodec::Opus:
        return MFAudioFormat_Opus;
    case QMediaFormat::AudioCodec::AC3:
        return MFAudioFormat_Dolby_AC3;
    case QMediaFormat::AudioCodec::EAC3:
        return MFAudioFormat_Dolby_DDPlus;
    }
    // Use AAC as the default audio codec
    return MFAudioFormat_AAC;
}

GUID QWindowsMultimediaUtils::containerForVideoFileFormat(QMediaFormat::FileFormat format)
{
    switch (format) {
    case QMediaFormat::FileFormat::MPEG4:
        return MFTranscodeContainerType_MPEG4;
    case QMediaFormat::FileFormat::ASF:
        return MFTranscodeContainerType_ASF;
    case QMediaFormat::FileFormat::AVI:
        return MFTranscodeContainerType_AVI;
    }
    return MFTranscodeContainerType_MPEG4;
}

GUID QWindowsMultimediaUtils::containerForAudioFileFormat(QMediaFormat::FileFormat format)
{
    switch (format) {
    case QMediaFormat::FileFormat::MP3:
        return MFTranscodeContainerType_MP3;
    case QMediaFormat::FileFormat::AAC:
        return MFTranscodeContainerType_ADTS;
    case QMediaFormat::FileFormat::Mpeg4Audio:
        return MFTranscodeContainerType_MPEG4;
    case QMediaFormat::FileFormat::ALAC:
        return MFTranscodeContainerType_MPEG4;
    case QMediaFormat::FileFormat::FLAC:
        return MFTranscodeContainerType_FLAC;
    case QMediaFormat::FileFormat::Wave:
        return MFTranscodeContainerType_WAVE;
    }
    return MFTranscodeContainerType_MPEG4;
}

QT_END_NAMESPACE
