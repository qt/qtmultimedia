// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QFFMPEGHWACCEL_CUDA_P_H
#define QFFMPEGHWACCEL_CUDA_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtFFmpegMediaPluginImpl/private/qffmpegtextureconverter_p.h>

#include <QtMultimedia/private/qvideotexturehelper_p.h>

#include <memory>

QT_BEGIN_NAMESPACE

class QRhi;

namespace QFFmpeg {

// HW-texture converter for AV_PIX_FMT_CUDA frames: copies decoded planes from CUDA device
// memory into a GL- or Vulkan-registered texture (GPU-to-GPU memory copy, not zero-copy).
// Zero-copy isn't possible here: NVDEC's decode output is pitch-linear device memory, which
// isn't a format GL/Vulkan can sample as a texture without a copy/retiling step.
//
// NB: The Vulkan path requires the CUDA and QRhi Vulkan devices to be the same GPU.
//
struct CUDATextureConverterFactory
{
    static std::shared_ptr<TextureConverterBackend> create(QRhi *rhi);
};

} // namespace QFFmpeg

QT_END_NAMESPACE

#endif
