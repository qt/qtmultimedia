// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindowsformatinfo_p.h"

#include <QtMultimedia/private/qcomtaskresource_p.h>
#include <QtMultimedia/private/qwindowsmultimediautils_p.h>
#include <QtGui/qimagewriter.h>
#include <QtCore/qhash.h>
#include <QtCore/qlist.h>
#include <QtCore/qset.h>
#include <QtCore/private/qcomptr_p.h>

#include <mfapi.h>
#include <mftransform.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

namespace {

template<typename T>
using CheckedCodecs = QHash<std::pair<T, QMediaFormat::ConversionMode>, bool>;

bool isSupportedMFT(const GUID &category, const MFT_REGISTER_TYPE_INFO &type, QMediaFormat::ConversionMode mode)
{
    UINT32 count = 0;
    QComTaskResource<IMFActivate *> activate;
    HRESULT hr = E_FAIL;

    // MFTEnumEx may crash inside third-party drivers (e.g. D3D12Core.dll,
    // RTWorkQ.dll) when enumerating hardware codecs. Wrap the call with SEH
    // to prevent the crash from taking down the whole application.
#ifdef Q_CC_MSVC
    __try {
#endif
        hr = MFTEnumEx(
                category,
                MFT_ENUM_FLAG_ALL,
                (mode == QMediaFormat::Encode) ? nullptr : &type,  // Input type
                (mode == QMediaFormat::Encode) ? &type : nullptr,  // Output type
                activate.address(),
                &count
                );
#ifdef Q_CC_MSVC
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        qWarning("MFTEnumEx crashed while enumerating codecs (SEH exception caught)");
        return false;
    }
#endif

    return SUCCEEDED(hr) && count > 0;
}

bool isSupportedCodec(QMediaFormat::AudioCodec codec, QMediaFormat::ConversionMode mode)
{
    return isSupportedMFT((mode == QMediaFormat::Encode) ? MFT_CATEGORY_AUDIO_ENCODER : MFT_CATEGORY_AUDIO_DECODER,
                          { MFMediaType_Audio, QWMF::audioFormatForCodec(codec) },
                          mode);
}

bool isSupportedCodec(QMediaFormat::VideoCodec codec, QMediaFormat::ConversionMode mode)
{
    return isSupportedMFT((mode == QMediaFormat::Encode) ? MFT_CATEGORY_VIDEO_ENCODER : MFT_CATEGORY_VIDEO_DECODER,
                          { MFMediaType_Video, QWMF::videoFormatForCodec(codec) },
                          mode);
}

template <typename T>
bool isSupportedCodec(T codec, QMediaFormat::ConversionMode m, CheckedCodecs<T> &checkedCodecs)
{
    if (auto it = checkedCodecs.constFind(std::pair{codec, m}); it != checkedCodecs.constEnd())
        return it.value();

    const bool supported = isSupportedCodec(codec, m);

    checkedCodecs.insert(std::pair{codec, m}, supported);
    return supported;
}

}

static QList<QImageCapture::FileFormat> getImageFormatList()
{
    QList<QImageCapture::FileFormat> list;
    const auto formats = QImageWriter::supportedImageFormats();

    for (const auto &f : formats) {
        auto format = QString::fromUtf8(f);
        if (format.compare(u"jpg", Qt::CaseInsensitive) == 0)
            list.append(QImageCapture::FileFormat::JPEG);
        else if (format.compare(u"png", Qt::CaseInsensitive) == 0)
            list.append(QImageCapture::FileFormat::PNG);
        else if (format.compare(u"webp", Qt::CaseInsensitive) == 0)
            list.append(QImageCapture::FileFormat::WebP);
        else if (format.compare(u"tiff", Qt::CaseInsensitive) == 0)
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

    fixupCodecMaps();
}

QWindowsFormatInfo::~QWindowsFormatInfo()
{
}

QT_END_NAMESPACE
