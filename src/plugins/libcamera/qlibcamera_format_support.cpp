// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qlibcamera_format_support_p.h"
#include "qlibcameralogging_p.h"

#include <QtMultimedia/private/qmultimedia_drm_support_p.h>
#include <QtMultimedia/private/qmultimedia_ranges_p.h>

#include <libcamera/base/span.h>
#include <libcamera/color_space.h>
#include <libcamera/control_ids.h>
#include <libcamera/controls.h>
#include <libcamera/geometry.h>
#include <libcamera/pixel_format.h>
#include <libcamera/stream.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

namespace QLibCamera {

using namespace libcamera;

namespace QLibCameraFormatSupport {

std::optional<QVideoFrameFormat::PixelFormat> pixelFormatFromLibcamera(const PixelFormat &format)
{
    using namespace QtMultimediaPrivate;
    return pixelFormatFromDrmFourcc(DRMFormat(format.fourcc()));
}

std::pair<QVideoFrameFormat::ColorSpace, QVideoFrameFormat::ColorRange>
colorSpaceFromLibcamera(const std::optional<ColorSpace> &colorSpace)
{
    if (!colorSpace)
        return { QVideoFrameFormat::ColorSpace_Undefined, QVideoFrameFormat::ColorRange_Unknown };

    QVideoFrameFormat::ColorSpace space = QVideoFrameFormat::ColorSpace_Undefined;
    switch (colorSpace->ycbcrEncoding) {
    case ColorSpace::YcbcrEncoding::Rec601:
        space = QVideoFrameFormat::ColorSpace_BT601;
        break;
    case ColorSpace::YcbcrEncoding::Rec709:
        space = QVideoFrameFormat::ColorSpace_BT709;
        break;
    case ColorSpace::YcbcrEncoding::Rec2020:
        space = QVideoFrameFormat::ColorSpace_BT2020;
        break;
    case ColorSpace::YcbcrEncoding::None:
        // RGB colorspace (e.g. sRGB) rather than YCbCr; fall back to the primaries.
        if (colorSpace->primaries == ColorSpace::Primaries::Rec2020)
            space = QVideoFrameFormat::ColorSpace_BT2020;
        break;
    }

    const QVideoFrameFormat::ColorRange range = colorSpace->range == ColorSpace::Range::Full
            ? QVideoFrameFormat::ColorRange_Full
            : QVideoFrameFormat::ColorRange_Video;

    return { space, range };
}

std::pair<float, float> frameRateRangeFromControls(const ControlInfoMap &controls)
{
    constexpr std::pair<float, float> fallbackRange{ 1.f, 30.f };

    const auto it = controls.find(&libcamera::controls::FrameDurationLimits);
    if (it == controls.end()) {
        qCDebug(lcLibCamera) << "FrameDurationLimits not advertised pre-acquire(); "
                                "falling back to a 1-30 fps range";
        return fallbackRange;
    }

    const ControlValue &min = it->second.min();
    const ControlValue &max = it->second.max();
    // FrameDurationLimits is declared as Control<Span<const int64_t, 2>>, but not every pipeline
    // handler actually populates ControlInfo::min()/max() as an array value of that shape (e.g.
    // libcamera's own "virtual" test pipeline handler does not, as of libcamera 0.7.0) --
    // ControlValue::get<Span<...>>() asserts isArray_ internally and aborts the process if that
    // assumption is wrong, so it must never be called without checking isArray() first.
    if (min.isNone() || max.isNone() || !min.isArray() || !max.isArray()) {
        qCDebug(lcLibCamera) << "FrameDurationLimits is not an array-typed min/max control; "
                                "falling back to a 1-30 fps range";
        return fallbackRange;
    }

    // FrameDurationLimits is { minDuration, maxDuration } in microseconds; convert to fps and
    // note that the *maximum* duration corresponds to the *minimum* frame rate and vice versa.
    const auto durations = min.get<Span<const int64_t, 2>>();
    const auto maxDurations = max.get<Span<const int64_t, 2>>();
    if (durations.size() < 1 || maxDurations.size() < 1)
        return fallbackRange;

    const int64_t minDurationUs = durations[0];
    const int64_t maxDurationUs = maxDurations[0];
    if (minDurationUs <= 0 || maxDurationUs <= 0)
        return fallbackRange;

    const float maxFrameRate = 1'000'000.f / float(minDurationUs);
    const float minFrameRate = 1'000'000.f / float(maxDurationUs);
    return { minFrameRate, maxFrameRate };
}

std::vector<QSize> sizesForPixelFormat(const StreamFormats &formats, const PixelFormat &pixelFormat)
{
    namespace ranges = QtMultimediaPrivate::ranges;

    std::vector<QSize> sizes;
    for (const Size &size : formats.sizes(pixelFormat))
        sizes.push_back(toQSize(size));

    // sizes() only intersects a fixed table of common resolutions against a continuous range, so
    // it can miss the camera's true native extremes; range() always reports them (as a bounding
    // box in the multi-entry/discrete case, or the range itself in the single-entry case), so
    // union them in rather than relying on sizes() alone.
    const SizeRange range = formats.range(pixelFormat);
    if (!range.min.isNull() || !range.max.isNull()) {
        sizes.push_back(toQSize(range.min));
        if (range.min != range.max)
            sizes.push_back(toQSize(range.max));
    }

    ranges::sort(sizes, [](const QSize &a, const QSize &b) {
        return std::pair(a.width(), a.height()) < std::pair(b.width(), b.height());
    });
    sizes.erase(std::unique(sizes.begin(), sizes.end()), sizes.end());
    return sizes;
}

QSize toQSize(const Size &size)
{
    return QSize{
        int(size.width),
        int(size.height),
    };
}

Size toSize(const QSize &qsize)
{
    return Size{
        unsigned(qsize.width()),
        unsigned(qsize.height()),
    };
}

} // namespace QLibCameraFormatSupport

} // namespace QLibCamera

QT_END_NAMESPACE
