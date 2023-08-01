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

#include "qffmpeg_p.h"
#include "qffmpeghwaccel_p.h"

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

AVPixelFormat findTargetSWFormat(AVPixelFormat sourceSWFormat, const AVCodec *codec,
                                 const HWAccel &accel);

AVPixelFormat findTargetFormat(AVPixelFormat sourceFormat, AVPixelFormat sourceSWFormat,
                               const AVCodec *codec, const HWAccel *accel);

std::pair<const AVCodec *, std::unique_ptr<HWAccel>> findHwEncoder(AVCodecID codecID,
                                                                   const QSize &sourceSize);

const AVCodec *findSwEncoder(AVCodecID codecID, AVPixelFormat sourceSWFormat);

/**
 * @brief adjustFrameRate get a rational frame rate be requested qreal rate.
 *        If the codec supports fixed frame rate (non-null supportedRates),
 *        the function selects the most suitable one,
 *        otherwise just makes AVRational from qreal.
 */
AVRational adjustFrameRate(const AVRational *supportedRates, qreal requestedRate);

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
AVRational adjustFrameTimeBase(const AVRational *supportedRates, AVRational frameRate);

QSize adjustVideoResolution(const AVCodec *codec, QSize requestedResolution);

} // namespace QFFmpeg

QT_END_NAMESPACE

#endif // QFFMPEGVIDEOENCODERUTILS_P_H
