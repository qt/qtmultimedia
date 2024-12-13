// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qandroidvideooutput_p.h"
#include "androidsurfacetexture_p.h"

#include <rhi/qrhi.h>
#include <QtGui/private/qopenglextensions_p.h>
#include <private/qhwvideobuffer_p.h>
#include <private/qvideoframeconverter_p.h>
#include <private/qplatformvideosink_p.h>
#include <private/qvideoframe_p.h>
#include <qvideosink.h>
#include <qopenglcontext.h>
#include <qopenglfunctions.h>
#include <qvideoframeformat.h>
#include <qthread.h>
#include <qfile.h>

QT_BEGIN_NAMESPACE

class QAndroidVideoFrameTextures : public QVideoFrameTextures
{
public:
    QAndroidVideoFrameTextures(QRhi *rhi, QSize size, quint64 handle)
    {
        m_tex.reset(rhi->newTexture(QRhiTexture::RGBA8, size, 1));
        m_tex->createFrom({quint64(handle), 0});
    }

    QRhiTexture *texture(uint plane) const override
    {
        return plane == 0 ? m_tex.get() : nullptr;
    }

private:
    std::unique_ptr<QRhiTexture> m_tex;
};

// QRhiWithThreadGuard keeps QRhi and QThread (that created it) alive to allow proper cleaning
class QRhiWithThreadGuard : public QObject {
    Q_OBJECT
public:
    QRhiWithThreadGuard(std::shared_ptr<QRhi> r, std::shared_ptr<AndroidTextureThread> t)
        : m_guardRhi(std::move(r)), m_thread(std::move(t)) {}
    ~QRhiWithThreadGuard();
protected:
    std::shared_ptr<QRhi> m_guardRhi;
private:
    std::shared_ptr<AndroidTextureThread> m_thread;
};

class AndroidTextureVideoBuffer : public QRhiWithThreadGuard, public QHwVideoBuffer
{
public:
    AndroidTextureVideoBuffer(std::shared_ptr<QRhi> rhi,
                              std::shared_ptr<AndroidTextureThread> thread,
                              std::unique_ptr<QRhiTexture> tex, const QSize &size)
        : QRhiWithThreadGuard(std::move(rhi), std::move(thread)),
          QHwVideoBuffer(QVideoFrame::RhiTextureHandle, m_guardRhi.get()),
          m_size(size),
          m_tex(std::move(tex))
    {}

    MapData map(QVideoFrame::MapMode mode) override;

    void unmap() override
    {
        m_image = {};
        m_mapMode = QVideoFrame::NotMapped;
    }

    QVideoFrameTexturesUPtr mapTextures(QRhi &rhi, QVideoFrameTexturesUPtr& /*oldTextures*/) override
    {
        return std::make_unique<QAndroidVideoFrameTextures>(&rhi, m_size, m_tex->nativeTexture().object);
    }

private:
    QSize m_size;
    std::unique_ptr<QRhiTexture> m_tex;
    QImage m_image;
    QVideoFrame::MapMode m_mapMode = QVideoFrame::NotMapped;
};

class ImageFromVideoFrameHelper : public QHwVideoBuffer
{
public:
    ImageFromVideoFrameHelper(AndroidTextureVideoBuffer &atvb)
        : QHwVideoBuffer(QVideoFrame::RhiTextureHandle, atvb.rhi()), m_atvb(atvb)
    {}
    QVideoFrameTexturesUPtr mapTextures(QRhi &rhi, QVideoFrameTexturesUPtr& oldTextures) override
    {
        return m_atvb.mapTextures(rhi, oldTextures);
    }

    MapData map(QVideoFrame::MapMode) override { return {}; }
    void unmap() override {}

private:
    AndroidTextureVideoBuffer &m_atvb;
};

QAbstractVideoBuffer::MapData AndroidTextureVideoBuffer::map(QVideoFrame::MapMode mode)
{
    QAbstractVideoBuffer::MapData mapData;

    if (m_mapMode == QVideoFrame::NotMapped && mode == QVideoFrame::ReadOnly) {
        m_mapMode = QVideoFrame::ReadOnly;
        m_image = qImageFromVideoFrame(QVideoFramePrivate::createFrame(
                std::make_unique<ImageFromVideoFrameHelper>(*this),
                QVideoFrameFormat(m_size, QVideoFrameFormat::Format_RGBA8888)));
        mapData.planeCount = 1;
        mapData.bytesPerLine[0] = m_image.bytesPerLine();
        mapData.dataSize[0] = static_cast<int>(m_image.sizeInBytes());
        mapData.data[0] = m_image.bits();
    }

    return mapData;
}

