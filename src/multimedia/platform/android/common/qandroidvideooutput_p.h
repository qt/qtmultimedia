/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
    void stop() override;
    void reset() override;

    void setSubtitle(const QString &subtitle);
private Q_SLOTS:
    void onFrameAvailable();

private:
    void initSurfaceTexture();
    bool renderAndReadbackFrame();
    void ensureExternalTexture(QRhi *rhi);

    QMutex m_mutex;
    QReadWriteLock m_subtitleLock;

    void clearSurfaceTexture();

    QVideoSink *m_sink = nullptr;
    QSize m_nativeSize;

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

    QMatrix4x4 externalTextureMatrix() const
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
