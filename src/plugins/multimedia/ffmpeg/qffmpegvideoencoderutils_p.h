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

AVPixelFormat findTargetSWFormat(AVPixelFormat sourceSWFormat, const HWAccel &accel);

AVPixelFormat findTargetFormat(AVPixelFormat sourceFormat, AVPixelFormat sourceSWFormat,
                               const AVCodec *codec, const HWAccel *accel);

std::pair<const AVCodec *, std::unique_ptr<HWAccel>> findHwEncoder(AVCodecID codecID,
                                                                   const QSize &sourceSize);

const AVCodec *findSwEncoder(AVCodecID codecID, AVPixelFormat sourceFormat,
                             AVPixelFormat sourceSWFormat);

} // namespace QFFmpeg

QT_END_NAMESPACE

#endif // QFFMPEGVIDEOENCODERUTILS_P_H
