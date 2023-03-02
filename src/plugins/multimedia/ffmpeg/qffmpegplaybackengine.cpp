// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qffmpegplaybackengine_p.h"

#include "qvideosink.h"
#include "qaudiooutput.h"
#include "private/qplatformaudiooutput_p.h"
#include "qiodevice.h"
#include "playbackengine/qffmpegdemuxer_p.h"
#include "playbackengine/qffmpegstreamdecoder_p.h"
#include "playbackengine/qffmpegsubtitlerenderer_p.h"
#include "playbackengine/qffmpegvideorenderer_p.h"
#include "playbackengine/qffmpegaudiorenderer_p.h"

#include <qloggingcategory.h>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

// The helper is needed since on some compilers std::unique_ptr
// doesn't have a default constructor in the case of sizeof(CustomDeleter) > 0
template<typename Array>
inline Array defaultObjectsArray()
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

PlaybackEngine::PlaybackEngine()
    : m_demuxer({}, {}),
      m_streams(defaultObjectsArray<decltype(m_streams)>()),
      m_renderers(defaultObjectsArray<decltype(m_renderers)>())
{
    qRegisterMetaType<QFFmpeg::Packet>();
    qRegisterMetaType<QFFmpeg::Frame>();
}

PlaybackEngine::~PlaybackEngine() {
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

    if (!isAtEnd(QPlatformMediaPlayer::SubtitleStream)
        && !m_renderers[QPlatformMediaPlayer::AudioStream]
        && !m_renderers[QPlatformMediaPlayer::VideoStream])
        return;

    if (std::exchange(m_state, QMediaPlayer::StoppedState) == QMediaPlayer::StoppedState)
        return;

    m_timeController.setPaused(true);
    m_timeController.sync(m_duration);

    forceUpdate();

    emit endOfStream();
}

void PlaybackEngine::onRendererSynchronized(std::chrono::steady_clock::time_point tp, qint64 pos)
{
    Q_ASSERT(QObject::sender() == m_renderers[QPlatformMediaPlayer::AudioStream].get());

    if (m_timeController.positionFromTime(tp) < pos) {
        // TODO: maybe check with an asset
        qWarning() << "Unexpected synchronization " << m_timeController.positionFromTime(tp) - pos;
    }

    m_timeController.sync(tp, pos);
}

