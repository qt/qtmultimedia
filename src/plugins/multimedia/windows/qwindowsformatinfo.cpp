// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindowsformatinfo_p.h"

#include <mfapi.h>
#include <mftransform.h>
#include <private/qcomptr_p.h>
#include <private/qwindowsmultimediautils_p.h>
#include <private/qcomtaskresource_p.h>

#include <QtCore/qlist.h>
#include <QtCore/qset.h>
#include <QtCore/qhash.h>
#include <QtGui/qimagewriter.h>

QT_BEGIN_NAMESPACE

namespace {

template<typename T>
using CheckedCodecs = QHash<QPair<T, QMediaFormat::ConversionMode>, bool>;

bool isSupportedMFT(const GUID &category, const MFT_REGISTER_TYPE_INFO &type, QMediaFormat::ConversionMode mode)
{
    UINT32 count = 0;
    IMFActivate **activateArrayRaw = nullptr;
    HRESULT hr = MFTEnumEx(
            category,
            MFT_ENUM_FLAG_ALL,
            (mode == QMediaFormat::Encode) ? nullptr : &type,  // Input type
            (mode == QMediaFormat::Encode) ? &type : nullptr,  // Output type
            &activateArrayRaw,
            &count
            );

    if (FAILED(hr))
        return false;

    QComTaskResource<IMFActivate *[], QComDeleter> activateArray(activateArrayRaw, count);
    for (UINT32 i = 0; i < count; ++i) {
        ComPtr<IMFTransform> transform;
        hr = activateArray[i]->ActivateObject(IID_PPV_ARGS(transform.GetAddressOf()));
        if (SUCCEEDED(hr))
            return true;
    }

    return false;
}

bool isSupportedCodec(QMediaFormat::AudioCodec codec, QMediaFormat::ConversionMode mode)
{
    return isSupportedMFT((mode == QMediaFormat::Encode) ? MFT_CATEGORY_AUDIO_ENCODER : MFT_CATEGORY_AUDIO_DECODER,
                          { MFMediaType_Audio, QWindowsMultimediaUtils::audioFormatForCodec(codec) },
                          mode);
}

bool isSupportedCodec(QMediaFormat::VideoCodec codec, QMediaFormat::ConversionMode mode)
{
    return isSupportedMFT((mode == QMediaFormat::Encode) ? MFT_CATEGORY_VIDEO_ENCODER : MFT_CATEGORY_VIDEO_DECODER,
                          { MFMediaType_Video, QWindowsMultimediaUtils::videoFormatForCodec(codec) },
                          mode);
}

template <typename T>
bool isSupportedCodec(T codec, QMediaFormat::ConversionMode m, CheckedCodecs<T> &checkedCodecs)
{
    if (auto it = checkedCodecs.constFind(qMakePair(codec, m)); it != checkedCodecs.constEnd())
        return it.value();

    const bool supported = isSupportedCodec(codec, m);

    checkedCodecs.insert(qMakePair(codec, m), supported);
    return supported;
}

}

static QList<QImageCapture::FileFormat> getImageFormatList()
{
    QList<QImageCapture::FileFormat> list;
    const auto formats = QImageWriter::supportedImageFormats();

    for (const auto &f : formats) {
        auto format = QString::fromUtf8(f);
        if (format.compare(QLatin1String("jpg"), Qt::CaseInsensitive) == 0)
            list.append(QImageCapture::FileFormat::JPEG);
        else if (format.compare(QLatin1String("png"), Qt::CaseInsensitive) == 0)
            list.append(QImageCapture::FileFormat::PNG);
        else if (format.compare(QLatin1String("webp"), Qt::CaseInsensitive) == 0)
            list.append(QImageCapture::FileFormat::WebP);
        else if (format.compare(QLatin1String("tiff"), Qt::CaseInsensitive) == 0)
            list.append(QImageCapture::FileFormat::Tiff);
    }

    return list;
}

