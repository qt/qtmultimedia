// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qvideoframeconverter_p.h"
#include "qvideoframeconversionhelper_p.h"
#include "qvideoframeformat.h"
#include "qvideoframe_p.h"
#include "qmultimediautils_p.h"
#include "qthreadlocalrhi_p.h"
#include "qcachedvalue_p.h"

#include <QtCore/qcoreapplication.h>
#include <QtCore/qsize.h>
#include <QtCore/qhash.h>
#include <QtCore/qfile.h>
#include <QtGui/qimage.h>
#include <QtCore/qloggingcategory.h>

#include <private/qvideotexturehelper_p.h>

#include <rhi/qrhi.h>

#ifdef Q_OS_DARWIN
#include <QtCore/private/qcore_mac_p.h>
#endif

QT_BEGIN_NAMESPACE

static Q_LOGGING_CATEGORY(qLcVideoFrameConverter, "qt.multimedia.video.frameconverter")

// TODO: investigate if we should use thread_local instead, QTBUG-133565
static const float g_quad[] = {
    // Rotation 0 CW
    1.f, -1.f,   1.f, 1.f,
    1.f,  1.f,   1.f, 0.f,
   -1.f, -1.f,   0.f, 1.f,
   -1.f,  1.f,   0.f, 0.f,
    // Rotation 90 CW
    1.f, -1.f,   1.f, 0.f,
    1.f,  1.f,   0.f, 0.f,
   -1.f, -1.f,   1.f, 1.f,
   -1.f,  1.f,   0.f, 1.f,
    // Rotation 180 CW
    1.f, -1.f,   0.f, 0.f,
    1.f,  1.f,   0.f, 1.f,
   -1.f, -1.f,   1.f, 0.f,
   -1.f,  1.f,   1.f, 1.f,
    // Rotation 270 CW
    1.f, -1.f,  0.f, 1.f,
    1.f,  1.f,  1.f, 1.f,
   -1.f, -1.f,  0.f, 0.f,
   -1.f,  1.f,  1.f, 0.f,
};

static bool pixelFormatHasAlpha(QVideoFrameFormat::PixelFormat format)
{
    switch (format) {
    case  QVideoFrameFormat::Format_ARGB8888:
    case  QVideoFrameFormat::Format_ARGB8888_Premultiplied:
    case  QVideoFrameFormat::Format_BGRA8888:
    case  QVideoFrameFormat::Format_BGRA8888_Premultiplied:
    case  QVideoFrameFormat::Format_ABGR8888:
    case  QVideoFrameFormat::Format_RGBA8888:
    case  QVideoFrameFormat::Format_AYUV:
    case  QVideoFrameFormat::Format_AYUV_Premultiplied:
        return true;
    default:
        return false;
    }
};

static QShader ensureShader(const QString &name)
{
    static QCachedValueMap<QString, QShader> shaderCache;

    return shaderCache.ensure(name, [&name]() {
        QFile f(name);
        return f.open(QIODevice::ReadOnly) ? QShader::fromSerialized(f.readAll()) : QShader();
    });
}

static void rasterTransform(QImage &image, VideoTransformation transformation)
{
    QTransform t;
    if (transformation.rotation != QtVideo::Rotation::None)
        t.rotate(qreal(transformation.rotation));
    if (transformation.mirrorredHorizontallyAfterRotation)
        t.scale(-1., 1);
    if (!t.isIdentity())
        image = image.transformed(t);
}

static void imageCleanupHandler(void *info)
{
    QByteArray *imageData = reinterpret_cast<QByteArray *>(info);
    delete imageData;
}

static bool updateTextures(QRhi *rhi,
                           std::unique_ptr<QRhiBuffer> &uniformBuffer,
                           std::unique_ptr<QRhiSampler> &textureSampler,
                           std::unique_ptr<QRhiShaderResourceBindings> &shaderResourceBindings,
                           std::unique_ptr<QRhiGraphicsPipeline> &graphicsPipeline,
                           std::unique_ptr<QRhiRenderPassDescriptor> &renderPass,
                           QVideoFrame &frame,
                           const QVideoFrameTexturesUPtr &videoFrameTextures)
{
    auto format = frame.surfaceFormat();
    auto pixelFormat = format.pixelFormat();

    auto textureDesc = QVideoTextureHelper::textureDescription(pixelFormat);

    QRhiShaderResourceBinding bindings[4];
    auto *b = bindings;
    *b++ = QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
                                                    uniformBuffer.get());
    for (int i = 0; i < textureDesc->nplanes; ++i)
        *b++ = QRhiShaderResourceBinding::sampledTexture(i + 1, QRhiShaderResourceBinding::FragmentStage,
                                                         videoFrameTextures->texture(i), textureSampler.get());
    shaderResourceBindings->setBindings(bindings, b);
    shaderResourceBindings->create();

    graphicsPipeline.reset(rhi->newGraphicsPipeline());
    graphicsPipeline->setTopology(QRhiGraphicsPipeline::TriangleStrip);

    QShader vs = ensureShader(QVideoTextureHelper::vertexShaderFileName(format));
    if (!vs.isValid())
        return false;

    QShader fs = ensureShader(QVideoTextureHelper::fragmentShaderFileName(format, rhi));
    if (!fs.isValid())
        return false;

    graphicsPipeline->setShaderStages({
        { QRhiShaderStage::Vertex, vs },
        { QRhiShaderStage::Fragment, fs }
    });

    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({
        { 4 * sizeof(float) }
    });
    inputLayout.setAttributes({
        { 0, 0, QRhiVertexInputAttribute::Float2, 0 },
        { 0, 1, QRhiVertexInputAttribute::Float2, 2 * sizeof(float) }
    });

    graphicsPipeline->setVertexInputLayout(inputLayout);
    graphicsPipeline->setShaderResourceBindings(shaderResourceBindings.get());
    graphicsPipeline->setRenderPassDescriptor(renderPass.get());
    graphicsPipeline->create();

    return true;
}

