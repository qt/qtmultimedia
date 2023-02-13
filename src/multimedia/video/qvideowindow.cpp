// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qvideowindow_p.h"
#include <QPlatformSurfaceEvent>
#include <qfile.h>
#include <qpainter.h>
#include <private/qguiapplication_p.h>
#include <private/qmemoryvideobuffer_p.h>
#include <qpa/qplatformintegration.h>

QT_BEGIN_NAMESPACE

static QSurface::SurfaceType platformSurfaceType()
{
#if defined(Q_OS_DARWIN)
    return QSurface::MetalSurface;
#elif defined (Q_OS_WIN)
    return QSurface::Direct3DSurface;
#endif

    auto *integration = QGuiApplicationPrivate::platformIntegration();

    if (!integration->hasCapability(QPlatformIntegration::OpenGL))
        return QSurface::RasterSurface;

    if (QCoreApplication::testAttribute(Qt::AA_ForceRasterWidgets))
        return QSurface::RasterSurface;

    if (integration->hasCapability(QPlatformIntegration::RasterGLSurface))
        return QSurface::RasterGLSurface;

    return QSurface::OpenGLSurface;
}

QVideoWindowPrivate::QVideoWindowPrivate(QVideoWindow *q)
    : q(q),
    m_sink(new QVideoSink)
{
    Q_ASSERT(q);

    if (QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::RhiBasedRendering)) {
        auto surfaceType = ::platformSurfaceType();
        q->setSurfaceType(surfaceType);
        switch (surfaceType) {
        case QSurface::RasterSurface:
        case QSurface::OpenVGSurface:
            // can't use those surfaces, need to render in SW
            m_graphicsApi = QRhi::Null;
            break;
        case QSurface::OpenGLSurface:
        case QSurface::RasterGLSurface:
            m_graphicsApi = QRhi::OpenGLES2;
            break;
        case QSurface::VulkanSurface:
            m_graphicsApi = QRhi::Vulkan;
            break;
        case QSurface::MetalSurface:
            m_graphicsApi = QRhi::Metal;
            break;
        case QSurface::Direct3DSurface:
            m_graphicsApi = QRhi::D3D11;
            break;
        }
    }

    QObject::connect(m_sink.get(), &QVideoSink::videoFrameChanged, q, &QVideoWindow::setVideoFrame);
}

QVideoWindowPrivate::~QVideoWindowPrivate()
{
    QObject::disconnect(m_sink.get(), &QVideoSink::videoFrameChanged,
            q, &QVideoWindow::setVideoFrame);
}

static const float g_vw_quad[] = {
    // 4 clockwise rotation of texture vertexes (the second pair)
    // Rotation 0
    -1.f, -1.f,   0.f, 0.f,
    -1.f, 1.f,    0.f, 1.f,
    1.f, -1.f,    1.f, 0.f,
    1.f, 1.f,     1.f, 1.f,
    // Rotation 90
    -1.f, -1.f,   0.f, 1.f,
    -1.f, 1.f,    1.f, 1.f,
    1.f, -1.f,    0.f, 0.f,
    1.f, 1.f,     1.f, 0.f,

    // Rotation 180
    -1.f, -1.f,   1.f, 1.f,
    -1.f, 1.f,    1.f, 0.f,
    1.f, -1.f,    0.f, 1.f,
    1.f, 1.f,     0.f, 0.f,
    // Rotation 270
    -1.f, -1.f,   1.f, 0.f,
    -1.f, 1.f,    0.f, 0.f,
    1.f, -1.f,    1.f, 1.f,
    1.f, 1.f,     0.f, 1.f
};

static QShader vwGetShader(const QString &name)
{
    QFile f(name);
    if (f.open(QIODevice::ReadOnly))
        return QShader::fromSerialized(f.readAll());

    return QShader();
}

