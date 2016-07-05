/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QANDROIDVIDEOOUTPUT_H
#define QANDROIDVIDEOOUTPUT_H

#include <qobject.h>
#include <qsize.h>
#include <qmutex.h>

QT_BEGIN_NAMESPACE

class AndroidSurfaceTexture;
class AndroidSurfaceHolder;
class QOpenGLFramebufferObject;
class QOpenGLShaderProgram;
class QAbstractVideoSurface;

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

private:
    Q_INVOKABLE void deleteTextureHelper(quint32 id);
    Q_INVOKABLE void deleteFboHelper(void *fbo);
    Q_INVOKABLE void deleteShaderProgramHelper(void *prog);
};

class QAndroidTextureVideoOutput : public QAndroidVideoOutput
{
    Q_OBJECT
public:
    explicit QAndroidTextureVideoOutput(QObject *parent = 0);
    ~QAndroidTextureVideoOutput() Q_DECL_OVERRIDE;

    QAbstractVideoSurface *surface() const;
    void setSurface(QAbstractVideoSurface *surface);

    AndroidSurfaceTexture *surfaceTexture() Q_DECL_OVERRIDE;

    bool isReady() Q_DECL_OVERRIDE;
    void setVideoSize(const QSize &) Q_DECL_OVERRIDE;
    void stop() Q_DECL_OVERRIDE;
    void reset() Q_DECL_OVERRIDE;

    void customEvent(QEvent *) Q_DECL_OVERRIDE;

private Q_SLOTS:
    void onFrameAvailable();

private:
    bool initSurfaceTexture();
    void renderFrameToFbo();
    void createGLResources();

    QMutex m_mutex;
    void clearSurfaceTexture();

    QAbstractVideoSurface *m_surface;
    QSize m_nativeSize;

    AndroidSurfaceTexture *m_surfaceTexture;

    quint32 m_externalTex;
    QOpenGLFramebufferObject *m_fbo;
    QOpenGLShaderProgram *m_program;
    QScopedPointer<OpenGLResourcesDeleter, QScopedPointerDeleteLater> m_glDeleter;

    bool m_surfaceTextureCanAttachToContext;

    friend class AndroidTextureVideoBuffer;
};

QT_END_NAMESPACE

#endif // QANDROIDVIDEOOUTPUT_H
