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

#include "qffmpegmediarecorder_p.h"
#include "qaudiodevice.h"
#include <private/qmediastoragelocation_p.h>
#include <private/qplatformcamera_p.h>
#include "qaudiosource.h"
#include <private/qplatformaudioinput_p.h>
#include "qaudiobuffer.h"

#include <qdebug.h>
#include <qeventloop.h>
#include <qstandardpaths.h>
#include <qmimetype.h>
#include <qloggingcategory.h>

Q_LOGGING_CATEGORY(qLcMediaEncoder, "qt.multimedia.encoder")

class QAudioSourceIO : public QIODevice
{
    Q_OBJECT
public:
    QAudioSourceIO(QAudioSource *source, int bufferSize)
        : QIODevice()
        , m_src(source)
        , bufferSize(bufferSize)
    {}
    qint64 readData(char *, qint64) override
    {
        return 0;
    }
    qint64 writeData(const char *data, qint64 len) override
    {
        int l = len;
        while (len > 0) {
            int toAppend = qMin(len, bufferSize - pcm.size());
            pcm.append(data, toAppend);
            data += toAppend;
            len -= toAppend;
            if (pcm.size() == bufferSize) {
                QAudioFormat fmt = m_src->format();
                qint64 time = fmt.durationForBytes(processed);
                QAudioBuffer buffer(pcm, fmt, time);
                emit newAudioBuffer(buffer);
                processed += bufferSize;
                pcm.clear();
            }
        }

        return l;
    }

Q_SIGNALS:
    void newAudioBuffer(const QAudioBuffer &buffer);
private:
    QAudioSource *m_src = nullptr;
    int bufferSize = 0;
    qint64 processed = 0;
    QByteArray pcm;
};

QFFmpegMediaRecorder::QFFmpegMediaRecorder(QMediaRecorder *parent)
  : QPlatformMediaRecorder(parent)
{
}

QFFmpegMediaRecorder::~QFFmpegMediaRecorder()
{
    finalize();
}

bool QFFmpegMediaRecorder::isLocationWritable(const QUrl &) const
{
    return true;
}

void QFFmpegMediaRecorder::handleSessionError(QMediaRecorder::Error code, const QString &description)
{
    error(code, description);
    stop();
}

qint64 QFFmpegMediaRecorder::duration() const
{
    return 0;
}

void QFFmpegMediaRecorder::record(QMediaEncoderSettings &settings)
{
    if (!m_session ||m_finalizing || state() != QMediaRecorder::StoppedState)
        return;

    const auto hasVideo = m_session->camera() && m_session->camera()->isActive();
    const auto hasAudio = m_session->audioInput() != nullptr;

    if (!hasVideo && !hasAudio) {
        error(QMediaRecorder::ResourceError, QMediaRecorder::tr("No camera or audio input"));
        return;
    }

    const auto audioOnly = settings.videoCodec() == QMediaFormat::VideoCodec::Unspecified;

    auto primaryLocation = audioOnly ? QStandardPaths::MusicLocation : QStandardPaths::MoviesLocation;
    auto container = settings.mimeType().preferredSuffix();
    auto location = QMediaStorageLocation::generateFileName(outputLocation().toLocalFile(), primaryLocation, container);

    QUrl actualSink = QUrl::fromLocalFile(QDir::currentPath()).resolved(location);
    qCDebug(qLcMediaEncoder) << "recording new video to" << actualSink;

    Q_ASSERT(!actualSink.isEmpty());

    const char *name = "mp4";
    auto *avFormat = av_guess_format(name, nullptr, nullptr);

    avFormatContext = avformat_alloc_context();
    avFormatContext->oformat = avFormat;

    QByteArray url = actualSink.toEncoded();
    avFormatContext->url = (char *)av_malloc(url.size() + 1);
    memcpy(avFormatContext->url, url.constData(), url.size() + 1);
    avFormatContext->pb = nullptr;
    avio_open2(&avFormatContext->pb, avFormatContext->url, AVIO_FLAG_WRITE, nullptr, nullptr);

    auto *audioInput = m_session->audioInput();
    if (audioInput) {
        QAudioFormat fmt;
        fmt.setChannelCount(settings.audioChannelCount() > 0 ? settings.audioChannelCount() : 1);
        fmt.setSampleRate(settings.audioSampleRate() > 0 ? settings.audioSampleRate() : 48000);
        fmt.setSampleFormat(QAudioFormat::Float);
        m_audioSource = new QAudioSource(audioInput->device, fmt);
        fmt = m_audioSource->format();
        m_audioSource->setVolume(audioInput->volume);

        avAudioStream = avformat_new_stream(avFormatContext, nullptr);
        avAudioStream->id = avFormatContext->nb_streams - 1;
        avAudioStream->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
        avAudioStream->codecpar->codec_id = avFormat->audio_codec;
        avAudioStream->codecpar->channels = fmt.channelCount();
        avAudioStream->codecpar->channel_layout = AV_CH_LAYOUT_MONO;
        avAudioStream->codecpar->sample_rate = fmt.sampleRate();
        avAudioStream->codecpar->format = AV_SAMPLE_FMT_FLT;
        avAudioStream->time_base = AVRational{ 1, fmt.sampleRate() };

        auto *codec = avcodec_find_encoder(avFormat->audio_codec);
        avAudioCodec = avcodec_alloc_context3(codec);
        avcodec_parameters_to_context(avAudioCodec, avAudioStream->codecpar);
        avcodec_open2(avAudioCodec, codec, nullptr);

        int bufferSize = avAudioCodec->frame_size;
        if (!bufferSize)
            bufferSize = 1024;
        bufferSize = fmt.bytesForFrames(bufferSize);

        m_audioIO = new QAudioSourceIO(m_audioSource, bufferSize);
        QObject::connect(m_audioIO, &QAudioSourceIO::newAudioBuffer, this, &QFFmpegMediaRecorder::newAudioBuffer);
    }

    int res = avformat_write_header(avFormatContext, nullptr);
    if (res < 0)
        qWarning() << "could not write header" << res;

    if (audioInput) {
        m_audioIO->open(QIODevice::WriteOnly);
        m_audioSource->start(m_audioIO);
    }

    durationChanged(0);
    stateChanged(QMediaRecorder::RecordingState);
    actualLocationChanged(QUrl::fromLocalFile(location));
}

