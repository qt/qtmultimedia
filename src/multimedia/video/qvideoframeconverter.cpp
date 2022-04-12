/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qvideoframeconverter_p.h"
#include "qvideoframeconversionhelper_p.h"
#include "qvideoframeformat.h"

#include <QtGui/private/qrhinull_p.h>
#if QT_CONFIG(opengl)
#include <QtGui/private/qrhigles2_p.h>
#include <QOffscreenSurface>
#endif
#if QT_CONFIG(vulkan)
#include <QtGui/private/qrhivulkan_p.h>
#endif
#ifdef Q_OS_WIN
#include <QtGui/private/qrhid3d11_p.h>
#endif
#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
#include <QtGui/private/qrhimetal_p.h>
#endif

#include <QtCore/qcoreapplication.h>
#include <QtCore/qsize.h>
#include <QtCore/qhash.h>
#include <QtCore/qfile.h>
#include <QtCore/qthreadstorage.h>
#include <QtGui/qimage.h>
#include <qpa/qplatformintegration.h>
#include <private/qvideotexturehelper_p.h>
#include <private/qabstractvideobuffer_p.h>
#include <private/qguiapplication_p.h>
#include <private/qrhi_p.h>


QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(qLcVideoFrameConverter, "qt.multimedia.video.frameconverter")

namespace {

struct State
{
    QRhi *rhi = nullptr;
#if QT_CONFIG(opengl)
    QOffscreenSurface *fallbackSurface = nullptr;
#endif
    bool cpuOnly = false;
    ~State() {
        delete rhi;
#if QT_CONFIG(opengl)
        delete fallbackSurface;
#endif
    }
};

}

static QThreadStorage<State> g_state;
static QHash<QString, QShader> g_shaderCache;

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

static QShader getShader(const QString &name)
{
    QShader shader = g_shaderCache.value(name);
    if (shader.isValid())
        return shader;

    QFile f(name);
    if (f.open(QIODevice::ReadOnly))
        shader = QShader::fromSerialized(f.readAll());

    if (shader.isValid())
        g_shaderCache[name] = shader;

    return shader;
}

static void rasterTransform(QImage &image, QVideoFrame::RotationAngle rotation,
                            bool mirrorX, bool mirrorY)
{
    QTransform t;
    if (mirrorX)
        t.scale(-1.f, 1.f);
    if (rotation != QVideoFrame::Rotation0)
        t.rotate(float(rotation));
    if (mirrorY)
        t.scale(1.f, -1.f);
    if (!t.isIdentity())
        image = image.transformed(t);
}

static void imageCleanupHandler(void *info)
{
    QByteArray *imageData = reinterpret_cast<QByteArray *>(info);
    delete imageData;
}

static QRhi *initializeRHI(QRhi::Implementation backend)
{
    if (g_state.localData().rhi || g_state.localData().cpuOnly)
        return g_state.localData().rhi;

    if (QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::RhiBasedRendering)) {

#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
        if (backend == QRhi::Metal || backend == QRhi::Null) {
            QRhiMetalInitParams params;
            g_state.localData().rhi = QRhi::create(QRhi::Metal, &params);
        }
#endif

#if defined(Q_OS_WIN)
        if (backend == QRhi::D3D11 || backend == QRhi::Null) {
            QRhiD3D11InitParams params;
            g_state.localData().rhi = QRhi::create(QRhi::D3D11, &params);
        }
#endif

#if QT_CONFIG(opengl)
        if (!g_state.localData().rhi && (backend == QRhi::OpenGLES2 || backend == QRhi::Null)) {
            if (QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::OpenGL)
                    && QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::RasterGLSurface)
                    && !QCoreApplication::testAttribute(Qt::AA_ForceRasterWidgets)) {

                g_state.localData().fallbackSurface = QRhiGles2InitParams::newFallbackSurface();
                QRhiGles2InitParams params;
                params.fallbackSurface = g_state.localData().fallbackSurface;
                g_state.localData().rhi = QRhi::create(QRhi::OpenGLES2, &params);
            }
        }
#endif
    }

    if (!g_state.localData().rhi) {
        g_state.localData().cpuOnly = true;
        qWarning() << Q_FUNC_INFO << ": No RHI backend. Using CPU conversion.";
    }

    return g_state.localData().rhi;
}

