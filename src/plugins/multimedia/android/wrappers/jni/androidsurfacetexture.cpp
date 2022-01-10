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

#include "androidsurfacetexture_p.h"
#include <QtCore/qmutex.h>
#include <QtCore/qcoreapplication.h>

QT_BEGIN_NAMESPACE

static const char QtSurfaceTextureListenerClassName[] = "org/qtproject/qt/android/multimedia/QtSurfaceTextureListener";
typedef QList<jlong> SurfaceTextures;
Q_GLOBAL_STATIC(SurfaceTextures, g_surfaceTextures);
Q_GLOBAL_STATIC(QMutex, g_textureMutex);

// native method for QtSurfaceTexture.java
static void notifyFrameAvailable(JNIEnv* , jobject, jlong id)
{
    const QMutexLocker lock(g_textureMutex());
    const int idx = g_surfaceTextures->indexOf(id);
    if (idx == -1)
        return;

    AndroidSurfaceTexture *obj = reinterpret_cast<AndroidSurfaceTexture *>(g_surfaceTextures->at(idx));
    if (obj)
        Q_EMIT obj->frameAvailable();
}

AndroidSurfaceTexture::AndroidSurfaceTexture(quint32 texName)
    : QObject()
{
    Q_STATIC_ASSERT(sizeof (jlong) >= sizeof (void *));
    m_surfaceTexture = QJniObject("android/graphics/SurfaceTexture", "(I)V", jint(texName));

    if (!m_surfaceTexture.isValid())
        return;

    const QMutexLocker lock(g_textureMutex());
    g_surfaceTextures->append(jlong(this));
    QJniObject listener(QtSurfaceTextureListenerClassName, "(J)V", jlong(this));
    setOnFrameAvailableListener(listener);
}

AndroidSurfaceTexture::~AndroidSurfaceTexture()
{
    if (m_surface.isValid())
        m_surface.callMethod<void>("release");

    if (m_surfaceTexture.isValid()) {
        release();
        const QMutexLocker lock(g_textureMutex());
        const int idx = g_surfaceTextures->indexOf(jlong(this));
        if (idx != -1)
            g_surfaceTextures->remove(idx);
    }
}

QMatrix4x4 AndroidSurfaceTexture::getTransformMatrix()
{
    QMatrix4x4 matrix;
    if (!m_surfaceTexture.isValid())
        return matrix;

    QJniEnvironment env;
    jfloatArray array = env->NewFloatArray(16);
    m_surfaceTexture.callMethod<void>("getTransformMatrix", "([F)V", array);
    env->GetFloatArrayRegion(array, 0, 16, matrix.data());
    env->DeleteLocalRef(array);

    return matrix;
}

void AndroidSurfaceTexture::release()
{
    m_surfaceTexture.callMethod<void>("release");
}

void AndroidSurfaceTexture::updateTexImage()
{
    if (!m_surfaceTexture.isValid())
        return;

    m_surfaceTexture.callMethod<void>("updateTexImage");
}

jobject AndroidSurfaceTexture::surfaceTexture()
{
    return m_surfaceTexture.object();
}

jobject AndroidSurfaceTexture::surface()
{
    if (!m_surface.isValid()) {
        m_surface = QJniObject("android/view/Surface",
                               "(Landroid/graphics/SurfaceTexture;)V",
                               m_surfaceTexture.object());
    }

    return m_surface.object();
}

jobject AndroidSurfaceTexture::surfaceHolder()
{
    if (!m_surfaceHolder.isValid()) {
        m_surfaceHolder = QJniObject("org/qtproject/qt/android/multimedia/QtSurfaceTextureHolder",
                                     "(Landroid/view/Surface;)V",
                                     surface());
    }

    return m_surfaceHolder.object();
}

void AndroidSurfaceTexture::attachToGLContext(quint32 texName)
{
    if (!m_surfaceTexture.isValid())
        return;

    m_surfaceTexture.callMethod<void>("attachToGLContext", "(I)V", texName);
}

void AndroidSurfaceTexture::detachFromGLContext()
{
    if (!m_surfaceTexture.isValid())
        return;

    m_surfaceTexture.callMethod<void>("detachFromGLContext");
}

bool AndroidSurfaceTexture::registerNativeMethods()
{
    static const JNINativeMethod methods[] = {
        {"notifyFrameAvailable", "(J)V", (void *)notifyFrameAvailable}
    };
    const int size = std::size(methods);
    if (QJniEnvironment().registerNativeMethods(QtSurfaceTextureListenerClassName, methods, size))
        return false;

    return true;
}

void AndroidSurfaceTexture::setOnFrameAvailableListener(const QJniObject &listener)
{
    m_surfaceTexture.callMethod<void>("setOnFrameAvailableListener",
                                      "(Landroid/graphics/SurfaceTexture$OnFrameAvailableListener;)V",
                                      listener.object());
}

QT_END_NAMESPACE
