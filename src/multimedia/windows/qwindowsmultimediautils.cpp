// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/qt_windows.h>
static_assert(WINVER >= _WIN32_WINNT_WIN10, "Win10 required for newer audio formats.");

#include "qwindowsmultimediautils_p.h"

#include <mfapi.h>
#include <mfidl.h>

QT_BEGIN_NAMESPACE

QVideoFrameFormat::PixelFormat QWindowsMultimediaUtils::pixelFormatFromMediaSubtype(const GUID &subtype)
{
    if (subtype == MFVideoFormat_ARGB32)
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
        return QVideoFrameFormat::Format_BGRA8888;
#else
        return QVideoFrameFormat::Format_ARGB8888;
#endif
    if (subtype == MFVideoFormat_RGB32)
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
        return QVideoFrameFormat::Format_BGRX8888;
#else
        return QVideoFrameFormat::Format_XRGB8888;
#endif
    if (subtype == MFVideoFormat_AYUV)
        return QVideoFrameFormat::Format_AYUV;
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
    if (subtype == MFVideoFormat_MJPG)
        return QVideoFrameFormat::Format_Jpeg;

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
    case QMediaFormat::VideoCodec::WMV:
        return MFVideoFormat_WMV3;
    case QMediaFormat::VideoCodec::MotionJPEG:
        return MFVideoFormat_MJPG;
    default:
        return MFVideoFormat_H264;
    }
}

QMediaFormat::VideoCodec QWindowsMultimediaUtils::codecForVideoFormat(GUID format)
{
    if (format == MFVideoFormat_MPG1)
        return QMediaFormat::VideoCodec::MPEG1;
    if (format == MFVideoFormat_MPEG2)
        return QMediaFormat::VideoCodec::MPEG2;
    if (format == MFVideoFormat_MP4V
            || format == MFVideoFormat_M4S2
            || format == MFVideoFormat_MP4S
            || format == MFVideoFormat_MP43)
        return QMediaFormat::VideoCodec::MPEG4;
    if (format == MFVideoFormat_H264)
        return QMediaFormat::VideoCodec::H264;
    if (format == MFVideoFormat_H265)
        return QMediaFormat::VideoCodec::H265;
    if (format == MFVideoFormat_VP80)
        return QMediaFormat::VideoCodec::VP8;
    if (format == MFVideoFormat_VP90)
        return QMediaFormat::VideoCodec::VP9;
    if (format == MFVideoFormat_AV1)
        return QMediaFormat::VideoCodec::AV1;
    if (format == MFVideoFormat_WMV1
            || format == MFVideoFormat_WMV2
            || format == MFVideoFormat_WMV3)
        return QMediaFormat::VideoCodec::WMV;
    if (format == MFVideoFormat_MJPG)
        return QMediaFormat::VideoCodec::MotionJPEG;
    return QMediaFormat::VideoCodec::Unspecified;
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
    case QMediaFormat::AudioCodec::WMA:
        return MFAudioFormat_WMAudioV9;
    default:
        return MFAudioFormat_AAC;
    }
}

QMediaFormat::AudioCodec QWindowsMultimediaUtils::codecForAudioFormat(GUID format)
{
    if (format == MFAudioFormat_MP3)
        return QMediaFormat::AudioCodec::MP3;
    if (format == MFAudioFormat_AAC)
        return QMediaFormat::AudioCodec::AAC;
    if (format == MFAudioFormat_ALAC)
        return QMediaFormat::AudioCodec::ALAC;
    if (format == MFAudioFormat_FLAC)
        return QMediaFormat::AudioCodec::FLAC;
    if (format == MFAudioFormat_Vorbis)
        return QMediaFormat::AudioCodec::Vorbis;
    if (format == MFAudioFormat_PCM)
        return QMediaFormat::AudioCodec::Wave;
    if (format == MFAudioFormat_Opus)
        return QMediaFormat::AudioCodec::Opus;
    if (format == MFAudioFormat_Dolby_AC3)
        return QMediaFormat::AudioCodec::AC3;
    if (format == MFAudioFormat_Dolby_DDPlus)
        return QMediaFormat::AudioCodec::EAC3;
    if (format == MFAudioFormat_WMAudioV8
            || format == MFAudioFormat_WMAudioV9
            || format == MFAudioFormat_WMAudio_Lossless)
        return QMediaFormat::AudioCodec::WMA;
    return QMediaFormat::AudioCodec::Unspecified;
}

GUID QWindowsMultimediaUtils::containerForVideoFileFormat(QMediaFormat::FileFormat format)
{
    switch (format) {
    case QMediaFormat::FileFormat::MPEG4:
        return MFTranscodeContainerType_MPEG4;
    case QMediaFormat::FileFormat::WMV:
        return MFTranscodeContainerType_ASF;
    case QMediaFormat::FileFormat::AVI:
        return MFTranscodeContainerType_AVI;
    default:
        return MFTranscodeContainerType_MPEG4;
    }
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
    case QMediaFormat::FileFormat::WMA:
        return MFTranscodeContainerType_ASF;
    case QMediaFormat::FileFormat::FLAC:
        return MFTranscodeContainerType_FLAC;
    case QMediaFormat::FileFormat::Wave:
        return MFTranscodeContainerType_WAVE;
    default:
        return MFTranscodeContainerType_MPEG4;
    }
}

QT_END_NAMESPACE