QWindowsFormatInfo::QWindowsFormatInfo()
{
    const QList<CodecMap> containerTable = {
        { QMediaFormat::MPEG4,
          { QMediaFormat::AudioCodec::AAC, QMediaFormat::AudioCodec::MP3, QMediaFormat::AudioCodec::ALAC, QMediaFormat::AudioCodec::AC3, QMediaFormat::AudioCodec::EAC3 },
          { QMediaFormat::VideoCodec::H264, QMediaFormat::VideoCodec::H265, QMediaFormat::VideoCodec::MotionJPEG } },
        { QMediaFormat::Matroska,
          { QMediaFormat::AudioCodec::AAC, QMediaFormat::AudioCodec::MP3, QMediaFormat::AudioCodec::ALAC, QMediaFormat::AudioCodec::AC3, QMediaFormat::AudioCodec::EAC3, QMediaFormat::AudioCodec::FLAC, QMediaFormat::AudioCodec::Vorbis, QMediaFormat::AudioCodec::Opus },
          { QMediaFormat::VideoCodec::H264, QMediaFormat::VideoCodec::H265, QMediaFormat::VideoCodec::VP8, QMediaFormat::VideoCodec::VP9, QMediaFormat::VideoCodec::MotionJPEG } },
        { QMediaFormat::WebM,
          { QMediaFormat::AudioCodec::Vorbis, QMediaFormat::AudioCodec::Opus },
          { QMediaFormat::VideoCodec::VP8, QMediaFormat::VideoCodec::VP9 } },
        { QMediaFormat::QuickTime,
          { QMediaFormat::AudioCodec::AAC, QMediaFormat::AudioCodec::MP3, QMediaFormat::AudioCodec::ALAC, QMediaFormat::AudioCodec::AC3, QMediaFormat::AudioCodec::EAC3 },
          { QMediaFormat::VideoCodec::H264, QMediaFormat::VideoCodec::H265, QMediaFormat::VideoCodec::MotionJPEG } },
        { QMediaFormat::AAC,
          { QMediaFormat::AudioCodec::AAC },
          {} },
        { QMediaFormat::MP3,
          { QMediaFormat::AudioCodec::MP3 },
          {} },
        { QMediaFormat::FLAC,
          { QMediaFormat::AudioCodec::FLAC },
          {} },
        { QMediaFormat::Mpeg4Audio,
          { QMediaFormat::AudioCodec::AAC, QMediaFormat::AudioCodec::MP3, QMediaFormat::AudioCodec::ALAC, QMediaFormat::AudioCodec::AC3, QMediaFormat::AudioCodec::EAC3 },
          {} },
        { QMediaFormat::WMA,
          { QMediaFormat::AudioCodec::WMA },
          {} },
        { QMediaFormat::WMV,
          { QMediaFormat::AudioCodec::WMA },
          { QMediaFormat::VideoCodec::WMV } }
    };

    const QSet<QMediaFormat::FileFormat> decoderFormats = {
        QMediaFormat::MPEG4,
        QMediaFormat::Matroska,
        QMediaFormat::WebM,
        QMediaFormat::QuickTime,
        QMediaFormat::AAC,
        QMediaFormat::MP3,
        QMediaFormat::FLAC,
        QMediaFormat::Mpeg4Audio,
        QMediaFormat::WMA,
        QMediaFormat::WMV,
    };

    const QSet<QMediaFormat::FileFormat> encoderFormats = {
        QMediaFormat::MPEG4,
        QMediaFormat::AAC,
        QMediaFormat::MP3,
        QMediaFormat::FLAC,
        QMediaFormat::Mpeg4Audio,
        QMediaFormat::WMA,
        QMediaFormat::WMV,
    };

    CheckedCodecs<QMediaFormat::AudioCodec> checkedAudioCodecs;
    CheckedCodecs<QMediaFormat::VideoCodec> checkedVideoCodecs;

    auto ensureCodecs = [&] (CodecMap &codecs, QMediaFormat::ConversionMode mode) {
        codecs.audio.removeIf([&] (auto codec) { return !isSupportedCodec(codec, mode, checkedAudioCodecs); });
        codecs.video.removeIf([&] (auto codec) { return !isSupportedCodec(codec, mode, checkedVideoCodecs); });
        return !codecs.video.empty() || !codecs.audio.empty();
    };

    for (const auto &codecMap : containerTable) {
        if (decoderFormats.contains(codecMap.format)) {
            auto m = codecMap;
            if (ensureCodecs(m, QMediaFormat::Decode))
                decoders.append(m);
        }

        if (encoderFormats.contains(codecMap.format)) {
            auto m = codecMap;
            if (ensureCodecs(m, QMediaFormat::Encode))
                encoders.append(m);
        }
    }

    imageFormats = getImageFormatList();
}

QWindowsFormatInfo::~QWindowsFormatInfo()
{
}

QT_END_NAMESPACE
