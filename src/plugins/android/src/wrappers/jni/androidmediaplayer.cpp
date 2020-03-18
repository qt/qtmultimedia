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

#include "androidmediaplayer.h"

#include <QString>
#include <QtCore/private/qjni_p.h>
#include <QtCore/private/qjnihelpers_p.h>
#include "androidsurfacetexture.h"
#include <QVector>
#include <QReadWriteLock>

static const char QtAndroidMediaPlayerClassName[] = "org/qtproject/qt5/android/multimedia/QtAndroidMediaPlayer";
typedef QVector<AndroidMediaPlayer *> MediaPlayerList;
Q_GLOBAL_STATIC(MediaPlayerList, mediaPlayers)
Q_GLOBAL_STATIC(QReadWriteLock, rwLock)

QT_BEGIN_NAMESPACE

AndroidMediaPlayer::AndroidMediaPlayer()
    : QObject()
{
    QWriteLocker locker(rwLock);
    auto context = QtAndroidPrivate::activity() ? QtAndroidPrivate::activity() : QtAndroidPrivate::service();
    const jlong id = reinterpret_cast<jlong>(this);
    mMediaPlayer = QJNIObjectPrivate(QtAndroidMediaPlayerClassName,
                                     "(Landroid/content/Context;J)V",
                                     context,
                                     id);
    mediaPlayers->append(this);
}

AndroidMediaPlayer::~AndroidMediaPlayer()
{
    QWriteLocker locker(rwLock);
    const int i = mediaPlayers->indexOf(this);
    Q_ASSERT(i != -1);
    mediaPlayers->remove(i);
}

void AndroidMediaPlayer::release()
{
    mMediaPlayer.callMethod<void>("release");
}

void AndroidMediaPlayer::reset()
{
    mMediaPlayer.callMethod<void>("reset");
}

int AndroidMediaPlayer::getCurrentPosition()
{
    return mMediaPlayer.callMethod<jint>("getCurrentPosition");
}

int AndroidMediaPlayer::getDuration()
{
    return mMediaPlayer.callMethod<jint>("getDuration");
}

bool AndroidMediaPlayer::isPlaying()
{
    return mMediaPlayer.callMethod<jboolean>("isPlaying");
}

int AndroidMediaPlayer::volume()
{
    return mMediaPlayer.callMethod<jint>("getVolume");
}

bool AndroidMediaPlayer::isMuted()
{
    return mMediaPlayer.callMethod<jboolean>("isMuted");
}

qreal AndroidMediaPlayer::playbackRate()
{
    qreal rate(1.0);

    if (QtAndroidPrivate::androidSdkVersion() < 23)
        return rate;

    QJNIObjectPrivate player = mMediaPlayer.callObjectMethod("getMediaPlayerHandle", "()Landroid/media/MediaPlayer;");
    if (player.isValid()) {
        QJNIObjectPrivate playbackParams = player.callObjectMethod("getPlaybackParams", "()Landroid/media/PlaybackParams;");
        if (playbackParams.isValid()) {
            const qreal speed = playbackParams.callMethod<jfloat>("getSpeed", "()F");
            QJNIEnvironmentPrivate env;
            if (env->ExceptionCheck()) {
#ifdef QT_DEBUG
                env->ExceptionDescribe();
#endif // QT_DEBUG
                env->ExceptionClear();
            } else {
                rate = speed;
            }
        }
    }

    return rate;
}

jobject AndroidMediaPlayer::display()
{
    return mMediaPlayer.callObjectMethod("display", "()Landroid/view/SurfaceHolder;").object();
}

void AndroidMediaPlayer::play()
{
    mMediaPlayer.callMethod<void>("start");
}

void AndroidMediaPlayer::pause()
{
    mMediaPlayer.callMethod<void>("pause");
}

void AndroidMediaPlayer::stop()
{
    mMediaPlayer.callMethod<void>("stop");
}

