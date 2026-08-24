// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosvideooutput_p.h"

#include "qohosglobal_p.h"
#include "qohossurfaceimage_p.h"

#include <private/qhwvideobuffer_p.h>
#include <private/qplatformvideosink_p.h>
#include <private/qvideoframe_p.h>
#include <private/qvideoframeconverter_p.h>

#include <rhi/qrhi.h>
#include <rhi/qrhi_platform.h>

#include <QtMultimedia/qvideoframe.h>
#include <QtMultimedia/qvideoframeformat.h>
#include <QtMultimedia/qvideosink.h>

#include <QtGui/qmatrix4x4.h>
#include <QtGui/qopenglcontext.h>
#include <QtGui/qoffscreensurface.h>

#include <QtCore/qfile.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qpointer.h>
#include <QtCore/qthread.h>

QT_BEGIN_NAMESPACE

namespace {

const float g_quad[] = {
    -1.f, -1.f, 0.f, 0.f,
    -1.f,  1.f, 0.f, 1.f,
     1.f,  1.f, 1.f, 1.f,
     1.f, -1.f, 1.f, 0.f
};

class QOhosVideoFrameTextures : public QVideoFrameTextures
{
public:
    QOhosVideoFrameTextures(QRhi *rhi, QSize size, quint64 handle)
    {
        m_tex.reset(rhi->newTexture(QRhiTexture::RGBA8, size, 1));
        m_tex->createFrom({ handle, 0 });
    }
    QRhiTexture *texture(uint plane) const override { return plane == 0 ? m_tex.get() : nullptr; }

private:
    std::unique_ptr<QRhiTexture> m_tex;
};

class QOhosTextureVideoBuffer : public QHwVideoBuffer
{
public:
    QOhosTextureVideoBuffer(std::unique_ptr<QRhiTexture> tex, const QSize &size,
                            std::weak_ptr<QRhi> producerRhi, QPointer<QObject> producer,
                            QPointer<QOpenGLContext> producerContext)
        : QHwVideoBuffer(QVideoFrame::RhiTextureHandle)
        , m_size(size)
        , m_tex(std::move(tex))
        , m_producerRhi(std::move(producerRhi))
        , m_producer(std::move(producer))
        , m_producerContext(std::move(producerContext))
    {
    }

    MapData map(QVideoFrame::MapMode mode) override
    {
        MapData data;
        if (m_mapMode != QVideoFrame::NotMapped || mode != QVideoFrame::ReadOnly)
            return data;
        m_mapMode = QVideoFrame::ReadOnly;
        if (m_image.isNull())
            m_image = readbackOnProducerThread();
        if (m_image.isNull()) {
            m_mapMode = QVideoFrame::NotMapped;
            return data;
        }
        data.planeCount = 1;
        data.bytesPerLine[0] = m_image.bytesPerLine();
        data.dataSize[0] = static_cast<int>(m_image.sizeInBytes());
        data.data[0] = m_image.bits();
        return data;
    }

    void unmap() override
    {
        m_image = {};
        m_mapMode = QVideoFrame::NotMapped;
    }

    QVideoFrameTexturesUPtr mapTextures(QRhi &rhi, QVideoFrameTexturesUPtr & /*old*/) override
    {
        // The texture lives in the producer (texture-thread) GL context. Sampling
        // requires the caller's RHI to share that GL context's resources. The main
        // window RHI does (we created the producer with shareContext = main rhi
        // context). A worker thread's QThreadLocal RHI does not — so report no
        // textures and let qImageFromVideoFrame fall back to CPU mapping.
        if (!isCompatibleRhi(rhi))
            return {};
        return std::make_unique<QOhosVideoFrameTextures>(&rhi, m_size,
                                                          m_tex->nativeTexture().object);
    }

private:
    bool isCompatibleRhi(QRhi &rhi) const
    {
        if (rhi.backend() != QRhi::OpenGLES2)
            return false;
        if (!m_producerContext)
            return false;
        const auto *handles =
                static_cast<const QRhiGles2NativeHandles *>(rhi.nativeHandles());
        if (!handles || !handles->context)
            return false;
        return handles->context->shareGroup() == m_producerContext->shareGroup();
    }

