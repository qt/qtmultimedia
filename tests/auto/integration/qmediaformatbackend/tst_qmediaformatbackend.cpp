// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QtMultimedia/qmediaformat.h>
#include <QtMultimedia/private/qplatformmediaintegration_p.h>
#include <private/mediabackendutils_p.h>
#include <private/formatutils_p.h>
#include <private/osdetection_p.h>
#include <set>
#include <map>

QT_USE_NAMESPACE

using namespace Qt::StringLiterals;

std::set<QMediaFormat::VideoCodec> supportedVideoEncoders(QMediaFormat::FileFormat fileFormat)
{
    std::map<QMediaFormat::FileFormat, std::set<QMediaFormat::VideoCodec>> videoEncoders;

    // Audio formats don't support any video encoders
    videoEncoders[QMediaFormat::FileFormat::Mpeg4Audio] = {};
    videoEncoders[QMediaFormat::FileFormat::AAC] = {};
    videoEncoders[QMediaFormat::FileFormat::WMA] = {};
    videoEncoders[QMediaFormat::FileFormat::MP3] = {};
    videoEncoders[QMediaFormat::FileFormat::FLAC] = {};
    videoEncoders[QMediaFormat::FileFormat::Wave] = {};

    // Ogg and WebM is not supported for encoding on any platform
    videoEncoders[QMediaFormat::FileFormat::Ogg] = {};
    videoEncoders[QMediaFormat::FileFormat::WebM] = {};

    if constexpr (isWindows) {
        videoEncoders[QMediaFormat::FileFormat::WMV] = {
            QMediaFormat::VideoCodec::MPEG1,      QMediaFormat::VideoCodec::MPEG2,
            QMediaFormat::VideoCodec::MPEG4,      QMediaFormat::VideoCodec::H264,
            QMediaFormat::VideoCodec::MotionJPEG,
        };
        videoEncoders[QMediaFormat::FileFormat::AVI] = {
            QMediaFormat::VideoCodec::MPEG1,      QMediaFormat::VideoCodec::MPEG2,
            QMediaFormat::VideoCodec::MPEG4,      QMediaFormat::VideoCodec::H264,
            QMediaFormat::VideoCodec::MotionJPEG,
        };
        videoEncoders[QMediaFormat::FileFormat::Matroska] = {
            QMediaFormat::VideoCodec::MPEG1,
            QMediaFormat::VideoCodec::MPEG2,
            QMediaFormat::VideoCodec::MPEG4,
            QMediaFormat::VideoCodec::MotionJPEG,
        };
        videoEncoders[QMediaFormat::FileFormat::MPEG4] = {
            QMediaFormat::VideoCodec::MPEG1,      QMediaFormat::VideoCodec::MPEG2,
            QMediaFormat::VideoCodec::MPEG4,      QMediaFormat::VideoCodec::H264,
            QMediaFormat::VideoCodec::MotionJPEG,
        };
        videoEncoders[QMediaFormat::FileFormat::QuickTime] = {
            QMediaFormat::VideoCodec::MPEG1,      QMediaFormat::VideoCodec::MPEG2,
            QMediaFormat::VideoCodec::MPEG4,      QMediaFormat::VideoCodec::H264,
            QMediaFormat::VideoCodec::MotionJPEG,
        };
    } else if constexpr (isAndroid) {
        videoEncoders[QMediaFormat::FileFormat::WMV] = {
            QMediaFormat::VideoCodec::MPEG1,
            QMediaFormat::VideoCodec::MPEG2,
            QMediaFormat::VideoCodec::MPEG4,
            QMediaFormat::VideoCodec::MotionJPEG,
        };
        videoEncoders[QMediaFormat::FileFormat::AVI] = {
            QMediaFormat::VideoCodec::MPEG1,
            QMediaFormat::VideoCodec::MPEG2,
            QMediaFormat::VideoCodec::MPEG4,
            QMediaFormat::VideoCodec::MotionJPEG,
        };
        videoEncoders[QMediaFormat::FileFormat::Matroska] = {
            QMediaFormat::VideoCodec::MPEG1,
            QMediaFormat::VideoCodec::MPEG2,
            QMediaFormat::VideoCodec::MPEG4,
            QMediaFormat::VideoCodec::MotionJPEG,
        };
        videoEncoders[QMediaFormat::FileFormat::MPEG4] = {
            QMediaFormat::VideoCodec::MPEG1,
            QMediaFormat::VideoCodec::MPEG2,
            QMediaFormat::VideoCodec::MPEG4,
            QMediaFormat::VideoCodec::MotionJPEG,
        };
        videoEncoders[QMediaFormat::FileFormat::QuickTime] = {
            QMediaFormat::VideoCodec::MPEG1,
            QMediaFormat::VideoCodec::MPEG2,
            QMediaFormat::VideoCodec::MPEG4,
            QMediaFormat::VideoCodec::MotionJPEG,
        };
    } else if constexpr (isLinux) {
        videoEncoders[QMediaFormat::FileFormat::WMV] = {
            QMediaFormat::VideoCodec::MPEG1,
            QMediaFormat::VideoCodec::MPEG2,
            QMediaFormat::VideoCodec::MPEG4,
            QMediaFormat::VideoCodec::MotionJPEG,
        };
        videoEncoders[QMediaFormat::FileFormat::AVI] = {
            QMediaFormat::VideoCodec::MPEG1,
            QMediaFormat::VideoCodec::MPEG2,
            QMediaFormat::VideoCodec::MPEG4,
            QMediaFormat::VideoCodec::MotionJPEG,
        };
        videoEncoders[QMediaFormat::FileFormat::Matroska] = {
            QMediaFormat::VideoCodec::MPEG1,
            QMediaFormat::VideoCodec::MPEG2,
            QMediaFormat::VideoCodec::MPEG4,
            QMediaFormat::VideoCodec::MotionJPEG,
        };
        videoEncoders[QMediaFormat::FileFormat::MPEG4] = {
            QMediaFormat::VideoCodec::MPEG1,
            QMediaFormat::VideoCodec::MPEG2,
            QMediaFormat::VideoCodec::MPEG4,
            QMediaFormat::VideoCodec::MotionJPEG,
        };
        videoEncoders[QMediaFormat::FileFormat::QuickTime] = {
            QMediaFormat::VideoCodec::MPEG1,
            QMediaFormat::VideoCodec::MPEG2,
            QMediaFormat::VideoCodec::MPEG4,
            QMediaFormat::VideoCodec::MotionJPEG,
        };
    } else if constexpr (isMacOS) {
        videoEncoders[QMediaFormat::FileFormat::WMV] = {
            QMediaFormat::VideoCodec::MPEG1,      QMediaFormat::VideoCodec::MPEG2,
            QMediaFormat::VideoCodec::MPEG4,      QMediaFormat::VideoCodec::H264,
            QMediaFormat::VideoCodec::MotionJPEG,
        };
        videoEncoders[QMediaFormat::FileFormat::AVI] = {
            QMediaFormat::VideoCodec::MPEG1,      QMediaFormat::VideoCodec::MPEG2,
            QMediaFormat::VideoCodec::MPEG4,      QMediaFormat::VideoCodec::H264,
            QMediaFormat::VideoCodec::MotionJPEG,
        };
        videoEncoders[QMediaFormat::FileFormat::Matroska] = {
            QMediaFormat::VideoCodec::MPEG1,
            QMediaFormat::VideoCodec::MPEG2,
            QMediaFormat::VideoCodec::MPEG4,
            QMediaFormat::VideoCodec::MotionJPEG,
        };
        videoEncoders[QMediaFormat::FileFormat::MPEG4] = {
            QMediaFormat::VideoCodec::MPEG1, QMediaFormat::VideoCodec::MPEG2,
            QMediaFormat::VideoCodec::MPEG4, QMediaFormat::VideoCodec::H264,
            QMediaFormat::VideoCodec::H265,  QMediaFormat::VideoCodec::MotionJPEG,
        };
        videoEncoders[QMediaFormat::FileFormat::QuickTime] = {
            QMediaFormat::VideoCodec::MPEG1, QMediaFormat::VideoCodec::MPEG2,
            QMediaFormat::VideoCodec::MPEG4, QMediaFormat::VideoCodec::H264,
            QMediaFormat::VideoCodec::H265,  QMediaFormat::VideoCodec::MotionJPEG,
        };
    }

    return videoEncoders[fileFormat];
}

