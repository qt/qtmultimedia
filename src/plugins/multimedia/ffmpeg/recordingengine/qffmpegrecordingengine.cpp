// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qffmpegrecordingengine_p.h"
#include "qffmpegencodinginitializer_p.h"
#include "qffmpegaudioencoder_p.h"
#include "qffmpegaudioinput_p.h"

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

static Q_LOGGING_CATEGORY(qLcFFmpegEncoder, "qt.multimedia.ffmpeg.encoder");

namespace QFFmpeg
{

namespace
{

template <typename Encoder, typename Source>
void connectToSource(Encoder *encoder, Source *source)
{
    Q_ASSERT(!encoder->source());
    encoder->setSource(source);

    auto setEncoderInterface = [encoder](auto input) {
        using Input = std::remove_pointer_t<decltype(input)>;
        input->setEncoderInterface(encoder);

        QObject::connect(encoder, &EncoderThread::canPushFrameChanged, input,
                         &Input::encoderUpdated);

        // Postpone emit 'encoderUpdated' as the encoding pipeline may be not
        // completely ready at the moment. The case is calling QMediaRecorder::stop
        // upon handling 'readyToSendFrame'
        QMetaObject::invokeMethod(input, &Input::encoderUpdated, Qt::QueuedConnection);
    };

    if constexpr (std::is_same_v<Encoder, VideoEncoder>) {
        QObject::connect(source, &QPlatformVideoSource::newVideoFrame, encoder,
                         &VideoEncoder::addFrame, Qt::DirectConnection);

        if (auto videoFrameInput = qobject_cast<QPlatformVideoFrameInput *>(source))
            setEncoderInterface(videoFrameInput);
    } else {
        QObject::connect(source, &QPlatformAudioBufferInputBase::newAudioBuffer, encoder,
                         &AudioEncoder::addBuffer, Qt::DirectConnection);

        if (auto audioBufferInput = qobject_cast<QPlatformAudioBufferInput *>(source))
            setEncoderInterface(audioBufferInput);
    }
}

void disconnectFromSource(EncoderThread *encoder)
{
    QObject *source = encoder->source();
    if (!source)
        return;

    // We should address the dependency AudioEncoder from QFFmpegAudioInput to
    // set null source here.
    // encoder->setSource(nullptr);

    auto unsetEncoderInterface = [](auto input) {
        input->setEncoderInterface(nullptr);
        emit input->encoderUpdated();
    };

    QObject::disconnect(source, nullptr, encoder, nullptr);

    if (auto videoFrameInput = qobject_cast<QPlatformVideoFrameInput *>(source))
        unsetEncoderInterface(videoFrameInput);
    else if (auto audioBufferInput = qobject_cast<QPlatformAudioBufferInput *>(source))
        unsetEncoderInterface(audioBufferInput);
}
}

RecordingEngine::RecordingEngine(const QMediaEncoderSettings &settings,
                 std::unique_ptr<EncodingFormatContext> context)
    : m_settings(settings), m_formatContext(std::move(context)), m_muxer(new Muxer(this))
{
    Q_ASSERT(m_formatContext);
    Q_ASSERT(m_formatContext->isAVIOOpen());
}

RecordingEngine::~RecordingEngine()
{
}

void RecordingEngine::addAudioInput(QFFmpegAudioInput *input)
{
    Q_ASSERT(input);

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
    connectToSource(audioEncoder, input);

    input->setRunning(true);
}

void RecordingEngine::addAudioBufferInput(QPlatformAudioBufferInput *input,
                                          const QAudioBuffer &firstBuffer)
{
    Q_ASSERT(input);
    const QAudioFormat format = firstBuffer.isValid() ? firstBuffer.format() : input->audioFormat();

    AudioEncoder *audioEncoder = createAudioEncoder(format);

    // set the buffer before connecting to avoid potential races
    if (firstBuffer.isValid())
        audioEncoder->addBuffer(firstBuffer);

    connectToSource(audioEncoder, input);
}

AudioEncoder *RecordingEngine::createAudioEncoder(const QAudioFormat &format)
{
    Q_ASSERT(format.isValid());

    auto audioEncoder = new AudioEncoder(*this, format, m_settings);
    m_audioEncoders.push_back(audioEncoder);

    return audioEncoder;
}

void RecordingEngine::addVideoSource(QPlatformVideoSource *source, const QVideoFrame &firstFrame)
{
    QVideoFrameFormat frameFormat =
            firstFrame.isValid() ? firstFrame.surfaceFormat() : source->frameFormat();

    Q_ASSERT(frameFormat.isValid());

    std::optional<AVPixelFormat> hwPixelFormat = source->ffmpegHWPixelFormat()
            ? AVPixelFormat(*source->ffmpegHWPixelFormat())
            : std::optional<AVPixelFormat>{};

    qCDebug(qLcFFmpegEncoder) << "adding video source" << source->metaObject()->className() << ":"
                              << "pixelFormat=" << frameFormat.pixelFormat()
                              << "frameSize=" << frameFormat.frameSize()
                              << "frameRate=" << frameFormat.streamFrameRate()
                              << "ffmpegHWPixelFormat=" << (hwPixelFormat ? *hwPixelFormat : AV_PIX_FMT_NONE);

    auto veUPtr = std::make_unique<VideoEncoder>(*this, m_settings, frameFormat, hwPixelFormat);
    if (!veUPtr->isValid()) {
        emit streamInitializationError(QMediaRecorder::FormatError,
                                       QLatin1StringView("Cannot initialize encoder"));
        return;
    }

    auto videoEncoder = veUPtr.release();
    m_videoEncoders.append(videoEncoder);

    // set the frame before connecting to avoid potential races
    if (firstFrame.isValid())
        videoEncoder->addFrame(firstFrame);

    connectToSource(videoEncoder, source);
}

void RecordingEngine::start()
{
    Q_ASSERT(m_initializer);
    m_initializer.reset();

    if (m_audioEncoders.empty() && m_videoEncoders.empty()) {
        emit sessionError(QMediaRecorder::ResourceError,
                          QLatin1StringView("No valid stream found for encoding"));
        return;
    }

    qCDebug(qLcFFmpegEncoder) << "RecordingEngine::start!";

    avFormatContext()->metadata = QFFmpegMetaData::toAVMetaData(m_metaData);

    Q_ASSERT(!m_isHeaderWritten);

    int res = avformat_write_header(avFormatContext(), nullptr);
    if (res < 0) {
        qWarning() << "could not write header, error:" << res << err2str(res);
        emit sessionError(QMediaRecorder::ResourceError,
                          QLatin1StringView("Cannot start writing the stream"));
        return;
    }

    m_isHeaderWritten = true;

    qCDebug(qLcFFmpegEncoder) << "stream header is successfully written";

    m_muxer->start();

    forEachEncoder([](QThread *thread) { thread->start(); });
}

void RecordingEngine::initialize(const std::vector<QPlatformAudioBufferInputBase *> &audioSources,
                                 const std::vector<QPlatformVideoSource *> &videoSources)
{
    qCDebug(qLcFFmpegEncoder) << ">>>>>>>>>>>>>>> initialize";

    m_initializer = std::make_unique<EncodingInitializer>(*this);
    m_initializer->start(audioSources, videoSources);
}

RecordingEngine::EncodingFinalizer::EncodingFinalizer(RecordingEngine &recordingEngine)
    : m_recordingEngine(recordingEngine)
{
    connect(this, &QThread::finished, this, &QObject::deleteLater);
}

void RecordingEngine::EncodingFinalizer::run()
{
    m_recordingEngine.forEachEncoder(&EncoderThread::stopAndDelete);

    if (m_recordingEngine.m_isHeaderWritten) {
        const int res = av_write_trailer(m_recordingEngine.avFormatContext());
        if (res < 0) {
            const auto errorDescription = err2str(res);
            qCWarning(qLcFFmpegEncoder) << "could not write trailer" << res << errorDescription;
            emit m_recordingEngine.sessionError(QMediaRecorder::FormatError,
                                                QLatin1String("Cannot write trailer: ")
                                                        + errorDescription);
        }
    }
    // else ffmpeg might crash

    // close AVIO before emitting finalizationDone.
    m_recordingEngine.m_formatContext->closeAVIO();

    qCDebug(qLcFFmpegEncoder) << "    done finalizing.";
    emit m_recordingEngine.finalizationDone();
    auto recordingEnginePtr = &m_recordingEngine;
    delete recordingEnginePtr;
}

void RecordingEngine::finalize()
{
    qCDebug(qLcFFmpegEncoder) << ">>>>>>>>>>>>>>> finalize";

    m_initializer.reset();

    forEachEncoder(&disconnectFromSource);

    auto *finalizer = new EncodingFinalizer(*this);
    finalizer->start();
}

void RecordingEngine::setPaused(bool paused)
{
    forEachEncoder(&EncoderThread::setPaused, paused);
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

template <typename F, typename... Args>
void RecordingEngine::forEachEncoder(F &&f, Args &&...args)
{
    for (AudioEncoder *audioEncoder : m_audioEncoders)
        std::invoke(f, audioEncoder, args...);
    for (VideoEncoder *videoEncoder : m_videoEncoders)
        std::invoke(f, videoEncoder, args...);
}
}

QT_END_NAMESPACE

#include "moc_qffmpegrecordingengine_p.cpp"