    QImage readbackOnProducerThread() const
    {
        auto producerRhi = m_producerRhi.lock();
        if (!producerRhi || !m_producer)
            return {};
        QImage out;
        QRhi *rhi = producerRhi.get();
        QRhiTexture *tex = m_tex.get();
        QMetaObject::invokeMethod(
                m_producer.data(),
                [rhi, tex, &out]() {
                    QRhiReadbackResult result;
                    bool done = false;
                    result.completed = [&done] { done = true; };
                    QRhiCommandBuffer *cb = nullptr;
                    if (rhi->beginOffscreenFrame(&cb) != QRhi::FrameOpSuccess)
                        return;
                    QRhiResourceUpdateBatch *rub = rhi->nextResourceUpdateBatch();
                    rub->readBackTexture({ tex }, &result);
                    cb->resourceUpdate(rub);
                    rhi->endOffscreenFrame();
                    if (!done || result.data.isEmpty())
                        return;
                    QImage img(reinterpret_cast<const uchar *>(result.data.constData()),
                               result.pixelSize.width(), result.pixelSize.height(),
                               result.pixelSize.width() * 4, QImage::Format_RGBA8888);
                    out = img.copy();
                },
                Qt::BlockingQueuedConnection);
        return out;
    }

    QSize m_size;
    std::unique_ptr<QRhiTexture> m_tex;
    QImage m_image;
    QVideoFrame::MapMode m_mapMode = QVideoFrame::NotMapped;
    std::weak_ptr<QRhi> m_producerRhi;
    QPointer<QObject> m_producer;
    QPointer<QOpenGLContext> m_producerContext;
};

class TextureCopy
{
public:
    TextureCopy(QRhi *rhi, QRhiTexture *externalTex) : m_rhi(rhi)
    {
        m_vertexBuffer.reset(m_rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer,
                                              sizeof(g_quad)));
        m_vertexBuffer->create();

        m_uniformBuffer.reset(m_rhi->newBuffer(QRhiBuffer::Dynamic,
                                               QRhiBuffer::UniformBuffer, 160));
        m_uniformBuffer->create();

        m_sampler.reset(m_rhi->newSampler(QRhiSampler::Nearest, QRhiSampler::Nearest,
                                          QRhiSampler::None, QRhiSampler::ClampToEdge,
                                          QRhiSampler::ClampToEdge));
        m_sampler->create();

        m_srb.reset(m_rhi->newShaderResourceBindings());
        m_srb->setBindings({
                QRhiShaderResourceBinding::uniformBuffer(
                        0,
                        QRhiShaderResourceBinding::VertexStage
                                | QRhiShaderResourceBinding::FragmentStage,
                        m_uniformBuffer.get()),
                QRhiShaderResourceBinding::sampledTexture(
                        1, QRhiShaderResourceBinding::FragmentStage, externalTex, m_sampler.get())
        });
        m_srb->create();

        m_vertexShader = loadShader(
                QStringLiteral(":/qt-project.org/multimedia/shaders/externalsampler.vert.qsb"));
        m_fragmentShader = loadShader(
                QStringLiteral(":/qt-project.org/multimedia/shaders/externalsampler.frag.qsb"));
    }

    std::unique_ptr<QRhiTexture> copyExternalTexture(QSize size, const QMatrix4x4 &externalTexMatrix);

private:
    static QShader loadShader(const QString &name)
    {
        QFile f(name);
        if (f.open(QIODevice::ReadOnly))
            return QShader::fromSerialized(f.readAll());
        return {};
    }

    QRhi *m_rhi{ nullptr };
    std::unique_ptr<QRhiBuffer> m_vertexBuffer;
    std::unique_ptr<QRhiBuffer> m_uniformBuffer;
    std::unique_ptr<QRhiSampler> m_sampler;
    std::unique_ptr<QRhiShaderResourceBindings> m_srb;
    QShader m_vertexShader;
    QShader m_fragmentShader;
};

