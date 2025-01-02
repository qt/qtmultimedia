// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "androidmediaplayer_p.h"
#include "androidsurfacetexture_p.h"

#include <QList>
#include <QReadWriteLock>
#include <QString>
#include <QtCore/qcoreapplication.h>
#include <qloggingcategory.h>

static const char QtAndroidMediaPlayerClassName[] = "org/qtproject/qt/android/multimedia/QtAndroidMediaPlayer";
typedef QList<AndroidMediaPlayer *> MediaPlayerList;
Q_GLOBAL_STATIC(MediaPlayerList, mediaPlayers)
Q_GLOBAL_STATIC(QReadWriteLock, rwLock)

QT_BEGIN_NAMESPACE

static Q_LOGGING_CATEGORY(lcAudio, "qt.multimedia.audio")

AndroidMediaPlayer::AndroidMediaPlayer()
    : QObject()
{
    QWriteLocker locker(rwLock);
    auto context = QNativeInterface::QAndroidApplication::context();
    const jlong id = reinterpret_cast<jlong>(this);
    mMediaPlayer = QJniObject(QtAndroidMediaPlayerClassName,
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

    if (QNativeInterface::QAndroidApplication::sdkVersion() < 23)
        return rate;

    QJniObject player = mMediaPlayer.callObjectMethod("getMediaPlayerHandle",
                                                      "()Landroid/media/MediaPlayer;");
    if (player.isValid()) {
        QJniObject playbackParams = player.callObjectMethod("getPlaybackParams",
                                                            "()Landroid/media/PlaybackParams;");
        if (playbackParams.isValid()) {
            QJniEnvironment env;
            auto methodId = env->GetMethodID(playbackParams.objectClass(), "getSpeed", "()F");
            const qreal speed = env->CallFloatMethod(playbackParams.object(), methodId);
            if (!env.checkAndClearExceptions())
                rate = speed;
        }
    }

    return rate;
}

jobject AndroidMediaPlayer::display()
{
    return mMediaPlayer.callObjectMethod("display", "()Landroid/view/SurfaceHolder;").object();
}

AndroidMediaPlayer::TrackInfo convertTrackInfo(int streamNumber, QJniObject androidTrackInfo)
{
    const QLatin1String unknownMimeType("application/octet-stream");
    const QLatin1String undefinedLanguage("und");

    if (!androidTrackInfo.isValid())
        return { streamNumber, AndroidMediaPlayer::TrackType::Unknown, undefinedLanguage,
                 unknownMimeType };

    QJniEnvironment env;
    auto methodId = env->GetMethodID(androidTrackInfo.objectClass(), "getType", "()I");
    const jint type = env->CallIntMethod(androidTrackInfo.object(), methodId);
    if (env.checkAndClearExceptions())
        return { streamNumber, AndroidMediaPlayer::TrackType::Unknown, undefinedLanguage,
                 unknownMimeType };

    if (type < 0 || type > 5) {
        return { streamNumber, AndroidMediaPlayer::TrackType::Unknown, undefinedLanguage,
                 unknownMimeType };
    }

    AndroidMediaPlayer::TrackType trackType = static_cast<AndroidMediaPlayer::TrackType>(type);

    auto languageObject = androidTrackInfo.callObjectMethod("getLanguage", "()Ljava/lang/String;");
    QString language = languageObject.isValid() ? languageObject.toString() : undefinedLanguage;

    auto mimeTypeObject = androidTrackInfo.callObjectMethod("getMime", "()Ljava/lang/String;");
    QString mimeType = mimeTypeObject.isValid() ? mimeTypeObject.toString() : unknownMimeType;

    return { streamNumber, trackType, language, mimeType };
}

QList<AndroidMediaPlayer::TrackInfo> AndroidMediaPlayer::tracksInfo()
{
    auto androidTracksInfoObject = mMediaPlayer.callObjectMethod(
            "getAllTrackInfo",
            "()[Lorg/qtproject/qt/android/multimedia/QtAndroidMediaPlayer$TrackInfo;");

    if (!androidTracksInfoObject.isValid())
        return QList<AndroidMediaPlayer::TrackInfo>();

    auto androidTracksInfo = androidTracksInfoObject.object<jobjectArray>();
    if (!androidTracksInfo)
        return QList<AndroidMediaPlayer::TrackInfo>();

    QJniEnvironment environment;
    auto numberofTracks = environment->GetArrayLength(androidTracksInfo);

    QList<AndroidMediaPlayer::TrackInfo> tracksInformation;

    for (int index = 0; index < numberofTracks; index++) {
        auto androidTrackInformation = environment->GetObjectArrayElement(androidTracksInfo, index);

        if (environment.checkAndClearExceptions()) {
            continue;
        }

        auto trackInfo = convertTrackInfo(index, androidTrackInformation);
        tracksInformation.insert(index, trackInfo);

        environment->DeleteLocalRef(androidTrackInformation);
    }

    return tracksInformation;
}

int AndroidMediaPlayer::activeTrack(TrackType androidTrackType)
{
    int type = static_cast<int>(androidTrackType);
    return mMediaPlayer.callMethod<jint>("getSelectedTrack", "(I)I", type);
}

void AndroidMediaPlayer::deselectTrack(int trackNumber)
{
    mMediaPlayer.callMethod<void>("deselectTrack", "(I)V", trackNumber);
}

void AndroidMediaPlayer::selectTrack(int trackNumber)
{
    mMediaPlayer.callMethod<void>("selectTrack", "(I)V", trackNumber);
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
    if (mAudioBlocked)
        return;

    mMediaPlayer.callMethod<void>("mute", "(Z)V", jboolean(mute));
}

void AndroidMediaPlayer::setDataSource(const QNetworkRequest &request)
{
    QJniObject string = QJniObject::fromString(request.url().toString(QUrl::FullyEncoded));

    mMediaPlayer.callMethod<void>("initHeaders", "()V");
    for (auto &header : request.rawHeaderList()) {
        auto value = request.rawHeader(header);
        mMediaPlayer.callMethod<void>("setHeader", "(Ljava/lang/String;Ljava/lang/String;)V",
            QJniObject::fromString(QLatin1String(header)).object(),
                                      QJniObject::fromString(QLatin1String(value)).object());
    }

    mMediaPlayer.callMethod<void>("setDataSource", "(Ljava/lang/String;)V", string.object());
}

void AndroidMediaPlayer::prepareAsync()
{
    mMediaPlayer.callMethod<void>("prepareAsync");
}

void AndroidMediaPlayer::setVolume(int volume)
{
    if (mAudioBlocked)
        return;

    mMediaPlayer.callMethod<void>("setVolume", "(I)V", jint(volume));
}

void AndroidMediaPlayer::blockAudio()
{
    mAudioBlocked = true;
}

void AndroidMediaPlayer::unblockAudio()
{
    mAudioBlocked = false;
}

void AndroidMediaPlayer::startSoundStreaming(const int inputId, const int outputId)
{
    QJniObject::callStaticMethod<void>("org/qtproject/qt/android/multimedia/QtAudioDeviceManager",
                                       "startSoundStreaming",
                                       inputId,
                                       outputId);
}

void AndroidMediaPlayer::stopSoundStreaming()
{
    QJniObject::callStaticMethod<void>(
        "org/qtproject/qt/android/multimedia/QtAudioDeviceManager", "stopSoundStreaming");
}

bool AndroidMediaPlayer::setPlaybackRate(qreal rate)
{
    if (QNativeInterface::QAndroidApplication::sdkVersion() < 23) {
        qWarning() << "Setting the playback rate on a media player requires"
                   << "Android 6.0 (API level 23) or later";
        return false;
    }

    return mMediaPlayer.callMethod<jboolean>("setPlaybackRate", jfloat(rate));
}

void AndroidMediaPlayer::setDisplay(AndroidSurfaceTexture *surfaceTexture)
{
    mMediaPlayer.callMethod<void>("setDisplay",
                                  "(Landroid/view/SurfaceHolder;)V",
                                  surfaceTexture ? surfaceTexture->surfaceHolder() : 0);
}

bool AndroidMediaPlayer::setAudioOutput(const QByteArray &deviceId)
{
    const bool ret = QJniObject::callStaticMethod<jboolean>(
                                    "org/qtproject/qt/android/multimedia/QtAudioDeviceManager",
                                    "setAudioOutput",
                                    "(I)Z",
                                    deviceId.toInt());

    if (!ret)
        qCWarning(lcAudio) << "Output device not set";

    return ret;
}

#if 0
void AndroidMediaPlayer::setAudioRole(QAudio::Role role)
{
    QString r;
    switch (role) {
    case QAudio::MusicRole:
        r = QLatin1String("CONTENT_TYPE_MUSIC");
        break;
    case QAudio::VideoRole:
        r = QLatin1String("CONTENT_TYPE_MOVIE");
        break;
    case QAudio::VoiceCommunicationRole:
        r = QLatin1String("USAGE_VOICE_COMMUNICATION");
        break;
    case QAudio::AlarmRole:
        r = QLatin1String("USAGE_ALARM");
        break;
    case QAudio::NotificationRole:
        r = QLatin1String("USAGE_NOTIFICATION");
        break;
    case QAudio::RingtoneRole:
        r = QLatin1String("USAGE_NOTIFICATION_RINGTONE");
        break;
    case QAudio::AccessibilityRole:
        r = QLatin1String("USAGE_ASSISTANCE_ACCESSIBILITY");
        break;
    case QAudio::SonificationRole:
        r = QLatin1String("CONTENT_TYPE_SONIFICATION");
        break;
    case QAudio::GameRole:
        r = QLatin1String("USAGE_GAME");
        break;
    default:
        return;
    }

    int type = 0; // CONTENT_TYPE_UNKNOWN
    int usage = 0; // USAGE_UNKNOWN

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

    mMediaPlayer.callMethod<void>("setAudioAttributes", "(II)V", jint(type), jint(usage));
}
#endif

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

static AndroidMediaPlayer *getMediaPlayer(jlong ptr)
{
    auto mediaplayer = reinterpret_cast<AndroidMediaPlayer *>(ptr);
    if (!mediaplayer || !mediaPlayers->contains(mediaplayer))
        return nullptr;

    return mediaplayer;
}

static void onTrackInfoChangedNative(JNIEnv *env, jobject thiz, jlong ptr)
{
    Q_UNUSED(env);
    Q_UNUSED(thiz);

    QReadLocker locker(rwLock);
    auto mediaplayer = getMediaPlayer(ptr);
    if (!mediaplayer)
        return;

    emit mediaplayer->tracksInfoChanged();
}

static void onTimedTextChangedNative(JNIEnv *env, jobject thiz, jstring timedText, jint time,
                                     jlong ptr)
{
    Q_UNUSED(env);
    Q_UNUSED(thiz);
    Q_UNUSED(time);

    QReadLocker locker(rwLock);

    auto mediaplayer = getMediaPlayer(ptr);
    if (!mediaplayer)
        return;

    QString subtitleText;
    if (timedText != nullptr)
        subtitleText = QString::fromUtf8(env->GetStringUTFChars(timedText, 0));

    emit mediaplayer->timedTextChanged(subtitleText);
}

bool AndroidMediaPlayer::registerNativeMethods()
{
    static const JNINativeMethod methods[] = {
        { "onErrorNative", "(IIJ)V", reinterpret_cast<void *>(onErrorNative) },
        { "onBufferingUpdateNative", "(IJ)V", reinterpret_cast<void *>(onBufferingUpdateNative) },
        { "onProgressUpdateNative", "(IJ)V", reinterpret_cast<void *>(onProgressUpdateNative) },
        { "onDurationChangedNative", "(IJ)V", reinterpret_cast<void *>(onDurationChangedNative) },
        { "onInfoNative", "(IIJ)V", reinterpret_cast<void *>(onInfoNative) },
        { "onVideoSizeChangedNative", "(IIJ)V",
          reinterpret_cast<void *>(onVideoSizeChangedNative) },
        { "onStateChangedNative", "(IJ)V", reinterpret_cast<void *>(onStateChangedNative) },
        { "onTrackInfoChangedNative", "(J)V", reinterpret_cast<void *>(onTrackInfoChangedNative) },
        { "onTimedTextChangedNative", "(Ljava/lang/String;IJ)V",
          reinterpret_cast<void *>(onTimedTextChangedNative) }
    };

    const int size = std::size(methods);
    return QJniEnvironment().registerNativeMethods(QtAndroidMediaPlayerClassName, methods, size);
}

QT_END_NAMESPACE

#include "moc_androidmediaplayer_p.cpp"
