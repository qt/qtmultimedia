// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "rhi_support_p.h"

namespace QtMultimediaTest {

using namespace Qt::Literals;

std::unique_ptr<QRhi> createNullRhi()
{
    QRhiNullInitParams params;
    return std::unique_ptr<QRhi>{ QRhi::create(QRhi::Null, &params) };
}

#if QT_CONFIG(opengl)
std::unique_ptr<QRhi> createOffscreenGlRhi(std::unique_ptr<QOffscreenSurface> &fallbackSurface)
{
    QRhiGles2InitParams glParams;
    glParams.format = QSurfaceFormat::defaultFormat();
    fallbackSurface.reset(QRhiGles2InitParams::newFallbackSurface());
    glParams.fallbackSurface = fallbackSurface.get();
    return std::unique_ptr<QRhi>(QRhi::create(QRhi::OpenGLES2, &glParams));
}
#endif

#if QT_CONFIG(metal)
std::unique_ptr<QRhi> createOffscreenMetalRhi()
{
    QRhiMetalInitParams metalParams;
    return std::unique_ptr<QRhi>(QRhi::create(QRhi::Metal, &metalParams));
}
#endif

#if defined(Q_OS_WIN)
std::unique_ptr<QRhi> createOffscreenD3D11Rhi()
{
    QRhiD3D11InitParams params;
    return std::unique_ptr<QRhi>(QRhi::create(QRhi::D3D11, &params));
}

std::unique_ptr<QRhi> createWarpD3D11Rhi(const QWindowsD3D11TestDeviceContext &warpDevice)
{
    QRhiD3D11NativeHandles nativeHandles;
    nativeHandles.dev = warpDevice.device.Get();
    nativeHandles.context = warpDevice.context.Get();

    QRhiD3D11InitParams params;
    return std::unique_ptr<QRhi>{ QRhi::create(QRhi::D3D11, &params, {}, &nativeHandles) };
}
#endif

q23::expected<QByteArray, QString> readBackPlane(QRhi &rhi, quint64 handle, QSize planeSize,
                                                 QRhiTexture::Format format)
{
    std::unique_ptr<QRhiTexture> texture(
            rhi.newTexture(format, planeSize, 1, QRhiTexture::UsedAsTransferSource));
    if (!texture->createFrom(QRhiTexture::NativeTexture{ handle, 0 }))
        return q23::unexpected(u"QRhiTexture::createFrom failed"_s);

    QRhiResourceUpdateBatch *batch = rhi.nextResourceUpdateBatch();
    QRhiReadbackResult result;
    batch->readBackTexture(texture.get(), &result);

    QRhiCommandBuffer *cb = nullptr;
    if (rhi.beginOffscreenFrame(&cb) != QRhi::FrameOpSuccess || !cb)
        return q23::unexpected(u"QRhi::beginOffscreenFrame failed"_s);
    cb->resourceUpdate(batch);
    rhi.endOffscreenFrame();

    return result.data;
}

} // namespace QtMultimediaTest