std::unique_ptr<QRhiGraphicsPipeline> newGraphicsPipeline(QRhi *rhi,
                                                          QRhiShaderResourceBindings *srb,
                                                          QRhiRenderPassDescriptor *rpd,
                                                          QShader vs, QShader fs)
{
    std::unique_ptr<QRhiGraphicsPipeline> gp(rhi->newGraphicsPipeline());
    gp->setTopology(QRhiGraphicsPipeline::TriangleFan);
    gp->setShaderStages({
            { QRhiShaderStage::Vertex, vs },
            { QRhiShaderStage::Fragment, fs }
    });
    QRhiVertexInputLayout layout;
    layout.setBindings({ { 4 * sizeof(float) } });
    layout.setAttributes({
            { 0, 0, QRhiVertexInputAttribute::Float2, 0 },
            { 0, 1, QRhiVertexInputAttribute::Float2, 2 * sizeof(float) }
    });
    gp->setVertexInputLayout(layout);
    gp->setShaderResourceBindings(srb);
    gp->setRenderPassDescriptor(rpd);
    gp->create();
    return gp;
}

std::unique_ptr<QRhiTexture>
TextureCopy::copyExternalTexture(QSize size, const QMatrix4x4 &externalTexMatrix)
{
    std::unique_ptr<QRhiTexture> tex(
            m_rhi->newTexture(QRhiTexture::RGBA8, size, 1, QRhiTexture::RenderTarget));
    if (!tex->create()) {
        qCWarning(qLcOhosMediaPlugin) << "Failed to create frame texture";
        return {};
    }

    std::unique_ptr<QRhiTextureRenderTarget> renderTarget(
            m_rhi->newTextureRenderTarget({ { tex.get() } }));
    std::unique_ptr<QRhiRenderPassDescriptor> rpd(
            renderTarget->newCompatibleRenderPassDescriptor());
    renderTarget->setRenderPassDescriptor(rpd.get());
    renderTarget->create();

    QRhiResourceUpdateBatch *rub = m_rhi->nextResourceUpdateBatch();
    rub->uploadStaticBuffer(m_vertexBuffer.get(), g_quad);

    const QMatrix4x4 identity;
    char *p = m_uniformBuffer->beginFullDynamicBufferUpdateForCurrentFrame();
    memcpy(p, identity.constData(), 64);
    memcpy(p + 64, externalTexMatrix.constData(), 64);
    const float opacity = 1.0f;
    memcpy(p + 64 + 64, &opacity, 4);
    m_uniformBuffer->endFullDynamicBufferUpdateForCurrentFrame();

    auto pipeline = newGraphicsPipeline(m_rhi, m_srb.get(), rpd.get(), m_vertexShader,
                                        m_fragmentShader);

    const QRhiCommandBuffer::VertexInput vbufBinding(m_vertexBuffer.get(), 0);
    QRhiCommandBuffer *cb = nullptr;
    if (m_rhi->beginOffscreenFrame(&cb) != QRhi::FrameOpSuccess)
        return {};

    cb->beginPass(renderTarget.get(), Qt::transparent, { 1.0f, 0 }, rub);
    cb->setGraphicsPipeline(pipeline.get());
    cb->setViewport({ 0, 0, float(size.width()), float(size.height()) });
    cb->setShaderResources(m_srb.get());
    cb->setVertexInput(0, 1, &vbufBinding);
    cb->draw(4);
    cb->endPass();
    m_rhi->endOffscreenFrame();

    return tex;
}

} // namespace

class QOhosTextureThread : public QThread
{
    Q_OBJECT
public:
    QOhosTextureThread() : QThread() { }

    ~QOhosTextureThread() override
    {
        QMetaObject::invokeMethod(this, &QOhosTextureThread::tearDown, Qt::BlockingQueuedConnection);
        quit();
        wait();
    }

    void launch()
    {
        QThread::start();
        moveToThread(this);
    }

    OHNativeWindow *nativeWindowBlocking(QRhi *rhi)
    {
        OHNativeWindow *window = nullptr;
        QMetaObject::invokeMethod(
                this, [&]() { window = ensureSurface(rhi); }, Qt::BlockingQueuedConnection);
        return window;
    }