std::set<QMediaFormat::VideoCodec> supportedVideoDecoders(QMediaFormat::FileFormat fileFormat)
{
    std::map<QMediaFormat::FileFormat, std::set<QMediaFormat::VideoCodec>> videoDecoders;

    if constexpr (isWindows) {
        videoDecoders[QMediaFormat::FileFormat::WMV] = {
            QMediaFormat::VideoCodec::MPEG1,      QMediaFormat::VideoCodec::MPEG2,
            QMediaFormat::VideoCodec::MPEG4,      QMediaFormat::VideoCodec::H264,
            QMediaFormat::VideoCodec::MotionJPEG,
        };
        videoDecoders[QMediaFormat::FileFormat::AVI] = {
            QMediaFormat::VideoCodec::MPEG1,      QMediaFormat::VideoCodec::MPEG2,
            QMediaFormat::VideoCodec::MPEG4,      QMediaFormat::VideoCodec::H264,
            QMediaFormat::VideoCodec::MotionJPEG,
        };
        videoDecoders[QMediaFormat::FileFormat::Matroska] = {
            QMediaFormat::VideoCodec::MPEG1, QMediaFormat::VideoCodec::MPEG2,
            QMediaFormat::VideoCodec::MPEG4, QMediaFormat::VideoCodec::H264,
            QMediaFormat::VideoCodec::H265,  QMediaFormat::VideoCodec::MotionJPEG,
        };
        videoDecoders[QMediaFormat::FileFormat::MPEG4] = {
            QMediaFormat::VideoCodec::MPEG1, QMediaFormat::VideoCodec::MPEG2,
            QMediaFormat::VideoCodec::MPEG4, QMediaFormat::VideoCodec::H264,
            QMediaFormat::VideoCodec::H265,  QMediaFormat::VideoCodec::MotionJPEG,
        };
        videoDecoders[QMediaFormat::FileFormat::Ogg] = {};
        videoDecoders[QMediaFormat::FileFormat::QuickTime] = {
            QMediaFormat::VideoCodec::MPEG1, QMediaFormat::VideoCodec::MPEG2,
            QMediaFormat::VideoCodec::MPEG4, QMediaFormat::VideoCodec::H264,
            QMediaFormat::VideoCodec::H265,  QMediaFormat::VideoCodec::MotionJPEG,
        };
        videoDecoders[QMediaFormat::FileFormat::WebM] = {};
        videoDecoders[QMediaFormat::FileFormat::Mpeg4Audio] = {};
        videoDecoders[QMediaFormat::FileFormat::AAC] = {
            QMediaFormat::VideoCodec::WMV,
        };
        videoDecoders[QMediaFormat::FileFormat::WMA] = {};
        videoDecoders[QMediaFormat::FileFormat::MP3] = {};
        videoDecoders[QMediaFormat::FileFormat::FLAC] = {};
        videoDecoders[QMediaFormat::FileFormat::Wave] = {};
    } else if constexpr (isAndroid) {
        videoDecoders[QMediaFormat::FileFormat::WMV] = {
            QMediaFormat::VideoCodec::MPEG1,
            QMediaFormat::VideoCodec::MPEG2,
            QMediaFormat::VideoCodec::MPEG4,
            QMediaFormat::VideoCodec::MotionJPEG,
        };
        videoDecoders[QMediaFormat::FileFormat::AVI] = {
            QMediaFormat::VideoCodec::MPEG1,
            QMediaFormat::VideoCodec::MPEG2,
            QMediaFormat::VideoCodec::MPEG4,
            QMediaFormat::VideoCodec::MotionJPEG,
        };
        videoDecoders[QMediaFormat::FileFormat::Matroska] = {
            QMediaFormat::VideoCodec::MPEG1,
            QMediaFormat::VideoCodec::MPEG2,
            QMediaFormat::VideoCodec::MPEG4,
            QMediaFormat::VideoCodec::MotionJPEG,
        };
        videoDecoders[QMediaFormat::FileFormat::MPEG4] = {
            QMediaFormat::VideoCodec::MPEG1,
            QMediaFormat::VideoCodec::MPEG2,
            QMediaFormat::VideoCodec::MPEG4,
            QMediaFormat::VideoCodec::MotionJPEG,
        };
        videoDecoders[QMediaFormat::FileFormat::Ogg] = {};
        videoDecoders[QMediaFormat::FileFormat::QuickTime] = {
            QMediaFormat::VideoCodec::MPEG1,
            QMediaFormat::VideoCodec::MPEG2,
            QMediaFormat::VideoCodec::MPEG4,
            QMediaFormat::VideoCodec::MotionJPEG,
        };
        videoDecoders[QMediaFormat::FileFormat::WebM] = {};
        videoDecoders[QMediaFormat::FileFormat::Mpeg4Audio] = {};
        videoDecoders[QMediaFormat::FileFormat::AAC] = {
            QMediaFormat::VideoCodec::WMV,
        };
        videoDecoders[QMediaFormat::FileFormat::WMA] = {};
        videoDecoders[QMediaFormat::FileFormat::MP3] = {};
        videoDecoders[QMediaFormat::FileFormat::FLAC] = {};
        videoDecoders[QMediaFormat::FileFormat::Wave] = {};
    } else if constexpr (isLinux) {
        videoDecoders[QMediaFormat::FileFormat::WMV] = {
            QMediaFormat::VideoCodec::MPEG1,
            QMediaFormat::VideoCodec::MPEG2,
            QMediaFormat::VideoCodec::MPEG4,
            QMediaFormat::VideoCodec::MotionJPEG,
        };
        videoDecoders[QMediaFormat::FileFormat::AVI] = {
            QMediaFormat::VideoCodec::MPEG1,
            QMediaFormat::VideoCodec::MPEG2,
            QMediaFormat::VideoCodec::MPEG4,
            QMediaFormat::VideoCodec::MotionJPEG,
        };
        videoDecoders[QMediaFormat::FileFormat::Matroska] = {
            QMediaFormat::VideoCodec::MPEG1,
            QMediaFormat::VideoCodec::MPEG2,
            QMediaFormat::VideoCodec::MPEG4,
            QMediaFormat::VideoCodec::MotionJPEG,
        };
        videoDecoders[QMediaFormat::FileFormat::MPEG4] = {
            QMediaFormat::VideoCodec::MPEG1,
            QMediaFormat::VideoCodec::MPEG2,
            QMediaFormat::VideoCodec::MPEG4,
            QMediaFormat::VideoCodec::MotionJPEG,
        };
        videoDecoders[QMediaFormat::FileFormat::Ogg] = {};
        videoDecoders[QMediaFormat::FileFormat::QuickTime] = {
            QMediaFormat::VideoCodec::MPEG1,
            QMediaFormat::VideoCodec::MPEG2,
            QMediaFormat::VideoCodec::MPEG4,
            QMediaFormat::VideoCodec::MotionJPEG,
        };
        videoDecoders[QMediaFormat::FileFormat::WebM] = {};
        videoDecoders[QMediaFormat::FileFormat::Mpeg4Audio] = {};
        videoDecoders[QMediaFormat::FileFormat::AAC] = {
            QMediaFormat::VideoCodec::WMV,
        };
        videoDecoders[QMediaFormat::FileFormat::WMA] = {};
        videoDecoders[QMediaFormat::FileFormat::MP3] = {};
        videoDecoders[QMediaFormat::FileFormat::FLAC] = {};
        videoDecoders[QMediaFormat::FileFormat::Wave] = {};
    } else if constexpr (isMacOS) {
        videoDecoders[QMediaFormat::FileFormat::WMV] = {
            QMediaFormat::VideoCodec::MPEG1,      QMediaFormat::VideoCodec::MPEG2,
            QMediaFormat::VideoCodec::MPEG4,      QMediaFormat::VideoCodec::H264,
            QMediaFormat::VideoCodec::MotionJPEG,
        };
        videoDecoders[QMediaFormat::FileFormat::AVI] = {
            QMediaFormat::VideoCodec::MPEG1,      QMediaFormat::VideoCodec::MPEG2,
            QMediaFormat::VideoCodec::MPEG4,      QMediaFormat::VideoCodec::H264,
            QMediaFormat::VideoCodec::MotionJPEG,
        };
        videoDecoders[QMediaFormat::FileFormat::Matroska] = {
            QMediaFormat::VideoCodec::MPEG1, QMediaFormat::VideoCodec::MPEG2,
            QMediaFormat::VideoCodec::MPEG4, QMediaFormat::VideoCodec::H264,
            QMediaFormat::VideoCodec::H265,  QMediaFormat::VideoCodec::MotionJPEG,
        };
        videoDecoders[QMediaFormat::FileFormat::MPEG4] = {
            QMediaFormat::VideoCodec::MPEG1, QMediaFormat::VideoCodec::MPEG2,
            QMediaFormat::VideoCodec::MPEG4, QMediaFormat::VideoCodec::H264,
            QMediaFormat::VideoCodec::H265,  QMediaFormat::VideoCodec::MotionJPEG,
        };
        videoDecoders[QMediaFormat::FileFormat::Ogg] = {};
        videoDecoders[QMediaFormat::FileFormat::QuickTime] = {
            QMediaFormat::VideoCodec::MPEG1, QMediaFormat::VideoCodec::MPEG2,
            QMediaFormat::VideoCodec::MPEG4, QMediaFormat::VideoCodec::H264,
            QMediaFormat::VideoCodec::H265,  QMediaFormat::VideoCodec::MotionJPEG,
        };
        videoDecoders[QMediaFormat::FileFormat::WebM] = {};
        videoDecoders[QMediaFormat::FileFormat::Mpeg4Audio] = {};
        videoDecoders[QMediaFormat::FileFormat::AAC] = {
            QMediaFormat::VideoCodec::WMV,
        };
        videoDecoders[QMediaFormat::FileFormat::WMA] = {};
        videoDecoders[QMediaFormat::FileFormat::MP3] = {};
        videoDecoders[QMediaFormat::FileFormat::FLAC] = {};
        videoDecoders[QMediaFormat::FileFormat::Wave] = {};
    }
    return videoDecoders[fileFormat];
}