void PlaybackEngine::setState(QMediaPlayer::PlaybackState state) {
    if (!m_context)
        return;

    if ( state == m_state )
        return;

    const auto prevState = std::exchange(m_state, state);

    if (m_state == QMediaPlayer::StoppedState) {
        m_timeController.setPaused(true);
        m_timeController.sync();
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
        return m_videoSink
                ? createPlaybackEngineObject<VideoRenderer>(m_timeController, m_videoSink)
                : RendererPtr{ {}, {} };
    case QPlatformMediaPlayer::AudioStream:
        return m_audioOutput
                ? createPlaybackEngineObject<AudioRenderer>(m_timeController, m_audioOutput)
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

void PlaybackEngine::seek(qint64 pos)
{
    pos = qBound(0, pos, m_duration);

    m_timeController.setPaused(true);
    m_timeController.sync(pos);

    forceUpdate();
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
    QString result = object.metaObject()->className();
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
    if (m_state == QMediaPlayer::StoppedState || !m_context)
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
    auto codec = codecForTrack(trackType);

    auto &renderer = m_renderers[trackType];

    if (!codec)
        return;

    if (!renderer) {
        renderer = createRenderer(trackType);

        if (!renderer)
            return;

        connect(renderer.get(), &Renderer::synchronized, this,
                &PlaybackEngine::onRendererSynchronized);

        if constexpr (shouldPauseStreams)
            connect(renderer.get(), &Renderer::forceStepDone, this,
                    &PlaybackEngine::updateObjectsPausedState);

        connect(renderer.get(), &PlaybackEngineObject::atEnd, this,
                &PlaybackEngine::onRendererFinished);
    }

    auto &stream = m_streams[trackType] =
            createPlaybackEngineObject<StreamDecoder>(*codec, renderer->seekPosition());

    Q_ASSERT(trackType == stream->trackType());

    connect(stream.get(), &StreamDecoder::requestHandleFrame, renderer.get(), &Renderer::render);
    connect(stream.get(), &PlaybackEngineObject::atEnd, renderer.get(),
            &Renderer::onFinalFrameReceived);
    connect(renderer.get(), &Renderer::frameProcessed, stream.get(),
            &StreamDecoder::onFrameProcessed);

    constexpr auto masterStreamType = QPlatformMediaPlayer::AudioStream;

    auto connectMasterWithSlave = [&](auto &slave) {
        auto master = m_renderers[masterStreamType].get();
        if (master && master != slave.get())
            connect(master, &Renderer::synchronized, slave.get(), &Renderer::syncSoft);
    };

    if (trackType == masterStreamType)
        forEachExistingObject<Renderer>(connectMasterWithSlave);
    else
        connectMasterWithSlave(renderer);
}

std::optional<Codec> PlaybackEngine::codecForTrack(QPlatformMediaPlayer::TrackType trackType)
{
    const auto streamIndex = m_currentAVStreamIndex[trackType];
    if (streamIndex < 0)
        return {};

    auto &result = m_codecs[trackType];

    if (!result) {
        auto maybeCodec = Codec::create(m_context->streams[streamIndex]);

        if (!maybeCodec) {
            emit errorOccured(QMediaPlayer::FormatError,
                              "Cannot create codec," + maybeCodec.error());
            return {};
        }

        result = maybeCodec.value();
    }

    return result;
}

void PlaybackEngine::createDemuxer()
{
    decltype(m_currentAVStreamIndex) streamIndexes = { -1, -1, -1 };

    bool hasStreams = false;
    forEachExistingObject<StreamDecoder>([&](auto &stream) {
        hasStreams = true;
        const auto trackType = stream->trackType();
        streamIndexes[trackType] = m_currentAVStreamIndex[trackType];
    });

    if (!hasStreams)
        return;

    m_demuxer =
            createPlaybackEngineObject<Demuxer>(m_context.get(), currentPosition(), streamIndexes);

    forEachExistingObject<StreamDecoder>([&](auto &stream) {
        connect(m_demuxer.get(), Demuxer::signalByTrackType(stream->trackType()), stream.get(),
                &StreamDecoder::decode);
        connect(m_demuxer.get(), &PlaybackEngineObject::atEnd, stream.get(),
                &StreamDecoder::onFinalPacketReceived);
        connect(stream.get(), &StreamDecoder::packetProcessed, m_demuxer.get(),
                &Demuxer::onPacketProcessed);
    });
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

bool PlaybackEngine::setMedia(const QUrl &media, QIODevice *stream)
{
    forEachExistingObject([](auto &object) { object.reset(); });
    deleteFreeThreads();

    m_codecs = {};

    if (auto error = recreateAVFormatContext(media, stream)) {
        emit errorOccured(error->code, error->description);
        return false;
    }

    forceUpdate();
    return true;
}

void PlaybackEngine::setVideoSink(QVideoSink *sink)
{
    if (std::exchange(m_videoSink, sink) != sink)
        forceUpdate();
}

void PlaybackEngine::setAudioSink(QPlatformAudioOutput *output) {
    setAudioSink(output ? output->q : nullptr);
}

void PlaybackEngine::setAudioSink(QAudioOutput *output)
{
    if (std::exchange(m_audioOutput, output) != output)
        forceUpdate();
}

qint64 PlaybackEngine::currentPosition() const {
    auto pos = std::numeric_limits<qint64>::max();

    for (auto trackType : { QPlatformMediaPlayer::VideoStream, QPlatformMediaPlayer::AudioStream })
        if (auto &renderer = m_renderers[trackType])
            pos = std::min(pos, renderer->lastPosition());

    return pos == std::numeric_limits<qint64>::max() ? m_timeController.currentPosition() : pos;
}

void PlaybackEngine::setActiveTrack(QPlatformMediaPlayer::TrackType trackType, int streamNumber)
{
    if (!MediaDataHolder::setActiveTrack(trackType, streamNumber))
        return;

    m_codecs[trackType] = {};

    m_renderers[trackType].reset();
    m_streams = defaultObjectsArray<decltype(m_streams)>();
    m_demuxer.reset();

    createObjectsIfNeeded();
    updateObjectsPausedState();
}

}

QT_END_NAMESPACE

#include "moc_qffmpegplaybackengine_p.cpp"
