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

std::optional<AVPixelFormat> findTargetFormat(AVPixelFormat sourceSWFormat, const Codec &codec,
                                              const HWAccel *accel,
                                              const AVPixelFormatSet &prohibitedFormats = {});

AVScore findSWFormatScores(const Codec &codec, AVPixelFormat sourceSWFormat);

/**
 * @brief adjustFrameRate resolves the effective frame rate to negotiate with a codec.
 *
 *        settingsRate is the user-requested rate from QMediaEncoderSettings
 *        (<= 0 means unset). sourceRate is the source stream's fixed rate
 *        (<= 0 means variable/unknown).
 *
 *        Target rate priority: settingsRate, if set, always wins. Otherwise sourceRate,
 *        if fixed, is used. Otherwise, if the codec cannot do variable rate
 *        (non-empty supportedRates), DefaultVideoFrameRate is used as a
 *        last-resort target. If the codec can do variable rate (empty
 *        supportedRates) and neither settingsRate nor sourceRate is set,
 *        the result stays variable ({0, 1}).
 *
 *        If the codec supports only fixed frame rates (non-null
 *        supportedRates), the function selects the closest supported rate
 *        to the resolved preference; otherwise it converts the preference
 *        directly to an AVRational.
 */
AVRational adjustFrameRate(QSpan<const AVRational> supportedRates, qreal settingsRate,
                           qreal sourceRate);

/**
 * @brief adjustFrameTimeBase gets adjusted timebase by a list of supported frame rates
 *        and an already adjusted frame rate.
 *
 *        Timebase is the fundamental unit of time (in seconds) in terms
 *        of which frame timestamps are represented.
 *        For fixed-fps content (non-null supportedRates, or isFixedRate true),
 *        timebase should be 1/framerate.
 *
 *        For more information, see AVStream::time_base and AVCodecContext::time_base.
 *
 *        The adjusted time base is supposed to be set to stream and codec context.
 */
AVRational adjustFrameTimeBase(QSpan<const AVRational> supportedRates, AVRational frameRate,
                               bool isFixedRate);

QSize adjustVideoResolution(const Codec &codec, QSize requestedResolution);

SwsFlags getScaleConversionType(const QSize &sourceSize, const QSize &targetSize);

} // namespace QFFmpeg

QT_END_NAMESPACE

#endif // QFFMPEGVIDEOENCODERUTILS_P_H