    QByteArray surfaceIdBlocking(QRhi *rhi)
    {
        QByteArray id;
        QMetaObject::invokeMethod(
                this,
                [&]() {
                    ensureSurface(rhi);
                    if (m_surfaceImage)
                        id = m_surfaceImage->surfaceId();
                },
                Qt::BlockingQueuedConnection);
        return id;
    }

    void setFrameSizeBlocking(QSize size)
    {
        QMetaObject::invokeMethod(
                this, [&]() { m_size = size; }, Qt::BlockingQueuedConnection);
    }

public slots:
    void tearDown()
    {
        m_surfaceImage.reset();
        m_externalTexture.reset();
        m_textureCopy.reset();
        m_rhi.reset();
    }

    void onFrameAvailable(quint64 index)
    {
        if (!m_surfaceImage || m_surfaceImage->index() != index)
            return;
        if (!m_surfaceImage->updateTexImage())
            return;
        if (!m_textureCopy)
            return;
        // GL bottom-left to top-left origin flip.
        static const QMatrix4x4 flipV(1.0f,  0.0f, 0.0f, 0.0f,
                                      0.0f, -1.0f, 0.0f, 1.0f,
                                      0.0f,  0.0f, 1.0f, 0.0f,
                                      0.0f,  0.0f, 0.0f, 1.0f);
        QMatrix4x4 matrix = m_surfaceImage->transformMatrix();
        matrix *= flipV;
        auto rgba = m_textureCopy->copyExternalTexture(m_size, matrix);
        if (!rgba)
            return;
        QPointer<QOpenGLContext> ctx;
        if (const auto *handles =
                    static_cast<const QRhiGles2NativeHandles *>(m_rhi->nativeHandles()))
            ctx = handles->context;
        auto buffer = std::make_unique<QOhosTextureVideoBuffer>(
                std::move(rgba), m_size, std::weak_ptr<QRhi>(m_rhi),
                QPointer<QObject>(this), ctx);
        QVideoFrame frame = QVideoFramePrivate::createFrame(
                std::move(buffer), QVideoFrameFormat(m_size, QVideoFrameFormat::Format_RGBA8888));
        emit newFrame(frame);
    }

signals:
    void newFrame(const QVideoFrame &frame);

private:
    OHNativeWindow *ensureSurface(QRhi *rhi)
    {
        // Re-use the existing surface if the parent RHI matches what we built
        // the texture thread RHI around. For headless camera (rhi == nullptr)
        // we accept any prior standalone surface.
        const bool reuse = m_surfaceImage && m_surfaceImage->isValid()
                && (rhi ? m_rhi.get() == rhi : m_isHeadless);
        if (reuse)
            return m_surfaceImage->nativeWindow();

        // Share with the main RHI's GL context so consumers can sample the
        // texture. For headless mode we create a standalone offscreen GLES2
        // RHI without a share context — frames are produced but consumers
        // (cameras, recorders) only need the native surface handle.
        QRhiGles2InitParams params;
        const auto *nativeHandles =
                rhi ? static_cast<const QRhiGles2NativeHandles *>(rhi->nativeHandles())
                    : nullptr;
        params.shareContext = nativeHandles ? nativeHandles->context : nullptr;
        params.fallbackSurface = QRhiGles2InitParams::newFallbackSurface();
        m_isHeadless = (rhi == nullptr);
        m_rhi.reset(QRhi::create(QRhi::OpenGLES2, &params));
        if (!m_rhi) {
            qCWarning(qLcOhosMediaPlugin) << "Failed to create offscreen GLES2 RHI";
            return nullptr;
        }

        m_externalTexture.reset(
                m_rhi->newTexture(QRhiTexture::RGBA8, m_size.isEmpty() ? QSize{ 1, 1 } : m_size, 1,
                                  QRhiTexture::ExternalOES));
        if (!m_externalTexture->create()) {
            qCWarning(qLcOhosMediaPlugin) << "External OES texture create failed";
            m_externalTexture.reset();
            m_rhi.reset();
            return nullptr;
        }

        const auto nativeTex = m_externalTexture->nativeTexture();
        m_surfaceImage = std::make_unique<QOhosSurfaceImage>(uint32_t(nativeTex.object));
        if (!m_surfaceImage->isValid()) {
            m_surfaceImage.reset();
            m_externalTexture.reset();
            m_rhi.reset();
            return nullptr;
        }

        const quint64 index = m_surfaceImage->index();
        connect(m_surfaceImage.get(), &QOhosSurfaceImage::frameAvailable, this,
                [this, index]() { onFrameAvailable(index); }, Qt::QueuedConnection);

        m_textureCopy = std::make_unique<TextureCopy>(m_rhi.get(), m_externalTexture.get());
        return m_surfaceImage->nativeWindow();
    }

