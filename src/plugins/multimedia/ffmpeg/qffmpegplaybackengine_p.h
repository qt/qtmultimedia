// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QFFMPEGPLAYBACKENGINE_P_H
#define QFFMPEGPLAYBACKENGINE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

/* Playback engine design description.
 *
 *
 * PLAYBACK ENGINE OBJECTS
 *
 * - Playback engine manages 7 objects inside, each one works in a separate thread.
 * Each object inherits PlaybackEngineObject. The objects are:
 *    Demuxer
 *    Stream Decoders: audio, video, subtitles
 *    Renderers: audio, video, subtitles
 *
 *
 * THREADS:
 *
 * - By default, each object works in a separate thread. It's easy to reconfigure
 *   to using several objects in thread.
 * - New thread is allocated if a new object is created and the engine doesn't
 *   have free threads. If it does, the thread is to be reused.
 * - If all objects for some thread are deleted, the thread becomes free and the engine
 *   postpones its termination.
 *
 * OBJECTS WEAK CONNECTIVITY
 *
 * - The objects know nothing about others and about PlaybackEngine.
 *   For any interactions the objects use slots/signals.
 *
 * - PlaybackEngine knows the objects object and is able to create/delete them and
 *   call their public methods.
 *
 */

#include <QtFFmpegMediaPluginImpl/private/qffmpegplaybackenginedefs_p.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpegtimecontroller_p.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpegmediadataholder_p.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpegcodeccontext_p.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpegplaybackutils_p.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpegtime_p.h>

#include <QtCore/qpointer.h>

#include <unordered_map>

QT_BEGIN_NAMESPACE

class QAudioSink;
class QVideoSink;
class QAudioOutput;
class QAudioBufferOutput;
class QFFmpegMediaPlayer;

namespace QFFmpeg
{

class PlaybackEngine : public QObject
{
    Q_OBJECT
public:
    PlaybackEngine();

    ~PlaybackEngine() override;

    void setMedia(MediaDataHolder media);

    void setVideoSink(QVideoSink *sink);

    void setAudioSink(QAudioOutput *output);

    void setAudioSink(QPlatformAudioOutput *output);

    void setAudioBufferOutput(QAudioBufferOutput *output);

    void setState(QMediaPlayer::PlaybackState state);

    void play() {
        setState(QMediaPlayer::PlayingState);
    }
    void pause() {
        setState(QMediaPlayer::PausedState);
    }
    void stop() {
        setState(QMediaPlayer::StoppedState);
    }

    void seek(TrackPosition pos);

    void setLoops(int loopsCount);

    void setPlaybackRate(float rate);

    float playbackRate() const;

    void setActiveTrack(QPlatformMediaPlayer::TrackType type, int streamNumber);

    TrackPosition currentPosition(bool topPos = true) const;

    TrackDuration duration() const;

    bool isSeekable() const;

    const QList<MediaDataHolder::StreamInfo> &
    streamInfo(QPlatformMediaPlayer::TrackType trackType) const;

    const QMediaMetaData &metaData() const;

    int activeTrack(QPlatformMediaPlayer::TrackType type) const;

signals:
    void endOfStream();
    void errorOccured(int, const QString &);
    void loopChanged();
    void buffered();

protected: // objects managing
    struct ObjectDeleter
    {
        void operator()(PlaybackEngineObject *) const;

        PlaybackEngine *engine = nullptr;
    };

    template<typename T>
    using ObjectPtr = std::unique_ptr<T, ObjectDeleter>;

    using RendererPtr = ObjectPtr<Renderer>;
    using StreamPtr = ObjectPtr<StreamDecoder>;

    template<typename T, typename... Args>
    ObjectPtr<T> createPlaybackEngineObject(Args &&...args);

    virtual RendererPtr createRenderer(QPlatformMediaPlayer::TrackType trackType);

    template <typename AudioOutput>
    void updateActiveAudioOutput(AudioOutput *output);

    void updateActiveVideoOutput(QVideoSink *sink, bool cleanOutput = false);

private:
    void createStreamAndRenderer(QPlatformMediaPlayer::TrackType trackType);

    void createDemuxer();

    void registerObject(PlaybackEngineObject &object);

    template<typename C, typename Action>
    void forEachExistingObject(Action &&action);

    template<typename Action>
    void forEachExistingObject(Action &&action);

    void forceUpdate();

    void recreateObjects();

    void createObjectsIfNeeded();

    void updateObjectsPausedState();

    void deleteFreeThreads();

    void onFirsPacketFound(quint64 id, TrackPosition absSeekPos);

    void onRendererSynchronized(quint64 id, RealClock::time_point timePoint,
                                TrackPosition trackPosition);

    void onRendererFinished();

    void onRendererLoopChanged(quint64 id, TrackPosition offset, int loopIndex);

    void triggerStepIfNeeded();

    static QString objectThreadName(const PlaybackEngineObject &object);

    std::optional<CodecContext> codecContextForTrack(QPlatformMediaPlayer::TrackType trackType);

    bool hasMediaStream() const;

    void finilizeTime(TrackPosition pos);

    void finalizeOutputs();

    bool hasRenderer(quint64 id) const;

    void updateVideoSinkSize(QVideoSink *prevSink = nullptr);

    TrackPosition boundPosition(TrackPosition position) const;

private:
    MediaDataHolder m_media;

    TimeController m_timeController;

    std::unordered_map<QString, std::unique_ptr<QThread>> m_threads;
    bool m_threadsDirty = false;

    QPointer<QVideoSink> m_videoSink;
    QPointer<QAudioOutput> m_audioOutput;
    QPointer<QAudioBufferOutput> m_audioBufferOutput;

    QMediaPlayer::PlaybackState m_state = QMediaPlayer::StoppedState;

    ObjectPtr<Demuxer> m_demuxer;
    std::array<StreamPtr, QPlatformMediaPlayer::NTrackTypes> m_streams;
    std::array<RendererPtr, QPlatformMediaPlayer::NTrackTypes> m_renderers;

    bool m_shouldUpdateTimeOnFirstPacket = false;
    bool m_seekPending = false;

    std::array<std::optional<CodecContext>, QPlatformMediaPlayer::NTrackTypes> m_codecContexts;
    int m_loops = QMediaPlayer::Once;
    LoopOffset m_currentLoopOffset;
};

template<typename T, typename... Args>
PlaybackEngine::ObjectPtr<T> PlaybackEngine::createPlaybackEngineObject(Args &&...args)
{
    auto result = ObjectPtr<T>(new T(std::forward<Args>(args)...), { this });
    registerObject(*result);
    return result;
}
} // namespace QFFmpeg

QT_END_NAMESPACE

#endif // QFFMPEGPLAYBACKENGINE_P_H
