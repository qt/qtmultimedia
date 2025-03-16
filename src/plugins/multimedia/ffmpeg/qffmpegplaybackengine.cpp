// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qffmpegplaybackengine_p.h"

#include "qvideosink.h"
#include "qaudiooutput.h"
#include "private/qplatformaudiooutput_p.h"
#include "private/qplatformvideosink_p.h"
#include "private/qaudiobufferoutput_p.h"
#include "qiodevice.h"
#include "playbackengine/qffmpegdemuxer_p.h"
#include "playbackengine/qffmpegstreamdecoder_p.h"
#include "playbackengine/qffmpegsubtitlerenderer_p.h"
#include "playbackengine/qffmpegvideorenderer_p.h"
#include "playbackengine/qffmpegaudiorenderer_p.h"

#include <qloggingcategory.h>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

Q_STATIC_LOGGING_CATEGORY(qLcPlaybackEngine, "qt.multimedia.ffmpeg.playbackengine");

// The helper is needed since on some compilers std::unique_ptr
// doesn't have a default constructor in the case of sizeof(CustomDeleter) > 0
template <typename Array>
inline static Array defaultObjectsArray()
{
    using T = typename Array::value_type;
    return { T{ {}, {} }, T{ {}, {} }, T{ {}, {} } };
}

// TODO: investigate what's better: profile and try network case
// Most likely, shouldPauseStreams = false is better because of:
//     - packet and frame buffers are not big, the saturration of the is pretty fast.
//     - after any pause a user has some preloaded buffers, so the playback is
//       supposed to be more stable in cases with a weak processor or bad internet.
//     - the code is simplier, usage is more convenient.
//
static constexpr bool shouldPauseStreams = false;

PlaybackEngine::PlaybackEngine(const QPlaybackOptions &options)
    : m_demuxer({}, {}),
      m_streams(defaultObjectsArray<decltype(m_streams)>()),
      m_renderers(defaultObjectsArray<decltype(m_renderers)>()),
      m_options{ options }
{
    qCDebug(qLcPlaybackEngine) << "Create PlaybackEngine";
    qRegisterMetaType<QFFmpeg::Packet>();
    qRegisterMetaType<QFFmpeg::Frame>();
    qRegisterMetaType<QFFmpeg::TrackPosition>();
    qRegisterMetaType<QFFmpeg::TrackDuration>();
}

PlaybackEngine::~PlaybackEngine() {
    qCDebug(qLcPlaybackEngine) << "Delete PlaybackEngine";

    finalizeOutputs();
    forEachExistingObject([](auto &object) { object.reset(); });
    deleteFreeThreads();
}

void PlaybackEngine::onRendererFinished()
{
    auto isAtEnd = [this](auto trackType) {
        return !m_renderers[trackType] || m_renderers[trackType]->isAtEnd();
    };

    if (!isAtEnd(QPlatformMediaPlayer::VideoStream))
        return;

    if (!isAtEnd(QPlatformMediaPlayer::AudioStream))
        return;

    if (!isAtEnd(QPlatformMediaPlayer::SubtitleStream) && !hasMediaStream())
        return;

    if (std::exchange(m_state, QMediaPlayer::StoppedState) == QMediaPlayer::StoppedState)
        return;

    finilizeTime(duration().asTimePoint());

    forceUpdate();

    qCDebug(qLcPlaybackEngine) << "Playback engine end of stream";

    emit endOfStream();
}

void PlaybackEngine::onRendererLoopChanged(quint64 id, TrackPosition offset, int loopIndex)
{
    if (!hasRenderer(id))
        return;

    if (loopIndex > m_currentLoopOffset.loopIndex) {
        m_currentLoopOffset = { offset, loopIndex };
        emit loopChanged();
    } else if (loopIndex == m_currentLoopOffset.loopIndex && offset != m_currentLoopOffset.loopStartTimeUs) {
        qWarning() << "Unexpected offset for loop" << loopIndex << ":" << offset.get() << "vs"
                   << m_currentLoopOffset.loopStartTimeUs.get();
        m_currentLoopOffset.loopStartTimeUs = offset;
    }
}

