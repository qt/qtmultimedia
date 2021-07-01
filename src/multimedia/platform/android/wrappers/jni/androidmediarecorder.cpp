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

#include "androidmediarecorder_p.h"

#include "androidcamera_p.h"
#include "androidsurfacetexture_p.h"
#include "androidsurfaceview_p.h"
#include "qandroidglobal_p.h"
#include "qandroidmultimediautils_p.h"
#include <qmap.h>

QT_BEGIN_NAMESPACE

typedef QMap<QString, QJniObject> CamcorderProfiles;
Q_GLOBAL_STATIC(CamcorderProfiles, g_camcorderProfiles)

static inline bool exceptionCheckAndClear()
{
#ifdef QT_DEBUG
    return QJniEnvironment().checkAndClearExceptions(QJniEnvironment::OutputMode::Verbose);
#else
    return QJniEnvironment().checkAndClearExceptions();
#endif // QT_DEBUG
}

static QString profileKey()
{
    return QStringLiteral("%1-%2");
}

bool AndroidCamcorderProfile::hasProfile(jint cameraId, Quality quality)
{
    if (g_camcorderProfiles->contains(profileKey().arg(cameraId).arg(quality)))
        return true;

    return QJniObject::callStaticMethod<jboolean>("android/media/CamcorderProfile",
                                                         "hasProfile",
                                                         "(II)Z",
                                                         cameraId,
                                                         quality);
}

AndroidCamcorderProfile AndroidCamcorderProfile::get(jint cameraId, Quality quality)
{
    const QString key = profileKey().arg(cameraId).arg(quality);
    QMap<QString, QJniObject>::const_iterator it = g_camcorderProfiles->constFind(key);

    if (it != g_camcorderProfiles->constEnd())
        return AndroidCamcorderProfile(*it);

    QJniObject camProfile = QJniObject::callStaticObjectMethod("android/media/CamcorderProfile",
                                                                             "get",
                                                                             "(II)Landroid/media/CamcorderProfile;",
                                                                             cameraId,
                                                                             quality);

    return AndroidCamcorderProfile((*g_camcorderProfiles)[key] = camProfile);
}

int AndroidCamcorderProfile::getValue(AndroidCamcorderProfile::Field field) const
{
    switch (field) {
    case audioBitRate:
        return m_camcorderProfile.getField<jint>("audioBitRate");
    case audioChannels:
        return m_camcorderProfile.getField<jint>("audioChannels");
    case audioCodec:
        return m_camcorderProfile.getField<jint>("audioCodec");
    case audioSampleRate:
        return m_camcorderProfile.getField<jint>("audioSampleRate");
    case duration:
        return m_camcorderProfile.getField<jint>("duration");
    case fileFormat:
        return m_camcorderProfile.getField<jint>("fileFormat");
    case quality:
        return m_camcorderProfile.getField<jint>("quality");
    case videoBitRate:
        return m_camcorderProfile.getField<jint>("videoBitRate");
    case videoCodec:
        return m_camcorderProfile.getField<jint>("videoCodec");
    case videoFrameHeight:
        return m_camcorderProfile.getField<jint>("videoFrameHeight");
    case videoFrameRate:
        return m_camcorderProfile.getField<jint>("videoFrameRate");
    case videoFrameWidth:
        return m_camcorderProfile.getField<jint>("videoFrameWidth");
    }

    return 0;
}

AndroidCamcorderProfile::AndroidCamcorderProfile(const QJniObject &camcorderProfile)
{
    m_camcorderProfile = camcorderProfile;
}

static const char QtMediaRecorderListenerClassName[] = "org/qtproject/qt/android/multimedia/QtMediaRecorderListener";
typedef QMap<jlong, AndroidMediaRecorder*> MediaRecorderMap;
Q_GLOBAL_STATIC(MediaRecorderMap, mediaRecorders)

static void notifyError(JNIEnv* , jobject, jlong id, jint what, jint extra)
{
    AndroidMediaRecorder *obj = mediaRecorders->value(id, 0);
    if (obj)
        emit obj->error(what, extra);
}

static void notifyInfo(JNIEnv* , jobject, jlong id, jint what, jint extra)
{
    AndroidMediaRecorder *obj = mediaRecorders->value(id, 0);
    if (obj)
        emit obj->info(what, extra);
}

AndroidMediaRecorder::AndroidMediaRecorder()
    : QObject()
    , m_id(reinterpret_cast<jlong>(this))
{
    m_mediaRecorder = QJniObject("android/media/MediaRecorder");
    if (m_mediaRecorder.isValid()) {
        QJniObject listener(QtMediaRecorderListenerClassName, "(J)V", m_id);
        m_mediaRecorder.callMethod<void>("setOnErrorListener",
                                         "(Landroid/media/MediaRecorder$OnErrorListener;)V",
                                         listener.object());
        m_mediaRecorder.callMethod<void>("setOnInfoListener",
                                         "(Landroid/media/MediaRecorder$OnInfoListener;)V",
                                         listener.object());
        mediaRecorders->insert(m_id, this);
    }
}

AndroidMediaRecorder::~AndroidMediaRecorder()
{
    mediaRecorders->remove(m_id);
}

void AndroidMediaRecorder::release()
{
    m_mediaRecorder.callMethod<void>("release");
}

