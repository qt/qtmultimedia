// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpipewire_spa_video_format_support_p.h"

#include <QtMultimedia/private/qcameradevice_p.h>
#include <QtMultimedia/private/qmultimedia_ranges_p.h>
#include <QtMultimedia/private/qpipewire_spa_pod_parser_support_p.h>
#include <QtMultimedia/private/qpipewire_videoformat_support_p.h>

#include <spa/pod/pod.h>

#include <iterator>

QT_BEGIN_NAMESPACE

namespace QtPipeWire {

namespace {

namespace ranges = QtMultimediaPrivate::ranges;

qreal fpsFromFraction(spa_fraction fraction)
{
    if (fraction.denom == 0)
        return 0.;
    return qreal(fraction.num) / qreal(fraction.denom);
}

QSize toSize(spa_rectangle rectangle)
{
    return QSize(int(rectangle.width), int(rectangle.height));
}

void appendUnique(std::vector<QSize> &list, QSize value)
{
    if (!value.isEmpty() && !ranges::contains(list, value))
        list.push_back(value);
}

void appendUnique(std::vector<QVideoFrameFormat::PixelFormat> &list,
                  QVideoFrameFormat::PixelFormat value)
{
    if (value != QVideoFrameFormat::Format_Invalid && !ranges::contains(list, value))
        list.push_back(value);
}

std::vector<QVideoFrameFormat::PixelFormat> parsePixelFormats(const spa_pod &pod)
{
    std::vector<QVideoFrameFormat::PixelFormat> result;

    if (auto format = spaParsePodPropertyScalar<spa_video_format>(pod, SPA_TYPE_OBJECT_Format,
                                                                  SPA_FORMAT_VIDEO_format)) {
        const QVideoFrameFormat::PixelFormat pixelFormat = toQtPixelFormat(*format);
        if (pixelFormat != QVideoFrameFormat::Format_Invalid)
            result.push_back(pixelFormat);
        return result;
    }

    if (auto choice = spaParsePodPropertyChoice<spa_video_format, SPA_CHOICE_Enum>(
                pod, SPA_TYPE_OBJECT_Format, SPA_FORMAT_VIDEO_format)) {
        appendUnique(result, toQtPixelFormat(choice->defaultValue()));
        if (choice->size() > 1) {
            for (spa_video_format format : choice->values())
                appendUnique(result, toQtPixelFormat(format));
        }
    }

    return result;
}

SpaObjectVideoFormat::ResolutionSet parseResolutions(SpaEnum<spa_rectangle> values)
{
    std::vector<QSize> result;
    appendUnique(result, toSize(values.defaultValue()));
    if (values.size() > 1) {
        for (spa_rectangle size : values.values())
            appendUnique(result, toSize(size));
    }
    return result;
}
SpaObjectVideoFormat::ResolutionSet parseResolutions(SpaRange<spa_rectangle> range)
{
    return SizeRange{
        .preferred = toSize(range.defaultValue),
        .min = toSize(range.minValue),
        .max = toSize(range.maxValue),
    };
}
SpaObjectVideoFormat::ResolutionSet parseResolutions(const spa_pod &pod)
{
    if (auto size = spaParsePodPropertyScalar<spa_rectangle>(pod, SPA_TYPE_OBJECT_Format,
                                                             SPA_FORMAT_VIDEO_size)) {
        const QSize resolution = toSize(*size);
        if (resolution.isEmpty())
            return std::vector<QSize>{};
        return std::vector{ resolution };
    }

    auto choice = spaParsePodPropertyChoice<spa_rectangle, SPA_CHOICE_Enum, SPA_CHOICE_Range>(
            pod, SPA_TYPE_OBJECT_Format, SPA_FORMAT_VIDEO_size);
    if (!choice)
        return std::vector<QSize>{};

    return std::visit([](const auto &arg) {
        return parseResolutions(arg);
    }, *choice);
}

// A SizeRange covers every size in between, but a QCameraFormat list has to
// be finite: report the bounds and the preferred size only, like
// QV4L2CameraDevices does for V4L2_FRMSIZE_TYPE_STEPWISE.
std::vector<QSize> expandResolutions(const SizeRange &range)
{
    std::vector<QSize> result;
    appendUnique(result, range.preferred);
    appendUnique(result, range.min);
    appendUnique(result, range.max);
    return result;
}
std::vector<QSize> expandResolutions(const std::vector<QSize> &list)
{
    return list;
}
std::vector<QSize> expandResolutions(const SpaObjectVideoFormat::ResolutionSet &resolutions)
{
    return std::visit([](const auto &arg) {
        return expandResolutions(arg);
    }, resolutions);
}

bool isEmpty(const std::vector<QSize> &list)
{
    return list.empty();
}
bool isEmpty(const SizeRange &)
{
    return false; // a SizeRange always carries a preferred size and both bounds
}
bool isEmpty(const SpaObjectVideoFormat::ResolutionSet &resolutions)
{
    using namespace QtMultimediaPrivate;
    return std::visit([](const auto &arg) {
        return isEmpty(arg);
    }, resolutions);
}

// Returns the inclusive frame rate bounds, both 0 if the pod does not describe
// a frame rate at all.
std::pair<qreal, qreal> parseFrameRateRange(const SpaEnum<spa_fraction> &values)
{
    std::vector<qreal> fpsValues{ fpsFromFraction(values.defaultValue()) };
    if (values.size() > 1)
        ranges::transform(values.values(), std::back_inserter(fpsValues), fpsFromFraction);

    return { ranges::min(fpsValues), ranges::max(fpsValues) };
}
std::pair<qreal, qreal> parseFrameRateRange(const SpaRange<spa_fraction> &range)
{
    return { fpsFromFraction(range.minValue), fpsFromFraction(range.maxValue) };
}

std::pair<qreal, qreal> parseFrameRateRange(const spa_pod &pod, unsigned property)
{
    if (auto rate =
                spaParsePodPropertyScalar<spa_fraction>(pod, SPA_TYPE_OBJECT_Format, property)) {
        const qreal fps = fpsFromFraction(*rate);
        return { fps, fps };
    }

    auto choice = spaParsePodPropertyChoice<spa_fraction, SPA_CHOICE_Enum, SPA_CHOICE_Range>(
            pod, SPA_TYPE_OBJECT_Format, property);
    if (!choice)
        return { 0., 0. };

    return std::visit([](const auto &arg) {
        return parseFrameRateRange(arg);
    }, *choice);
}

} // namespace

std::optional<SpaObjectVideoFormat> SpaObjectVideoFormat::parse(const spa_pod *pod)
{
    if (!pod)
        return std::nullopt;

    const auto mediaType = spaParsePodPropertyScalar<spa_media_type>(*pod, SPA_TYPE_OBJECT_Format,
                                                                     SPA_FORMAT_mediaType);
    if (mediaType != SPA_MEDIA_TYPE_video)
        return std::nullopt;

    const auto mediaSubtype = spaParsePodPropertyScalar<spa_media_subtype>(
            *pod, SPA_TYPE_OBJECT_Format, SPA_FORMAT_mediaSubtype);
    if (!mediaSubtype)
        return std::nullopt;

    SpaObjectVideoFormat result{
        .mediaSubtype = *mediaSubtype,
        .pixelFormats = {},
        .resolutions = std::vector<QSize>{},
        .minFrameRate = 0.,
        .maxFrameRate = 0.,
    };

    switch (*mediaSubtype) {
    case SPA_MEDIA_SUBTYPE_raw:
        result.pixelFormats = parsePixelFormats(*pod);
        break;
    case SPA_MEDIA_SUBTYPE_mjpg:
        // Compressed subtypes carry no SPA_FORMAT_VIDEO_format property; the
        // pixel format is implied by the subtype.
        result.pixelFormats = { QVideoFrameFormat::Format_Jpeg };
        break;
    default:
        // h264, dsp, and friends are not something a QCamera can consume.
        return std::nullopt;
    }

    if (result.pixelFormats.empty())
        return std::nullopt;

    result.resolutions = parseResolutions(*pod);
    if (isEmpty(result.resolutions))
        return std::nullopt;

    std::tie(result.minFrameRate, result.maxFrameRate) =
            parseFrameRateRange(*pod, SPA_FORMAT_VIDEO_framerate);

    // Variable frame rate nodes report framerate 0/1 and put the real upper
    // bound in maxFramerate instead.
    if (result.maxFrameRate == 0.) {
        std::tie(result.minFrameRate, result.maxFrameRate) =
                parseFrameRateRange(*pod, SPA_FORMAT_VIDEO_maxFramerate);
    }

    return result;
}

QList<QCameraFormat> toCameraFormats(QSpan<const SpaObjectVideoFormat> formats)
{
    QList<QCameraFormat> result;

    for (const SpaObjectVideoFormat &format : formats) {
        for (QVideoFrameFormat::PixelFormat pixelFormat : format.pixelFormats) {
            for (QSize resolution : expandResolutions(format.resolutions)) {
                auto entry = std::make_unique<QCameraFormatPrivate>();
                entry->pixelFormat = pixelFormat;
                entry->resolution = resolution;
                entry->minFrameRate = format.minFrameRate;
                entry->maxFrameRate = format.maxFrameRate;

                QCameraFormat cameraFormat = QCameraFormatPrivate::create(std::move(entry));
                if (!result.contains(cameraFormat))
                    result.push_back(cameraFormat);
            }
        }
    }

    return result;
}

} // namespace QtPipeWire

QT_END_NAMESPACE