void PlaybackEngine::onFirsPacketFound(quint64 id, TrackPosition absSeekPos)
{
    if (!m_demuxer || m_demuxer->id() != id)
        return;

    if (m_shouldUpdateTimeOnFirstPacket) {
        const auto timePoint = SteadyClock::now();
        const SteadyClock::time_point expectedTimePoint =
                m_timeController.timeFromPosition(absSeekPos);
        const auto delay = std::chrono::duration_cast<std::chrono::microseconds>(
                timePoint - expectedTimePoint);
        qCDebug(qLcPlaybackEngine) << "Delay of demuxer initialization:" << delay;
        m_timeController.sync(timePoint, absSeekPos);

        m_shouldUpdateTimeOnFirstPacket = false; // turn the flag back to ensure the consistency.
    }

    forEachExistingObject<Renderer>([&](auto &renderer) { renderer->start(m_timeController); });
}

void PlaybackEngine::onRendererSynchronized(quint64 id, SteadyClock::time_point tp, TrackPosition pos)
{
    if (!hasRenderer(id))
        return;

    Q_ASSERT(m_renderers[QPlatformMediaPlayer::AudioStream]
             && m_renderers[QPlatformMediaPlayer::AudioStream]->id() == id);

    m_timeController.sync(tp, pos);

    forEachExistingObject<Renderer>([&](auto &renderer) {
        if (id != renderer->id())
            renderer->syncSoft(tp, pos);
    });
}

void PlaybackEngine::setState(QMediaPlayer::PlaybackState state) {
    if (!m_media.avContext())
        return;

    if (state == m_state)
        return;

    const auto prevState = std::exchange(m_state, state);

    if (m_state == QMediaPlayer::StoppedState) {
        finalizeOutputs();
        finilizeTime(TrackPosition(0));
    }

    if (prevState == QMediaPlayer::StoppedState || m_state == QMediaPlayer::StoppedState)
        recreateObjects();

    if (prevState == QMediaPlayer::StoppedState)
        triggerStepIfNeeded();

    updateObjectsPausedState();
}

void PlaybackEngine::updateObjectsPausedState()
{
    const auto paused = m_state != QMediaPlayer::PlayingState;
    m_timeController.setPaused(paused);

    forEachExistingObject([&](auto &object) {
        bool objectPaused = false;

        if constexpr (std::is_same_v<decltype(*object), Renderer &>)
            objectPaused = paused;
        else if constexpr (shouldPauseStreams) {
            auto streamPaused = [](bool p, auto &r) {
                const auto needMoreFrames = r && r->stepInProgress();
                return p && !needMoreFrames;
            };

            if constexpr (std::is_same_v<decltype(*object), StreamDecoder &>)
                objectPaused = streamPaused(paused, renderer(object->trackType()));
            else
                objectPaused = std::accumulate(m_renderers.begin(), m_renderers.end(), paused,
                                               streamPaused);
        }

        object->setPaused(objectPaused);
    });
}

void PlaybackEngine::ObjectDeleter::operator()(PlaybackEngineObject *object) const
{
    Q_ASSERT(engine);
    if (!std::exchange(engine->m_threadsDirty, true))
        QMetaObject::invokeMethod(engine, &PlaybackEngine::deleteFreeThreads, Qt::QueuedConnection);

    object->kill();
}

void PlaybackEngine::registerObject(PlaybackEngineObject &object)
{
    connect(&object, &PlaybackEngineObject::error, this, &PlaybackEngine::errorOccured);

    auto threadName = objectThreadName(object);
    auto &thread = m_threads[threadName];
    if (!thread) {
        thread = std::make_unique<QThread>();
        thread->setObjectName(threadName);
        thread->start();
    }

    Q_ASSERT(object.thread() != thread.get());
    object.moveToThread(thread.get());
}