std::set<QMediaFormat::AudioCodec> supportedAudioEncoders(QMediaFormat::FileFormat fileFormat)
{
    std::map<QMediaFormat::FileFormat, std::set<QMediaFormat::AudioCodec>> audioEncoders;

    if constexpr (isWindows) {
        audioEncoders[QMediaFormat::FileFormat::WMV] = {
            QMediaFormat::AudioCodec::MP3,  QMediaFormat::AudioCodec::AAC,
            QMediaFormat::AudioCodec::AC3,  QMediaFormat::AudioCodec::EAC3,
            QMediaFormat::AudioCodec::FLAC, QMediaFormat::AudioCodec::Wave,
        };
        audioEncoders[QMediaFormat::FileFormat::AVI] = {
            QMediaFormat::AudioCodec::MP3,  QMediaFormat::AudioCodec::AAC,
            QMediaFormat::AudioCodec::AC3,  QMediaFormat::AudioCodec::EAC3,
            QMediaFormat::AudioCodec::FLAC, QMediaFormat::AudioCodec::Wave,
        };
        audioEncoders[QMediaFormat::FileFormat::Matroska] = {
            QMediaFormat::AudioCodec::MP3,  QMediaFormat::AudioCodec::AAC,
            QMediaFormat::AudioCodec::AC3,  QMediaFormat::AudioCodec::EAC3,
            QMediaFormat::AudioCodec::FLAC, QMediaFormat::AudioCodec::Wave,
            QMediaFormat::AudioCodec::ALAC,
        };
        audioEncoders[QMediaFormat::FileFormat::MPEG4] = {
            QMediaFormat::AudioCodec::MP3,  QMediaFormat::AudioCodec::AAC,
            QMediaFormat::AudioCodec::AC3,  QMediaFormat::AudioCodec::EAC3,
            QMediaFormat::AudioCodec::FLAC, QMediaFormat::AudioCodec::Wave,
            QMediaFormat::AudioCodec::ALAC,
        };
        audioEncoders[QMediaFormat::FileFormat::Ogg] = {
            QMediaFormat::AudioCodec::FLAC,
        };
        audioEncoders[QMediaFormat::FileFormat::QuickTime] = {
            QMediaFormat::AudioCodec::MP3,  QMediaFormat::AudioCodec::AAC,
            QMediaFormat::AudioCodec::AC3,  QMediaFormat::AudioCodec::EAC3,
            QMediaFormat::AudioCodec::FLAC, QMediaFormat::AudioCodec::Wave,
            QMediaFormat::AudioCodec::ALAC,
        };
        audioEncoders[QMediaFormat::FileFormat::WebM] = {};
        audioEncoders[QMediaFormat::FileFormat::Mpeg4Audio] = {
            QMediaFormat::AudioCodec::MP3,  QMediaFormat::AudioCodec::AAC,
            QMediaFormat::AudioCodec::AC3,  QMediaFormat::AudioCodec::EAC3,
            QMediaFormat::AudioCodec::FLAC, QMediaFormat::AudioCodec::Wave,
            QMediaFormat::AudioCodec::ALAC,
        };
        audioEncoders[QMediaFormat::FileFormat::AAC] = {
            QMediaFormat::AudioCodec::AAC,
        };
        audioEncoders[QMediaFormat::FileFormat::WMA] = {
            QMediaFormat::AudioCodec::MP3,  QMediaFormat::AudioCodec::AAC,
            QMediaFormat::AudioCodec::AC3,  QMediaFormat::AudioCodec::EAC3,
            QMediaFormat::AudioCodec::FLAC, QMediaFormat::AudioCodec::Wave,
        };
        audioEncoders[QMediaFormat::FileFormat::MP3] = {
            QMediaFormat::AudioCodec::MP3,
        };
        audioEncoders[QMediaFormat::FileFormat::FLAC] = {
            QMediaFormat::AudioCodec::FLAC,
        };
        audioEncoders[QMediaFormat::FileFormat::Wave] = {
            QMediaFormat::AudioCodec::Wave,
        };
    } else if constexpr (isAndroid) {
        audioEncoders[QMediaFormat::FileFormat::WMV] = {
            QMediaFormat::AudioCodec::AAC,  QMediaFormat::AudioCodec::AC3,
            QMediaFormat::AudioCodec::EAC3, QMediaFormat::AudioCodec::FLAC,
            QMediaFormat::AudioCodec::Wave,
        };
        audioEncoders[QMediaFormat::FileFormat::AVI] = {
            QMediaFormat::AudioCodec::AAC,  QMediaFormat::AudioCodec::AC3,
            QMediaFormat::AudioCodec::EAC3, QMediaFormat::AudioCodec::FLAC,
            QMediaFormat::AudioCodec::Wave,
        };
        audioEncoders[QMediaFormat::FileFormat::Matroska] = {
            QMediaFormat::AudioCodec::AAC,  QMediaFormat::AudioCodec::AC3,
            QMediaFormat::AudioCodec::EAC3, QMediaFormat::AudioCodec::FLAC,
            QMediaFormat::AudioCodec::Wave, QMediaFormat::AudioCodec::ALAC,
        };
        audioEncoders[QMediaFormat::FileFormat::MPEG4] = {
            QMediaFormat::AudioCodec::AAC,  QMediaFormat::AudioCodec::AC3,
            QMediaFormat::AudioCodec::EAC3, QMediaFormat::AudioCodec::FLAC,
            QMediaFormat::AudioCodec::Wave, QMediaFormat::AudioCodec::ALAC,
        };
        audioEncoders[QMediaFormat::FileFormat::Ogg] = {
            QMediaFormat::AudioCodec::FLAC,
        };
        audioEncoders[QMediaFormat::FileFormat::QuickTime] = {
            QMediaFormat::AudioCodec::AAC,  QMediaFormat::AudioCodec::AC3,
            QMediaFormat::AudioCodec::EAC3, QMediaFormat::AudioCodec::FLAC,
            QMediaFormat::AudioCodec::Wave, QMediaFormat::AudioCodec::ALAC,
        };
        audioEncoders[QMediaFormat::FileFormat::WebM] = {};
        audioEncoders[QMediaFormat::FileFormat::Mpeg4Audio] = {
            QMediaFormat::AudioCodec::AAC,  QMediaFormat::AudioCodec::AC3,
            QMediaFormat::AudioCodec::EAC3, QMediaFormat::AudioCodec::FLAC,
            QMediaFormat::AudioCodec::Wave, QMediaFormat::AudioCodec::ALAC,
        };
        audioEncoders[QMediaFormat::FileFormat::AAC] = {
            QMediaFormat::AudioCodec::AAC,
        };
        audioEncoders[QMediaFormat::FileFormat::WMA] = {
            QMediaFormat::AudioCodec::AAC,  QMediaFormat::AudioCodec::AC3,
            QMediaFormat::AudioCodec::EAC3, QMediaFormat::AudioCodec::FLAC,
            QMediaFormat::AudioCodec::Wave,
        };
        audioEncoders[QMediaFormat::FileFormat::MP3] = {};
        audioEncoders[QMediaFormat::FileFormat::FLAC] = {
            QMediaFormat::AudioCodec::FLAC,
        };
        audioEncoders[QMediaFormat::FileFormat::Wave] = {
            QMediaFormat::AudioCodec::Wave,
        };
    } else if constexpr (isLinux) {
        audioEncoders[QMediaFormat::FileFormat::WMV] = {
            QMediaFormat::AudioCodec::AAC,  QMediaFormat::AudioCodec::AC3,
            QMediaFormat::AudioCodec::EAC3, QMediaFormat::AudioCodec::FLAC,
            QMediaFormat::AudioCodec::Wave,
        };
        audioEncoders[QMediaFormat::FileFormat::AVI] = {
            QMediaFormat::AudioCodec::AAC,  QMediaFormat::AudioCodec::AC3,
            QMediaFormat::AudioCodec::EAC3, QMediaFormat::AudioCodec::FLAC,
            QMediaFormat::AudioCodec::Wave,
        };
        audioEncoders[QMediaFormat::FileFormat::Matroska] = {
            QMediaFormat::AudioCodec::AAC,  QMediaFormat::AudioCodec::AC3,
            QMediaFormat::AudioCodec::EAC3, QMediaFormat::AudioCodec::FLAC,
            QMediaFormat::AudioCodec::Wave, QMediaFormat::AudioCodec::ALAC,
        };
        audioEncoders[QMediaFormat::FileFormat::MPEG4] = {
            QMediaFormat::AudioCodec::AAC,  QMediaFormat::AudioCodec::AC3,
            QMediaFormat::AudioCodec::EAC3, QMediaFormat::AudioCodec::FLAC,
            QMediaFormat::AudioCodec::Wave, QMediaFormat::AudioCodec::ALAC,
        };
        audioEncoders[QMediaFormat::FileFormat::Ogg] = {
            QMediaFormat::AudioCodec::FLAC,
        };
        audioEncoders[QMediaFormat::FileFormat::QuickTime] = {
            QMediaFormat::AudioCodec::AAC,  QMediaFormat::AudioCodec::AC3,
            QMediaFormat::AudioCodec::EAC3, QMediaFormat::AudioCodec::FLAC,
            QMediaFormat::AudioCodec::Wave, QMediaFormat::AudioCodec::ALAC,
        };
        audioEncoders[QMediaFormat::FileFormat::WebM] = {};
        audioEncoders[QMediaFormat::FileFormat::Mpeg4Audio] = {
            QMediaFormat::AudioCodec::AAC,  QMediaFormat::AudioCodec::AC3,
            QMediaFormat::AudioCodec::EAC3, QMediaFormat::AudioCodec::FLAC,
            QMediaFormat::AudioCodec::Wave, QMediaFormat::AudioCodec::ALAC,
        };
        audioEncoders[QMediaFormat::FileFormat::AAC] = {
            QMediaFormat::AudioCodec::AAC,
        };
        audioEncoders[QMediaFormat::FileFormat::WMA] = {
            QMediaFormat::AudioCodec::AAC,  QMediaFormat::AudioCodec::AC3,
            QMediaFormat::AudioCodec::EAC3, QMediaFormat::AudioCodec::FLAC,
            QMediaFormat::AudioCodec::Wave,
        };
        audioEncoders[QMediaFormat::FileFormat::MP3] = {};
        audioEncoders[QMediaFormat::FileFormat::FLAC] = {
            QMediaFormat::AudioCodec::FLAC,
        };
        audioEncoders[QMediaFormat::FileFormat::Wave] = {
            QMediaFormat::AudioCodec::Wave,
        };
    } else if constexpr (isMacOS) {
        audioEncoders[QMediaFormat::FileFormat::WMV] = {
            QMediaFormat::AudioCodec::AAC,  QMediaFormat::AudioCodec::AC3,
            QMediaFormat::AudioCodec::EAC3, QMediaFormat::AudioCodec::FLAC,
            QMediaFormat::AudioCodec::Wave,
        };
        audioEncoders[QMediaFormat::FileFormat::AVI] = {
            QMediaFormat::AudioCodec::AAC,  QMediaFormat::AudioCodec::AC3,
            QMediaFormat::AudioCodec::EAC3, QMediaFormat::AudioCodec::FLAC,
            QMediaFormat::AudioCodec::Wave,
        };
        audioEncoders[QMediaFormat::FileFormat::Matroska] = {
            QMediaFormat::AudioCodec::AAC,  QMediaFormat::AudioCodec::AC3,
            QMediaFormat::AudioCodec::EAC3, QMediaFormat::AudioCodec::FLAC,
            QMediaFormat::AudioCodec::Wave, QMediaFormat::AudioCodec::ALAC,
        };
        audioEncoders[QMediaFormat::FileFormat::MPEG4] = {
            QMediaFormat::AudioCodec::AAC,  QMediaFormat::AudioCodec::AC3,
            QMediaFormat::AudioCodec::EAC3, QMediaFormat::AudioCodec::FLAC,
            QMediaFormat::AudioCodec::Wave, QMediaFormat::AudioCodec::ALAC,
        };
        audioEncoders[QMediaFormat::FileFormat::Ogg] = {
            QMediaFormat::AudioCodec::FLAC,
        };
        audioEncoders[QMediaFormat::FileFormat::QuickTime] = {
            QMediaFormat::AudioCodec::AAC,  QMediaFormat::AudioCodec::AC3,
            QMediaFormat::AudioCodec::EAC3, QMediaFormat::AudioCodec::FLAC,
            QMediaFormat::AudioCodec::Wave, QMediaFormat::AudioCodec::ALAC,
        };
        audioEncoders[QMediaFormat::FileFormat::WebM] = {};
        audioEncoders[QMediaFormat::FileFormat::Mpeg4Audio] = {
            QMediaFormat::AudioCodec::AAC,  QMediaFormat::AudioCodec::AC3,
            QMediaFormat::AudioCodec::EAC3, QMediaFormat::AudioCodec::FLAC,
            QMediaFormat::AudioCodec::Wave, QMediaFormat::AudioCodec::ALAC,
        };
        audioEncoders[QMediaFormat::FileFormat::AAC] = {
            QMediaFormat::AudioCodec::AAC,
        };
        audioEncoders[QMediaFormat::FileFormat::WMA] = {
            QMediaFormat::AudioCodec::AAC,  QMediaFormat::AudioCodec::AC3,
            QMediaFormat::AudioCodec::EAC3, QMediaFormat::AudioCodec::FLAC,
            QMediaFormat::AudioCodec::Wave,
        };
        audioEncoders[QMediaFormat::FileFormat::MP3] = {};
        audioEncoders[QMediaFormat::FileFormat::FLAC] = {
            QMediaFormat::AudioCodec::FLAC,
        };
        audioEncoders[QMediaFormat::FileFormat::Wave] = {
            QMediaFormat::AudioCodec::Wave,
        };
    }
    return audioEncoders[fileFormat];
}