void QVideoWindowPrivate::initRhi()
{
    if (m_graphicsApi == QRhi::Null)
        return;

    QRhi::Flags rhiFlags = {};//QRhi::EnableDebugMarkers | QRhi::EnableProfiling;

#if QT_CONFIG(opengl)
    if (m_graphicsApi == QRhi::OpenGLES2) {
        m_fallbackSurface.reset(QRhiGles2InitParams::newFallbackSurface(q->format()));
        QRhiGles2InitParams params;
        params.fallbackSurface = m_fallbackSurface.get();
        params.window = q;
        params.format = q->format();
        m_rhi.reset(QRhi::create(QRhi::OpenGLES2, &params, rhiFlags));
    }
#endif

#if QT_CONFIG(vulkan)
    if (m_graphicsApi == QRhi::Vulkan) {
        QRhiVulkanInitParams params;
        params.inst = q->vulkanInstance();
        params.window = q;
        m_rhi.reset(QRhi::create(QRhi::Vulkan, &params, rhiFlags));
    }
#endif

#ifdef Q_OS_WIN
    if (m_graphicsApi == QRhi::D3D11) {
        QRhiD3D11InitParams params;
        params.enableDebugLayer = true;
        m_rhi.reset(QRhi::create(QRhi::D3D11, &params, rhiFlags));
    }
#endif

#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
    if (m_graphicsApi == QRhi::Metal) {
        QRhiMetalInitParams params;
        m_rhi.reset(QRhi::create(QRhi::Metal, &params, rhiFlags));
    }
#endif
    if (!m_rhi)
        return;

    m_swapChain.reset(m_rhi->newSwapChain());
    m_swapChain->setWindow(q);
    if (m_swapChain->isFormatSupported(QRhiSwapChain::HDRExtendedSrgbLinear))
        m_swapChain->setFormat(QRhiSwapChain::HDRExtendedSrgbLinear);
    m_renderPass.reset(m_swapChain->newCompatibleRenderPassDescriptor());
    m_swapChain->setRenderPassDescriptor(m_renderPass.get());

    m_vertexBuf.reset(m_rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(g_vw_quad)));
    m_vertexBuf->create();
    m_vertexBufReady = false;

    m_uniformBuf.reset(m_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, sizeof(QVideoTextureHelper::UniformData)));
    m_uniformBuf->create();

    m_textureSampler.reset(m_rhi->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                             QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge));
    m_textureSampler->create();

    m_shaderResourceBindings.reset(m_rhi->newShaderResourceBindings());
    m_subtitleResourceBindings.reset(m_rhi->newShaderResourceBindings());

    m_subtitleUniformBuf.reset(m_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, sizeof(QVideoTextureHelper::UniformData)));
    m_subtitleUniformBuf->create();

    Q_ASSERT(NVideoFrameSlots >= m_rhi->resourceLimit(QRhi::FramesInFlight));
}

void QVideoWindowPrivate::setupGraphicsPipeline(QRhiGraphicsPipeline *pipeline, QRhiShaderResourceBindings *bindings, const QVideoFrameFormat &fmt)
{

    pipeline->setTopology(QRhiGraphicsPipeline::TriangleStrip);
    QShader vs = vwGetShader(QVideoTextureHelper::vertexShaderFileName(fmt));
    Q_ASSERT(vs.isValid());
    QShader fs = vwGetShader(QVideoTextureHelper::fragmentShaderFileName(fmt, m_swapChain->format()));
    Q_ASSERT(fs.isValid());
    pipeline->setShaderStages({
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
    pipeline->setVertexInputLayout(inputLayout);
    pipeline->setShaderResourceBindings(bindings);
    pipeline->setRenderPassDescriptor(m_renderPass.get());
    pipeline->create();
}

void QVideoWindowPrivate::updateTextures(QRhiResourceUpdateBatch *rub)
{
    m_texturesDirty = false;

    // We render a 1x1 black pixel when we don't have a video
    if (!m_currentFrame.isValid())
        m_currentFrame = QVideoFrame(new QMemoryVideoBuffer(QByteArray{4, 0}, 4),
                                     QVideoFrameFormat(QSize(1,1), QVideoFrameFormat::Format_RGBA8888));

    m_frameTextures = QVideoTextureHelper::createTextures(m_currentFrame, m_rhi.get(), rub, std::move(m_frameTextures));
    if (!m_frameTextures)
        return;

    QRhiShaderResourceBinding bindings[4];
    auto *b = bindings;
    *(b++) = QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
                                                   m_uniformBuf.get());

    auto fmt = m_currentFrame.surfaceFormat();
    auto textureDesc = QVideoTextureHelper::textureDescription(fmt.pixelFormat());

    for (int i = 0; i < textureDesc->nplanes; ++i)
        (*b++) = QRhiShaderResourceBinding::sampledTexture(i + 1, QRhiShaderResourceBinding::FragmentStage,
                                                           m_frameTextures->texture(i), m_textureSampler.get());
    m_shaderResourceBindings->setBindings(bindings, b);
    m_shaderResourceBindings->create();

    if (fmt != format) {
        format = fmt;
        if (!m_graphicsPipeline)
            m_graphicsPipeline.reset(m_rhi->newGraphicsPipeline());

        setupGraphicsPipeline(m_graphicsPipeline.get(), m_shaderResourceBindings.get(), format);
    }
}

void QVideoWindowPrivate::updateSubtitle(QRhiResourceUpdateBatch *rub, const QSize &frameSize)
{
    m_subtitleDirty = false;
    m_hasSubtitle = !m_currentFrame.subtitleText().isEmpty();
    if (!m_hasSubtitle)
        return;

    m_subtitleLayout.update(frameSize, m_currentFrame.subtitleText());
    QSize size = m_subtitleLayout.bounds.size().toSize();

    QImage img = m_subtitleLayout.toImage();

    m_subtitleTexture.reset(m_rhi->newTexture(QRhiTexture::RGBA8, size));
    m_subtitleTexture->create();
    rub->uploadTexture(m_subtitleTexture.get(), img);

    QRhiShaderResourceBinding bindings[2];

    bindings[0] = QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
                                                           m_subtitleUniformBuf.get());

    bindings[1] = QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage,
                                                            m_subtitleTexture.get(), m_textureSampler.get());
    m_subtitleResourceBindings->setBindings(bindings, bindings + 2);
    m_subtitleResourceBindings->create();

    if (!m_subtitlePipeline) {
        m_subtitlePipeline.reset(m_rhi->newGraphicsPipeline());

        QRhiGraphicsPipeline::TargetBlend blend;
        blend.enable = true;
        m_subtitlePipeline->setTargetBlends({ blend });
        setupGraphicsPipeline(m_subtitlePipeline.get(), m_subtitleResourceBindings.get(), QVideoFrameFormat(QSize(1, 1), QVideoFrameFormat::Format_RGBA8888));
    }
}