PlaybackEngine::RendererPtr
PlaybackEngine::createRenderer(QPlatformMediaPlayer::TrackType trackType)
{
    switch (trackType) {
    case QPlatformMediaPlayer::VideoStream:
        return m_videoSink ? createPlaybackEngineObject<VideoRenderer>(
                       m_timeController, m_videoSink, m_media.transformation())
                           : RendererPtr{ {}, {} };
    case QPlatformMediaPlayer::AudioStream:
        return m_audioOutput || m_audioBufferOutput
                ? createPlaybackEngineObject<AudioRenderer>(
                          m_timeController, m_audioOutput, m_audioBufferOutput, m_pitchCompensation)
                : RendererPtr{ {}, {} };
    case QPlatformMediaPlayer::SubtitleStream:
        return m_videoSink
                ? createPlaybackEngineObject<SubtitleRenderer>(m_timeController, m_videoSink)
                : RendererPtr{ {}, {} };
    default:
        return { {}, {} };
    }
}

template<typename C, typename Action>
void PlaybackEngine::forEachExistingObject(Action &&action)
{
    auto handleNotNullObject = [&](auto &object) {
        if constexpr (std::is_base_of_v<C, std::remove_reference_t<decltype(*object)>>)
            if (object)
                action(object);
    };

    handleNotNullObject(m_demuxer);
    std::for_each(m_streams.begin(), m_streams.end(), handleNotNullObject);
    std::for_each(m_renderers.begin(), m_renderers.end(), handleNotNullObject);
}

template<typename Action>
void PlaybackEngine::forEachExistingObject(Action &&action)
{
    forEachExistingObject<PlaybackEngineObject>(std::forward<Action>(action));
}

void PlaybackEngine::seek(TrackPosition pos)
{
    pos = boundPosition(pos);

    m_timeController.setPaused(true);
    m_timeController.sync(m_currentLoopOffset.loopStartTimeUs.asDuration() + pos);
    m_seekPending = true;

    forceUpdate();
}

void PlaybackEngine::setLoops(int loops)
{
    if (!isSeekable()) {
        qWarning() << "Cannot set loops for non-seekable source";
        return;
    }

    if (std::exchange(m_loops, loops) == loops)
        return;

    qCDebug(qLcPlaybackEngine) << "set playback engine loops:" << loops << "prev loops:" << m_loops
                               << "index:" << m_currentLoopOffset.loopIndex;

    if (m_demuxer)
        m_demuxer->setLoops(loops);
}

void PlaybackEngine::triggerStepIfNeeded()
{
    if (m_state != QMediaPlayer::PausedState)
        return;

    if (m_renderers[QPlatformMediaPlayer::VideoStream])
        m_renderers[QPlatformMediaPlayer::VideoStream]->doForceStep();

    // TODO: maybe trigger SubtitleStream.
    // If trigger it, we have to make seeking for the current subtitle frame more stable.
    // Or set some timeout for seeking.
}

QString PlaybackEngine::objectThreadName(const PlaybackEngineObject &object)
{
    QString result = QString::fromLatin1(object.metaObject()->className());
    if (auto stream = qobject_cast<const StreamDecoder *>(&object))
        result += QString::number(stream->trackType());

    return result;
}

void PlaybackEngine::setPlaybackRate(float rate) {
    if (rate == playbackRate())
        return;

    m_timeController.setPlaybackRate(rate);
    forEachExistingObject<Renderer>([rate](auto &renderer) { renderer->setPlaybackRate(rate); });
}

float PlaybackEngine::playbackRate() const {
    return m_timeController.playbackRate();
}

void PlaybackEngine::recreateObjects()
{
    m_timeController.setPaused(true);

    forEachExistingObject([](auto &object) { object.reset(); });

    createObjectsIfNeeded();
}

void PlaybackEngine::createObjectsIfNeeded()
{
    if (m_state == QMediaPlayer::StoppedState || !m_media.avContext())
        return;

    for (int i = 0; i < QPlatformMediaPlayer::NTrackTypes; ++i)
        createStreamAndRenderer(static_cast<QPlatformMediaPlayer::TrackType>(i));

    createDemuxer();
}

