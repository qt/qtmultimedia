// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qffmpegrecordingengine_p.h"
#include "qffmpegmediaformatinfo_p.h"
#include "qffmpegvideoframeencoder_p.h"
#include "private/qmultimediautils_p.h"

#include <qdebug.h>
#include "qffmpegaudioencoder_p.h"
#include "qffmpegaudioinput_p.h"
#include <private/qplatformcamera_p.h>
#include "qffmpegvideobuffer_p.h"
#include "qffmpegvideoencoder_p.h"
#include "qffmpegmediametadata_p.h"
#include "qffmpegmuxer_p.h"
#include <qloggingcategory.h>

QT_BEGIN_NAMESPACE

static Q_LOGGING_CATEGORY(qLcFFmpegEncoder, "qt.multimedia.ffmpeg.encoder");

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
}

void RecordingEngine::addAudioInput(QFFmpegAudioInput *input)
{
    m_audioEncoder = new AudioEncoder(*this, input, m_settings);
    addMediaFrameHandler(input, &QFFmpegAudioInput::newAudioBuffer, m_audioEncoder,
                         &AudioEncoder::addBuffer);
    input->setRunning(true);
}

void RecordingEngine::addVideoSource(QPlatformVideoSource * source)
{
    auto frameFormat = source->frameFormat();

    if (!frameFormat.isValid()) {
        qCWarning(qLcFFmpegEncoder) << "Cannot add source; invalid vide frame format";
        emit error(QMediaRecorder::ResourceError,
                   QLatin1StringView("Cannot get video source format"));
        return;
    }

    std::optional<AVPixelFormat> hwPixelFormat = source->ffmpegHWPixelFormat()
            ? AVPixelFormat(*source->ffmpegHWPixelFormat())
            : std::optional<AVPixelFormat>{};

    qCDebug(qLcFFmpegEncoder) << "adding video source" << source->metaObject()->className() << ":"
                              << "pixelFormat=" << frameFormat.pixelFormat()
                              << "frameSize=" << frameFormat.frameSize()
                              << "frameRate=" << frameFormat.frameRate() << "ffmpegHWPixelFormat="
                              << (hwPixelFormat ? *hwPixelFormat : AV_PIX_FMT_NONE);

    auto veUPtr = std::make_unique<VideoEncoder>(*this, m_settings, frameFormat, hwPixelFormat);
    if (!veUPtr->isValid()) {
        emit error(QMediaRecorder::FormatError, QLatin1StringView("Cannot initialize encoder"));
        return;
    }

    auto ve = veUPtr.release();
    addMediaFrameHandler(source, &QPlatformVideoSource::newVideoFrame, ve, &VideoEncoder::addFrame);
    m_videoEncoders.append(ve);
}

void RecordingEngine::start()
{
    qCDebug(qLcFFmpegEncoder) << "RecordingEngine::start!";

    avFormatContext()->metadata = QFFmpegMetaData::toAVMetaData(m_metaData);

    Q_ASSERT(!m_isHeaderWritten);

    int res = avformat_write_header(avFormatContext(), nullptr);
    if (res < 0) {
        qWarning() << "could not write header, error:" << res << err2str(res);
        emit error(QMediaRecorder::ResourceError, "Cannot start writing the stream");
        return;
    }

    m_isHeaderWritten = true;

    qCDebug(qLcFFmpegEncoder) << "stream header is successfully written";

    m_muxer->start();
    if (m_audioEncoder)
        m_audioEncoder->start();
    for (auto *videoEncoder : m_videoEncoders)
        if (videoEncoder->isValid())
            videoEncoder->start();
}

RecordingEngine::EncodingFinalizer::EncodingFinalizer(RecordingEngine &recordingEngine)
    : m_recordingEngine(recordingEngine)
{
    connect(this, &QThread::finished, this, &QObject::deleteLater);
}

void RecordingEngine::EncodingFinalizer::run()
{
    if (m_recordingEngine.m_audioEncoder)
        m_recordingEngine.m_audioEncoder->stopAndDelete();
    for (auto &videoEncoder : m_recordingEngine.m_videoEncoders)
        videoEncoder->stopAndDelete();
    m_recordingEngine.m_muxer->stopAndDelete();

    if (m_recordingEngine.m_isHeaderWritten) {
        const int res = av_write_trailer(m_recordingEngine.avFormatContext());
        if (res < 0) {
            const auto errorDescription = err2str(res);
            qCWarning(qLcFFmpegEncoder) << "could not write trailer" << res << errorDescription;
            emit m_recordingEngine.error(QMediaRecorder::FormatError,
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

    for (auto &conn : m_connections)
        disconnect(conn);

    auto *finalizer = new EncodingFinalizer(*this);
    finalizer->start();
}

void RecordingEngine::setPaused(bool p)
{
    if (m_audioEncoder)
        m_audioEncoder->setPaused(p);
    for (auto &videoEncoder : m_videoEncoders)
        videoEncoder->setPaused(p);
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

template<typename... Args>
void RecordingEngine::addMediaFrameHandler(Args &&...args)
{
    auto connection = connect(std::forward<Args>(args)..., Qt::DirectConnection);
    m_connections.append(connection);
}
}

QT_END_NAMESPACE

#include "moc_qffmpegrecordingengine_p.cpp"
