// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only


#include "qwindowsmultimediautils_p.h"

#include <QtCore/qt_windows.h>
#include <QtCore/qvarlengtharray.h>
#include <QtCore/private/qflatmap_p.h>

#include <mfapi.h>
#include <mfidl.h>

#include <cstring>

static_assert(WINVER >= _WIN32_WINNT_WIN10, "Win10 required for newer audio formats.");

QT_BEGIN_NAMESPACE

namespace QWMF {

namespace {

struct GuidLess
{
    bool operator()(const GUID &lhs, const GUID &rhs) const
    {
        return std::memcmp(&lhs, &rhs, sizeof(GUID)) < 0;
    }
};

} // namespace

QVideoFrameFormat::PixelFormat pixelFormatFromMediaSubtype(const GUID &subtype)
{
    static const QVarLengthFlatMap<GUID, QVideoFrameFormat::PixelFormat, 13, GuidLess> map({
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
        { MFVideoFormat_ARGB32, QVideoFrameFormat::Format_BGRA8888 },
        { MFVideoFormat_RGB32, QVideoFrameFormat::Format_BGRX8888 },
#else
        { MFVideoFormat_ARGB32, QVideoFrameFormat::Format_ARGB8888 },
        { MFVideoFormat_RGB32, QVideoFrameFormat::Format_XRGB8888 },
#endif
        { MFVideoFormat_AYUV, QVideoFrameFormat::Format_AYUV },
        { MFVideoFormat_I420, QVideoFrameFormat::Format_YUV420P },
        { MFVideoFormat_UYVY, QVideoFrameFormat::Format_UYVY },
        { MFVideoFormat_YV12, QVideoFrameFormat::Format_YV12 },
        { MFVideoFormat_NV12, QVideoFrameFormat::Format_NV12 },
        { MFVideoFormat_YUY2, QVideoFrameFormat::Format_YUYV },
        { MFVideoFormat_P010, QVideoFrameFormat::Format_P010 },
        { MFVideoFormat_P016, QVideoFrameFormat::Format_P016 },
        { MFVideoFormat_L8, QVideoFrameFormat::Format_Y8 },
        { MFVideoFormat_L16, QVideoFrameFormat::Format_Y16 },
        { MFVideoFormat_MJPG, QVideoFrameFormat::Format_Jpeg },
    });

    const auto it = map.find(subtype);
    return it != map.end() ? it->second : QVideoFrameFormat::Format_Invalid;
}

QVideoFrameFormat::ColorRange colorRangeFromNominalRange(UINT32 nominalRange)
{
    switch (nominalRange) {
    case MFNominalRange_0_255:
        return QVideoFrameFormat::ColorRange_Full;
    case MFNominalRange_16_235:
        return QVideoFrameFormat::ColorRange_Video;
    default:
        return QVideoFrameFormat::ColorRange_Unknown;
    }
}

QVideoFrameFormat::ColorSpace colorSpaceFromMatrix(UINT32 yuvMatrix)
{
    switch (yuvMatrix) {
    case MFVideoTransferMatrix_BT709:
        return QVideoFrameFormat::ColorSpace_BT709;
    case MFVideoTransferMatrix_BT601:
        return QVideoFrameFormat::ColorSpace_BT601;
    case MFVideoTransferMatrix_BT2020_10:
    case MFVideoTransferMatrix_BT2020_12:
        return QVideoFrameFormat::ColorSpace_BT2020;
    default:
        return QVideoFrameFormat::ColorSpace_Undefined;
    }
}

GUID videoFormatForCodec(QMediaFormat::VideoCodec codec)
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

QMediaFormat::VideoCodec codecForVideoFormat(GUID format)
{
    static const QVarLengthFlatMap<GUID, QMediaFormat::VideoCodec, 15, GuidLess> map({
        { MFVideoFormat_MPG1, QMediaFormat::VideoCodec::MPEG1 },
        { MFVideoFormat_MPEG2, QMediaFormat::VideoCodec::MPEG2 },
        { MFVideoFormat_MP4V, QMediaFormat::VideoCodec::MPEG4 },
        { MFVideoFormat_M4S2, QMediaFormat::VideoCodec::MPEG4 },
        { MFVideoFormat_MP4S, QMediaFormat::VideoCodec::MPEG4 },
        { MFVideoFormat_MP43, QMediaFormat::VideoCodec::MPEG4 },
        { MFVideoFormat_H264, QMediaFormat::VideoCodec::H264 },
        { MFVideoFormat_H265, QMediaFormat::VideoCodec::H265 },
        { MFVideoFormat_VP80, QMediaFormat::VideoCodec::VP8 },
        { MFVideoFormat_VP90, QMediaFormat::VideoCodec::VP9 },
        { MFVideoFormat_AV1, QMediaFormat::VideoCodec::AV1 },
        { MFVideoFormat_WMV1, QMediaFormat::VideoCodec::WMV },
        { MFVideoFormat_WMV2, QMediaFormat::VideoCodec::WMV },
        { MFVideoFormat_WMV3, QMediaFormat::VideoCodec::WMV },
        { MFVideoFormat_MJPG, QMediaFormat::VideoCodec::MotionJPEG },
    });

    const auto it = map.find(format);
    return it != map.end() ? it->second : QMediaFormat::VideoCodec::Unspecified;
}

GUID audioFormatForCodec(QMediaFormat::AudioCodec codec)
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

QMediaFormat::AudioCodec codecForAudioFormat(GUID format)
{
    static const QVarLengthFlatMap<GUID, QMediaFormat::AudioCodec, 12, GuidLess> map({
        { MFAudioFormat_MP3, QMediaFormat::AudioCodec::MP3 },
        { MFAudioFormat_AAC, QMediaFormat::AudioCodec::AAC },
        { MFAudioFormat_ALAC, QMediaFormat::AudioCodec::ALAC },
        { MFAudioFormat_FLAC, QMediaFormat::AudioCodec::FLAC },
        { MFAudioFormat_Vorbis, QMediaFormat::AudioCodec::Vorbis },
        { MFAudioFormat_PCM, QMediaFormat::AudioCodec::Wave },
        { MFAudioFormat_Opus, QMediaFormat::AudioCodec::Opus },
        { MFAudioFormat_Dolby_AC3, QMediaFormat::AudioCodec::AC3 },
        { MFAudioFormat_Dolby_DDPlus, QMediaFormat::AudioCodec::EAC3 },
        { MFAudioFormat_WMAudioV8, QMediaFormat::AudioCodec::WMA },
        { MFAudioFormat_WMAudioV9, QMediaFormat::AudioCodec::WMA },
        { MFAudioFormat_WMAudio_Lossless, QMediaFormat::AudioCodec::WMA },
    });

    const auto it = map.find(format);
    return it != map.end() ? it->second : QMediaFormat::AudioCodec::Unspecified;
}

std::optional<GUID> containerForVideoFileFormat(QMediaFormat::FileFormat format)
{
    switch (format) {
    case QMediaFormat::FileFormat::MPEG4:
        return MFTranscodeContainerType_MPEG4;
    case QMediaFormat::FileFormat::WMV:
        return MFTranscodeContainerType_ASF;
    case QMediaFormat::FileFormat::AVI:
        return MFTranscodeContainerType_AVI;
    default:
        return std::nullopt;
    }
}

std::optional<GUID> containerForAudioFileFormat(QMediaFormat::FileFormat format)
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
        return std::nullopt;
    }
}

} // namespace QWMF

QT_END_NAMESPACE
