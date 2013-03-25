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

#include "jmediaplayer.h"

#include <QString>
#include <qpa/qplatformnativeinterface.h>
#include <qguiapplication.h>
#include <QtPlatformSupport/private/qjnihelpers_p.h>

namespace {

jclass mediaPlayerClass = 0;

QMap<jlong, JMediaPlayer *> mplayers;

}

QT_BEGIN_NAMESPACE

bool JMediaPlayer::mActivitySet = false;

JMediaPlayer::JMediaPlayer()
    : QObject()
    , QJNIObject(mediaPlayerClass, "(J)V", reinterpret_cast<jlong>(this))
    , mId(reinterpret_cast<jlong>(this))
    , mDisplay(0)
{
    mplayers.insert(mId, this);

    if (!mActivitySet) {
        QPlatformNativeInterface *nativeInterface = QGuiApplication::platformNativeInterface();
        jobject activity = static_cast<jobject>(nativeInterface->nativeResourceForIntegration("QtActivity"));
        QJNIObject::callStaticMethod<void>(mediaPlayerClass,
                                           "setActivity",
                                           "(Landroid/app/Activity;)V",
                                           activity);
        mActivitySet = true;
    }
}

JMediaPlayer::~JMediaPlayer()
{
    mplayers.remove(mId);
}

void JMediaPlayer::release()
{
    callMethod<void>("release");
}

void JMediaPlayer::onError(qint32 what, qint32 extra)
{
    Q_EMIT error(what, extra);
}

void JMediaPlayer::onBufferingUpdate(qint32 percent)
{
    Q_EMIT bufferingUpdate(percent);
}

void JMediaPlayer::onInfo(qint32 what, qint32 extra)
{
    Q_EMIT info(what, extra);
}

void JMediaPlayer::onMediaPlayerInfo(qint32 what, qint32 extra)
{
    Q_EMIT mediaPlayerInfo(what, extra);
}

void JMediaPlayer::onVideoSizeChanged(qint32 width, qint32 height)
{
    Q_EMIT videoSizeChanged(width, height);
}

int JMediaPlayer::getCurrentPosition()
{
    return callMethod<jint>("getCurrentPosition");
}

int JMediaPlayer::getDuration()
{
    return callMethod<jint>("getDuration");
}

bool JMediaPlayer::isPlaying()
{
    return callMethod<jboolean>("isPlaying");
}

int JMediaPlayer::volume()
{
    return callMethod<jint>("getVolume");
}

bool JMediaPlayer::isMuted()
{
    return callMethod<jboolean>("isMuted");
}

void JMediaPlayer::play()
{
    callMethod<void>("start");
}

void JMediaPlayer::pause()
{
    callMethod<void>("pause");
}

void JMediaPlayer::stop()
{
    callMethod<void>("stop");
}

void JMediaPlayer::seekTo(qint32 msec)
{
    callMethod<void>("seekTo", "(I)V", jint(msec));
}

void JMediaPlayer::setMuted(bool mute)
{
    callMethod<void>("mute", "(Z)V", jboolean(mute));
}

void JMediaPlayer::setDataSource(const QString &path)
{
    QJNILocalRef<jstring> string = qt_toJString(path);
    callMethod<void>("setMediaPath", "(Ljava/lang/String;)V", string.object());
}

void JMediaPlayer::setVolume(int volume)
{
    callMethod<void>("setVolume", "(I)V", jint(volume));
}

void JMediaPlayer::setDisplay(jobject surfaceHolder)
{
    mDisplay = surfaceHolder;
    callMethod<void>("setDisplay", "(Landroid/view/SurfaceHolder;)V", mDisplay);
}

QT_END_NAMESPACE

static void onErrorNative(JNIEnv *env, jobject thiz, jint what, jint extra, jlong id)
{
    Q_UNUSED(env);
    Q_UNUSED(thiz);
    JMediaPlayer *const mp = mplayers[id];
    if (!mp)
        return;

    mp->onError(what, extra);
}

static void onBufferingUpdateNative(JNIEnv *env, jobject thiz, jint percent, jlong id)
{
    Q_UNUSED(env);
    Q_UNUSED(thiz);
    JMediaPlayer *const mp = mplayers[id];
    if (!mp)
        return;

    mp->onBufferingUpdate(percent);
}

static void onInfoNative(JNIEnv *env, jobject thiz, jint what, jint extra, jlong id)
{
    Q_UNUSED(env);
    Q_UNUSED(thiz);
    JMediaPlayer *const mp = mplayers[id];
    if (!mp)
        return;

    mp->onInfo(what, extra);
}

static void onMediaPlayerInfoNative(JNIEnv *env, jobject thiz, jint what, jint extra, jlong id)
{
    Q_UNUSED(env);
    Q_UNUSED(thiz);
    JMediaPlayer *const mp = mplayers[id];
    if (!mp)
        return;

    mp->onMediaPlayerInfo(what, extra);
}

static void onVideoSizeChangedNative(JNIEnv *env,
                                     jobject thiz,
                                     jint width,
                                     jint height,
                                     jlong id)
{
    Q_UNUSED(env);
    Q_UNUSED(thiz);
    JMediaPlayer *const mp = mplayers[id];
    if (!mp)
        return;

    mp->onVideoSizeChanged(width, height);
}

QT_BEGIN_NAMESPACE

bool JMediaPlayer::initJNI(JNIEnv *env)
{
    jclass jClass = env->FindClass("org/qtproject/qt5/android/multimedia/QtAndroidMediaPlayer");

    if (jClass) {
        mediaPlayerClass = static_cast<jclass>(env->NewGlobalRef(jClass));

        JNINativeMethod methods[] = {
            {"onErrorNative", "(IIJ)V", reinterpret_cast<void *>(onErrorNative)},
            {"onBufferingUpdateNative", "(IJ)V", reinterpret_cast<void *>(onBufferingUpdateNative)},
            {"onInfoNative", "(IIJ)V", reinterpret_cast<void *>(onInfoNative)},
            {"onMediaPlayerInfoNative", "(IIJ)V", reinterpret_cast<void *>(onMediaPlayerInfoNative)},
            {"onVideoSizeChangedNative", "(IIJ)V", reinterpret_cast<void *>(onVideoSizeChangedNative)}
        };

        if (env->RegisterNatives(mediaPlayerClass,
                                 methods,
                                 sizeof(methods) / sizeof(methods[0])) < 0) {
            return false;
        }
    }

    return true;
}

QT_END_NAMESPACE