std::set<QMediaFormat::AudioCodec> supportedAudioDecoders(QMediaFormat::FileFormat fileFormat)
{
    std::map<QMediaFormat::FileFormat, std::set<QMediaFormat::AudioCodec>> audioDecoders;

    if constexpr (isWindows) {
        audioDecoders[QMediaFormat::FileFormat::WMV] = {
            QMediaFormat::AudioCodec::MP3,  QMediaFormat::AudioCodec::AAC,
            QMediaFormat::AudioCodec::AC3,  QMediaFormat::AudioCodec::EAC3,
            QMediaFormat::AudioCodec::FLAC, QMediaFormat::AudioCodec::Wave,
        };
        audioDecoders[QMediaFormat::FileFormat::AVI] = {
            QMediaFormat::AudioCodec::MP3,  QMediaFormat::AudioCodec::AAC,
            QMediaFormat::AudioCodec::AC3,  QMediaFormat::AudioCodec::EAC3,
            QMediaFormat::AudioCodec::FLAC, QMediaFormat::AudioCodec::Wave,
        };
        audioDecoders[QMediaFormat::FileFormat::Matroska] = {
            QMediaFormat::AudioCodec::MP3,  QMediaFormat::AudioCodec::AAC,
            QMediaFormat::AudioCodec::AC3,  QMediaFormat::AudioCodec::EAC3,
            QMediaFormat::AudioCodec::FLAC, QMediaFormat::AudioCodec::Wave,
            QMediaFormat::AudioCodec::ALAC,
        };
        audioDecoders[QMediaFormat::FileFormat::MPEG4] = {
            QMediaFormat::AudioCodec::MP3,  QMediaFormat::AudioCodec::AAC,
            QMediaFormat::AudioCodec::AC3,  QMediaFormat::AudioCodec::EAC3,
            QMediaFormat::AudioCodec::FLAC, QMediaFormat::AudioCodec::Wave,
            QMediaFormat::AudioCodec::WMA,  QMediaFormat::AudioCodec::ALAC,
        };
        audioDecoders[QMediaFormat::FileFormat::Ogg] = {
            QMediaFormat::AudioCodec::FLAC,
        };
        audioDecoders[QMediaFormat::FileFormat::QuickTime] = {
            QMediaFormat::AudioCodec::MP3,  QMediaFormat::AudioCodec::AAC,
            QMediaFormat::AudioCodec::AC3,  QMediaFormat::AudioCodec::EAC3,
            QMediaFormat::AudioCodec::FLAC, QMediaFormat::AudioCodec::Wave,
            QMediaFormat::AudioCodec::ALAC,
        };
        audioDecoders[QMediaFormat::FileFormat::WebM] = {};
        audioDecoders[QMediaFormat::FileFormat::Mpeg4Audio] = {
            QMediaFormat::AudioCodec::MP3,  QMediaFormat::AudioCodec::AAC,
            QMediaFormat::AudioCodec::AC3,  QMediaFormat::AudioCodec::EAC3,
            QMediaFormat::AudioCodec::FLAC, QMediaFormat::AudioCodec::Wave,
            QMediaFormat::AudioCodec::ALAC,
        };
        audioDecoders[QMediaFormat::FileFormat::AAC] = {
            QMediaFormat::AudioCodec::AAC,
            QMediaFormat::AudioCodec::WMA,
        };
        audioDecoders[QMediaFormat::FileFormat::WMA] = {
            QMediaFormat::AudioCodec::MP3,  QMediaFormat::AudioCodec::AAC,
            QMediaFormat::AudioCodec::AC3,  QMediaFormat::AudioCodec::EAC3,
            QMediaFormat::AudioCodec::FLAC, QMediaFormat::AudioCodec::Wave,
        };
        audioDecoders[QMediaFormat::FileFormat::MP3] = {
            QMediaFormat::AudioCodec::MP3,
        };
        audioDecoders[QMediaFormat::FileFormat::FLAC] = {
            QMediaFormat::AudioCodec::FLAC,
        };
        audioDecoders[QMediaFormat::FileFormat::Wave] = {
            QMediaFormat::AudioCodec::Wave,
        };
    } else if constexpr (isAndroid) {
        audioDecoders[QMediaFormat::FileFormat::WMV] = {
            QMediaFormat::AudioCodec::AAC,  QMediaFormat::AudioCodec::AC3,
            QMediaFormat::AudioCodec::EAC3, QMediaFormat::AudioCodec::FLAC,
            QMediaFormat::AudioCodec::Wave,
        };
        audioDecoders[QMediaFormat::FileFormat::AVI] = {
            QMediaFormat::AudioCodec::AAC,  QMediaFormat::AudioCodec::AC3,
            QMediaFormat::AudioCodec::EAC3, QMediaFormat::AudioCodec::FLAC,
            QMediaFormat::AudioCodec::Wave,
        };
        audioDecoders[QMediaFormat::FileFormat::Matroska] = {
            QMediaFormat::AudioCodec::AAC,  QMediaFormat::AudioCodec::AC3,
            QMediaFormat::AudioCodec::EAC3, QMediaFormat::AudioCodec::FLAC,
            QMediaFormat::AudioCodec::Wave, QMediaFormat::AudioCodec::ALAC,
        };
        audioDecoders[QMediaFormat::FileFormat::MPEG4] = {
            QMediaFormat::AudioCodec::AAC,  QMediaFormat::AudioCodec::AC3,
            QMediaFormat::AudioCodec::EAC3, QMediaFormat::AudioCodec::FLAC,
            QMediaFormat::AudioCodec::Wave, QMediaFormat::AudioCodec::ALAC,
        };
        audioDecoders[QMediaFormat::FileFormat::Ogg] = {
            QMediaFormat::AudioCodec::FLAC,
            QMediaFormat::AudioCodec::WMA,
        };
        audioDecoders[QMediaFormat::FileFormat::QuickTime] = {
            QMediaFormat::AudioCodec::AAC,  QMediaFormat::AudioCodec::AC3,
            QMediaFormat::AudioCodec::EAC3, QMediaFormat::AudioCodec::FLAC,
            QMediaFormat::AudioCodec::Wave, QMediaFormat::AudioCodec::ALAC,
        };
        audioDecoders[QMediaFormat::FileFormat::WebM] = {};
        audioDecoders[QMediaFormat::FileFormat::Mpeg4Audio] = {
            QMediaFormat::AudioCodec::AAC,  QMediaFormat::AudioCodec::AC3,
            QMediaFormat::AudioCodec::EAC3, QMediaFormat::AudioCodec::FLAC,
            QMediaFormat::AudioCodec::Wave, QMediaFormat::AudioCodec::ALAC,
        };
        audioDecoders[QMediaFormat::FileFormat::AAC] = {
            QMediaFormat::AudioCodec::AAC,
            QMediaFormat::AudioCodec::WMA,
        };
        audioDecoders[QMediaFormat::FileFormat::WMA] = {
            QMediaFormat::AudioCodec::AAC,  QMediaFormat::AudioCodec::AC3,
            QMediaFormat::AudioCodec::EAC3, QMediaFormat::AudioCodec::FLAC,
            QMediaFormat::AudioCodec::Wave,
        };
        audioDecoders[QMediaFormat::FileFormat::MP3] = {};
        audioDecoders[QMediaFormat::FileFormat::FLAC] = {
            QMediaFormat::AudioCodec::FLAC,
        };
        audioDecoders[QMediaFormat::FileFormat::Wave] = {
            QMediaFormat::AudioCodec::Wave,
        };
    } else if constexpr (isLinux) {
        audioDecoders[QMediaFormat::FileFormat::WMV] = {
            QMediaFormat::AudioCodec::AAC,  QMediaFormat::AudioCodec::AC3,
            QMediaFormat::AudioCodec::EAC3, QMediaFormat::AudioCodec::FLAC,
            QMediaFormat::AudioCodec::Wave,
        };
        audioDecoders[QMediaFormat::FileFormat::AVI] = {
            QMediaFormat::AudioCodec::AAC,  QMediaFormat::AudioCodec::AC3,
            QMediaFormat::AudioCodec::EAC3, QMediaFormat::AudioCodec::FLAC,
            QMediaFormat::AudioCodec::Wave,
        };
        audioDecoders[QMediaFormat::FileFormat::Matroska] = {
            QMediaFormat::AudioCodec::AAC,  QMediaFormat::AudioCodec::AC3,
            QMediaFormat::AudioCodec::EAC3, QMediaFormat::AudioCodec::FLAC,
            QMediaFormat::AudioCodec::Wave, QMediaFormat::AudioCodec::ALAC,
        };
        audioDecoders[QMediaFormat::FileFormat::MPEG4] = {
            QMediaFormat::AudioCodec::AAC,  QMediaFormat::AudioCodec::AC3,
            QMediaFormat::AudioCodec::EAC3, QMediaFormat::AudioCodec::FLAC,
            QMediaFormat::AudioCodec::Wave, QMediaFormat::AudioCodec::ALAC,
        };
        audioDecoders[QMediaFormat::FileFormat::Ogg] = {
            QMediaFormat::AudioCodec::FLAC,
            QMediaFormat::AudioCodec::WMA,
        };
        audioDecoders[QMediaFormat::FileFormat::QuickTime] = {
            QMediaFormat::AudioCodec::AAC,  QMediaFormat::AudioCodec::AC3,
            QMediaFormat::AudioCodec::EAC3, QMediaFormat::AudioCodec::FLAC,
            QMediaFormat::AudioCodec::Wave, QMediaFormat::AudioCodec::ALAC,
        };
        audioDecoders[QMediaFormat::FileFormat::WebM] = {};
        audioDecoders[QMediaFormat::FileFormat::Mpeg4Audio] = {
            QMediaFormat::AudioCodec::AAC,  QMediaFormat::AudioCodec::AC3,
            QMediaFormat::AudioCodec::EAC3, QMediaFormat::AudioCodec::FLAC,
            QMediaFormat::AudioCodec::Wave, QMediaFormat::AudioCodec::ALAC,
        };
        audioDecoders[QMediaFormat::FileFormat::AAC] = {
            QMediaFormat::AudioCodec::AAC,
            QMediaFormat::AudioCodec::WMA,
        };
        audioDecoders[QMediaFormat::FileFormat::WMA] = {
            QMediaFormat::AudioCodec::AAC,  QMediaFormat::AudioCodec::AC3,
            QMediaFormat::AudioCodec::EAC3, QMediaFormat::AudioCodec::FLAC,
            QMediaFormat::AudioCodec::Wave,
        };
        audioDecoders[QMediaFormat::FileFormat::MP3] = {};
        audioDecoders[QMediaFormat::FileFormat::FLAC] = {
            QMediaFormat::AudioCodec::FLAC,
        };
        audioDecoders[QMediaFormat::FileFormat::Wave] = {
            QMediaFormat::AudioCodec::Wave,
        };
    } else if constexpr (isMacOS) {
        audioDecoders[QMediaFormat::FileFormat::WMV] = {
            QMediaFormat::AudioCodec::AAC,  QMediaFormat::AudioCodec::AC3,
            QMediaFormat::AudioCodec::EAC3, QMediaFormat::AudioCodec::FLAC,
            QMediaFormat::AudioCodec::Wave,
        };
        audioDecoders[QMediaFormat::FileFormat::AVI] = {
            QMediaFormat::AudioCodec::AAC,  QMediaFormat::AudioCodec::AC3,
            QMediaFormat::AudioCodec::EAC3, QMediaFormat::AudioCodec::FLAC,
            QMediaFormat::AudioCodec::Wave,
        };
        audioDecoders[QMediaFormat::FileFormat::Matroska] = {
            QMediaFormat::AudioCodec::AAC,  QMediaFormat::AudioCodec::AC3,
            QMediaFormat::AudioCodec::EAC3, QMediaFormat::AudioCodec::FLAC,
            QMediaFormat::AudioCodec::Wave, QMediaFormat::AudioCodec::ALAC,
        };
        audioDecoders[QMediaFormat::FileFormat::MPEG4] = {
            QMediaFormat::AudioCodec::AAC,  QMediaFormat::AudioCodec::AC3,
            QMediaFormat::AudioCodec::EAC3, QMediaFormat::AudioCodec::FLAC,
            QMediaFormat::AudioCodec::Wave, QMediaFormat::AudioCodec::ALAC,
        };
        audioDecoders[QMediaFormat::FileFormat::Ogg] = {
            QMediaFormat::AudioCodec::FLAC,
            QMediaFormat::AudioCodec::WMA,
        };
        audioDecoders[QMediaFormat::FileFormat::QuickTime] = {
            QMediaFormat::AudioCodec::AAC,  QMediaFormat::AudioCodec::AC3,
            QMediaFormat::AudioCodec::EAC3, QMediaFormat::AudioCodec::FLAC,
            QMediaFormat::AudioCodec::Wave, QMediaFormat::AudioCodec::ALAC,
        };
        audioDecoders[QMediaFormat::FileFormat::WebM] = {};
        audioDecoders[QMediaFormat::FileFormat::Mpeg4Audio] = {
            QMediaFormat::AudioCodec::AAC,  QMediaFormat::AudioCodec::AC3,
            QMediaFormat::AudioCodec::EAC3, QMediaFormat::AudioCodec::FLAC,
            QMediaFormat::AudioCodec::Wave, QMediaFormat::AudioCodec::ALAC,
        };
        audioDecoders[QMediaFormat::FileFormat::AAC] = {
            QMediaFormat::AudioCodec::AAC,
            QMediaFormat::AudioCodec::WMA,
        };
        audioDecoders[QMediaFormat::FileFormat::WMA] = {
            QMediaFormat::AudioCodec::AAC,  QMediaFormat::AudioCodec::AC3,
            QMediaFormat::AudioCodec::EAC3, QMediaFormat::AudioCodec::FLAC,
            QMediaFormat::AudioCodec::Wave,
        };
        audioDecoders[QMediaFormat::FileFormat::MP3] = {};
        audioDecoders[QMediaFormat::FileFormat::FLAC] = {
            QMediaFormat::AudioCodec::FLAC,
        };
        audioDecoders[QMediaFormat::FileFormat::Wave] = {
            QMediaFormat::AudioCodec::Wave,
        };
    }
    return audioDecoders[fileFormat];
}