static QImage convertJPEG(const QVideoFrame &frame, const VideoTransformation &transform)
{
    QVideoFrame varFrame = frame;
    if (!varFrame.map(QVideoFrame::ReadOnly)) {
        qCDebug(qLcVideoFrameConverter) << Q_FUNC_INFO << ": frame mapping failed";
        return {};
    }
    QImage image;
    image.loadFromData(varFrame.bits(0), varFrame.mappedBytes(0), "JPG");
    varFrame.unmap();
    rasterTransform(image, transform);
    return image;
}

static QImage convertCPU(const QVideoFrame &frame, const VideoTransformation &transform)
{
    VideoFrameConvertFunc convert = qConverterForFormat(frame.pixelFormat());
    if (!convert) {
        qCDebug(qLcVideoFrameConverter) << Q_FUNC_INFO << ": unsupported pixel format" << frame.pixelFormat();
        return {};
    } else {
        QVideoFrame varFrame = frame;
        if (!varFrame.map(QVideoFrame::ReadOnly)) {
            qCDebug(qLcVideoFrameConverter) << Q_FUNC_INFO << ": frame mapping failed";
            return {};
        }
        auto format = pixelFormatHasAlpha(varFrame.pixelFormat()) ? QImage::Format_ARGB32_Premultiplied : QImage::Format_RGB32;
        QImage image = QImage(varFrame.width(), varFrame.height(), format);
        convert(varFrame, image.bits());
        varFrame.unmap();
        rasterTransform(image, transform);
        return image;
    }
}

QImage qImageFromVideoFrame(const QVideoFrame &frame, bool forceCpu)
{
    // by default, surface transformation is applied, as full transformation is used for presentation only
    return qImageFromVideoFrame(frame, qNormalizedSurfaceTransformation(frame.surfaceFormat()),
                                forceCpu);
}

