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
#include <QtCore/private/qjni_p.h>
#include <QtCore/private/qjnihelpers_p.h>
#include <QMap>

static jclass mediaPlayerClass = Q_NULLPTR;
typedef QMap<jlong, JMediaPlayer *> MediaPlayerMap;
Q_GLOBAL_STATIC(MediaPlayerMap, mediaPlayers)

QT_BEGIN_NAMESPACE

JMediaPlayer::JMediaPlayer()
    : QObject()
{

    const jlong id = reinterpret_cast<jlong>(this);
    mMediaPlayer = QJNIObjectPrivate(mediaPlayerClass,
                                      "(Landroid/app/Activity;J)V",
                                      QtAndroidPrivate::activity(),
                                      id);
    (*mediaPlayers)[id] = this;
}

JMediaPlayer::~JMediaPlayer()
{
    mediaPlayers->remove(reinterpret_cast<jlong>(this));
}

void JMediaPlayer::release()
{
    mMediaPlayer.callMethod<void>("release");
}

void JMediaPlayer::reset()
{
    mMediaPlayer.callMethod<void>("reset");
}

int JMediaPlayer::getCurrentPosition()
{
    return mMediaPlayer.callMethod<jint>("getCurrentPosition");
}

int JMediaPlayer::getDuration()
{
    return mMediaPlayer.callMethod<jint>("getDuration");
}

bool JMediaPlayer::isPlaying()
{
    return mMediaPlayer.callMethod<jboolean>("isPlaying");
}

int JMediaPlayer::volume()
{
    return mMediaPlayer.callMethod<jint>("getVolume");
}

bool JMediaPlayer::isMuted()
{
    return mMediaPlayer.callMethod<jboolean>("isMuted");
}

jobject JMediaPlayer::display()
{
    return mMediaPlayer.callObjectMethod("display", "()Landroid/view/SurfaceHolder;").object();
}

void JMediaPlayer::play()
{
    mMediaPlayer.callMethod<void>("start");
}

void JMediaPlayer::pause()
{
    mMediaPlayer.callMethod<void>("pause");
}

void JMediaPlayer::stop()
{
    mMediaPlayer.callMethod<void>("stop");
}

void JMediaPlayer::seekTo(qint32 msec)
{
    mMediaPlayer.callMethod<void>("seekTo", "(I)V", jint(msec));
}

void JMediaPlayer::setMuted(bool mute)
{
    mMediaPlayer.callMethod<void>("mute", "(Z)V", jboolean(mute));
}

void JMediaPlayer::setDataSource(const QString &path)
{
    QJNIObjectPrivate string = QJNIObjectPrivate::fromString(path);
    mMediaPlayer.callMethod<void>("setDataSource", "(Ljava/lang/String;)V", string.object());
}

void JMediaPlayer::prepareAsync()
{
    mMediaPlayer.callMethod<void>("prepareAsync");
}

void JMediaPlayer::setVolume(int volume)
{
    mMediaPlayer.callMethod<void>("setVolume", "(I)V", jint(volume));
}

void JMediaPlayer::setDisplay(jobject surfaceHolder)
{
    mMediaPlayer.callMethod<void>("setDisplay", "(Landroid/view/SurfaceHolder;)V", surfaceHolder);
}

QT_END_NAMESPACE

static void onErrorNative(JNIEnv *env, jobject thiz, jint what, jint extra, jlong id)
{
    Q_UNUSED(env);
    Q_UNUSED(thiz);
    JMediaPlayer *const mp = (*mediaPlayers)[id];
    if (!mp)
        return;

    Q_EMIT mp->error(what, extra);
}

static void onBufferingUpdateNative(JNIEnv *env, jobject thiz, jint percent, jlong id)
{
    Q_UNUSED(env);
    Q_UNUSED(thiz);
    JMediaPlayer *const mp = (*mediaPlayers)[id];
    if (!mp)
        return;

    Q_EMIT mp->bufferingChanged(percent);
}

static void onProgressUpdateNative(JNIEnv *env, jobject thiz, jint progress, jlong id)
{
    Q_UNUSED(env);
    Q_UNUSED(thiz);
    JMediaPlayer *const mp = (*mediaPlayers)[id];
    if (!mp)
        return;

    Q_EMIT mp->progressChanged(progress);
}

static void onDurationChangedNative(JNIEnv *env, jobject thiz, jint duration, jlong id)
{
    Q_UNUSED(env);
    Q_UNUSED(thiz);
    JMediaPlayer *const mp = (*mediaPlayers)[id];
    if (!mp)
        return;

    Q_EMIT mp->durationChanged(duration);
}

static void onInfoNative(JNIEnv *env, jobject thiz, jint what, jint extra, jlong id)
{
    Q_UNUSED(env);
    Q_UNUSED(thiz);
    JMediaPlayer *const mp = (*mediaPlayers)[id];
    if (!mp)
        return;

    Q_EMIT mp->info(what, extra);
}

static void onStateChangedNative(JNIEnv *env, jobject thiz, jint state, jlong id)
{
    Q_UNUSED(env);
    Q_UNUSED(thiz);
    JMediaPlayer *const mp = (*mediaPlayers)[id];
    if (!mp)
        return;

    Q_EMIT mp->stateChanged(state);
}

static void onVideoSizeChangedNative(JNIEnv *env,
                                     jobject thiz,
                                     jint width,
                                     jint height,
                                     jlong id)
{
    Q_UNUSED(env);
    Q_UNUSED(thiz);
    JMediaPlayer *const mp = (*mediaPlayers)[id];
    if (!mp)
        return;

    Q_EMIT mp->videoSizeChanged(width, height);
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
            {"onProgressUpdateNative", "(IJ)V", reinterpret_cast<void *>(onProgressUpdateNative)},
            {"onDurationChangedNative", "(IJ)V", reinterpret_cast<void *>(onDurationChangedNative)},
            {"onInfoNative", "(IIJ)V", reinterpret_cast<void *>(onInfoNative)},
            {"onVideoSizeChangedNative", "(IIJ)V", reinterpret_cast<void *>(onVideoSizeChangedNative)},
            {"onStateChangedNative", "(IJ)V", reinterpret_cast<void *>(onStateChangedNative)}
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