static const float g_quad[] = {
    -1.f, -1.f, 0.f, 0.f,
    -1.f,  1.f, 0.f, 1.f,
    1.f,  1.f, 1.f, 1.f,
    1.f, -1.f, 1.f, 0.f
};

class TextureCopy
{
    static QShader getShader(const QString &name)
    {
        QFile f(name);
        if (f.open(QIODevice::ReadOnly))
            return QShader::fromSerialized(f.readAll());
        return {};
    }

public:
    TextureCopy(QRhi *rhi, QRhiTexture *externalTex)
        : m_rhi(rhi)
    {
        m_vertexBuffer.reset(m_rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(g_quad)));
        m_vertexBuffer->create();

        m_uniformBuffer.reset(m_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 64 + 64 + 4 + 4));
        m_uniformBuffer->create();

        m_sampler.reset(m_rhi->newSampler(QRhiSampler::Nearest, QRhiSampler::Nearest, QRhiSampler::None,
                                          QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge));
        m_sampler->create();

        m_srb.reset(m_rhi->newShaderResourceBindings());
        m_srb->setBindings({
                QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, m_uniformBuffer.get()),
                QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, externalTex, m_sampler.get())
        });
        m_srb->create();

        m_vertexShader = getShader(QStringLiteral(":/qt-project.org/multimedia/shaders/externalsampler.vert.qsb"));
        Q_ASSERT(m_vertexShader.isValid());
        m_fragmentShader = getShader(QStringLiteral(":/qt-project.org/multimedia/shaders/externalsampler.frag.qsb"));
        Q_ASSERT(m_fragmentShader.isValid());
    }

    std::unique_ptr<QRhiTexture> copyExternalTexture(QSize size, const QMatrix4x4 &externalTexMatrix);

private:
    QRhi *m_rhi = nullptr;
    std::unique_ptr<QRhiBuffer> m_vertexBuffer;
    std::unique_ptr<QRhiBuffer> m_uniformBuffer;
    std::unique_ptr<QRhiSampler> m_sampler;
    std::unique_ptr<QRhiShaderResourceBindings> m_srb;
    QShader m_vertexShader;
    QShader m_fragmentShader;
};

static std::unique_ptr<QRhiGraphicsPipeline> newGraphicsPipeline(QRhi *rhi,
                                                                 QRhiShaderResourceBindings *shaderResourceBindings,
                                                                 QRhiRenderPassDescriptor *renderPassDescriptor,
                                                                 QShader vertexShader,
                                                                 QShader fragmentShader)
{
    std::unique_ptr<QRhiGraphicsPipeline> gp(rhi->newGraphicsPipeline());
    gp->setTopology(QRhiGraphicsPipeline::TriangleFan);
    gp->setShaderStages({
            { QRhiShaderStage::Vertex, vertexShader },
            { QRhiShaderStage::Fragment, fragmentShader }
    });
    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({
            { 4 * sizeof(float) }
    });
    inputLayout.setAttributes({
            { 0, 0, QRhiVertexInputAttribute::Float2, 0 },
            { 0, 1, QRhiVertexInputAttribute::Float2, 2 * sizeof(float) }
    });
    gp->setVertexInputLayout(inputLayout);
    gp->setShaderResourceBindings(shaderResourceBindings);
    gp->setRenderPassDescriptor(renderPassDescriptor);
    gp->create();

    return gp;
}