bool AndroidMediaRecorder::prepare()
{
    QJniEnvironment env;
    auto methodId = env->GetMethodID(m_mediaRecorder.objectClass(), "prepare", "()V");
    env->CallVoidMethod(m_mediaRecorder.object(), methodId);

    if (exceptionCheckAndClear())
        return false;
    return true;
}

void AndroidMediaRecorder::reset()
{
    m_mediaRecorder.callMethod<void>("reset");
}

bool AndroidMediaRecorder::start()
{
    QJniEnvironment env;
    auto methodId = env->GetMethodID(m_mediaRecorder.objectClass(), "start", "()V");
    env->CallVoidMethod(m_mediaRecorder.object(), methodId);

    if (exceptionCheckAndClear())
        return false;
    return true;
}

void AndroidMediaRecorder::stop()
{
    m_mediaRecorder.callMethod<void>("stop");
}

void AndroidMediaRecorder::setAudioChannels(int numChannels)
{
    m_mediaRecorder.callMethod<void>("setAudioChannels", "(I)V", numChannels);
}

void AndroidMediaRecorder::setAudioEncoder(AudioEncoder encoder)
{
    QJniEnvironment env;
    m_mediaRecorder.callMethod<void>("setAudioEncoder", "(I)V", int(encoder));
}

void AndroidMediaRecorder::setAudioEncodingBitRate(int bitRate)
{
    m_mediaRecorder.callMethod<void>("setAudioEncodingBitRate", "(I)V", bitRate);
}

void AndroidMediaRecorder::setAudioSamplingRate(int samplingRate)
{
    m_mediaRecorder.callMethod<void>("setAudioSamplingRate", "(I)V", samplingRate);
}

void AndroidMediaRecorder::setAudioSource(AudioSource source)
{
    m_mediaRecorder.callMethod<void>("setAudioSource", "(I)V", int(source));
}

bool AndroidMediaRecorder::setAudioInput(const QByteArray &id)
{
    const bool ret = QJniObject::callStaticMethod<jboolean>("org/qtproject/qt/android/multimedia/QtAudioDeviceManager",
                                                    "setAudioInput",
                                                    "(Landroid/media/MediaRecorder;I)Z",
                                                    m_mediaRecorder.object(),
                                                    id.toInt());
    if (!ret)
        qCWarning(QLoggingCategory("mediarecorder")) << "No default input device was set";

    return ret;
}

void AndroidMediaRecorder::setCamera(AndroidCamera *camera)
{
    QJniObject cam = camera->getCameraObject();
    m_mediaRecorder.callMethod<void>("setCamera", "(Landroid/hardware/Camera;)V", cam.object());
}

void AndroidMediaRecorder::setVideoEncoder(VideoEncoder encoder)
{
    m_mediaRecorder.callMethod<void>("setVideoEncoder", "(I)V", int(encoder));
}

void AndroidMediaRecorder::setVideoEncodingBitRate(int bitRate)
{
    m_mediaRecorder.callMethod<void>("setVideoEncodingBitRate", "(I)V", bitRate);
}

void AndroidMediaRecorder::setVideoFrameRate(int rate)
{
    m_mediaRecorder.callMethod<void>("setVideoFrameRate", "(I)V", rate);
}

void AndroidMediaRecorder::setVideoSize(const QSize &size)
{
    m_mediaRecorder.callMethod<void>("setVideoSize", "(II)V", size.width(), size.height());
}

void AndroidMediaRecorder::setVideoSource(VideoSource source)
{
    m_mediaRecorder.callMethod<void>("setVideoSource", "(I)V", int(source));
}

void AndroidMediaRecorder::setOrientationHint(int degrees)
{
    m_mediaRecorder.callMethod<void>("setOrientationHint", "(I)V", degrees);
}

void AndroidMediaRecorder::setOutputFormat(OutputFormat format)
{
    m_mediaRecorder.callMethod<void>("setOutputFormat", "(I)V", int(format));
}

void AndroidMediaRecorder::setOutputFile(const QString &path)
{
    m_mediaRecorder.callMethod<void>("setOutputFile",
                                     "(Ljava/lang/String;)V",
                                     QJniObject::fromString(path).object());
}

void AndroidMediaRecorder::setSurfaceTexture(AndroidSurfaceTexture *texture)
{
    m_mediaRecorder.callMethod<void>("setPreviewDisplay",
                                     "(Landroid/view/Surface;)V",
                                     texture->surface());
}

void AndroidMediaRecorder::setSurfaceHolder(AndroidSurfaceHolder *holder)
{
    QJniObject surfaceHolder(holder->surfaceHolder());
    QJniObject surface = surfaceHolder.callObjectMethod("getSurface",
                                                               "()Landroid/view/Surface;");
    if (!surface.isValid())
        return;

    m_mediaRecorder.callMethod<void>("setPreviewDisplay",
                                     "(Landroid/view/Surface;)V",
                                     surface.object());
}

bool AndroidMediaRecorder::registerNativeMethods()
{
    static JNINativeMethod methods[] = {
        {"notifyError", "(JII)V", (void *)notifyError},
        {"notifyInfo", "(JII)V", (void *)notifyInfo}
    };

    const int size = sizeof(methods) / sizeof(methods[0]);
    return QJniEnvironment().registerNativeMethods(QtMediaRecorderListenerClassName, methods, size);
}

QT_END_NAMESPACE
