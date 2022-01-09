/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "qandroidvideooutput_p.h"

#include "androidsurfacetexture_p.h"
#include <qvideosink.h>
#include "private/qabstractvideobuffer_p.h"
#include "private/qplatformvideosink_p.h"
#include <QVideoFrameFormat>
#include <QFile>
#include <QtGui/private/qrhigles2_p.h>
#include <QOpenGLContext>
#include <QPainter>
#include <QPainterPath>
#include <QMutexLocker>
#include <QTextLayout>
#include <QTextFormat>

QT_BEGIN_NAMESPACE

void GraphicsResourceDeleter::deleteResourcesHelper(const QList<QRhiResource *> &res)
{
    qDeleteAll(res);
}

void GraphicsResourceDeleter::deleteRhiHelper(QRhi *rhi, QOffscreenSurface *surf)
{
    delete rhi;
    delete surf;
}

void GraphicsResourceDeleter::deleteThisHelper()
{
    delete this;
}

bool AndroidTextureVideoBuffer::updateReadbackFrame()
{
    // Even though the texture was updated in a previous call, we need to re-check
    // that this has not become a stale buffer, e.g., if the output size changed or
    // has since became invalid.
    if (!m_output->m_nativeSize.isValid())
        return false;

    // Size changed
    if (m_output->m_nativeSize != m_size)
        return false;

    // In the unlikely event that we don't have a valid fbo, but have a valid size,
    // force an update.
    const bool forceUpdate = !m_output->m_readbackTex;
    if (m_textureUpdated && !forceUpdate)
        return true;

    // update the video texture (called from the render thread)
    return (m_textureUpdated = m_output->renderAndReadbackFrame());
}

QAbstractVideoBuffer::MapData AndroidTextureVideoBuffer::map(QVideoFrame::MapMode mode)
{
    MapData mapData;
    if (m_mapMode == QVideoFrame::NotMapped && mode == QVideoFrame::ReadOnly && updateReadbackFrame()) {
        m_mapMode = mode;
        m_image = m_output->m_readbackImage;
        mapData.nPlanes = 1;
        mapData.bytesPerLine[0] = m_image.bytesPerLine();
        mapData.size[0] = static_cast<int>(m_image.sizeInBytes());
        mapData.data[0] = m_image.bits();
    }
    return mapData;
}

static QMatrix4x4 extTransformMatrix(AndroidSurfaceTexture *surfaceTexture)
{
    QMatrix4x4 m = surfaceTexture->getTransformMatrix();
    // flip it back, see
    // http://androidxref.com/9.0.0_r3/xref/frameworks/native/libs/gui/GLConsumer.cpp#866
    // (NB our matrix ctor takes row major)
    static const QMatrix4x4 flipV(1.0f,  0.0f, 0.0f, 0.0f,
                                  0.0f, -1.0f, 0.0f, 1.0f,
                                  0.0f,  0.0f, 1.0f, 0.0f,
                                  0.0f,  0.0f, 0.0f, 1.0f);
    m *= flipV;
    return m;
}

quint64 AndroidTextureVideoBuffer::textureHandle(int plane) const
{
    if (plane != 0 || !rhi || !m_output->m_nativeSize.isValid())
        return 0;

    m_output->ensureExternalTexture(rhi);
    m_output->m_surfaceTexture->updateTexImage();
    m_externalMatrix = extTransformMatrix(m_output->m_surfaceTexture);
    return m_output->m_externalTex->nativeTexture().object;
}

QAndroidTextureVideoOutput::QAndroidTextureVideoOutput(QObject *parent) : QAndroidVideoOutput(parent) { }

QAndroidTextureVideoOutput::~QAndroidTextureVideoOutput()
{
    clearSurfaceTexture();

    if (m_graphicsDeleter) { // Make sure all of these are deleted on the render thread.
        m_graphicsDeleter->deleteResources({
                m_externalTex,
                m_readbackSrc,
                m_readbackTex,
                m_readbackVBuf,
                m_readbackUBuf,
                m_externalTexSampler,
                m_readbackSrb,
                m_readbackRenderTarget,
                m_readbackRpDesc,
                m_readbackPs
        });

        m_graphicsDeleter->deleteRhi(m_readbackRhi, m_readbackRhiFallbackSurface);
        m_graphicsDeleter->deleteThis();
    }
}