std::unique_ptr<QRhiTexture> TextureCopy::copyExternalTexture(QSize size, const QMatrix4x4 &externalTexMatrix)
{
    std::unique_ptr<QRhiTexture> tex(m_rhi->newTexture(QRhiTexture::RGBA8, size, 1, QRhiTexture::RenderTarget));
    if (!tex->create()) {
        qWarning("Failed to create frame texture");
        return {};
    }

    std::unique_ptr<QRhiTextureRenderTarget> renderTarget(m_rhi->newTextureRenderTarget({ { tex.get() } }));
    std::unique_ptr<QRhiRenderPassDescriptor> renderPassDescriptor(renderTarget->newCompatibleRenderPassDescriptor());
    renderTarget->setRenderPassDescriptor(renderPassDescriptor.get());
    renderTarget->create();

    QRhiResourceUpdateBatch *rub = m_rhi->nextResourceUpdateBatch();
    rub->uploadStaticBuffer(m_vertexBuffer.get(), g_quad);

    QMatrix4x4 identity;
    char *p = m_uniformBuffer->beginFullDynamicBufferUpdateForCurrentFrame();
    memcpy(p, identity.constData(), 64);
    memcpy(p + 64, externalTexMatrix.constData(), 64);
    float opacity = 1.0f;
    memcpy(p + 64 + 64, &opacity, 4);
    m_uniformBuffer->endFullDynamicBufferUpdateForCurrentFrame();

    auto graphicsPipeline = newGraphicsPipeline(m_rhi, m_srb.get(), renderPassDescriptor.get(),
                                                m_vertexShader, m_fragmentShader);

    const QRhiCommandBuffer::VertexInput vbufBinding(m_vertexBuffer.get(), 0);

    QRhiCommandBuffer *cb = nullptr;
    if (m_rhi->beginOffscreenFrame(&cb) != QRhi::FrameOpSuccess)
        return {};

    cb->beginPass(renderTarget.get(), Qt::transparent, { 1.0f, 0 }, rub);
    cb->setGraphicsPipeline(graphicsPipeline.get());
    cb->setViewport({0, 0, float(size.width()), float(size.height())});
    cb->setShaderResources(m_srb.get());
    cb->setVertexInput(0, 1, &vbufBinding);
    cb->draw(4);
    cb->endPass();
    m_rhi->endOffscreenFrame();

    QOpenGLContext *ctx = QOpenGLContext::currentContext();
    QOpenGLFunctions *f = ctx->functions();
    static_cast<QOpenGLExtensions *>(f)->flushShared();

    return tex;
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

class AndroidTextureThread : public QThread
{
    Q_OBJECT
public:
    AndroidTextureThread(QAndroidTextureVideoOutput * vo)
        : QThread()
        , m_videoOutput(vo)
    {
    }

    ~AndroidTextureThread() {
        QMetaObject::invokeMethod(this,
                &AndroidTextureThread::clearSurfaceTexture, Qt::BlockingQueuedConnection);
        this->quit();
        this->wait();
    }

    void start()
    {
        QThread::start();
        moveToThread(this);
    }

    void initRhi(QOpenGLContext *context)
    {
        QRhiGles2InitParams params;
        params.shareContext = context;
        params.fallbackSurface = QRhiGles2InitParams::newFallbackSurface();
        m_rhi.reset(QRhi::create(QRhi::OpenGLES2, &params));
    }

public slots:
    void onFrameAvailable(quint64 index)
    {
        // Check if 'm_surfaceTexture' is not reset and if the current index is the same that
        // was used for creating connection because there can be pending frames in queue.
        if (m_surfaceTexture && m_surfaceTexture->index() == index) {
            m_surfaceTexture->updateTexImage();
            auto matrix = extTransformMatrix(m_surfaceTexture.get());
            auto tex = m_textureCopy->copyExternalTexture(m_size, matrix);
            auto *buffer = new AndroidTextureVideoBuffer(m_rhi, m_videoOutput->getSurfaceThread(), std::move(tex), m_size);
            QVideoFrame frame(buffer, QVideoFrameFormat(m_size, QVideoFrameFormat::Format_RGBA8888));
            emit newFrame(frame);
        }
    }

    void clearFrame() { emit newFrame({}); }

    void setFrameSize(QSize size) { m_size = size; }

    void clearSurfaceTexture()
    {
        m_surfaceTexture.reset();
        m_texture.reset();
        m_textureCopy.reset();
        m_rhi.reset();
    }

    AndroidSurfaceTexture *createSurfaceTexture(QRhi *rhi)
    {
        if (m_surfaceTexture)
            return m_surfaceTexture.get();

        QOpenGLContext *ctx = rhi
                ? static_cast<const QRhiGles2NativeHandles *>(rhi->nativeHandles())->context
                : nullptr;
        initRhi(ctx);

        m_texture.reset(m_rhi->newTexture(QRhiTexture::RGBA8, m_size, 1, QRhiTexture::ExternalOES));
        m_texture->create();
        m_surfaceTexture = std::make_unique<AndroidSurfaceTexture>(m_texture->nativeTexture().object);
        if (m_surfaceTexture->surfaceTexture()) {
            const quint64 index = m_surfaceTexture->index();
            connect(m_surfaceTexture.get(), &AndroidSurfaceTexture::frameAvailable, this,
                    [this, index] () { this->onFrameAvailable(index); });

            m_textureCopy = std::make_unique<TextureCopy>(m_rhi.get(), m_texture.get());

        } else {
            m_texture.reset();
            m_surfaceTexture.reset();
        }

        return m_surfaceTexture.get();
    }

signals:
    void newFrame(const QVideoFrame &);

private:
    QAndroidTextureVideoOutput * m_videoOutput;
    std::shared_ptr<QRhi> m_rhi;
    std::unique_ptr<AndroidSurfaceTexture> m_surfaceTexture;
    std::unique_ptr<QRhiTexture> m_texture;
    std::unique_ptr<TextureCopy> m_textureCopy;
    QSize m_size;
};

QRhiWithThreadGuard::~QRhiWithThreadGuard() {
    // It may happen that reseting m_rhi shared_ptr will delete it (if it is the last reference)
    // QRHI need to be deleted from the thread that created it.
    QMetaObject::invokeMethod(m_thread.get(), [&]() {m_guardRhi.reset();}, Qt::BlockingQueuedConnection);
}

QAndroidTextureVideoOutput::QAndroidTextureVideoOutput(QVideoSink *sink, QObject *parent)
    : QAndroidVideoOutput(parent)
    , m_sink(sink)
{
    if (!m_sink) {
        qDebug() << "Cannot create QAndroidTextureVideoOutput without a sink.";
        m_surfaceThread = nullptr;
        return;
    }

    startNewSurfaceThread();
}

void QAndroidTextureVideoOutput::startNewSurfaceThread()
{
    m_surfaceThread = std::make_shared<AndroidTextureThread>(this);
    connect(m_surfaceThread.get(), &AndroidTextureThread::newFrame,
            this, &QAndroidTextureVideoOutput::newFrame);
    m_surfaceThread->start();
}

QAndroidTextureVideoOutput::~QAndroidTextureVideoOutput()
{
    // Make sure that no more VideFrames will be created by surfaceThread
    QMetaObject::invokeMethod(m_surfaceThread.get(),
        &AndroidTextureThread::clearSurfaceTexture, Qt::BlockingQueuedConnection);
}

void QAndroidTextureVideoOutput::setSubtitle(const QString &subtitle)
{
    if (m_sink) {
        auto *sink = m_sink->platformVideoSink();
        if (sink)
            sink->setSubtitleText(subtitle);
    }
}

bool QAndroidTextureVideoOutput::shouldTextureBeUpdated() const
{
    return m_sink->rhi() && m_surfaceCreatedWithoutRhi;
}

AndroidSurfaceTexture *QAndroidTextureVideoOutput::surfaceTexture()
{
    if (!m_sink)
        return nullptr;

    AndroidSurfaceTexture *surface = nullptr;
    QMetaObject::invokeMethod(m_surfaceThread.get(), [&]() {
                auto rhi = m_sink->rhi();
                if (!rhi) {
                    m_surfaceCreatedWithoutRhi = true;
                }
                else if (m_surfaceCreatedWithoutRhi) {
                    m_surfaceThread->clearSurfaceTexture();
                    m_surfaceCreatedWithoutRhi = false;
                }
                surface = m_surfaceThread->createSurfaceTexture(rhi);
            },
            Qt::BlockingQueuedConnection);
    return surface;
}

void QAndroidTextureVideoOutput::setVideoSize(const QSize &size)
{
    if (m_nativeSize == size)
        return;

    m_nativeSize = size;
    QMetaObject::invokeMethod(m_surfaceThread.get(),
            [&](){ m_surfaceThread->setFrameSize(size); },
            Qt::BlockingQueuedConnection);
}

void QAndroidTextureVideoOutput::stop()
{
    m_nativeSize = {};
    QMetaObject::invokeMethod(m_surfaceThread.get(), [&](){ m_surfaceThread->clearFrame(); });
}

void QAndroidTextureVideoOutput::reset()
{
    if (m_sink)
        m_sink->platformVideoSink()->setVideoFrame({});
    QMetaObject::invokeMethod(m_surfaceThread.get(), &AndroidTextureThread::clearSurfaceTexture);
}

void QAndroidTextureVideoOutput::newFrame(const QVideoFrame &frame)
{
    if (m_sink)
        m_sink->setVideoFrame(frame);
}

QT_END_NAMESPACE

#include "qandroidvideooutput.moc"
#include "moc_qandroidvideooutput_p.cpp"