void QVideoWindowPrivate::init()
{
    if (initialized)
        return;
    initialized = true;

    initRhi();

    if (!m_rhi)
        backingStore = new QBackingStore(q);
    else
        m_sink->setRhi(m_rhi.get());
}

void QVideoWindowPrivate::resizeSwapChain()
{
    m_hasSwapChain = m_swapChain->createOrResize();
}

void QVideoWindowPrivate::releaseSwapChain()
{
    if (m_hasSwapChain) {
        m_hasSwapChain = false;
        m_swapChain->destroy();
    }
}

void QVideoWindowPrivate::render()
{
    if (!initialized)
        init();

    if (!q->isExposed() || !isExposed)
        return;

    QRect rect(0, 0, q->width(), q->height());

    if (backingStore) {
        if (backingStore->size() != q->size())
            backingStore->resize(q->size());

        backingStore->beginPaint(rect);

        QPaintDevice *device = backingStore->paintDevice();
        if (!device)
            return;
        QPainter painter(device);

        m_currentFrame.paint(&painter, rect, { Qt::black, aspectRatioMode });
        painter.end();

        backingStore->endPaint();
        backingStore->flush(rect);
        return;
    }

    int frameRotationIndex = (m_currentFrame.rotationAngle() / 90) % 4;
    QSize frameSize = m_currentFrame.size();
    if (frameRotationIndex % 2)
        frameSize.transpose();
    QSize scaled = frameSize.scaled(rect.size(), aspectRatioMode);
    QRect videoRect = QRect(QPoint(0, 0), scaled);
    videoRect.moveCenter(rect.center());
    QRect subtitleRect = videoRect.intersected(rect);

    if (m_swapChain->currentPixelSize() != m_swapChain->surfacePixelSize())
        resizeSwapChain();

    if (!m_hasSwapChain)
        return;

    QRhi::FrameOpResult r = m_rhi->beginFrame(m_swapChain.get());

    // keep the video frames alive until we know that they are not needed anymore
    m_videoFrameSlots[m_rhi->currentFrameSlot()] = m_currentFrame;

    if (r == QRhi::FrameOpSwapChainOutOfDate) {
        resizeSwapChain();
        if (!m_hasSwapChain)
            return;
        r = m_rhi->beginFrame(m_swapChain.get());
    }
    if (r != QRhi::FrameOpSuccess) {
        qWarning("beginFrame failed with %d, retry", r);
        q->requestUpdate();
        return;
    }

    QRhiResourceUpdateBatch *rub = m_rhi->nextResourceUpdateBatch();

    if (!m_vertexBufReady) {
        m_vertexBufReady = true;
        rub->uploadStaticBuffer(m_vertexBuf.get(), g_vw_quad);
    }

    if (m_texturesDirty)
        updateTextures(rub);

    if (m_subtitleDirty || m_subtitleLayout.videoSize != subtitleRect.size())
        updateSubtitle(rub, subtitleRect.size());

    float mirrorFrame = m_currentFrame.mirrored() ? -1.f : 1.f;
    float xscale = mirrorFrame * float(videoRect.width())/float(rect.width());
    float yscale = -1.f * float(videoRect.height())/float(rect.height());

    QMatrix4x4 transform;
    transform.scale(xscale, yscale);

    float maxNits = 100;
    if (m_swapChain->format() == QRhiSwapChain::HDRExtendedSrgbLinear) {
        auto info = m_swapChain->hdrInfo();
        if (info.limitsType == QRhiSwapChainHdrInfo::ColorComponentValue)
            maxNits = 100 * info.limits.colorComponentValue.maxColorComponentValue;
        else
            maxNits = info.limits.luminanceInNits.maxLuminance;
    }

    QByteArray uniformData;
    QVideoTextureHelper::updateUniformData(&uniformData, m_currentFrame.surfaceFormat(), m_currentFrame, transform, 1.f, maxNits);
    rub->updateDynamicBuffer(m_uniformBuf.get(), 0, uniformData.size(), uniformData.constData());

    if (m_hasSubtitle) {
        QMatrix4x4 st;
        st.translate(0, -2.f * (float(m_subtitleLayout.bounds.center().y())  + float(subtitleRect.top()))/ float(rect.height()) + 1.f);
        st.scale(float(m_subtitleLayout.bounds.width())/float(rect.width()),
                -1.f * float(m_subtitleLayout.bounds.height())/float(rect.height()));

        QByteArray uniformData;
        QVideoFrameFormat fmt(m_subtitleLayout.bounds.size().toSize(), QVideoFrameFormat::Format_ARGB8888);
        QVideoTextureHelper::updateUniformData(&uniformData, fmt, QVideoFrame(), st, 1.f);
        rub->updateDynamicBuffer(m_subtitleUniformBuf.get(), 0, uniformData.size(), uniformData.constData());
    }

    QRhiCommandBuffer *cb = m_swapChain->currentFrameCommandBuffer();
    cb->beginPass(m_swapChain->currentFrameRenderTarget(), Qt::black, { 1.0f, 0 }, rub);
    cb->setGraphicsPipeline(m_graphicsPipeline.get());
    auto size = m_swapChain->currentPixelSize();
    cb->setViewport({ 0, 0, float(size.width()), float(size.height()) });
    cb->setShaderResources(m_shaderResourceBindings.get());

    quint32 vertexOffset = quint32(sizeof(float)) * 16 * frameRotationIndex;
    const QRhiCommandBuffer::VertexInput vbufBinding(m_vertexBuf.get(), vertexOffset);
    cb->setVertexInput(0, 1, &vbufBinding);
    cb->draw(4);

    if (m_hasSubtitle) {
        cb->setGraphicsPipeline(m_subtitlePipeline.get());
        cb->setShaderResources(m_subtitleResourceBindings.get());
        const QRhiCommandBuffer::VertexInput vbufBinding(m_vertexBuf.get(), 0);
        cb->setVertexInput(0, 1, &vbufBinding);
        cb->draw(4);
    }

    cb->endPass();

    m_rhi->endFrame(m_swapChain.get());
}