void AndroidMediaPlayer::seekTo(qint32 msec)
{
    mMediaPlayer.callMethod<void>("seekTo", "(I)V", jint(msec));
}

void AndroidMediaPlayer::setMuted(bool mute)
{
    mMediaPlayer.callMethod<void>("mute", "(Z)V", jboolean(mute));
}

void AndroidMediaPlayer::setDataSource(const QNetworkRequest &request)
{
    QJNIObjectPrivate string = QJNIObjectPrivate::fromString(request.url().toString(QUrl::FullyEncoded));

    mMediaPlayer.callMethod<void>("initHeaders", "()V");
    for (auto &header : request.rawHeaderList()) {
        auto value = request.rawHeader(header);
        mMediaPlayer.callMethod<void>("setHeader", "(Ljava/lang/String;Ljava/lang/String;)V",
            QJNIObjectPrivate::fromString(header).object(),  QJNIObjectPrivate::fromString(value).object());
    }

    mMediaPlayer.callMethod<void>("setDataSource", "(Ljava/lang/String;)V", string.object());
}

void AndroidMediaPlayer::prepareAsync()
{
    mMediaPlayer.callMethod<void>("prepareAsync");
}

void AndroidMediaPlayer::setVolume(int volume)
{
    mMediaPlayer.callMethod<void>("setVolume", "(I)V", jint(volume));
}

bool AndroidMediaPlayer::setPlaybackRate(qreal rate)
{
    if (QtAndroidPrivate::androidSdkVersion() < 23) {
        qWarning("Setting the playback rate on a media player requires Android 6.0 (API level 23) or later");
        return false;
    }

    QJNIEnvironmentPrivate env;

    QJNIObjectPrivate player = mMediaPlayer.callObjectMethod("getMediaPlayerHandle", "()Landroid/media/MediaPlayer;");
    if (player.isValid()) {
        QJNIObjectPrivate playbackParams = player.callObjectMethod("getPlaybackParams", "()Landroid/media/PlaybackParams;");
        if (playbackParams.isValid()) {
            playbackParams.callObjectMethod("setSpeed", "(F)Landroid/media/PlaybackParams;", jfloat(rate));
            // pitch can only be > 0
            if (!qFuzzyIsNull(rate))
                playbackParams.callObjectMethod("setPitch", "(F)Landroid/media/PlaybackParams;", jfloat(qAbs(rate)));
            player.callMethod<void>("setPlaybackParams", "(Landroid/media/PlaybackParams;)V", playbackParams.object());
            if (Q_UNLIKELY(env->ExceptionCheck())) {
#ifdef QT_DEBUG
                env->ExceptionDescribe();
#endif // QT_DEBUG
                env->ExceptionClear();
                qWarning() << "Invalid playback rate" << rate;
                return false;
            } else {
                return true;
            }
        }
    }

    return false;
}

void AndroidMediaPlayer::setDisplay(AndroidSurfaceTexture *surfaceTexture)
{
    mMediaPlayer.callMethod<void>("setDisplay",
                                  "(Landroid/view/SurfaceHolder;)V",
                                  surfaceTexture ? surfaceTexture->surfaceHolder() : 0);
}

void AndroidMediaPlayer::setAudioRole(QAudio::Role role)
{
    QString str;
    switch (role) {
    case QAudio::MusicRole:
        str = QLatin1String("CONTENT_TYPE_MUSIC");
        break;
    case QAudio::VideoRole:
        str = QLatin1String("CONTENT_TYPE_MOVIE");
        break;
    case QAudio::VoiceCommunicationRole:
        str = QLatin1String("USAGE_VOICE_COMMUNICATION");
        break;
    case QAudio::AlarmRole:
        str = QLatin1String("USAGE_ALARM");
        break;
    case QAudio::NotificationRole:
        str = QLatin1String("USAGE_NOTIFICATION");
        break;
    case QAudio::RingtoneRole:
        str = QLatin1String("USAGE_NOTIFICATION_RINGTONE");
        break;
    case QAudio::AccessibilityRole:
        str = QLatin1String("USAGE_ASSISTANCE_ACCESSIBILITY");
        break;
    case QAudio::SonificationRole:
        str = QLatin1String("CONTENT_TYPE_SONIFICATION");
        break;
    case QAudio::GameRole:
        str = QLatin1String("USAGE_GAME");
        break;
    default:
        break;
    }

    setCustomAudioRole(str);
}