void PlaybackEngine::forceUpdate()
{
    recreateObjects();
    triggerStepIfNeeded();
    updateObjectsPausedState();
}

void PlaybackEngine::createStreamAndRenderer(QPlatformMediaPlayer::TrackType trackType)
{
    auto codecContext = codecContextForTrack(trackType);

    auto &renderer = m_renderers[trackType];

    if (!codecContext)
        return;

    if (!renderer) {
        renderer = createRenderer(trackType);

        if (!renderer)
            return;

        connect(renderer.get(), &Renderer::synchronized, this,
                &PlaybackEngine::onRendererSynchronized);

        connect(renderer.get(), &Renderer::loopChanged, this,
                &PlaybackEngine::onRendererLoopChanged);

        if constexpr (shouldPauseStreams)
            connect(renderer.get(), &Renderer::forceStepDone, this,
                    &PlaybackEngine::updateObjectsPausedState);

        connect(renderer.get(), &PlaybackEngineObject::atEnd, this,
                &PlaybackEngine::onRendererFinished);
    }

    auto &stream = m_streams[trackType] =
            createPlaybackEngineObject<StreamDecoder>(*codecContext, renderer->seekPosition());

    Q_ASSERT(trackType == stream->trackType());

    connect(stream.get(), &StreamDecoder::requestHandleFrame, renderer.get(), &Renderer::render);
    connect(stream.get(), &PlaybackEngineObject::atEnd, renderer.get(),
            &Renderer::onFinalFrameReceived);
    connect(renderer.get(), &Renderer::frameProcessed, stream.get(),
            &StreamDecoder::onFrameProcessed);
}

std::optional<CodecContext> PlaybackEngine::codecContextForTrack(QPlatformMediaPlayer::TrackType trackType)
{
    const auto streamIndex = m_media.currentStreamIndex(trackType);
    if (streamIndex < 0)
        return {};

    auto &codecContext = m_codecContexts[trackType];

    if (!codecContext) {
        qCDebug(qLcPlaybackEngine)
                << "Create codec for stream:" << streamIndex << "trackType:" << trackType;
        auto maybeCodecContext = CodecContext::create(m_media.avContext()->streams[streamIndex],
                                                      m_media.avContext(), m_options);

        if (!maybeCodecContext) {
            emit errorOccured(QMediaPlayer::FormatError,
                              u"Cannot create codec," + maybeCodecContext.error());
            return {};
        }

        codecContext = maybeCodecContext.value();
    }

    return codecContext;
}

bool PlaybackEngine::hasMediaStream() const
{
    return m_renderers[QPlatformMediaPlayer::AudioStream]
            || m_renderers[QPlatformMediaPlayer::VideoStream];
}

void PlaybackEngine::createDemuxer()
{
    std::array<int, QPlatformMediaPlayer::NTrackTypes> streamIndexes = { -1, -1, -1 };

    bool hasStreams = false;
    forEachExistingObject<StreamDecoder>([&](auto &stream) {
        hasStreams = true;
        const auto trackType = stream->trackType();
        streamIndexes[trackType] = m_media.currentStreamIndex(trackType);
    });

    if (!hasStreams)
        return;

    const TrackPosition currentLoopPosUs = currentPosition(false);

    m_demuxer = createPlaybackEngineObject<Demuxer>(m_media.avContext(), currentLoopPosUs,
                                                    m_seekPending, m_currentLoopOffset,
                                                    streamIndexes, m_loops);

    m_seekPending = false;

    connect(m_demuxer.get(), &Demuxer::packetsBuffered, this, &PlaybackEngine::buffered);

    forEachExistingObject<StreamDecoder>([&](auto &stream) {
        connect(m_demuxer.get(), Demuxer::signalByTrackType(stream->trackType()), stream.get(),
                &StreamDecoder::decode);
        connect(m_demuxer.get(), &PlaybackEngineObject::atEnd, stream.get(),
                &StreamDecoder::onFinalPacketReceived);
        connect(stream.get(), &StreamDecoder::packetProcessed, m_demuxer.get(),
                &Demuxer::onPacketProcessed);
    });

    m_shouldUpdateTimeOnFirstPacket = true;
    connect(m_demuxer.get(), &Demuxer::firstPacketFound, this, &PlaybackEngine::onFirsPacketFound);
}

