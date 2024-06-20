// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "androidmediarecorder_p.h"
#include "androidcamera_p.h"
#include "androidsurfacetexture_p.h"
#include "androidsurfaceview_p.h"
#include "qandroidglobal_p.h"
#include "qandroidmultimediautils_p.h"

#include <qmap.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qlogging.h>
#include <QtCore/qurl.h>

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(lcMediaRecorder, "qt.multimedia.mediarecorder.android");

typedef QMap<QString, QJniObject> CamcorderProfiles;
Q_GLOBAL_STATIC(CamcorderProfiles, g_camcorderProfiles)

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

static const char QtMediaRecorderListenerClassName[] =
        "org/qtproject/qt/android/multimedia/QtMediaRecorderListener";
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
    if (m_isVideoSourceSet || m_isAudioSourceSet)
        reset();

    release();
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

    if (env.checkAndClearExceptions())
        return false;
    return true;
}

void AndroidMediaRecorder::reset()
{
    m_mediaRecorder.callMethod<void>("reset");
    m_isAudioSourceSet = false; // Now setAudioSource can be used again.
    m_isVideoSourceSet = false;
}

bool AndroidMediaRecorder::start()
{
    QJniEnvironment env;
    auto methodId = env->GetMethodID(m_mediaRecorder.objectClass(), "start", "()V");
    env->CallVoidMethod(m_mediaRecorder.object(), methodId);

    if (env.checkAndClearExceptions())
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
    if (!m_isAudioSourceSet) {
        QJniEnvironment env;
        auto methodId = env->GetMethodID(m_mediaRecorder.objectClass(), "setAudioSource", "(I)V");
        env->CallVoidMethod(m_mediaRecorder.object(), methodId, source);
        if (!env.checkAndClearExceptions())
            m_isAudioSourceSet = true;
    } else {
        qCWarning(lcMediaRecorder) << "Audio source already set. Not setting a new source.";
    }
}

bool AndroidMediaRecorder::isAudioSourceSet() const
{
    return m_isAudioSourceSet;
}

bool AndroidMediaRecorder::setAudioInput(const QByteArray &id)
{
    const bool ret = QJniObject::callStaticMethod<jboolean>(
                "org/qtproject/qt/android/multimedia/QtAudioDeviceManager",
                "setAudioInput",
                "(Landroid/media/MediaRecorder;I)Z",
                m_mediaRecorder.object(),
                id.toInt());
    if (!ret)
        qCWarning(lcMediaRecorder) << "No default input device was set.";

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
    QJniEnvironment env;

    auto methodId = env->GetMethodID(m_mediaRecorder.objectClass(), "setVideoSource", "(I)V");
    env->CallVoidMethod(m_mediaRecorder.object(), methodId, source);

    if (!env.checkAndClearExceptions())
        m_isVideoSourceSet = true;
}

void AndroidMediaRecorder::setOrientationHint(int degrees)
{
    m_mediaRecorder.callMethod<void>("setOrientationHint", "(I)V", degrees);
}

void AndroidMediaRecorder::setOutputFormat(OutputFormat format)
{
    QJniEnvironment env;
    auto methodId = env->GetMethodID(m_mediaRecorder.objectClass(), "setOutputFormat", "(I)V");
    env->CallVoidMethod(m_mediaRecorder.object(), methodId, format);
    // setAudioSource cannot be set after outputFormat is set.
    if (!env.checkAndClearExceptions())
        m_isAudioSourceSet = true;
}

void AndroidMediaRecorder::setOutputFile(const QString &path)
{
    if (QUrl(path).scheme() == QLatin1String("content")) {
        const QJniObject fileDescriptor = QJniObject::callStaticObjectMethod(
                    "org/qtproject/qt/android/QtNative",
                    "openFdObjectForContentUrl",
                    "(Landroid/content/Context;Ljava/lang/String;Ljava/lang/String;)Ljava/io/FileDescriptor;",
                    QNativeInterface::QAndroidApplication::context().object(),
                    QJniObject::fromString(path).object(),
                    QJniObject::fromString(QLatin1String("rw")).object());

        m_mediaRecorder.callMethod<void>("setOutputFile",
                                         "(Ljava/io/FileDescriptor;)V",
                                         fileDescriptor.object());
    } else {
        m_mediaRecorder.callMethod<void>("setOutputFile",
                                         "(Ljava/lang/String;)V",
                                         QJniObject::fromString(path).object());
    }
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
    static const JNINativeMethod methods[] = {
        {"notifyError", "(JII)V", (void *)notifyError},
        {"notifyInfo", "(JII)V", (void *)notifyInfo}
    };

    const int size = std::size(methods);
    return QJniEnvironment().registerNativeMethods(QtMediaRecorderListenerClassName, methods, size);
}

QT_END_NAMESPACE

#include "moc_androidmediarecorder_p.cpp"