static bool updateTextures(QRhi *rhi,
                           QRhiResourceUpdateBatch *rub,
                           std::unique_ptr<QRhiBuffer> &uniformBuffer,
                           std::unique_ptr<QRhiSampler> &textureSampler,
                           std::unique_ptr<QRhiShaderResourceBindings> &shaderResourceBindings,
                           std::unique_ptr<QRhiGraphicsPipeline> &graphicsPipeline,
                           std::unique_ptr<QRhiRenderPassDescriptor> &renderPass,
                           const QVideoFrame &frame,
                           std::unique_ptr<QRhiTexture> (&textures)[QVideoTextureHelper::TextureDescription::maxPlanes])
{
    auto format = frame.surfaceFormat();
    auto pixelFormat = format.pixelFormat();

    auto textureDesc = QVideoTextureHelper::textureDescription(pixelFormat);

    for (int i = 0; i < QVideoTextureHelper::TextureDescription::maxPlanes; ++i)
        QVideoTextureHelper::updateRhiTexture(frame, rhi, rub, i, textures[i]);

    QRhiShaderResourceBinding bindings[4];
    auto *b = bindings;
    *b++ = QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
                                                    uniformBuffer.get());
    for (int i = 0; i < textureDesc->nplanes; ++i)
        *b++ = QRhiShaderResourceBinding::sampledTexture(i + 1, QRhiShaderResourceBinding::FragmentStage,
                                                         textures[i].get(), textureSampler.get());
    shaderResourceBindings->setBindings(bindings, b);
    shaderResourceBindings->create();

    graphicsPipeline.reset(rhi->newGraphicsPipeline());
    graphicsPipeline->setTopology(QRhiGraphicsPipeline::TriangleStrip);

    QShader vs = getShader(QVideoTextureHelper::vertexShaderFileName(format));
    if (!vs.isValid())
        return false;

    QShader fs = getShader(QVideoTextureHelper::fragmentShaderFileName(format));
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

static QImage convertJPEG(const QVideoFrame &frame, QVideoFrame::RotationAngle rotation, bool mirrorX, bool mirrorY)
{
    QVideoFrame varFrame = frame;
    if (!varFrame.map(QVideoFrame::ReadOnly)) {
        qCDebug(qLcVideoFrameConverter) << Q_FUNC_INFO << ": frame mapping failed";
        return {};
    }
    QImage image;
    image.loadFromData(varFrame.bits(0), varFrame.mappedBytes(0), "JPG");
    varFrame.unmap();
    rasterTransform(image, rotation, mirrorX, mirrorY);
    return image;
}

static QImage convertCPU(const QVideoFrame &frame, QVideoFrame::RotationAngle rotation, bool mirrorX, bool mirrorY)
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
        rasterTransform(image, rotation, mirrorX, mirrorY);
        return image;
    }
}

