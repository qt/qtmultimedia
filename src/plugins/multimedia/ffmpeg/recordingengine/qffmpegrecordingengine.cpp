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
    m_audioEncoder = new AudioEncoder(this, input, m_settings);
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

    auto veUPtr = std::make_unique<VideoEncoder>(this, m_settings, frameFormat, hwPixelFormat);
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

RecordingEngine::EncodingFinalizer::EncodingFinalizer(RecordingEngine *e) : m_encoder(e)
{
    connect(this, &QThread::finished, this, &QObject::deleteLater);
}

void RecordingEngine::EncodingFinalizer::run()
{
    if (m_encoder->m_audioEncoder)
        m_encoder->m_audioEncoder->stopAndDelete();
    for (auto &videoEncoder : m_encoder->m_videoEncoders)
        videoEncoder->stopAndDelete();
    m_encoder->m_muxer->stopAndDelete();

    if (m_encoder->m_isHeaderWritten) {
        const int res = av_write_trailer(m_encoder->avFormatContext());
        if (res < 0) {
            const auto errorDescription = err2str(res);
            qCWarning(qLcFFmpegEncoder) << "could not write trailer" << res << errorDescription;
            emit m_encoder->error(QMediaRecorder::FormatError,
                                  QLatin1String("Cannot write trailer: ") + errorDescription);
        }
    }
    // else ffmpeg might crash

    // close AVIO before emitting finalizationDone.
    m_encoder->m_formatContext->closeAVIO();

    qCDebug(qLcFFmpegEncoder) << "    done finalizing.";
    emit m_encoder->finalizationDone();
    delete m_encoder;
}

void RecordingEngine::finalize()
{
    qCDebug(qLcFFmpegEncoder) << ">>>>>>>>>>>>>>> finalize";

    for (auto &conn : m_connections)
        disconnect(conn);

    auto *finalizer = new EncodingFinalizer(this);
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

struct QVideoFrameHolder
{
    QVideoFrame f;
    QImage i;
};

static void freeQVideoFrame(void *opaque, uint8_t *)
{
    delete reinterpret_cast<QVideoFrameHolder *>(opaque);
}

void VideoEncoder::processOne()
{
    retrievePackets();

    auto frame = takeFrame();
    if (!frame.isValid())
        return;

    if (!isValid())
        return;

//    qCDebug(qLcFFmpegEncoder) << "new video buffer" << frame.startTime();

    AVFrameUPtr avFrame;

    auto *videoBuffer = dynamic_cast<QFFmpegVideoBuffer *>(frame.videoBuffer());
    if (videoBuffer) {
        // ffmpeg video buffer, let's use the native AVFrame stored in there
        auto *hwFrame = videoBuffer->getHWFrame();
        if (hwFrame && hwFrame->format == m_frameEncoder->sourceFormat())
            avFrame.reset(av_frame_clone(hwFrame));
    }

    if (!avFrame) {
        frame.map(QVideoFrame::ReadOnly);
        auto size = frame.size();
        avFrame = makeAVFrame();
        avFrame->format = m_frameEncoder->sourceFormat();
        avFrame->width = size.width();
        avFrame->height = size.height();

        for (int i = 0; i < 4; ++i) {
            avFrame->data[i] = const_cast<uint8_t *>(frame.bits(i));
            avFrame->linesize[i] = frame.bytesPerLine(i);
        }

        QImage img;
        if (frame.pixelFormat() == QVideoFrameFormat::Format_Jpeg) {
            // the QImage is cached inside the video frame, so we can take the pointer to the image data here
            img = frame.toImage();
            avFrame->data[0] = (uint8_t *)img.bits();
            avFrame->linesize[0] = img.bytesPerLine();
        }

        Q_ASSERT(avFrame->data[0]);
        // ensure the video frame and it's data is alive as long as it's being used in the encoder
        avFrame->opaque_ref = av_buffer_create(nullptr, 0, freeQVideoFrame, new QVideoFrameHolder{frame, img}, 0);
    }

    if (m_baseTime.loadAcquire() == std::numeric_limits<qint64>::min()) {
        m_baseTime.storeRelease(frame.startTime() - m_lastFrameTime);
        qCDebug(qLcFFmpegEncoder) << ">>>> adjusting base time to" << m_baseTime.loadAcquire()
                                  << frame.startTime() << m_lastFrameTime;
    }

    qint64 time = frame.startTime() - m_baseTime.loadAcquire();
    m_lastFrameTime = frame.endTime() - m_baseTime.loadAcquire();

    setAVFrameTime(*avFrame, m_frameEncoder->getPts(time), m_frameEncoder->getTimeBase());

    m_encoder->newTimeStamp(time / 1000);

    qCDebug(qLcFFmpegEncoder) << ">>> sending frame" << avFrame->pts << time << m_lastFrameTime;
    int ret = m_frameEncoder->sendFrame(std::move(avFrame));
    if (ret < 0) {
        qCDebug(qLcFFmpegEncoder) << "error sending frame" << ret << err2str(ret);
        emit m_encoder->error(QMediaRecorder::ResourceError, err2str(ret));
    }
}

}

QT_END_NAMESPACE

#include "moc_qffmpegrecordingengine_p.cpp"
