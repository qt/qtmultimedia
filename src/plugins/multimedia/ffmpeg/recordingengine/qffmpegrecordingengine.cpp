// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qffmpegrecordingengine_p.h"
#include "qffmpegencodinginitializer_p.h"
#include "qffmpegaudioencoder_p.h"
#include "qffmpegaudioinput_p.h"
#include "qffmpegrecordingengineutils_p.h"

#include "private/qmultimediautils_p.h"
#include "private/qplatformaudiobufferinput_p.h"
#include "private/qplatformvideosource_p.h"
#include "private/qplatformvideoframeinput_p.h"

#include "qdebug.h"
#include "qffmpegvideoencoder_p.h"
#include "qffmpegmediametadata_p.h"
#include "qffmpegmuxer_p.h"
#include "qloggingcategory.h"

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(qLcFFmpegEncoder, "qt.multimedia.ffmpeg.encoder");

namespace QFFmpeg
{

RecordingEngine::RecordingEngine(const QMediaEncoderSettings &settings,
                 std::unique_ptr<EncodingFormatContext> context)
    : m_settings(settings), m_formatContext(std::move(context)), m_muxer(new Muxer(this))
{
    Q_ASSERT(m_formatContext);
    Q_ASSERT(m_formatContext->isAVIOOpen());
}

RecordingEngine::~RecordingEngine()
{
    Q_ASSERT(m_state == State::Finalizing);
}

void RecordingEngine::addAudioInput(QFFmpegAudioInput *input)
{
    Q_ASSERT(input);
    Q_ASSERT(m_state == State::FormatsInitializing);

    if (input->device.isNull()) {
        emit streamInitializationError(QMediaRecorder::ResourceError,
                                       QLatin1StringView("Audio device is null"));
        return;
    }

    const QAudioFormat format = input->device.preferredFormat();

    if (!format.isValid()) {
        emit streamInitializationError(
                QMediaRecorder::FormatError,
                QLatin1StringView("Audio device has invalid preferred format"));
        return;
    }

    AudioEncoder *audioEncoder = createAudioEncoder(format);
    connectEncoderToSource(audioEncoder, input);
}

void RecordingEngine::addAudioBufferInput(QPlatformAudioBufferInput *input,
                                          const QAudioBuffer &firstBuffer)
{
    Q_ASSERT(input);
    Q_ASSERT(m_state == State::FormatsInitializing);

    const QAudioFormat format = firstBuffer.isValid() ? firstBuffer.format() : input->audioFormat();

    AudioEncoder *audioEncoder = createAudioEncoder(format);

    // set the buffer before connecting to avoid potential races
    if (firstBuffer.isValid())
        audioEncoder->addBuffer(firstBuffer);

    connectEncoderToSource(audioEncoder, input);
}

AudioEncoder *RecordingEngine::createAudioEncoder(const QAudioFormat &format)
{
    Q_ASSERT(format.isValid());

    auto audioEncoder = new AudioEncoder(*this, format, m_settings);

    m_audioEncoders.emplace_back(audioEncoder);
    connect(audioEncoder, &EncoderThread::endOfSourceStream, this,
            &RecordingEngine::handleSourceEndOfStream);
    connect(audioEncoder, &EncoderThread::initialized, this,
            &RecordingEngine::handleEncoderInitialization, Qt::SingleShotConnection);
    if (m_autoStop)
        audioEncoder->setAutoStop(true);

    return audioEncoder;
}

void RecordingEngine::addVideoSource(QPlatformVideoSource *source, const QVideoFrame &firstFrame)
{
    Q_ASSERT(m_state == State::FormatsInitializing);

    QVideoFrameFormat frameFormat =
            firstFrame.isValid() ? firstFrame.surfaceFormat() : source->frameFormat();

    Q_ASSERT(frameFormat.isValid());

    if (firstFrame.isValid() && frameFormat.streamFrameRate() <= 0.f) {
        const qint64 startTime = firstFrame.startTime();
        const qint64 endTime = firstFrame.endTime();
        if (startTime != -1 && endTime > startTime)
            frameFormat.setStreamFrameRate(static_cast<qreal>(VideoFrameTimeBase)
                                           / (endTime - startTime));
    }

    std::optional<AVPixelFormat> hwPixelFormat = source->ffmpegHWPixelFormat()
            ? AVPixelFormat(*source->ffmpegHWPixelFormat())
            : std::optional<AVPixelFormat>{};

    qCDebug(qLcFFmpegEncoder) << "adding video source" << source->metaObject()->className() << ":"
                              << "pixelFormat=" << frameFormat.pixelFormat()
                              << "frameSize=" << frameFormat.frameSize()
                              << "frameRate=" << frameFormat.streamFrameRate()
                              << "ffmpegHWPixelFormat=" << (hwPixelFormat ? *hwPixelFormat : AV_PIX_FMT_NONE);

    auto videoEncoder = new VideoEncoder(*this, m_settings, frameFormat, hwPixelFormat);
    m_videoEncoders.emplace_back(videoEncoder);
    if (m_autoStop)
        videoEncoder->setAutoStop(true);

    connect(videoEncoder, &EncoderThread::endOfSourceStream, this,
            &RecordingEngine::handleSourceEndOfStream);

    connect(videoEncoder, &EncoderThread::initialized, this,
            &RecordingEngine::handleEncoderInitialization, Qt::SingleShotConnection);

    // set the frame before connecting to avoid potential races
    if (firstFrame.isValid())
        videoEncoder->addFrame(firstFrame);

    connectEncoderToSource(videoEncoder, source);
}

bool RecordingEngine::startEncoders()
{
    Q_ASSERT(m_state == State::FormatsInitializing);
    Q_ASSERT(m_formatsInitializer);
    m_formatsInitializer.reset();

    if (m_audioEncoders.empty() && m_videoEncoders.empty()) {
        emit sessionError(QMediaRecorder::ResourceError,
                          QLatin1StringView("No valid stream found for encoding"));
        return false;
    }

    m_state = State::EncodersInitializing;

    forEachEncoder([](EncoderThread *encoder) { //
        encoder->start();
    });

    return true;
}

bool RecordingEngine::initialize(const std::vector<QAudioBufferSource *> &audioSources,
                                 const std::vector<QPlatformVideoSource *> &videoSources)
{
    Q_ASSERT(m_state == State::None);

    m_state = State::FormatsInitializing;
    m_formatsInitializer = std::make_unique<EncodingInitializer>(*this);
    return m_formatsInitializer->start(audioSources, videoSources);
}

RecordingEngine::EncodingFinalizer::EncodingFinalizer(RecordingEngine &recordingEngine,
                                                      bool writeTrailer)
    : m_recordingEngine(recordingEngine), m_writeTrailer(writeTrailer)
{
    setObjectName(QStringLiteral("EncodingFinalizer"));
    Q_ASSERT(m_recordingEngine.m_state == State::Finalizing);
    connect(this, &QThread::finished, this, &QObject::deleteLater);
}

void RecordingEngine::EncodingFinalizer::run()
{
    Q_ASSERT(m_recordingEngine.m_state == State::Finalizing);

    m_recordingEngine.stopAndDeleteThreads();

    if (m_writeTrailer) {
        const int res = av_write_trailer(m_recordingEngine.avFormatContext());
        if (res < 0) {
            qCWarning(qLcFFmpegEncoder) << "could not write trailer" << res << AVError(res);
            emit m_recordingEngine.sessionError(QMediaRecorder::FormatError,
                                                QLatin1String("Cannot write trailer: ")
                                                        + err2str(res));
        }
    }
    // else ffmpeg might crash

    // close AVIO before emitting finalizationDone.
    m_recordingEngine.m_formatContext->closeAVIO();

    qCDebug(qLcFFmpegEncoder) << "Media recording finalized";
    emit m_recordingEngine.finalizationDone();
    auto recordingEnginePtr = &m_recordingEngine;
    delete recordingEnginePtr;
}

void RecordingEngine::finalize()
{
    qCDebug(qLcFFmpegEncoder) << "Media recording finalizing";

    Q_ASSERT(m_state == State::FormatsInitializing || m_state == State::EncodersInitializing
             || m_state == State::Encoding);

    Q_ASSERT((m_state == State::FormatsInitializing) == !!m_formatsInitializer);

    m_formatsInitializer.reset();

    forEachEncoder(&disconnectEncoderFromSource);
    if (m_state != State::Encoding)
        forEachEncoder(&EncoderThread::startEncoding, false);

    const bool shouldWriteTrailer = m_state == State::Encoding;
    m_state = State::Finalizing;

    EncodingFinalizer *finalizer = new EncodingFinalizer(*this, shouldWriteTrailer);
    finalizer->start();
}

void RecordingEngine::setPaused(bool paused)
{
    forEachEncoder(&EncoderThread::setPaused, paused);
}

void RecordingEngine::setAutoStop(bool autoStop)
{
    m_autoStop = autoStop;
    forEachEncoder(&EncoderThread::setAutoStop, autoStop);
    handleSourceEndOfStream();
}

void RecordingEngine::setMetaData(const QMediaMetaData &metaData)
{
    m_metaData = metaData;
}

void RecordingEngine::newTimeStamp(qint64 time)
{
    QMutexLocker locker(&m_timeMutex);
    if (time > m_timeRecorded) {
        m_timeRecorded = time;
        emit durationChanged(time);
    }
}

bool RecordingEngine::isEndOfSourceStreams() const
{
    return allOfEncoders(&EncoderThread::isEndOfSourceStream);
}

void RecordingEngine::handleSourceEndOfStream()
{
    if (m_autoStop && isEndOfSourceStreams())
        emit autoStopped();
}

void RecordingEngine::handleEncoderInitialization()
{
    Q_ASSERT(m_state == State::EncodersInitializing || m_state == State::Finalizing);

    if (m_state == State::Finalizing)
        return; // outdated event, drop it

    ++m_initializedEncodersCount;

    Q_ASSERT(m_initializedEncodersCount <= encodersCount());

    if (m_initializedEncodersCount < encodersCount())
        return;

    Q_ASSERT(allOfEncoders(&EncoderThread::isInitialized));

    qCDebug(qLcFFmpegEncoder) << "Encoders initialized; writing header";

    avFormatContext()->metadata = QFFmpegMetaData::toAVMetaData(m_metaData);

    const int res = avformat_write_header(avFormatContext(), nullptr);
    if (res < 0) {
        qWarning() << "could not write header, error:" << res << AVError(res);
        emit sessionError(QMediaRecorder::ResourceError,
                          QLatin1StringView("Cannot start writing the stream"));
        return;
    }

    qCDebug(qLcFFmpegEncoder) << "Stream header is successfully written";

    m_state = State::Encoding;
    m_muxer->start();
    forEachEncoder(&EncoderThread::startEncoding, true);
}

void RecordingEngine::stopAndDeleteThreads()
{
    m_audioEncoders.clear();
    m_videoEncoders.clear();
    m_muxer.reset();
}

template <typename F, typename... Args>
void RecordingEngine::forEachEncoder(F &&f, Args &&...args)
{
    for (auto &audioEncoder : m_audioEncoders)
        std::invoke(f, audioEncoder.get(), args...);
    for (auto &videoEncoder : m_videoEncoders)
        std::invoke(f, videoEncoder.get(), args...);
}

template <typename F>
bool RecordingEngine::allOfEncoders(F &&f) const
{
    auto predicate = [&f](const auto &encoder) { return std::invoke(f, encoder.get()); };

    return std::all_of(m_audioEncoders.cbegin(), m_audioEncoders.cend(), predicate)
            && std::all_of(m_videoEncoders.cbegin(), m_videoEncoders.cend(), predicate);
}
} // namespace QFFmpeg

QT_END_NAMESPACE

#include "moc_qffmpegrecordingengine_p.cpp"
