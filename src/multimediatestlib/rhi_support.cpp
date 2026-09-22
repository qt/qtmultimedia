// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "rhi_support_p.h"

#include <QtMultimedia/private/qvideotexturehelper_p.h>
#include <QtMultimedia/qvideoframeformat.h>

#include <QtCore/qfile.h>

namespace QtMultimediaTest {

using namespace Qt::Literals;

std::unique_ptr<QRhi> createNullRhi()
{
    QRhiNullInitParams params;
    return std::unique_ptr<QRhi>{ QRhi::create(QRhi::Null, &params) };
}

#if QT_CONFIG(opengl)
q23::expected<OffscreenGlRhi, QString> createOffscreenGlRhi()
{
    std::unique_ptr<QOffscreenSurface> fallbackSurface{
        QRhiGles2InitParams::newFallbackSurface(),
    };

    QRhiGles2InitParams glParams;
    glParams.format = QSurfaceFormat::defaultFormat();
    glParams.fallbackSurface = fallbackSurface.get();
    std::unique_ptr<QRhi> rhi{
        QRhi::create(QRhi::OpenGLES2, &glParams),
    };
    if (!rhi)
        return q23::unexpected(u"QRhi::create failed"_s);

    return OffscreenGlRhi{
        std::move(fallbackSurface),
        std::move(rhi),
    };
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

#if QT_CONFIG(opengl)

namespace {

q23::expected<QShader, QString> loadShader(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return q23::unexpected(u"Could not open shader file: "_s + path);
    QShader shader = QShader::fromSerialized(file.readAll());
    if (!shader.isValid())
        return q23::unexpected(u"Invalid/unsupported shader file: "_s + path);
    return shader;
}

} // namespace

// Unlike readBackPlane(), EXTERNAL_OES textures can't be read back directly: they're only
// consumable by a shader (samplerExternalOES). Renders the texture through a full-screen quad
// using this module's own externalsampler shader into a plain RGBA8 target, then reads that back.
q23::expected<QByteArray, QString> readBackExternalOesTexture(QRhi &rhi, quint64 handle, QSize size)
{
    std::unique_ptr<QRhiTexture> sourceTexture{
        rhi.newTexture(QRhiTexture::RGBA8, size, 1, QRhiTexture::ExternalOES),
    };
    if (!sourceTexture->createFrom(QRhiTexture::NativeTexture{ handle, 0 }))
        return q23::unexpected(u"Failed to wrap the native handle as an ExternalOES QRhiTexture"_s);

    std::unique_ptr<QRhiTexture> targetTexture{
        rhi.newTexture(QRhiTexture::RGBA8, size, 1,
                       QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource),
    };
    if (!targetTexture->create())
        return q23::unexpected(u"Failed to create the readback render target texture"_s);

    QRhiColorAttachment colorAttachment(targetTexture.get());
    std::unique_ptr<QRhiTextureRenderTarget> renderTarget{
        rhi.newTextureRenderTarget(QRhiTextureRenderTargetDescription(colorAttachment)),
    };
    std::unique_ptr<QRhiRenderPassDescriptor> renderPassDescriptor{
        renderTarget->newCompatibleRenderPassDescriptor(),
    };
    renderTarget->setRenderPassDescriptor(renderPassDescriptor.get());
    if (!renderTarget->create())
        return q23::unexpected(u"Failed to create the readback render target"_s);

    const QVideoFrameFormat format(size, QVideoFrameFormat::Format_SamplerExternalOES);
    auto vertexShader = loadShader(QVideoTextureHelper::vertexShaderFileName(format));
    if (!vertexShader)
        return q23::unexpected(vertexShader.error());
    auto fragmentShader = loadShader(QVideoTextureHelper::fragmentShaderFileName(format, &rhi));
    if (!fragmentShader)
        return q23::unexpected(fragmentShader.error());

    std::unique_ptr<QRhiSampler> sampler{
        rhi.newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                       QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge),
    };
    if (!sampler->create())
        return q23::unexpected(u"Failed to create the sampler"_s);

    std::unique_ptr<QRhiBuffer> uniformBuffer{
        rhi.newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer,
                      sizeof(QVideoTextureHelper::UniformData)),
    };
    if (!uniformBuffer->create())
        return q23::unexpected(u"Failed to create the uniform buffer"_s);

