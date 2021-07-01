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
#include <private/qabstractvideobuffer_p.h>
#include <qmatrix4x4.h>

QT_BEGIN_NAMESPACE

class AndroidSurfaceTexture;
class AndroidSurfaceHolder;
class QOpenGLFramebufferObject;
class QOpenGLShaderProgram;
class QWindow;
class QOpenGLContext;
class QVideoSink;
class QRhi;

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

class OpenGLResourcesDeleter : public QObject
{
    Q_OBJECT
public:
    void deleteTexture(quint32 id) { QMetaObject::invokeMethod(this, "deleteTextureHelper", Qt::AutoConnection, Q_ARG(quint32, id)); }
    void deleteFbo(QOpenGLFramebufferObject *fbo) { QMetaObject::invokeMethod(this, "deleteFboHelper", Qt::AutoConnection, Q_ARG(void *, fbo)); }
    void deleteShaderProgram(QOpenGLShaderProgram *prog) { QMetaObject::invokeMethod(this, "deleteShaderProgramHelper", Qt::AutoConnection, Q_ARG(void *, prog)); }
    void deleteThis() { QMetaObject::invokeMethod(this, "deleteThisHelper"); }

private:
    Q_INVOKABLE void deleteTextureHelper(quint32 id);
    Q_INVOKABLE void deleteFboHelper(void *fbo);
    Q_INVOKABLE void deleteShaderProgramHelper(void *prog);
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

private Q_SLOTS:
    void onFrameAvailable();

private:
    void initSurfaceTexture();
    bool renderFrameToFbo();
    void createGLResources();

    QMutex m_mutex;
    void clearSurfaceTexture();

    QVideoSink *m_sink = nullptr;
    QSize m_nativeSize;

    AndroidSurfaceTexture *m_surfaceTexture = nullptr;

    quint32 m_externalTex = 0;
    QOpenGLFramebufferObject *m_fbo = nullptr;
    QOpenGLShaderProgram *m_program = nullptr;
    OpenGLResourcesDeleter *m_glDeleter = nullptr;

    QWindow *m_offscreenSurface = nullptr;
    QOpenGLContext *m_glContext = nullptr;

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
    bool updateFrame()
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
        const bool forceUpdate = !m_output->m_fbo;

        if (m_textureUpdated && !forceUpdate)
            return true;

        // update the video texture (called from the render thread)
        return (m_textureUpdated = m_output->renderFrameToFbo());
    }

    QVideoFrame::MapMode m_mapMode = QVideoFrame::NotMapped;
    QAndroidTextureVideoOutput *m_output = nullptr;
    QImage m_image;
    QSize m_size;
    mutable QMatrix4x4 m_externalMatrix;
    bool m_textureUpdated = false;
};

QT_END_NAMESPACE

#endif // QANDROIDVIDEOOUTPUT_H