template <typename T>
QString enumToString(T enumValue)
{
    QMetaEnum metaEnum = QMetaEnum::fromType<decltype(enumValue)>();
    return QString("%1::%2::%3")
            .arg(metaEnum.scope(), metaEnum.name(), metaEnum.valueToKey(qToUnderlying(enumValue)));
}

class tst_qmediaformatbackend : public QObject
{
    Q_OBJECT

private slots:

    void isSupported_returnsTrue_whenFormatAndVideoCodecIsSupported_data()
    {
        QTest::addColumn<QMediaFormat::FileFormat>("fileFormat");
        QTest::addColumn<QMediaFormat::VideoCodec>("videoCodec");

        for (const QMediaFormat::FileFormat f : allFileFormats()) {
            for (const QMediaFormat::VideoCodec c : allVideoCodecs()) {
                const QByteArray formatName = QMediaFormat::fileFormatName(f).toLatin1();
                const QByteArray codecName = QMediaFormat::videoCodecName(c).toLatin1();
                QTest::addRow("%s,%s", formatName.data(), codecName.data()) << f << c;
            }
        }
    }

    void isSupported_returnsTrue_whenFormatAndVideoCodecIsSupported()
    {
        QSKIP_IF_NOT_FFMPEG("This test verifies only the FFmpeg media backend");

        QFETCH(QMediaFormat::FileFormat, fileFormat);
        QFETCH(QMediaFormat::VideoCodec, videoCodec);

        QMediaFormat format(fileFormat);
        format.setVideoCodec(videoCodec);

        const std::set<QMediaFormat::VideoCodec> supportedEncoders =
                supportedVideoEncoders(fileFormat);

        const std::set<QMediaFormat::VideoCodec> supportedDecoders =
                supportedVideoDecoders(fileFormat);

        bool isSupportedEncoder = supportedEncoders.find(videoCodec) != supportedEncoders.end();
        bool isSupportedDecoder = supportedDecoders.find(videoCodec) != supportedDecoders.end();

        QCOMPARE_EQ(format.isSupported(QMediaFormat::Encode), isSupportedEncoder);
        QCOMPARE_EQ(format.isSupported(QMediaFormat::Decode), isSupportedDecoder);
    }