void PlaybackEngine::deleteFreeThreads() {
    m_threadsDirty = false;
    auto freeThreads = std::move(m_threads);

    forEachExistingObject([&](auto &object) {
        m_threads.insert(freeThreads.extract(objectThreadName(*object)));
    });

    for (auto &[name, thr] : freeThreads)
        thr->quit();

    for (auto &[name, thr] : freeThreads)
        thr->wait();
}

void PlaybackEngine::setMedia(MediaDataHolder media)
{
    Q_ASSERT(!m_media.avContext()); // Playback engine does not support reloading media
    Q_ASSERT(m_state == QMediaPlayer::StoppedState);
    Q_ASSERT(m_threads.empty());

    m_media = std::move(media);
    updateVideoSinkSize();
}

void PlaybackEngine::setVideoSink(QVideoSink *sink)
{
    auto prev = std::exchange(m_videoSink, sink);
    if (prev == sink)
        return;

    updateVideoSinkSize(prev);
    updateActiveVideoOutput(sink);

    if (!sink || !prev) {
        // might need some improvements
        forceUpdate();
    }
}

void PlaybackEngine::setAudioSink(QPlatformAudioOutput *output) {
    setAudioSink(output ? output->q : nullptr);
}

void PlaybackEngine::setAudioSink(QAudioOutput *output)
{
    QAudioOutput *prev = std::exchange(m_audioOutput, output);
    if (prev == output)
        return;

    updateActiveAudioOutput(output);

    if (!output || !prev) {
        // might need some improvements
        forceUpdate();
    }
}

void PlaybackEngine::setAudioBufferOutput(QAudioBufferOutput *output)
{
    QAudioBufferOutput *prev = std::exchange(m_audioBufferOutput, output);
    if (prev == output)
        return;
    updateActiveAudioOutput(output);
}

TrackPosition PlaybackEngine::currentPosition(bool topPos) const
{
    std::optional<TrackPosition> pos;

    for (size_t i = 0; i < m_renderers.size(); ++i) {
        const auto &renderer = m_renderers[i];
        if (!renderer)
            continue;

        // skip subtitle stream for finding lower rendering position
        if (!topPos && i == QPlatformMediaPlayer::SubtitleStream && hasMediaStream())
            continue;

        const auto rendererPos = renderer->lastPosition();
        pos = !pos       ? rendererPos
                : topPos ? std::max(*pos, rendererPos)
                         : std::min(*pos, rendererPos);
    }

    if (!pos)
        pos = m_timeController.currentPosition();

    return boundPosition(*pos - m_currentLoopOffset.loopStartTimeUs.asDuration());
}

TrackDuration PlaybackEngine::duration() const
{
    return m_media.duration();
}

bool PlaybackEngine::isSeekable() const { return m_media.isSeekable(); }

const QList<MediaDataHolder::StreamInfo> &
PlaybackEngine::streamInfo(QPlatformMediaPlayer::TrackType trackType) const
{
    return m_media.streamInfo(trackType);
}

const QMediaMetaData &PlaybackEngine::metaData() const
{
    return m_media.metaData();
}

int PlaybackEngine::activeTrack(QPlatformMediaPlayer::TrackType type) const
{
    return m_media.activeTrack(type);
}

void PlaybackEngine::setPitchCompensation(bool enabled)
{
    m_pitchCompensation = enabled;
    if (AudioRenderer *renderer = getAudioRenderer())
        renderer->setPitchCompensation(enabled);
}