void AndroidMediaPlayer::setCustomAudioRole(const QString &role)
{
    QStringList roles = role.split(",", Qt::SkipEmptyParts);

    int type = 0; // CONTENT_TYPE_UNKNOWN
    int usage = 0; // USAGE_UNKNOWN
    for (int i = 0; i < qMin(2, roles.size()); ++i) {
        auto r = roles[i];
        if (r == QLatin1String("CONTENT_TYPE_MOVIE"))
            type = 3;
        else if (r == QLatin1String("CONTENT_TYPE_MUSIC"))
            type = 2;
        else if (r == QLatin1String("CONTENT_TYPE_SONIFICATION"))
            type = 4;
        else if (r == QLatin1String("CONTENT_TYPE_SPEECH"))
            type = 1;
        else if (r == QLatin1String("USAGE_ALARM"))
            usage = 4;
        else if (r == QLatin1String("USAGE_ASSISTANCE_ACCESSIBILITY"))
            usage = 11;
        else if (r == QLatin1String("USAGE_ASSISTANCE_NAVIGATION_GUIDANCE"))
            usage = 12;
        else if (r == QLatin1String("USAGE_ASSISTANCE_SONIFICATION"))
            usage = 13;
        else if (r == QLatin1String("USAGE_ASSISTANT"))
            usage = 16;
        else if (r == QLatin1String("USAGE_GAME"))
            usage = 14;
        else if (r == QLatin1String("USAGE_MEDIA"))
            usage = 1;
        else if (r == QLatin1String("USAGE_NOTIFICATION"))
            usage = 5;
        else if (r == QLatin1String("USAGE_NOTIFICATION_COMMUNICATION_DELAYED"))
            usage = 9;
        else if (r == QLatin1String("USAGE_NOTIFICATION_COMMUNICATION_INSTANT"))
            usage = 8;
        else if (r == QLatin1String("USAGE_NOTIFICATION_COMMUNICATION_REQUEST"))
            usage = 7;
        else if (r == QLatin1String("USAGE_NOTIFICATION_EVENT"))
            usage = 10;
        else if (r == QLatin1String("USAGE_NOTIFICATION_RINGTONE"))
            usage = 6;
        else if (r == QLatin1String("USAGE_VOICE_COMMUNICATION"))
            usage = 2;
        else if (r == QLatin1String("USAGE_VOICE_COMMUNICATION_SIGNALLING"))
            usage = 3;
    }

    mMediaPlayer.callMethod<void>("setAudioAttributes", "(II)V", jint(type), jint(usage));
}

static void onErrorNative(JNIEnv *env, jobject thiz, jint what, jint extra, jlong id)
{
    Q_UNUSED(env);
    Q_UNUSED(thiz);
    QReadLocker locker(rwLock);
    const int i = mediaPlayers->indexOf(reinterpret_cast<AndroidMediaPlayer *>(id));
    if (Q_UNLIKELY(i == -1))
        return;

    Q_EMIT (*mediaPlayers)[i]->error(what, extra);
}

static void onBufferingUpdateNative(JNIEnv *env, jobject thiz, jint percent, jlong id)
{
    Q_UNUSED(env);
    Q_UNUSED(thiz);
    QReadLocker locker(rwLock);
    const int i = mediaPlayers->indexOf(reinterpret_cast<AndroidMediaPlayer *>(id));
    if (Q_UNLIKELY(i == -1))
        return;

    Q_EMIT (*mediaPlayers)[i]->bufferingChanged(percent);
}