QImage qImageFromVideoFrame(const QVideoFrame &frame, const VideoTransformation &transformation,
                            bool forceCpu)
{
#ifdef Q_OS_DARWIN
    QMacAutoReleasePool releasePool;
#endif

    std::unique_ptr<QRhiRenderPassDescriptor> renderPass;
    std::unique_ptr<QRhiBuffer> vertexBuffer;
    std::unique_ptr<QRhiBuffer> uniformBuffer;
    std::unique_ptr<QRhiTexture> targetTexture;
    std::unique_ptr<QRhiTextureRenderTarget> renderTarget;
    std::unique_ptr<QRhiSampler> textureSampler;
    std::unique_ptr<QRhiShaderResourceBindings> shaderResourceBindings;
    std::unique_ptr<QRhiGraphicsPipeline> graphicsPipeline;

    if (frame.size().isEmpty() || frame.pixelFormat() == QVideoFrameFormat::Format_Invalid)
        return {};

    if (frame.pixelFormat() == QVideoFrameFormat::Format_Jpeg)
        return convertJPEG(frame, transformation);

    if (forceCpu) // For test purposes
        return convertCPU(frame, transformation);

    QRhi *rhi = nullptr;

    if (QHwVideoBuffer *buffer = QVideoFramePrivate::hwBuffer(frame))
        rhi = buffer->rhi();

    if (!rhi || !rhi->thread()->isCurrentThread())
        rhi = ensureThreadLocalRhi(rhi);

    if (!rhi || rhi->isRecordingFrame())
        return convertCPU(frame, transformation);

    // Do conversion using shaders

    const QSize frameSize = qRotatedFrameSize(frame.size(), frame.surfaceFormat().rotation());

    vertexBuffer.reset(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(g_quad)));
    vertexBuffer->create();

    uniformBuffer.reset(rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, sizeof(QVideoTextureHelper::UniformData)));
    uniformBuffer->create();

    textureSampler.reset(rhi->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                         QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge));
    textureSampler->create();

    shaderResourceBindings.reset(rhi->newShaderResourceBindings());

    targetTexture.reset(rhi->newTexture(QRhiTexture::RGBA8, frameSize, 1, QRhiTexture::RenderTarget));
    if (!targetTexture->create()) {
        qCDebug(qLcVideoFrameConverter) << "Failed to create target texture. Using CPU conversion.";
        return convertCPU(frame, transformation);
    }

    renderTarget.reset(rhi->newTextureRenderTarget({ { targetTexture.get() } }));
    renderPass.reset(renderTarget->newCompatibleRenderPassDescriptor());
    renderTarget->setRenderPassDescriptor(renderPass.get());
    renderTarget->create();

    QRhiCommandBuffer *cb = nullptr;
    QRhi::FrameOpResult r = rhi->beginOffscreenFrame(&cb);
    if (r != QRhi::FrameOpSuccess) {
        qCDebug(qLcVideoFrameConverter) << "Failed to set up offscreen frame. Using CPU conversion.";
        return convertCPU(frame, transformation);
    }

    QRhiResourceUpdateBatch *rub = rhi->nextResourceUpdateBatch();
    Q_ASSERT(rub);

    rub->uploadStaticBuffer(vertexBuffer.get(), g_quad);

    QVideoFrame frameTmp = frame;
    auto videoFrameTextures = QVideoTextureHelper::createTextures(frameTmp, *rhi, *rub, {});
    if (!videoFrameTextures) {
        qCDebug(qLcVideoFrameConverter) << "Failed obtain textures. Using CPU conversion.";
        return convertCPU(frame, transformation);
    }

    if (!updateTextures(rhi, uniformBuffer, textureSampler, shaderResourceBindings,
                        graphicsPipeline, renderPass, frameTmp, videoFrameTextures)) {
        qCDebug(qLcVideoFrameConverter) << "Failed to update textures. Using CPU conversion.";
        return convertCPU(frame, transformation);
    }

    float xScale = transformation.mirrorredHorizontallyAfterRotation ? -1.0 : 1.0;
    float yScale = 1.f;

    if (rhi->isYUpInFramebuffer())
        yScale = -yScale;

    QMatrix4x4 transform;
    transform.scale(xScale, yScale);

    QByteArray uniformData(sizeof(QVideoTextureHelper::UniformData), Qt::Uninitialized);
    QVideoTextureHelper::updateUniformData(&uniformData, rhi, frame.surfaceFormat(), frame,
                                           transform, 1.f);
    rub->updateDynamicBuffer(uniformBuffer.get(), 0, uniformData.size(), uniformData.constData());

    cb->beginPass(renderTarget.get(), Qt::black, { 1.0f, 0 }, rub);
    cb->setGraphicsPipeline(graphicsPipeline.get());

    cb->setViewport({ 0, 0, float(frameSize.width()), float(frameSize.height()) });
    cb->setShaderResources(shaderResourceBindings.get());

    const quint32 vertexOffset = quint32(sizeof(float)) * 16 * transformation.rotationIndex();
    const QRhiCommandBuffer::VertexInput vbufBinding(vertexBuffer.get(), vertexOffset);
    cb->setVertexInput(0, 1, &vbufBinding);
    cb->draw(4);

    QRhiReadbackDescription readDesc(targetTexture.get());
    QRhiReadbackResult readResult;
    bool readCompleted = false;

    readResult.completed = [&readCompleted] { readCompleted = true; };

    rub = rhi->nextResourceUpdateBatch();
    rub->readBackTexture(readDesc, &readResult);

    cb->endPass(rub);

    rhi->endOffscreenFrame();

    if (!readCompleted) {
        qCDebug(qLcVideoFrameConverter) << "Failed to read back texture. Using CPU conversion.";
        return convertCPU(frame, transformation);
    }

    QByteArray *imageData = new QByteArray(readResult.data);

    return QImage(reinterpret_cast<const uchar *>(imageData->constData()),
                  readResult.pixelSize.width(), readResult.pixelSize.height(),
                  QImage::Format_RGBA8888_Premultiplied, imageCleanupHandler, imageData);
}

QImage videoFramePlaneAsImage(QVideoFrame &frame, int plane, QImage::Format targetFormat,
                              QSize targetSize)
{
    if (plane >= frame.planeCount())
        return {};

    if (!frame.map(QVideoFrame::ReadOnly)) {
        qWarning() << "Cannot map a video frame in ReadOnly mode!";
        return {};
    }

    auto frameHandle = QVideoFramePrivate::handle(frame);

    // With incrementing the reference counter, we share the mapped QVideoFrame
    // with the target QImage. The function imageCleanupFunction is going to adopt
    // the frameHandle by QVideoFrame and dereference it upon the destruction.
    frameHandle->ref.ref();

    auto imageCleanupFunction = [](void *data) {
        QVideoFrame frame = reinterpret_cast<QVideoFramePrivate *>(data)->adoptThisByVideoFrame();
        Q_ASSERT(frame.isMapped());
        frame.unmap();
    };

    const auto bytesPerLine = frame.bytesPerLine(plane);
    const auto height =
            bytesPerLine ? qMin(targetSize.height(), frame.mappedBytes(plane) / bytesPerLine) : 0;

    return QImage(reinterpret_cast<const uchar *>(frame.bits(plane)), targetSize.width(), height,
                  bytesPerLine, targetFormat, imageCleanupFunction, frameHandle);
}

QT_END_NAMESPACE

