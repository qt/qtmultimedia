/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "jsurfacetexture.h"
#include <QtCore/private/qjni_p.h>
#include <QtCore/private/qjnihelpers_p.h>

QT_BEGIN_NAMESPACE

static jclass g_qtSurfaceTextureListenerClass = 0;
static QMap<int, JSurfaceTexture*> g_objectMap;

// native method for QtSurfaceTexture.java
static void notifyFrameAvailable(JNIEnv* , jobject, int id)
{
    JSurfaceTexture *obj = g_objectMap.value(id, 0);
    if (obj)
        Q_EMIT obj->frameAvailable();
}

JSurfaceTexture::JSurfaceTexture(unsigned int texName)
    : QObject()
    , m_texID(int(texName))
{
    // API level 11 or higher is required
    if (QtAndroidPrivate::androidSdkVersion() < 11) {
        qWarning("Camera preview and video playback require Android 3.0 (API level 11) or later.");
        return;
    }

    QJNIEnvironmentPrivate env;
    m_surfaceTexture = QJNIObjectPrivate("android/graphics/SurfaceTexture", "(I)V", jint(texName));
    if (env->ExceptionCheck()) {
#ifdef QT_DEBUG
        env->ExceptionDescribe();
#endif // QT_DEBUG
        env->ExceptionClear();
    }

    if (m_surfaceTexture.isValid())
        g_objectMap.insert(int(texName), this);

    QJNIObjectPrivate listener(g_qtSurfaceTextureListenerClass, "(I)V", jint(texName));
    m_surfaceTexture.callMethod<void>("setOnFrameAvailableListener",
                                      "(Landroid/graphics/SurfaceTexture$OnFrameAvailableListener;)V",
                                      listener.object());
}

JSurfaceTexture::~JSurfaceTexture()
{
    if (m_surfaceTexture.isValid()) {
        release();
        g_objectMap.remove(m_texID);
    }
}

QMatrix4x4 JSurfaceTexture::getTransformMatrix()
{
    QMatrix4x4 matrix;
    if (!m_surfaceTexture.isValid())
        return matrix;

    QJNIEnvironmentPrivate env;

    jfloatArray array = env->NewFloatArray(16);
    m_surfaceTexture.callMethod<void>("getTransformMatrix", "([F)V", array);
    env->GetFloatArrayRegion(array, 0, 16, matrix.data());
    env->DeleteLocalRef(array);

    return matrix;
}

void JSurfaceTexture::release()
{
    if (QtAndroidPrivate::androidSdkVersion() < 14)
        return;

    m_surfaceTexture.callMethod<void>("release");
}

void JSurfaceTexture::updateTexImage()
{
    if (!m_surfaceTexture.isValid())
        return;

    m_surfaceTexture.callMethod<void>("updateTexImage");
}

jobject JSurfaceTexture::object()
{
    return m_surfaceTexture.object();
}

static JNINativeMethod methods[] = {
    {"notifyFrameAvailable", "(I)V", (void *)notifyFrameAvailable}
};

bool JSurfaceTexture::initJNI(JNIEnv *env)
{
    // SurfaceTexture is available since API 11.
    if (QtAndroidPrivate::androidSdkVersion() < 11)
        return false;

    jclass clazz = env->FindClass("org/qtproject/qt5/android/multimedia/QtSurfaceTextureListener");
    if (env->ExceptionCheck())
        env->ExceptionClear();

    if (clazz) {
        g_qtSurfaceTextureListenerClass = static_cast<jclass>(env->NewGlobalRef(clazz));
        if (env->RegisterNatives(g_qtSurfaceTextureListenerClass,
                                 methods,
                                 sizeof(methods) / sizeof(methods[0])) < 0) {
            return false;
        }
    }

    return true;
}

QT_END_NAMESPACE