    void isSupported_returnsTrue_whenFormatAndAudioCodecIsSupported_data()
    {
        QTest::addColumn<QMediaFormat::FileFormat>("fileFormat");
        QTest::addColumn<QMediaFormat::AudioCodec>("audioCodec");

        for (const QMediaFormat::FileFormat f : allFileFormats()) {
            for (const QMediaFormat::AudioCodec c : allAudioCodecs()) {
                const QByteArray formatName = QMediaFormat::fileFormatName(f).toLatin1();
                const QByteArray codecName = QMediaFormat::audioCodecName(c).toLatin1();
                QTest::addRow("%s,%s", formatName.data(), codecName.data()) << f << c;
            }
        }
    }

    void isSupported_returnsTrue_whenFormatAndAudioCodecIsSupported()
    {
        QSKIP_IF_NOT_FFMPEG("This test verifies only the FFmpeg media backend");

        QFETCH(QMediaFormat::FileFormat, fileFormat);
        QFETCH(QMediaFormat::AudioCodec, audioCodec);

        QMediaFormat format(fileFormat);
        format.setAudioCodec(audioCodec);

        const std::set<QMediaFormat::AudioCodec> supportedEncoders =
                supportedAudioEncoders(fileFormat);

        const std::set<QMediaFormat::AudioCodec> supportedDecoders =
                supportedAudioDecoders(fileFormat);

        bool isSupportedEncoder = supportedEncoders.find(audioCodec) != supportedEncoders.end();
        bool isSupportedDecoder = supportedDecoders.find(audioCodec) != supportedDecoders.end();

        QCOMPARE_EQ(format.isSupported(QMediaFormat::Encode), isSupportedEncoder);
        QCOMPARE_EQ(format.isSupported(QMediaFormat::Decode), isSupportedDecoder);
    }

