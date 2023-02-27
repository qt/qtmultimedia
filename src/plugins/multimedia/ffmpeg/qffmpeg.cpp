// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpeg_p.h"

#include <qdebug.h>

#include <algorithm>
#include <vector>
#include <array>
#include <optional>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

namespace {

enum CodecStorageType {
    ENCODERS,
    DECODERS,

    // TODO: maybe split sw/hw codecs

    CODEC_STORAGE_TYPE_COUNT
};

using CodecsStorage = std::vector<const AVCodec *>;

struct CodecsComparator
{
    bool operator()(const AVCodec *a, const AVCodec *b) const { return a->id < b->id; }

    bool operator()(const AVCodec *a, AVCodecID id) const { return a->id < id; }
};

const CodecsStorage &codecsStorage(CodecStorageType codecsType)
{
    static const auto &storages = []() {
        std::array<CodecsStorage, CODEC_STORAGE_TYPE_COUNT> result;
        void *opaque = nullptr;

        while (auto codec = av_codec_iterate(&opaque)) {
            if (av_codec_is_decoder(codec))
                result[DECODERS].emplace_back(codec);
            if (av_codec_is_encoder(codec))
                result[ENCODERS].emplace_back(codec);
        }

        for (auto &storage : result) {
            storage.shrink_to_fit();

            // we should ensure the original order
            std::stable_sort(storage.begin(), storage.end(), CodecsComparator{});
        }

        return result;
    }();

    return storages[codecsType];
}

static const char *preferredHwCodecNameSuffix(bool isEncoder, AVHWDeviceType deviceType)
{
    switch (deviceType) {
    case AV_HWDEVICE_TYPE_VAAPI:
        return "_vaapi";
    case AV_HWDEVICE_TYPE_MEDIACODEC:
        return "_mediacodec";
    case AV_HWDEVICE_TYPE_VIDEOTOOLBOX:
        return "_videotoolbox";
    case AV_HWDEVICE_TYPE_D3D11VA:
    case AV_HWDEVICE_TYPE_DXVA2:
        return "_mf";
    case AV_HWDEVICE_TYPE_CUDA:
    case AV_HWDEVICE_TYPE_VDPAU:
        return isEncoder ? "_nvenc" : "_cuvid";
    default:
        return nullptr;
    }
}

using CodecScore = int;
constexpr CodecScore BestCodec = std::numeric_limits<CodecScore>::max();
constexpr CodecScore NotSuitableCodec = std::numeric_limits<CodecScore>::min();

template<typename CodecScoreGetter>
const AVCodec *findAVCodec(CodecStorageType codecsType, AVCodecID codecId,
                           const CodecScoreGetter &scoreGetter)
{
    const auto &storage = codecsStorage(codecsType);
    auto it = std::lower_bound(storage.begin(), storage.end(), codecId, CodecsComparator{});

    const AVCodec *result = nullptr;
    CodecScore resultScore = NotSuitableCodec;

    for (; it != storage.end() && (*it)->id == codecId && resultScore != BestCodec; ++it) {
        const auto score = scoreGetter(*it);

        if (score > resultScore) {
            resultScore = score;
            result = *it;
        }
    }

    return result;
}

CodecScore hwCodecNameScores(const AVCodec *codec, AVHWDeviceType deviceType)
{
    if (auto suffix = preferredHwCodecNameSuffix(av_codec_is_encoder(codec), deviceType)) {
        const auto substr = strstr(codec->name, suffix);
        if (substr && !substr[strlen(suffix)])
            return BestCodec;

        return 0;
    }

    return BestCodec;
}

const AVCodec *findAVCodec(CodecStorageType codecsType, AVCodecID codecId,
                           const std::optional<AVHWDeviceType> &deviceType,
                           const std::optional<PixelOrSampleFormat> &format)
{
    return findAVCodec(codecsType, codecId, [&](const AVCodec *codec) {
        if (codec->capabilities & AV_CODEC_CAP_EXPERIMENTAL)
            return NotSuitableCodec; //TODO: maybe check additionally

        if (format && !isAVFormatSupported(codec, *format))
            return NotSuitableCodec;

        if (!deviceType)
            return BestCodec; // find any codec with the id

        if (*deviceType == AV_HWDEVICE_TYPE_NONE && !avcodec_get_hw_config(codec, 0))
            return BestCodec;

        if (*deviceType != AV_HWDEVICE_TYPE_NONE) {
            for (int index = 0; auto config = avcodec_get_hw_config(codec, index); ++index) {
                if (config->device_type != deviceType)
                    continue;

                if (format && config->pix_fmt != AV_PIX_FMT_NONE && config->pix_fmt != *format)
                    continue;

                return hwCodecNameScores(codec, *deviceType);
            }
        }

        return NotSuitableCodec;
    });
}

} // namespace

const AVCodec *findAVDecoder(AVCodecID codecId, const std::optional<AVHWDeviceType> &deviceType,
                             const std::optional<PixelOrSampleFormat> &format)
{
    return findAVCodec(DECODERS, codecId, deviceType, format);
}

const AVCodec *findAVEncoder(AVCodecID codecId, const std::optional<AVHWDeviceType> &deviceType,
                             const std::optional<PixelOrSampleFormat> &format)
{
    return findAVCodec(ENCODERS, codecId, deviceType, format);
}

bool isAVFormatSupported(const AVCodec *codec, PixelOrSampleFormat format)
{
    auto isFormatSupportedImpl = [format](auto fmts) {
        if (fmts)
            for (; *fmts != -1; ++fmts)
                if (*fmts == format)
                    return true;

        return false;
    };

    if (codec->type == AVMEDIA_TYPE_VIDEO)
        return isFormatSupportedImpl(codec->pix_fmts);

    if (codec->type == AVMEDIA_TYPE_AUDIO)
        return isFormatSupportedImpl(codec->sample_fmts);

    return false;
}

} // namespace QFFmpeg

QT_END_NAMESPACE