void PlaybackEngine::setActiveTrack(QPlatformMediaPlayer::TrackType trackType, int streamNumber)
{
    if (!m_media.setActiveTrack(trackType, streamNumber))
        return;

    m_codecContexts[trackType] = {};

    m_renderers[trackType].reset();
    m_streams = defaultObjectsArray<decltype(m_streams)>();
    m_demuxer.reset();

    updateVideoSinkSize();
    createObjectsIfNeeded();
    updateObjectsPausedState();

    // We strive to have a smooth playback if we change the active track. It means that
    // we don't want to do any time shiftings. Instead, we rely on the fact that
    // buffers in renderers are not empty to compensate the demuxer's lag.
    m_shouldUpdateTimeOnFirstPacket = false;
}

void PlaybackEngine::finilizeTime(TrackPosition pos)
{
    Q_ASSERT(pos >= TrackPosition(0) && pos <= duration().asTimePoint());

    m_timeController.setPaused(true);
    m_timeController.sync(pos);
    m_currentLoopOffset = {};
}

void PlaybackEngine::finalizeOutputs()
{
    if (m_audioBufferOutput)
        updateActiveAudioOutput(static_cast<QAudioBufferOutput *>(nullptr));
    if (m_audioOutput)
        updateActiveAudioOutput(static_cast<QAudioOutput *>(nullptr));
    updateActiveVideoOutput(nullptr, true);
}

bool PlaybackEngine::hasRenderer(quint64 id) const
{
    return std::any_of(m_renderers.begin(), m_renderers.end(),
                       [id](auto &renderer) { return renderer && renderer->id() == id; });
}

template <typename AudioOutput>
void PlaybackEngine::updateActiveAudioOutput(AudioOutput *output)
{
    if (AudioRenderer *renderer = getAudioRenderer())
        renderer->setOutput(output);
}

void PlaybackEngine::updateActiveVideoOutput(QVideoSink *sink, bool cleanOutput)
{
    if (auto renderer = qobject_cast<SubtitleRenderer *>(
                m_renderers[QPlatformMediaPlayer::SubtitleStream].get()))
        renderer->setOutput(sink, cleanOutput);
    if (auto renderer =
                qobject_cast<VideoRenderer *>(m_renderers[QPlatformMediaPlayer::VideoStream].get()))
        renderer->setOutput(sink, cleanOutput);
}

void PlaybackEngine::updateVideoSinkSize(QVideoSink *prevSink)
{
    auto platformVideoSink = m_videoSink ? m_videoSink->platformVideoSink() : nullptr;
    if (!platformVideoSink)
        return;

    if (prevSink && prevSink->platformVideoSink())
        platformVideoSink->setNativeSize(prevSink->platformVideoSink()->nativeSize());
    else {
        const auto streamIndex = m_media.currentStreamIndex(QPlatformMediaPlayer::VideoStream);
        if (streamIndex >= 0) {
            const auto context = m_media.avContext();
            const auto stream = context->streams[streamIndex];
            const AVRational pixelAspectRatio =
                    av_guess_sample_aspect_ratio(context, stream, nullptr);
            // auto size = metaData().value(QMediaMetaData::Resolution)
            const QSize size =
                    qCalculateFrameSize({ stream->codecpar->width, stream->codecpar->height },
                                        { pixelAspectRatio.num, pixelAspectRatio.den });

            platformVideoSink->setNativeSize(
                    qRotatedFrameSize(size, m_media.transformation().rotation));
        }
    }
}

TrackPosition PlaybackEngine::boundPosition(TrackPosition position) const
{
    position = qMax(position, TrackPosition(0));
    return duration() > TrackDuration(0) ? qMin(position, duration().asTimePoint()) : position;
}

AudioRenderer *PlaybackEngine::getAudioRenderer()
{
    return qobject_cast<AudioRenderer *>(m_renderers[QPlatformMediaPlayer::AudioStream].get());
}

} // namespace QFFmpeg

QT_END_NAMESPACE

#include "moc_qffmpegplaybackengine_p.cpp"