/*!
    \class QVideoWindow
    \internal
*/
QVideoWindow::QVideoWindow(QScreen *screen)
    : QWindow(screen)
    , d(new QVideoWindowPrivate(this))
{
}

QVideoWindow::QVideoWindow(QWindow *parent)
    : QWindow(parent)
    , d(new QVideoWindowPrivate(this))
{
}

QVideoWindow::~QVideoWindow() = default;

QVideoSink *QVideoWindow::videoSink() const
{
    return d->m_sink.get();
}

Qt::AspectRatioMode QVideoWindow::aspectRatioMode() const
{
    return d->aspectRatioMode;
}

void QVideoWindow::setAspectRatioMode(Qt::AspectRatioMode mode)
{
    if (d->aspectRatioMode == mode)
        return;
    d->aspectRatioMode = mode;
    emit aspectRatioModeChanged(mode);
}

bool QVideoWindow::event(QEvent *e)
{
    switch (e->type()) {
    case QEvent::UpdateRequest:
        d->render();
        return true;

    case QEvent::PlatformSurface:
        // this is the proper time to tear down the swapchain (while the native window and surface are still around)
        if (static_cast<QPlatformSurfaceEvent *>(e)->surfaceEventType() == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed) {
            d->releaseSwapChain();
            d->isExposed = false;
        }
        break;
    case QEvent::Expose:
        d->isExposed = isExposed();
        if (d->isExposed)
            requestUpdate();
        return true;

    default:
        break;
    }

    return QWindow::event(e);
}

void QVideoWindow::resizeEvent(QResizeEvent *resizeEvent)
{
    if (!d->backingStore)
        return;
    if (!d->initialized)
        d->init();
    d->backingStore->resize(resizeEvent->size());
}

void QVideoWindow::setVideoFrame(const QVideoFrame &frame)
{
    if (d->m_currentFrame.subtitleText() != frame.subtitleText())
        d->m_subtitleDirty = true;
    d->m_currentFrame = frame;
    d->m_texturesDirty = true;
    if (d->isExposed)
        requestUpdate();
}

QT_END_NAMESPACE

#include "moc_qvideowindow_p.cpp"
