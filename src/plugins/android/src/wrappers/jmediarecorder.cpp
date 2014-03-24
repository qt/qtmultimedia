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

#include "jmediarecorder.h"

#include "jcamera.h"
#include <QtCore/private/qjni_p.h>
#include <qmap.h>

QT_BEGIN_NAMESPACE

static jclass g_qtMediaRecorderClass = 0;
static QMap<jlong, JMediaRecorder*> g_objectMap;

static void notifyError(JNIEnv* , jobject, jlong id, jint what, jint extra)
{
    JMediaRecorder *obj = g_objectMap.value(id, 0);
    if (obj)
        emit obj->error(what, extra);
}

static void notifyInfo(JNIEnv* , jobject, jlong id, jint what, jint extra)
{
    JMediaRecorder *obj = g_objectMap.value(id, 0);
    if (obj)
        emit obj->info(what, extra);
}

JMediaRecorder::JMediaRecorder()
    : QObject()
    , QJNIObjectPrivate(g_qtMediaRecorderClass, "(J)V", reinterpret_cast<jlong>(this))
    , m_id(reinterpret_cast<jlong>(this))
{
    if (isValid())
        g_objectMap.insert(m_id, this);
}

JMediaRecorder::~JMediaRecorder()
{
    g_objectMap.remove(m_id);
}

void JMediaRecorder::release()
{
    callMethod<void>("release");
}

bool JMediaRecorder::prepare()
{
    QJNIEnvironmentPrivate env;
    callMethod<void>("prepare");
    if (env->ExceptionCheck()) {
#ifdef QT_DEBUG
        env->ExceptionDescribe();
#endif
        env->ExceptionClear();
        return false;
    }
    return true;
}

void JMediaRecorder::reset()
{
    callMethod<void>("reset");
}

bool JMediaRecorder::start()
{
    QJNIEnvironmentPrivate env;
    callMethod<void>("start");
    if (env->ExceptionCheck()) {
#ifdef QT_DEBUG
        env->ExceptionDescribe();
#endif
        env->ExceptionClear();
        return false;
    }
    return true;
}

void JMediaRecorder::stop()
{
    QJNIEnvironmentPrivate env;
    callMethod<void>("stop");
    if (env->ExceptionCheck()) {
#ifdef QT_DEBUG
        env->ExceptionDescribe();
#endif
        env->ExceptionClear();
    }
}

void JMediaRecorder::setAudioChannels(int numChannels)
{
    callMethod<void>("setAudioChannels", "(I)V", numChannels);
}

void JMediaRecorder::setAudioEncoder(AudioEncoder encoder)
{
    QJNIEnvironmentPrivate env;
    callMethod<void>("setAudioEncoder", "(I)V", int(encoder));
    if (env->ExceptionCheck()) {
#ifdef QT_DEBUG
        env->ExceptionDescribe();
#endif
        env->ExceptionClear();
    }
}

void JMediaRecorder::setAudioEncodingBitRate(int bitRate)
{
    callMethod<void>("setAudioEncodingBitRate", "(I)V", bitRate);
}

void JMediaRecorder::setAudioSamplingRate(int samplingRate)
{
    callMethod<void>("setAudioSamplingRate", "(I)V", samplingRate);
}

void JMediaRecorder::setAudioSource(AudioSource source)
{
    QJNIEnvironmentPrivate env;
    callMethod<void>("setAudioSource", "(I)V", int(source));
    if (env->ExceptionCheck()) {
#ifdef QT_DEBUG
        env->ExceptionDescribe();
#endif
        env->ExceptionClear();
    }
}

void JMediaRecorder::setCamera(JCamera *camera)
{
    QJNIObjectPrivate cam = camera->getCameraObject();
    callMethod<void>("setCamera", "(Landroid/hardware/Camera;)V", cam.object());
}

void JMediaRecorder::setVideoEncoder(VideoEncoder encoder)
{
    QJNIEnvironmentPrivate env;
    callMethod<void>("setVideoEncoder", "(I)V", int(encoder));
    if (env->ExceptionCheck()) {
#ifdef QT_DEBUG
        env->ExceptionDescribe();
#endif
        env->ExceptionClear();
    }
}

void JMediaRecorder::setVideoEncodingBitRate(int bitRate)
{
    callMethod<void>("setVideoEncodingBitRate", "(I)V", bitRate);
}

void JMediaRecorder::setVideoFrameRate(int rate)
{
    QJNIEnvironmentPrivate env;
    callMethod<void>("setVideoFrameRate", "(I)V", rate);
    if (env->ExceptionCheck()) {
#ifdef QT_DEBUG
        env->ExceptionDescribe();
#endif
        env->ExceptionClear();
    }
}

void JMediaRecorder::setVideoSize(const QSize &size)
{
    QJNIEnvironmentPrivate env;
    callMethod<void>("setVideoSize", "(II)V", size.width(), size.height());
    if (env->ExceptionCheck()) {
#ifdef QT_DEBUG
        env->ExceptionDescribe();
#endif
        env->ExceptionClear();
    }
}

void JMediaRecorder::setVideoSource(VideoSource source)
{
    QJNIEnvironmentPrivate env;
    callMethod<void>("setVideoSource", "(I)V", int(source));
    if (env->ExceptionCheck()) {
#ifdef QT_DEBUG
        env->ExceptionDescribe();
#endif
        env->ExceptionClear();
    }
}

void JMediaRecorder::setOrientationHint(int degrees)
{
    QJNIEnvironmentPrivate env;
    callMethod<void>("setOrientationHint", "(I)V", degrees);
    if (env->ExceptionCheck()) {
#ifdef QT_DEBUG
        env->ExceptionDescribe();
#endif
        env->ExceptionClear();
    }
}

void JMediaRecorder::setOutputFormat(OutputFormat format)
{
    QJNIEnvironmentPrivate env;
    callMethod<void>("setOutputFormat", "(I)V", int(format));
    if (env->ExceptionCheck()) {
#ifdef QT_DEBUG
        env->ExceptionDescribe();
#endif
        env->ExceptionClear();
    }
}

void JMediaRecorder::setOutputFile(const QString &path)
{
    QJNIEnvironmentPrivate env;
    callMethod<void>("setOutputFile",
                     "(Ljava/lang/String;)V",
                     QJNIObjectPrivate::fromString(path).object());
    if (env->ExceptionCheck()) {
#ifdef QT_DEBUG
        env->ExceptionDescribe();
#endif
        env->ExceptionClear();
    }
}

static JNINativeMethod methods[] = {
    {"notifyError", "(JII)V", (void *)notifyError},
    {"notifyInfo", "(JII)V", (void *)notifyInfo}
};

bool JMediaRecorder::initJNI(JNIEnv *env)
{
    jclass clazz = env->FindClass("org/qtproject/qt5/android/multimedia/QtMediaRecorder");
    if (env->ExceptionCheck())
        env->ExceptionClear();

    if (clazz) {
        g_qtMediaRecorderClass = static_cast<jclass>(env->NewGlobalRef(clazz));
        if (env->RegisterNatives(g_qtMediaRecorderClass,
                                 methods,
                                 sizeof(methods) / sizeof(methods[0])) < 0) {
            return false;
        }
    }

    return true;
}

QT_END_NAMESPACE
