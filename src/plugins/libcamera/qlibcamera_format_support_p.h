// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QLIBCAMERA_FORMAT_SUPPORT_P_H
#define QLIBCAMERA_FORMAT_SUPPORT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtMultimedia/qvideoframeformat.h>
#include <QtCore/qsize.h>

#include <optional>
#include <utility>
#include <vector>

namespace libcamera {
class ColorSpace;
class ControlInfoMap;
class PixelFormat;
class Size;
class StreamFormats;
} // namespace libcamera

QT_BEGIN_NAMESPACE

namespace QLibCamera {

namespace QLibCameraFormatSupport {

std::optional<QVideoFrameFormat::PixelFormat>
pixelFormatFromLibcamera(const libcamera::PixelFormat &);

std::pair<QVideoFrameFormat::ColorSpace, QVideoFrameFormat::ColorRange>
colorSpaceFromLibcamera(const std::optional<libcamera::ColorSpace> &);

// Extracts a [minFrameRate, maxFrameRate] pair from controls::FrameDurationLimits. Not every pipeline
// handler advertises this pre-acquire(), so this returns a generic 1-30 fps fallback range rather than failing
std::pair<float, float> frameRateRangeFromControls(const libcamera::ControlInfoMap &);

std::vector<QSize> sizesForPixelFormat(const libcamera::StreamFormats &,
                                       const libcamera::PixelFormat &);

QSize toQSize(const libcamera::Size &);
libcamera::Size toSize(const QSize &);

} // namespace QLibCameraFormatSupport

} // namespace QLibCamera

QT_END_NAMESPACE

#endif // QLIBCAMERA_FORMAT_SUPPORT_P_H
