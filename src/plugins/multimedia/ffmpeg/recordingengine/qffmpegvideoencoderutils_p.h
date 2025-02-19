// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QFFMPEGVIDEOENCODERUTILS_P_H
#define QFFMPEGVIDEOENCODERUTILS_P_H

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

#include <QtFFmpegMediaPluginImpl/private/qffmpegdefs_p.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpeghwaccel_p.h>

#include <QtCore/qspan.h>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

std::optional<AVPixelFormat> findTargetSWFormat(AVPixelFormat sourceSWFormat, const Codec &codec,
                                                const HWAccel &accel,
                                                const AVPixelFormatSet &prohibitedFormats = {});

std::optional<AVPixelFormat> findTargetFormat(AVPixelFormat sourceFormat,
                                              AVPixelFormat sourceSWFormat, const Codec &codec,
                                              const HWAccel *accel,
                                              const AVPixelFormatSet &prohibitedFormats = {});

AVScore findSWFormatScores(const Codec &codec, AVPixelFormat sourceSWFormat);

/**
 * @brief adjustFrameRate get a rational frame rate be requested qreal rate.
 *        If the codec supports fixed frame rate (non-null supportedRates),
 *        the function selects the most suitable one,
 *        otherwise just makes AVRational from qreal.
 */
AVRational adjustFrameRate(QSpan<const AVRational> supportedRates, qreal requestedRate);

/**
 * @brief adjustFrameTimeBase gets adjusted timebase by a list of supported frame rates
 *        and an already adjusted frame rate.
 *
 *        Timebase is the fundamental unit of time (in seconds) in terms
 *        of which frame timestamps are represented.
 *        For fixed-fps content (non-null supportedRates),
 *        timebase should be 1/framerate.
 *
 *        For more information, see AVStream::time_base and AVCodecContext::time_base.
 *
 *        The adjusted time base is supposed to be set to stream and codec context.
 */
AVRational adjustFrameTimeBase(QSpan<const AVRational> supportedRates, AVRational frameRate);

QSize adjustVideoResolution(const Codec &codec, QSize requestedResolution);

int getScaleConversionType(const QSize &sourceSize, const QSize &targetSize);

} // namespace QFFmpeg

QT_END_NAMESPACE

#endif // QFFMPEGVIDEOENCODERUTILS_P_H
