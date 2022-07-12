// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QANDROIDVIDEOOUTPUT_H
#define QANDROIDVIDEOOUTPUT_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qobject.h>
#include <qsize.h>
#include <qmutex.h>
#include <qreadwritelock.h>
#include <private/qabstractvideobuffer_p.h>
#include <qmatrix4x4.h>
#include <QtGui/private/qrhi_p.h>
#include <QtGui/qoffscreensurface.h>
#include <QPixmap>

QT_BEGIN_NAMESPACE

class AndroidSurfaceTexture;
class AndroidSurfaceHolder;
class QVideoSink;

class QAndroidVideoOutput : public QObject
{
    Q_OBJECT
public:
    virtual ~QAndroidVideoOutput() { }

    virtual AndroidSurfaceTexture *surfaceTexture() { return 0; }
    virtual AndroidSurfaceHolder *surfaceHolder() { return 0; }

    virtual bool isReady() { return true; }

    virtual void setVideoSize(const QSize &) { }
    virtual void start() { }
    virtual void stop() { }
    virtual void reset() { }

Q_SIGNALS:
    void readyChanged(bool);

protected:
    QAndroidVideoOutput(QObject *parent) : QObject(parent) { }
};

class GraphicsResourceDeleter : public QObject
{
    Q_OBJECT
public:
    void deleteResources(const QList<QRhiResource *> &res) { QMetaObject::invokeMethod(this, "deleteResourcesHelper", Qt::AutoConnection, Q_ARG(QList<QRhiResource*>, res)); }
    void deleteRhi(QRhi *rhi, QOffscreenSurface *surf) { QMetaObject::invokeMethod(this, "deleteRhiHelper", Qt::AutoConnection, Q_ARG(QRhi*, rhi), Q_ARG(QOffscreenSurface*, surf)); }
    void deleteThis() { QMetaObject::invokeMethod(this, "deleteThisHelper"); }

private:
    Q_INVOKABLE void deleteResourcesHelper(const QList<QRhiResource *> &res);
    Q_INVOKABLE void deleteRhiHelper(QRhi *rhi, QOffscreenSurface *surf);
    Q_INVOKABLE void deleteThisHelper();
};

class QAndroidTextureVideoOutput : public QAndroidVideoOutput
{
    Q_OBJECT
public:
    explicit QAndroidTextureVideoOutput(QObject *parent = 0);
    ~QAndroidTextureVideoOutput() override;

    QVideoSink *surface() const;
    void setSurface(QVideoSink *surface);

    AndroidSurfaceTexture *surfaceTexture() override;

    bool isReady() override;
    void setVideoSize(const QSize &) override;
    void start() override;
    void stop() override;
    void reset() override;
    void renderFrame();

    void setSubtitle(const QString &subtitle);
private Q_SLOTS:
    void onFrameAvailable();

private:
    void initSurfaceTexture();
    bool renderAndReadbackFrame();
    void ensureExternalTexture(QRhi *rhi);

    bool moveToOpenGLContextThread();

    QMutex m_mutex;
    QReadWriteLock m_subtitleLock;

    void clearSurfaceTexture();

    QVideoSink *m_sink = nullptr;
    QSize m_nativeSize;
    bool m_started = false;
    bool m_renderFrame = false;

    AndroidSurfaceTexture *m_surfaceTexture = nullptr;

    QRhiTexture *m_externalTex = nullptr;

    QRhi *m_readbackRhi = nullptr;
    QOffscreenSurface *m_readbackRhiFallbackSurface = nullptr;
    QRhiTexture *m_readbackSrc = nullptr;
    QRhiTexture *m_readbackTex = nullptr;
    QRhiBuffer *m_readbackVBuf = nullptr;
    QRhiBuffer *m_readbackUBuf = nullptr;
    QRhiSampler *m_externalTexSampler = nullptr;
    QRhiShaderResourceBindings *m_readbackSrb = nullptr;
    QRhiTextureRenderTarget *m_readbackRenderTarget = nullptr;
    QRhiRenderPassDescriptor *m_readbackRpDesc = nullptr;
    QRhiGraphicsPipeline *m_readbackPs = nullptr;

    QImage m_readbackImage;
    QByteArray m_readbackImageData;

    QString m_subtitleText;
    QPixmap m_subtitlePixmap;

    GraphicsResourceDeleter *m_graphicsDeleter = nullptr;

    QThread *m_thread = QThread::currentThread();

    friend class AndroidTextureVideoBuffer;
};


class AndroidTextureVideoBuffer : public QAbstractVideoBuffer
{
public:
    AndroidTextureVideoBuffer(QRhi *rhi, QAndroidTextureVideoOutput *output, const QSize &size)
        : QAbstractVideoBuffer(rhi ? QVideoFrame::RhiTextureHandle : QVideoFrame::NoHandle, rhi)
        , m_output(output)
        , m_size(size)
    {
    }

    virtual ~AndroidTextureVideoBuffer() {}

    QVideoFrame::MapMode mapMode() const override { return m_mapMode; }

    MapData map(QVideoFrame::MapMode mode) override;

    void unmap() override
    {
        m_image = QImage();
        m_mapMode = QVideoFrame::NotMapped;
    }

    quint64 textureHandle(int plane) const override;

    QMatrix4x4 externalTextureMatrix() const override
    {
        return m_externalMatrix;
    }

private:
    bool updateReadbackFrame();

    QVideoFrame::MapMode m_mapMode = QVideoFrame::NotMapped;
    QAndroidTextureVideoOutput *m_output = nullptr;
    QImage m_image;
    QSize m_size;
    mutable QMatrix4x4 m_externalMatrix;
    bool m_textureUpdated = false;
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QList<QRhiResource *>)
Q_DECLARE_METATYPE(QRhi*)

#endif // QANDROIDVIDEOOUTPUT_H