void QFFmpegMediaRecorder::pause()
{
    if (!m_session || m_finalizing || state() != QMediaRecorder::RecordingState)
        return;

    if (m_audioSource)
        m_audioSource->suspend();
    // ####
    stateChanged(QMediaRecorder::PausedState);
}

void QFFmpegMediaRecorder::resume()
{
    if (!m_session || m_finalizing || state() != QMediaRecorder::PausedState)
        return;

    if (m_audioSource)
        m_audioSource->resume();
    // ####
    stateChanged(QMediaRecorder::RecordingState);
}

void QFFmpegMediaRecorder::stop()
{
    if (!m_session || m_finalizing || state() == QMediaRecorder::StoppedState)
        return;
    if (m_audioSource)
        m_audioSource->stop();
    qCDebug(qLcMediaEncoder) << "stop";
    m_finalizing = true;
    finalize();
}

void QFFmpegMediaRecorder::finalize()
{
    if (!m_session)
        return;

    qCDebug(qLcMediaEncoder) << "finalize";

    if (avAudioStream) {
        int ret = avcodec_send_frame(avAudioCodec, nullptr);
        if (ret != 0) {
            char errStr[1024];
            av_strerror(ret, errStr, 1024);
            qDebug() << "error sending final frame" << ret << errStr;
        }
        while (1) {
            AVPacket *packet = av_packet_alloc();
            ret = avcodec_receive_packet(avAudioCodec, packet);
            if (ret == AVERROR_EOF || ret < 0)
                break;
            av_interleaved_write_frame(avFormatContext, packet);
            av_packet_unref(packet);
        }
    }
    // ####

    av_write_trailer(avFormatContext);
    // ### free all data
    avformat_free_context(avFormatContext);
    avFormatContext = nullptr;

    m_finalizing = false;
    stateChanged(QMediaRecorder::StoppedState);
}

void QFFmpegMediaRecorder::setMetaData(const QMediaMetaData &metaData)
{
    if (!m_session)
        return;
    m_metaData = metaData;
}

QMediaMetaData QFFmpegMediaRecorder::metaData() const
{
    return m_metaData;
}

void QFFmpegMediaRecorder::setCaptureSession(QPlatformMediaCaptureSession *session)
{
    auto *captureSession = static_cast<QFFmpegMediaCaptureSession *>(session);
    if (m_session == captureSession)
        return;

    if (m_session)
        stop();

    m_session = captureSession;
    if (!m_session)
        return;
}

void QFFmpegMediaRecorder::newAudioBuffer(const QAudioBuffer &buffer)
{
    if (!avFormatContext)
        return;

    qDebug() << "new audio buffer" << buffer.byteCount() << buffer.format() << buffer.sampleCount();
    AVPacket *packet = av_packet_alloc();
    while (1) {
        int ret = avcodec_receive_packet(avAudioCodec, packet);
        if (ret < 0) {
            if (ret != AVERROR(EAGAIN)) {
                char errStr[1024];
                av_strerror(ret, errStr, 1024);
                qDebug() << "receive frame" << ret << errStr;
            }
            break;
        }
        qDebug() << "writing packet" << packet->size;
        av_interleaved_write_frame(avFormatContext, packet);
    }

    AVFrame *frame = av_frame_alloc();
    frame->format = avAudioCodec->sample_fmt;
    frame->channel_layout = avAudioCodec->channel_layout;
    frame->channels = avAudioCodec->channels;
    frame->sample_rate = avAudioCodec->sample_rate;
    frame->nb_samples = buffer.sampleCount();
    if (frame->nb_samples)
        av_frame_get_buffer(frame, 0);

    qDebug() << buffer.byteCount() << frame->buf[0]->size;
    memcpy(frame->buf[0]->data, buffer.constData<uint8_t *>(), buffer.byteCount());
    frame->pts = audioSamplesWritten;
    audioSamplesWritten += buffer.sampleCount();

    qDebug() << "sending frame" << buffer.byteCount();
    int ret = avcodec_send_frame(avAudioCodec, frame);
    if (ret < 0) {
        char errStr[1024];
        av_strerror(ret, errStr, 1024);
        qDebug() << "error sending frame" << ret << errStr;
    }
    av_packet_unref(packet);
}

#include "qffmpegmediarecorder.moc"
