// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFFMPEGHWACCEL_MEDIACODEC_P_H
#define QFFMPEGHWACCEL_MEDIACODEC_P_H

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

#include <QtFFmpegMediaPluginImpl/private/qffmpeghwaccel_p.h>
#include <memory>

namespace QFFmpeg {
struct Frame;

class MediaCodecTextureConverter : public TextureConverterBackend
{
public:
    MediaCodecTextureConverter(QRhi *rhi) : TextureConverterBackend(rhi){};

    QVideoFrameTexturesUPtr createTextures(AVFrame *frame,
                                           QVideoFrameTexturesUPtr &oldTextures) override;

    QVideoFrameTexturesHandlesUPtr
    createTextureHandles(AVFrame *frame, QVideoFrameTexturesHandlesUPtr oldHandles) override;

    static void setupDecoderSurface(AVCodecContext *s);
private:
    std::unique_ptr<QRhiTexture> externalTexture;
    quint64 m_currentSurfaceIndex = 0;
};
}
#endif // QFFMPEGHWACCEL_MEDIACODEC_P_H