    void isSupported_returnsTrue_whenAudioAndVideoCodecsAreCombined_data()
    {
        QTest::addColumn<QMediaFormat>("format");
        QTest::addColumn<QMediaFormat::ConversionMode>("conversionMode");

        for (const QMediaFormat &format : allMediaFormats(true)) {
            const auto formatName = QMediaFormat::fileFormatName(format.fileFormat());
            const auto audioCodecName = QMediaFormat::audioCodecName(format.audioCodec());
            const auto videoCodecName = QMediaFormat::videoCodecName(format.videoCodec());

            for (const auto &mode : { QMediaFormat::Encode, QMediaFormat::Decode }) {
                const auto convEnum = QMetaEnum::fromType<QMediaFormat::ConversionMode>();
                QTest::addRow("%s,%s,%s,%s", formatName.toLatin1().data(),
                              audioCodecName.toLatin1().data(), videoCodecName.toLatin1().data(),
                              convEnum.valueToKey(mode))
                        << format << mode;
            }
        }
    }

    void isSupported_returnsTrue_whenAudioAndVideoCodecsAreCombined()
    {
        QSKIP_IF_NOT_FFMPEG("This test verifies only the FFmpeg media backend");

        QFETCH(QMediaFormat, format);
        QFETCH(QMediaFormat::ConversionMode, conversionMode);

        QMediaFormat audioFormat(format.fileFormat());
        audioFormat.setAudioCodec(format.audioCodec());

        QMediaFormat videoFormat(format.fileFormat());
        videoFormat.setVideoCodec(format.videoCodec());

        // Check that format.isSupported() returns the same as
        // audioFormat.isSupported() && videoFormat.isSupported()
        QCOMPARE_EQ(format.isSupported(conversionMode),
                    audioFormat.isSupported(conversionMode)
                            && videoFormat.isSupported(conversionMode));
    }