void QAndroidTextureVideoOutput::setSubtitle(const QString &subtitle)
{
    if (!m_sink)
        return;
    auto *sink = m_sink->platformVideoSink();
    sink->setSubtitleText(subtitle);
}

QVideoSink *QAndroidTextureVideoOutput::surface() const
{
    return m_sink;
}

void QAndroidTextureVideoOutput::setSurface(QVideoSink *surface)
{
    if (surface == m_sink)
        return;

    m_sink = surface;
}

bool QAndroidTextureVideoOutput::isReady()
{
    return true;
}

void QAndroidTextureVideoOutput::initSurfaceTexture()
{
    if (m_surfaceTexture)
        return;

    if (!m_sink)
        return;

    QMutexLocker locker(&m_mutex);

    m_surfaceTexture = new AndroidSurfaceTexture(m_externalTex ? m_externalTex->nativeTexture().object : 0);

    if (m_surfaceTexture->surfaceTexture() != 0) {
        connect(m_surfaceTexture, &AndroidSurfaceTexture::frameAvailable,
                this, &QAndroidTextureVideoOutput::onFrameAvailable);
    } else {
        delete m_surfaceTexture;
        m_surfaceTexture = nullptr;
        if (m_graphicsDeleter)
            m_graphicsDeleter->deleteResources({ m_externalTex });
        m_externalTex = nullptr;
    }
}

void QAndroidTextureVideoOutput::clearSurfaceTexture()
{
    QMutexLocker locker(&m_mutex);
    if (m_surfaceTexture) {
        delete m_surfaceTexture;
        m_surfaceTexture = nullptr;
    }

    // Also reset the attached texture
    if (m_graphicsDeleter)
        m_graphicsDeleter->deleteResources({ m_externalTex });
    m_externalTex = nullptr;
}

AndroidSurfaceTexture *QAndroidTextureVideoOutput::surfaceTexture()
{
    initSurfaceTexture();
    return m_surfaceTexture;
}

void QAndroidTextureVideoOutput::setVideoSize(const QSize &size)
{
    QMutexLocker locker(&m_mutex);
    if (m_nativeSize == size)
        return;

    stop();

    m_nativeSize = size;
}

void QAndroidTextureVideoOutput::start()
{
    m_started = true;
    renderAndReadbackFrame();
}

void QAndroidTextureVideoOutput::stop()
{
    m_nativeSize = QSize();
    m_started = false;
}

void QAndroidTextureVideoOutput::reset()
{
    // flush pending frame
    if (m_sink)
        m_sink->platformVideoSink()->setVideoFrame(QVideoFrame());

    clearSurfaceTexture();
}

void QAndroidTextureVideoOutput::onFrameAvailable()
{
    if (!m_nativeSize.isValid() || !m_sink || !m_started)
        return;

    QRhi *rhi = m_sink ? m_sink->rhi() : nullptr;
    auto *buffer = new AndroidTextureVideoBuffer(rhi, this, m_nativeSize);
    const QVideoFrameFormat::PixelFormat format = rhi ? QVideoFrameFormat::Format_SamplerExternalOES
                                                      : QVideoFrameFormat::Format_RGBA8888;
    QVideoFrame frame(buffer, QVideoFrameFormat(m_nativeSize, format));
    m_sink->platformVideoSink()->setVideoFrame(frame);

    QMetaObject::invokeMethod(m_surfaceTexture
                              , "frameAvailable"
                              , Qt::QueuedConnection
                              );
}

static const float g_quad[] = {
    -1.f, -1.f, 0.f, 0.f,
    -1.f,  1.f, 0.f, 1.f,
     1.f,  1.f, 1.f, 1.f,
     1.f, -1.f, 1.f, 0.f
};

static QShader getShader(const QString &name)
{
    QFile f(name);
    if (f.open(QIODevice::ReadOnly))
        return QShader::fromSerialized(f.readAll());

    return QShader();
}