QImage qImageFromVideoFrame(const QVideoFrame &frame, QVideoFrame::RotationAngle rotation, bool mirrorX, bool mirrorY)
{
#ifdef Q_OS_DARWIN
    QMacAutoReleasePool releasePool;
#endif

    if (!g_state.hasLocalData())
        g_state.setLocalData({});

    std::unique_ptr<QRhiRenderPassDescriptor> renderPass;
    std::unique_ptr<QRhiBuffer> vertexBuffer;
    std::unique_ptr<QRhiBuffer> uniformBuffer;
    std::unique_ptr<QRhiTexture> targetTexture;
    std::unique_ptr<QRhiTextureRenderTarget> renderTarget;
    std::unique_ptr<QRhiSampler> textureSampler;
    std::unique_ptr<QRhiShaderResourceBindings> shaderResourceBindings;
    std::unique_ptr<QRhiGraphicsPipeline> graphicsPipeline;
    std::unique_ptr<QRhiTexture> frameTextures[QVideoTextureHelper::TextureDescription::maxPlanes];

    if (frame.size().isEmpty() || frame.pixelFormat() == QVideoFrameFormat::Format_Invalid)
        return {};

    if (frame.pixelFormat() == QVideoFrameFormat::Format_Jpeg)
        return convertJPEG(frame, rotation, mirrorX, mirrorY);

    QRhi *rhi = nullptr;
    QRhi::Implementation backend = QRhi::Null;

    if (frame.videoBuffer()) {
        rhi = frame.videoBuffer()->rhi();
        if (rhi)
            backend = rhi->backend();
    }

    if (!rhi || rhi->thread() != QThread::currentThread())
        rhi = initializeRHI(backend);

    if (!rhi || rhi->isRecordingFrame())
        return convertCPU(frame, rotation, mirrorX, mirrorY);

    // Do conversion using shaders

    const int rotationIndex = (rotation / 90) % 4;

    QSize frameSize = frame.size();
    if (rotationIndex % 2)
        frameSize.transpose();

    vertexBuffer.reset(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(g_quad)));
    vertexBuffer->create();

    uniformBuffer.reset(rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 64 + 64 + 4 + 4 + 4 + 4));
    uniformBuffer->create();

    textureSampler.reset(rhi->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                         QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge));
    textureSampler->create();

    shaderResourceBindings.reset(rhi->newShaderResourceBindings());

    targetTexture.reset(rhi->newTexture(QRhiTexture::RGBA8, frameSize, 1, QRhiTexture::RenderTarget));
    if (!targetTexture->create()) {
        qCDebug(qLcVideoFrameConverter) << "Failed to create target texture. Using CPU conversion.";
        return convertCPU(frame, rotation, mirrorX, mirrorY);
    }

    renderTarget.reset(rhi->newTextureRenderTarget({ { targetTexture.get() } }));
    renderPass.reset(renderTarget->newCompatibleRenderPassDescriptor());
    renderTarget->setRenderPassDescriptor(renderPass.get());
    renderTarget->create();

    QRhiCommandBuffer *cb = nullptr;
    QRhi::FrameOpResult r = rhi->beginOffscreenFrame(&cb);
    if (r != QRhi::FrameOpSuccess) {
        qCDebug(qLcVideoFrameConverter) << "Failed to set up offscreen frame. Using CPU conversion.";
        return convertCPU(frame, rotation, mirrorX, mirrorY);
    }

    QRhiResourceUpdateBatch *rub = rhi->nextResourceUpdateBatch();

    rub->uploadStaticBuffer(vertexBuffer.get(), g_quad);

    if (!updateTextures(rhi, rub, uniformBuffer, textureSampler, shaderResourceBindings,
                        graphicsPipeline, renderPass, frame, frameTextures)) {
        qCDebug(qLcVideoFrameConverter) << "Failed to update textures. Using CPU conversion.";
        return convertCPU(frame, rotation, mirrorX, mirrorY);
    }

    float xScale = mirrorX ? -1.0 : 1.0;
    float yScale = mirrorY ? -1.0 : 1.0;

    if (rhi->isYUpInFramebuffer())
        yScale = -yScale;

    QMatrix4x4 transform;
    transform.scale(xScale, yScale);

    QByteArray uniformData(64 + 64 + 4 + 4, Qt::Uninitialized);
    QVideoTextureHelper::updateUniformData(&uniformData, frame.surfaceFormat(), frame, transform, 1.f);
    rub->updateDynamicBuffer(uniformBuffer.get(), 0, uniformData.size(), uniformData.constData());

    cb->beginPass(renderTarget.get(), Qt::transparent, { 1.0f, 0 }, rub);
    cb->setGraphicsPipeline(graphicsPipeline.get());

    cb->setViewport({ 0, 0, float(frameSize.width()), float(frameSize.height()) });
    cb->setShaderResources(shaderResourceBindings.get());

    quint32 vertexOffset = quint32(sizeof(float)) * 16 * rotationIndex;
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
        return convertCPU(frame, rotation, mirrorX, mirrorY);
    }

    QByteArray *imageData = new QByteArray(readResult.data);

    return QImage(reinterpret_cast<const uchar *>(imageData->constData()),
                  readResult.pixelSize.width(), readResult.pixelSize.height(),
                  QImage::Format_RGBA8888_Premultiplied, imageCleanupHandler, imageData);
}

QT_END_NAMESPACE