    void resolveForEncoding_doesNotChangeFormat_whenFormatIsSupported_data()
    {
        QTest::addColumn<QMediaFormat>("format");
        QTest::addColumn<const QMediaFormat::ResolveFlags>("resolveFlags");

        for (const QMediaFormat &format : allMediaFormats(true)) {
            const auto formatName = QMediaFormat::fileFormatName(format.fileFormat());
            const auto audioCodecName = QMediaFormat::audioCodecName(format.audioCodec());
            const auto videoCodecName = QMediaFormat::videoCodecName(format.videoCodec());

            for (const auto resolveFlags : { QMediaFormat::NoFlags, QMediaFormat::RequiresVideo }) {
                const auto flagName =
                        resolveFlags == QMediaFormat::NoFlags ? "NoFlags" : "RequiresVideo";
                QTest::addRow("%s,%s,%s,%s", formatName.toLatin1().data(),
                              audioCodecName.toLatin1().data(), videoCodecName.toLatin1().data(),
                              flagName)
                        << format << resolveFlags;
            }
        }
    }

    void resolveForEncoding_doesNotChangeFormat_whenFormatIsSupported()
    {
        QSKIP_IF_NOT_FFMPEG("This test verifies only the FFmpeg media backend");

        QFETCH(QMediaFormat, format);
        QFETCH(const QMediaFormat::ResolveFlags, resolveFlags);

        // resolveForEncoding should not do anything if the format is already
        // supported and file format and required codecs are specified. Note
        // in particular that resolveForEncoding will clear the video codec if
        // the resolve flag does not specify that video is required.
        bool resolveShouldDoNothing = format.isSupported(QMediaFormat::Encode)
                && format.fileFormat() != QMediaFormat::UnspecifiedFormat
                && format.audioCodec() != QMediaFormat::AudioCodec::Unspecified;

        if (resolveFlags == QMediaFormat::RequiresVideo)
            resolveShouldDoNothing = resolveShouldDoNothing
                    && format.videoCodec() != QMediaFormat::VideoCodec::Unspecified;
        else
            resolveShouldDoNothing = resolveShouldDoNothing
                    && format.videoCodec() == QMediaFormat::VideoCodec::Unspecified;

        QMediaFormat originalFormat = format;

        format.resolveForEncoding(resolveFlags);
        if (resolveShouldDoNothing)
            QCOMPARE_EQ(format, originalFormat);
        else
            QCOMPARE_NE(format, originalFormat);
    }

    void resolveForEncoding_givesSupportedFormat_whenCalledWithAllCodecs_data()
    {
        QTest::addColumn<QMediaFormat>("format");
        QTest::addColumn<const QMediaFormat::ResolveFlags>("resolveFlags");

        for (const QMediaFormat &format : allMediaFormats(true)) {
            const auto formatName = QMediaFormat::fileFormatName(format.fileFormat());
            const auto audioCodecName = QMediaFormat::audioCodecName(format.audioCodec());
            const auto videoCodecName = QMediaFormat::videoCodecName(format.videoCodec());

            for (const auto resolveFlags : { QMediaFormat::NoFlags, QMediaFormat::RequiresVideo }) {
                const auto flagName =
                        resolveFlags == QMediaFormat::NoFlags ? "NoFlags" : "RequiresVideo";
                QTest::addRow("%s,%s,%s,%s", formatName.toLatin1().data(),
                              audioCodecName.toLatin1().data(), videoCodecName.toLatin1().data(),
                              flagName)
                        << format << resolveFlags;
            }
        }
    }

    void resolveForEncoding_givesSupportedFormat_whenCalledWithAllCodecs()
    {
        QSKIP_IF_NOT_FFMPEG("This test verifies only the FFmpeg media backend");

        QFETCH(QMediaFormat, format);
        QFETCH(const QMediaFormat::ResolveFlags, resolveFlags);

        format.resolveForEncoding(resolveFlags);

        QCOMPARE_NE(format.fileFormat(), QMediaFormat::FileFormat::UnspecifiedFormat);
        QCOMPARE_NE(format.audioCodec(), QMediaFormat::AudioCodec::Unspecified);
        if (resolveFlags == QMediaFormat::NoFlags)
            QCOMPARE_EQ(format.videoCodec(), QMediaFormat::VideoCodec::Unspecified);
        else
            QCOMPARE_NE(format.videoCodec(), QMediaFormat::VideoCodec::Unspecified);

        QVERIFY(format.isSupported(QMediaFormat::Encode));
    }

    void print_formatSupport_video_encoding_noVerify_data()
    {
        QTest::addColumn<QMediaFormat::ConversionMode>("conversionMode");
        QTest::addColumn<QString>("variableName");

        QTest::addRow("Video decoders") << QMediaFormat::ConversionMode::Decode << "videoDecoders";
        QTest::addRow("Video encoders") << QMediaFormat::ConversionMode::Encode << "videoEncoders";
    }

    void print_formatSupport_video_encoding_noVerify()
    {
        QFETCH(const QMediaFormat::ConversionMode, conversionMode);
        QFETCH(const QString, variableName);

        // This test does not verify anything, but prints out all supported video formats
        QStringList output;
        output << "std::map<QMediaFormat::FileFormat, std::set<QMediaFormat::VideoCodec>> "
               << variableName << ";";
        for (const QMediaFormat::FileFormat f : allFileFormats()) {
            output << variableName << "[" << enumToString(f) << "] = {";
            for (const QMediaFormat::VideoCodec c : allVideoCodecs()) {
                QMediaFormat format(f);
                format.setVideoCodec(c);
                if (format.isSupported(conversionMode))
                    output << enumToString(c) << ",";
            }
            output << "};";
        }
        qWarning() << output.join("");
    }

    void print_formatSupport_audio_encoding_noVerify_data()
    {
        QTest::addColumn<QMediaFormat::ConversionMode>("conversionMode");
        QTest::addColumn<QString>("variableName");

        QTest::addRow("Audio decoders") << QMediaFormat::ConversionMode::Decode << "audioDecoders";
        QTest::addRow("Audio encoders") << QMediaFormat::ConversionMode::Encode << "audioEncoders";
    }
    void print_formatSupport_audio_encoding_noVerify()
    {
        QFETCH(const QMediaFormat::ConversionMode, conversionMode);
        QFETCH(const QString, variableName);

        // This test does not verify anything, but prints out all supported video formats
        QStringList output;
        output << "std::map<QMediaFormat::FileFormat, std::set<QMediaFormat::AudioCodec>> "
               << variableName << ";";
        for (const QMediaFormat::FileFormat f : allFileFormats()) {
            output << variableName << "[" << enumToString(f) << "] = {";
            for (const QMediaFormat::AudioCodec c : allAudioCodecs()) {
                QMediaFormat format(f);
                format.setAudioCodec(c);
                if (format.isSupported(conversionMode))
                    output << enumToString(c) << ",";
            }
            output << "};";
        }
        qWarning() << output.join("");
    }
};

QTEST_GUILESS_MAIN(tst_qmediaformatbackend)
#include "tst_qmediaformatbackend.moc"