static void onProgressUpdateNative(JNIEnv *env, jobject thiz, jint progress, jlong id)
{
    Q_UNUSED(env);
    Q_UNUSED(thiz);
    QReadLocker locker(rwLock);
    const int i = mediaPlayers->indexOf(reinterpret_cast<AndroidMediaPlayer *>(id));
    if (Q_UNLIKELY(i == -1))
        return;

    Q_EMIT (*mediaPlayers)[i]->progressChanged(progress);
}

static void onDurationChangedNative(JNIEnv *env, jobject thiz, jint duration, jlong id)
{
    Q_UNUSED(env);
    Q_UNUSED(thiz);
    QReadLocker locker(rwLock);
    const int i = mediaPlayers->indexOf(reinterpret_cast<AndroidMediaPlayer *>(id));
    if (Q_UNLIKELY(i == -1))
        return;

    Q_EMIT (*mediaPlayers)[i]->durationChanged(duration);
}

static void onInfoNative(JNIEnv *env, jobject thiz, jint what, jint extra, jlong id)
{
    Q_UNUSED(env);
    Q_UNUSED(thiz);
    QReadLocker locker(rwLock);
    const int i = mediaPlayers->indexOf(reinterpret_cast<AndroidMediaPlayer *>(id));
    if (Q_UNLIKELY(i == -1))
        return;

    Q_EMIT (*mediaPlayers)[i]->info(what, extra);
}

static void onStateChangedNative(JNIEnv *env, jobject thiz, jint state, jlong id)
{
    Q_UNUSED(env);
    Q_UNUSED(thiz);
    QReadLocker locker(rwLock);
    const int i = mediaPlayers->indexOf(reinterpret_cast<AndroidMediaPlayer *>(id));
    if (Q_UNLIKELY(i == -1))
        return;

    Q_EMIT (*mediaPlayers)[i]->stateChanged(state);
}

static void onVideoSizeChangedNative(JNIEnv *env,
                                     jobject thiz,
                                     jint width,
                                     jint height,
                                     jlong id)
{
    Q_UNUSED(env);
    Q_UNUSED(thiz);
    QReadLocker locker(rwLock);
    const int i = mediaPlayers->indexOf(reinterpret_cast<AndroidMediaPlayer *>(id));
    if (Q_UNLIKELY(i == -1))
        return;

    Q_EMIT (*mediaPlayers)[i]->videoSizeChanged(width, height);
}

bool AndroidMediaPlayer::initJNI(JNIEnv *env)
{
    jclass clazz = QJNIEnvironmentPrivate::findClass(QtAndroidMediaPlayerClassName,
                                                     env);

    static const JNINativeMethod methods[] = {
        {"onErrorNative", "(IIJ)V", reinterpret_cast<void *>(onErrorNative)},
        {"onBufferingUpdateNative", "(IJ)V", reinterpret_cast<void *>(onBufferingUpdateNative)},
        {"onProgressUpdateNative", "(IJ)V", reinterpret_cast<void *>(onProgressUpdateNative)},
        {"onDurationChangedNative", "(IJ)V", reinterpret_cast<void *>(onDurationChangedNative)},
        {"onInfoNative", "(IIJ)V", reinterpret_cast<void *>(onInfoNative)},
        {"onVideoSizeChangedNative", "(IIJ)V", reinterpret_cast<void *>(onVideoSizeChangedNative)},
        {"onStateChangedNative", "(IJ)V", reinterpret_cast<void *>(onStateChangedNative)}
    };

    if (clazz && env->RegisterNatives(clazz,
                                      methods,
                                      sizeof(methods) / sizeof(methods[0])) != JNI_OK) {
            return false;
    }

    return true;
}

QT_END_NAMESPACE