    std::unique_ptr<QRhiShaderResourceBindings> srb(rhi.newShaderResourceBindings());
    srb->setBindings({
            QRhiShaderResourceBinding::uniformBuffer(
                    0,
                    QRhiShaderResourceBinding::VertexStage
                            | QRhiShaderResourceBinding::FragmentStage,
                    uniformBuffer.get()),
            QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage,
                                                      sourceTexture.get(), sampler.get()),
    });
    if (!srb->create())
        return q23::unexpected(u"Failed to create the shader resource bindings"_s);

    // A single full-screen quad (vertexPosition: vec4, vertexTexCoord: vec2, interleaved),
    // matching the vertex layout every one of this module's video shaders expects.
    static constexpr float vertexData[] = {
        // x,    y,   z,   w,   u,   v
        -1.f, -1.f, 0.f, 1.f, 0.f, 0.f, //
        1.f,  -1.f, 0.f, 1.f, 1.f, 0.f, //
        -1.f, 1.f,  0.f, 1.f, 0.f, 1.f, //
        1.f,  1.f,  0.f, 1.f, 1.f, 1.f,
    };
    std::unique_ptr<QRhiBuffer> vertexBuffer{
        rhi.newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(vertexData)),
    };
    if (!vertexBuffer->create())
        return q23::unexpected(u"Failed to create the vertex buffer"_s);

    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({ { 6 * sizeof(float) } });
    inputLayout.setAttributes({
            { 0, 0, QRhiVertexInputAttribute::Float4, 0 },
            { 0, 1, QRhiVertexInputAttribute::Float2, 4 * sizeof(float) },
    });

    std::unique_ptr<QRhiGraphicsPipeline> pipeline(rhi.newGraphicsPipeline());
    pipeline->setShaderStages({
            { QRhiShaderStage::Vertex, *vertexShader },
            { QRhiShaderStage::Fragment, *fragmentShader },
    });
    pipeline->setVertexInputLayout(inputLayout);
    pipeline->setShaderResourceBindings(srb.get());
    pipeline->setRenderPassDescriptor(renderPassDescriptor.get());
    pipeline->setTopology(QRhiGraphicsPipeline::TriangleStrip);
    if (!pipeline->create())
        return q23::unexpected(u"Failed to create the graphics pipeline"_s);

    QRhiCommandBuffer *cb = nullptr;
    if (rhi.beginOffscreenFrame(&cb) != QRhi::FrameOpSuccess || !cb)
        return q23::unexpected(u"QRhi::beginOffscreenFrame failed"_s);

    QRhiResourceUpdateBatch *batch = rhi.nextResourceUpdateBatch();
    batch->uploadStaticBuffer(vertexBuffer.get(), vertexData);

    QVideoTextureHelper::UniformData uniformData{};
    const QMatrix4x4 identity;
    memcpy(uniformData.transformMatrix, identity.constData(), sizeof(uniformData.transformMatrix));
    memcpy(uniformData.colorMatrix, identity.constData(), sizeof(uniformData.colorMatrix));
    uniformData.opacity = 1.f;
    batch->updateDynamicBuffer(uniformBuffer.get(), 0, sizeof(uniformData), &uniformData);

    cb->beginPass(renderTarget.get(), Qt::black, { 1.0f, 0 }, batch);
    cb->setGraphicsPipeline(pipeline.get());
    cb->setViewport({ 0, 0, float(size.width()), float(size.height()) });
    cb->setShaderResources(srb.get());
    const QRhiCommandBuffer::VertexInput vertexBinding(vertexBuffer.get(), 0);
    cb->setVertexInput(0, 1, &vertexBinding);
    cb->draw(4);
    cb->endPass();

    QRhiReadbackResult result;
    QRhiResourceUpdateBatch *readbackBatch = rhi.nextResourceUpdateBatch();
    readbackBatch->readBackTexture(targetTexture.get(), &result);
    cb->resourceUpdate(readbackBatch);

    rhi.endOffscreenFrame();

    return result.data;
}

#endif // QT_CONFIG(opengl)

} // namespace QtMultimediaTest
