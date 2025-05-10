// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qdarwinformatsinfo_p.h"
#include <AVFoundation/AVFoundation.h>
#include <qdebug.h>

QT_BEGIN_NAMESPACE

static struct {
    const char *name;
    QMediaFormat::FileFormat value;
} mediaContainerMap[] = {
    { "video/x-ms-asf", QMediaFormat::WMV },
    { "video/avi", QMediaFormat::AVI },
    { "video/x-matroska", QMediaFormat::Matroska },
    { "video/mp4", QMediaFormat::MPEG4 },
    { "video/quicktime", QMediaFormat::QuickTime },
    { "video/ogg", QMediaFormat::Ogg },
    { "audio/mp3", QMediaFormat::MP3 },
    { "audio/flac", QMediaFormat::FLAC },
    { nullptr, QMediaFormat::UnspecifiedFormat }
};

static struct {
    const char *name;
    QMediaFormat::VideoCodec value;
} videoCodecMap[] = {
    // See CMVideoCodecType for the four character code names of codecs
    { "; codecs=\"mp1v\"", QMediaFormat::VideoCodec::MPEG1 },
    { "; codecs=\"mp2v\"", QMediaFormat::VideoCodec::MPEG2 },
    { "; codecs=\"mp4v\"", QMediaFormat::VideoCodec::MPEG4 },
    { "; codecs=\"avc1\"", QMediaFormat::VideoCodec::H264 },
    { "; codecs=\"hvc1\"", QMediaFormat::VideoCodec::H265 },
    { "; codecs=\"vp09\"", QMediaFormat::VideoCodec::VP9 },
    { "; codecs=\"av01\"", QMediaFormat::VideoCodec::AV1 }, // ### ????
    { "; codecs=\"jpeg\"", QMediaFormat::VideoCodec::MotionJPEG },
    { nullptr, QMediaFormat::VideoCodec::Unspecified }
};

static struct {
    const char *name;
    QMediaFormat::AudioCodec value;
} audioCodecMap[] = {
    // AudioFile.h
    // ### The next two entries do not work, probably because they contain non a space and period and AVFoundation doesn't like that
    // We know they are supported on all Apple platforms, so we'll add them manually below
//    { "; codecs=\".mp3\"", QMediaFormat::AudioCodec::MP3 },
//    { "; codecs=\"aac \"", QMediaFormat::AudioCodec::AAC },
    { "; codecs=\"ac-3\"", QMediaFormat::AudioCodec::AC3 },
    { "; codecs=\"ec-3\"", QMediaFormat::AudioCodec::EAC3 },
    { "; codecs=\"flac\"", QMediaFormat::AudioCodec::FLAC },
    { "; codecs=\"alac\"", QMediaFormat::AudioCodec::ALAC },
    { "; codecs=\"opus\"", QMediaFormat::AudioCodec::Opus },
    { nullptr, QMediaFormat::AudioCodec::Unspecified },
};

QDarwinFormatInfo::QDarwinFormatInfo()
{
    auto avtypes = [AVURLAsset audiovisualMIMETypes];
    for (AVFileType filetype in avtypes) {
        auto *m = mediaContainerMap;
        while (m->name) {
            if (strcmp(filetype.UTF8String, m->name)) {
                ++m;
                continue;
            }

            QList<QMediaFormat::VideoCodec> video;
            QList<QMediaFormat::AudioCodec> audio;

            auto *v = videoCodecMap;
            while (v->name) {
                QByteArray extendedMimetype = m->name;
                extendedMimetype += v->name;
                if ([AVURLAsset isPlayableExtendedMIMEType:[NSString stringWithUTF8String:extendedMimetype.constData()]])
                    video << v->value;
                ++v;
            }

            auto *a = audioCodecMap;
            while (a->name) {
                QByteArray extendedMimetype = m->name;
                extendedMimetype += a->name;
                if ([AVURLAsset isPlayableExtendedMIMEType:[NSString stringWithUTF8String:extendedMimetype.constData()]])
                    audio << a->value;
                ++a;
            }
            // Added manually, see comment in the list above
            if (m->value <= QMediaFormat::AAC)
                audio << QMediaFormat::AudioCodec::AAC;
            if (m->value < QMediaFormat::AAC || m->value == QMediaFormat::MP3)
                audio << QMediaFormat::AudioCodec::MP3;

            decoders << CodecMap{ m->value, audio, video };
            ++m;
        }
    }

    // seems AVFoundation only supports those for encoding
    encoders = {
        { QMediaFormat::MPEG4,
          { QMediaFormat::AudioCodec::AAC, QMediaFormat::AudioCodec::ALAC },
          { QMediaFormat::VideoCodec::H264, QMediaFormat::VideoCodec::H265, QMediaFormat::VideoCodec::MotionJPEG } },
        { QMediaFormat::QuickTime,
          { QMediaFormat::AudioCodec::AAC, QMediaFormat::AudioCodec::ALAC },
          { QMediaFormat::VideoCodec::H264, QMediaFormat::VideoCodec::H265, QMediaFormat::VideoCodec::MotionJPEG } },
        { QMediaFormat::Mpeg4Audio,
          { QMediaFormat::AudioCodec::AAC },
          {} },
        { QMediaFormat::Wave,
            { QMediaFormat::AudioCodec::Wave },
            {} },
    };

    // ###
    imageFormats << QImageCapture::JPEG;
}

QDarwinFormatInfo::~QDarwinFormatInfo() = default;

int QDarwinFormatInfo::audioFormatForCodec(QMediaFormat::AudioCodec codec)
{
    int codecId = kAudioFormatMPEG4AAC;
    switch (codec) {
    case QMediaFormat::AudioCodec::Unspecified:
    case QMediaFormat::AudioCodec::DolbyTrueHD:
    case QMediaFormat::AudioCodec::Vorbis:
    case QMediaFormat::AudioCodec::WMA:
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
    case QMediaFormat::AudioCodec::ALAC:
        codecId = kAudioFormatAppleLossless;
        break;
    case QMediaFormat::AudioCodec::Opus:
        codecId = kAudioFormatOpus;
        break;
    case QMediaFormat::AudioCodec::Wave:
        codecId = kAudioFormatLinearPCM;
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
    case QMediaFormat::VideoCodec::WMV:
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

NSString *QDarwinFormatInfo::avFileTypeForContainerFormat(QMediaFormat::FileFormat container)
{
    switch (container) {
        case QMediaFormat::MPEG4:
            return AVFileTypeMPEG4;
        case QMediaFormat::QuickTime:
            return AVFileTypeQuickTimeMovie;
        case QMediaFormat::MP3:
            return AVFileTypeMPEGLayer3;
        case QMediaFormat::Mpeg4Audio:
            return AVFileTypeAppleM4A;
        case QMediaFormat::Wave:
            return AVFileTypeWAVE;
        default:
            return AVFileTypeQuickTimeMovie;
    }
}

QT_END_NAMESPACE