    std::shared_ptr<QRhi> m_rhi;
    std::unique_ptr<QRhiTexture> m_externalTexture;
    std::unique_ptr<QOhosSurfaceImage> m_surfaceImage;
    std::unique_ptr<TextureCopy> m_textureCopy;
    QSize m_size{ 1, 1 };
    bool m_isHeadless{ false };
};

QOhosVideoOutput::QOhosVideoOutput(QVideoSink *sink, QObject *parent)
    : QObject(parent), m_sink(sink)
{
    m_textureThread = std::make_shared<QOhosTextureThread>();
    connect(m_textureThread.get(), &QOhosTextureThread::newFrame, this,
            &QOhosVideoOutput::onNewFrame, Qt::QueuedConnection);
    m_textureThread->launch();

    if (auto *p = sink ? sink->platformVideoSink() : nullptr) {
        connect(p, &QPlatformVideoSink::rhiChanged, this, &QOhosVideoOutput::onRhiChanged);
    }
}

void QOhosVideoOutput::onRhiChanged()
{
    if (!m_sink || !m_sink->rhi())
        return;
    if (m_surfaceCreatedWithoutRhi) {
        QMetaObject::invokeMethod(m_textureThread.get(), &QOhosTextureThread::tearDown,
                                  Qt::BlockingQueuedConnection);
        m_surfaceCreatedWithoutRhi = false;
    }
    emit surfaceReady();
}

QOhosVideoOutput::~QOhosVideoOutput()
{
    QMetaObject::invokeMethod(m_textureThread.get(), &QOhosTextureThread::tearDown,
                              Qt::BlockingQueuedConnection);
}

OHNativeWindow *QOhosVideoOutput::nativeWindow()
{
    auto *rhi = m_sink ? m_sink->rhi() : nullptr;
    if (!rhi) {
        m_surfaceCreatedWithoutRhi = true;
    } else if (m_surfaceCreatedWithoutRhi) {
        QMetaObject::invokeMethod(m_textureThread.get(), &QOhosTextureThread::tearDown,
                                  Qt::BlockingQueuedConnection);
        m_surfaceCreatedWithoutRhi = false;
    }
    return m_textureThread->nativeWindowBlocking(rhi);
}

QByteArray QOhosVideoOutput::surfaceId()
{
    // The camera framework needs a surface even when there's no sink or the
    // sink isn't bound to a window yet. Spin up an offscreen RHI internally so
    // capture can proceed headless; surfaceReady will reattach once the sink
    // gets a real RHI.
    auto *rhi = m_sink ? m_sink->rhi() : nullptr;
    if (!rhi)
        m_surfaceCreatedWithoutRhi = true;
    else if (m_surfaceCreatedWithoutRhi) {
        QMetaObject::invokeMethod(m_textureThread.get(), &QOhosTextureThread::tearDown,
                                  Qt::BlockingQueuedConnection);
        m_surfaceCreatedWithoutRhi = false;
    }
    return m_textureThread->surfaceIdBlocking(rhi);
}

void QOhosVideoOutput::setVideoSize(const QSize &size)
{
    if (m_videoSize == size || !size.isValid())
        return;
    m_videoSize = size;
    m_textureThread->setFrameSizeBlocking(size);
    if (m_sink) {
        if (auto *p = m_sink->platformVideoSink())
            p->setNativeSize(size);
    }
}

void QOhosVideoOutput::onNewFrame(const QVideoFrame &frame)
{
    if (m_sink)
        m_sink->setVideoFrame(frame);
}

QT_END_NAMESPACE

#include "qohosvideooutput.moc"
#include "moc_qohosvideooutput_p.cpp"