bool QAndroidTextureVideoOutput::renderAndReadbackFrame()
{
    QMutexLocker locker(&m_mutex);

    if (!m_nativeSize.isValid() || !m_surfaceTexture)
        return false;

    if (!m_readbackRhi) {
        QRhi *sinkRhi = m_sink ? m_sink->rhi() : nullptr;
        if (sinkRhi && sinkRhi->backend() == QRhi::OpenGLES2) {
            // There is an rhi from the sink, e.g. VideoOutput.  We lack the necessary
            // insight to use that directly, so create our own a QRhi that just wraps the
            // same QOpenGLContext.
            sinkRhi->finish();
            QRhiGles2NativeHandles h = *static_cast<const QRhiGles2NativeHandles *>(sinkRhi->nativeHandles());
            m_readbackRhiFallbackSurface = QRhiGles2InitParams::newFallbackSurface(h.context->format());
            QRhiGles2InitParams initParams;
            initParams.format = h.context->format();
            initParams.fallbackSurface = m_readbackRhiFallbackSurface;
            m_readbackRhi = QRhi::create(QRhi::OpenGLES2, &initParams, {}, &h);
        } else {
            // No rhi from the sink, e.g. QVideoWidget.
            // We will fire up our own QRhi with its own QOpenGLContext.
            m_readbackRhiFallbackSurface = QRhiGles2InitParams::newFallbackSurface({});
            QRhiGles2InitParams initParams;
            initParams.fallbackSurface = m_readbackRhiFallbackSurface;
            m_readbackRhi = QRhi::create(QRhi::OpenGLES2, &initParams);
        }
    }

    if (!m_readbackRhi) {
        qWarning("Failed to create QRhi for video frame readback");
        return false;
    }

    QRhiCommandBuffer *cb = nullptr;
    if (m_readbackRhi->beginOffscreenFrame(&cb) != QRhi::FrameOpSuccess)
        return false;

    if (!m_readbackTex || m_readbackTex->pixelSize() != m_nativeSize) {
        delete m_readbackRenderTarget;
        delete m_readbackRpDesc;
        delete m_readbackTex;
        m_readbackTex = m_readbackRhi->newTexture(QRhiTexture::RGBA8, m_nativeSize, 1, QRhiTexture::RenderTarget);
        if (!m_readbackTex->create()) {
            qWarning("Failed to create readback texture");
            return false;
        }
        m_readbackRenderTarget = m_readbackRhi->newTextureRenderTarget({ { m_readbackTex } });
        m_readbackRpDesc = m_readbackRenderTarget->newCompatibleRenderPassDescriptor();
        m_readbackRenderTarget->setRenderPassDescriptor(m_readbackRpDesc);
        m_readbackRenderTarget->create();
    }

    m_readbackRhi->makeThreadLocalNativeContextCurrent();
    ensureExternalTexture(m_readbackRhi);
    m_surfaceTexture->updateTexImage();

    // The only purpose of m_readbackSrc is to be nice and have a QRhiTexture that belongs
    // to m_readbackRhi, not the sink's rhi if there is one. The underlying native object
    // (and the rhi's OpenGL context) are the same regardless.
    if (!m_readbackSrc)
        m_readbackSrc = m_readbackRhi->newTexture(QRhiTexture::RGBA8, m_nativeSize, 1, QRhiTexture::ExternalOES);

    // Keep the object the same (therefore all references to m_readbackSrc in
    // the srb or other objects stay valid all the time), just call createFrom
    // if the native external texture changes.
    const quint64 texId = m_externalTex->nativeTexture().object;
    if (m_readbackSrc->nativeTexture().object != texId)
        m_readbackSrc->createFrom({ texId, 0 });

    QRhiResourceUpdateBatch *rub = nullptr;
    if (!m_readbackVBuf) {
        m_readbackVBuf = m_readbackRhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(g_quad));
        m_readbackVBuf->create();
        if (!rub)
            rub = m_readbackRhi->nextResourceUpdateBatch();
        rub->uploadStaticBuffer(m_readbackVBuf, g_quad);
    }

    if (!m_readbackUBuf) {
        m_readbackUBuf = m_readbackRhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 64 + 64 + 4 + 4);
        m_readbackUBuf->create();
    }

    if (!m_externalTexSampler) {
        m_externalTexSampler = m_readbackRhi->newSampler(QRhiSampler::Nearest, QRhiSampler::Nearest, QRhiSampler::None,
                                                         QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge);
        m_externalTexSampler->create();
    }

    if (!m_readbackSrb) {
        m_readbackSrb = m_readbackRhi->newShaderResourceBindings();
        m_readbackSrb->setBindings({
                QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, m_readbackUBuf),
                QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, m_readbackSrc, m_externalTexSampler)
        });
        m_readbackSrb->create();
    }

    if (!m_readbackPs) {
        m_readbackPs = m_readbackRhi->newGraphicsPipeline();
        m_readbackPs->setTopology(QRhiGraphicsPipeline::TriangleFan);
        QShader vs = getShader(QStringLiteral(":/qt-project.org/multimedia/shaders/externalsampler.vert.qsb"));
        Q_ASSERT(vs.isValid());
        QShader fs = getShader(QStringLiteral(":/qt-project.org/multimedia/shaders/externalsampler.frag.qsb"));
        Q_ASSERT(fs.isValid());
        m_readbackPs->setShaderStages({
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
        m_readbackPs->setVertexInputLayout(inputLayout);
        m_readbackPs->setShaderResourceBindings(m_readbackSrb);
        m_readbackPs->setRenderPassDescriptor(m_readbackRpDesc);
        m_readbackPs->create();
    }

    QMatrix4x4 identity;
    char *p = m_readbackUBuf->beginFullDynamicBufferUpdateForCurrentFrame();
    memcpy(p, identity.constData(), 64);
    QMatrix4x4 extMatrix = extTransformMatrix(m_surfaceTexture);
    memcpy(p + 64, extMatrix.constData(), 64);
    float opacity = 1.0f;
    memcpy(p + 64 + 64, &opacity, 4);
    m_readbackUBuf->endFullDynamicBufferUpdateForCurrentFrame();

    cb->beginPass(m_readbackRenderTarget, Qt::transparent, { 1.0f, 0 }, rub);
    cb->setGraphicsPipeline(m_readbackPs);
    cb->setViewport(QRhiViewport(0, 0, m_nativeSize.width(), m_nativeSize.height()));
    cb->setShaderResources();
    const QRhiCommandBuffer::VertexInput vbufBinding(m_readbackVBuf, 0);
    cb->setVertexInput(0, 1, &vbufBinding);
    cb->draw(4);

    QRhiReadbackDescription readDesc(m_readbackTex);
    QRhiReadbackResult readResult;
    bool readCompleted = false;
    // invoked at latest in the endOffscreenFrame() below
    readResult.completed = [&readCompleted] { readCompleted = true; };
    rub = m_readbackRhi->nextResourceUpdateBatch();
    rub->readBackTexture(readDesc, &readResult);

    cb->endPass(rub);

    m_readbackRhi->endOffscreenFrame();

    if (!readCompleted)
        return false;

    // implicit sharing, keep the data alive
    m_readbackImageData = readResult.data;
    // the QImage does not own the data
    m_readbackImage = QImage(reinterpret_cast<const uchar *>(m_readbackImageData.constData()),
                             readResult.pixelSize.width(), readResult.pixelSize.height(),
                             QImage::Format_ARGB32_Premultiplied);

    return true;
}

void QAndroidTextureVideoOutput::ensureExternalTexture(QRhi *rhi)
{
    if (!m_graphicsDeleter)
        m_graphicsDeleter = new GraphicsResourceDeleter;

    if (!m_externalTex) {
        m_surfaceTexture->detachFromGLContext();
        m_externalTex = rhi->newTexture(QRhiTexture::RGBA8, m_nativeSize, 1, QRhiTexture::ExternalOES);
        if (!m_externalTex->create())
            qWarning("Failed to create native texture object");
        m_surfaceTexture->attachToGLContext(m_externalTex->nativeTexture().object);
    }
}

QT_END_NAMESPACE
