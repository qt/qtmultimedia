/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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
//#define DEBUG_DECODER

#include "qffmpegaudiodecoder_p.h"
#include "qffmpegdecoder_p.h"
#include "qffmpegmediaformatinfo_p.h"
#include "qaudiobuffer.h"

#include <qloggingcategory.h>

Q_LOGGING_CATEGORY(qLcAudioDecoder, "qt.multimedia.ffmpeg.audioDecoder")

#define MAX_BUFFERS_IN_QUEUE 4

QT_BEGIN_NAMESPACE

namespace QFFmpeg
{

class SteppingAudioRenderer : public Renderer
{
public:
    SteppingAudioRenderer(AudioDecoder *decoder, const QAudioFormat &format);
    ~SteppingAudioRenderer()
    {
        swr_free(&resampler);
    }

    void createResampler(const Codec *codec);

    void loop();
    AudioDecoder *m_decoder;
    QAudioFormat m_format;
    SwrContext *resampler = nullptr;
};

class AudioDecoder : public Decoder
{
    Q_OBJECT
public:
    AudioDecoder() = default;

    void setup(QFFmpegAudioDecoder *decoder, const QAudioFormat &format)
    {
        m_audioDecoder = decoder;
        connect(this, &AudioDecoder::newAudioBuffer, decoder, &QFFmpegAudioDecoder::newAudioBuffer);
        connect(demuxer, &Demuxer::atEnd, m_audioDecoder, &QFFmpegAudioDecoder::finished);
        m_format = format;
        audioRenderer = new SteppingAudioRenderer(this, format);
        audioRenderer->start();
        auto *stream = demuxer->addStream(m_currentAVStreamIndex[QPlatformMediaPlayer::AudioStream]);
        audioRenderer->setStream(stream);
    }

    void nextBuffer()
    {
        audioRenderer->singleStep();
        audioRenderer->unPause();
    }

Q_SIGNALS:
    void newAudioBuffer(const QAudioBuffer &b);

private:
    QFFmpegAudioDecoder *m_audioDecoder = nullptr;
    QAudioFormat m_format;
};

SteppingAudioRenderer::SteppingAudioRenderer(AudioDecoder *decoder, const QAudioFormat &format)
    : Renderer(decoder, QPlatformMediaPlayer::AudioStream)
    , m_decoder(decoder)
    , m_format(format)
{
}

void SteppingAudioRenderer::createResampler(const Codec *codec)
{
    qCDebug(qLcAudioDecoder) << "createResampler";
    AVStream *audioStream = codec->stream();

    if (!m_format.isValid()) {
        // want the native format
        m_format.setSampleFormat(QFFmpegMediaFormatInfo::sampleFormat(AVSampleFormat(audioStream->codecpar->format)));
        m_format.setSampleRate(audioStream->codecpar->sample_rate);
        m_format.setChannelConfig(QAudioFormat::ChannelConfig(audioStream->codecpar->channel_layout));
    }

    QAudioFormat::ChannelConfig config = m_format.channelConfig();
    if (config == QAudioFormat::ChannelConfigUnknown) {
        switch (m_format.channelCount()) {
        case 1:
            config = QAudioFormat::ChannelConfigMono;
            break;
        case 2:
            config = QAudioFormat::ChannelConfigStereo;
            break;
        case 3:
            config = QAudioFormat::ChannelConfig2Dot1;
            break;
        case 4:
            config = QAudioFormat::channelConfig(QAudioFormat::FrontLeft, QAudioFormat::FrontRight,
                                                 QAudioFormat::BackLeft, QAudioFormat::BackRight);
            break;
        case 5:
            config = QAudioFormat::ChannelConfigSurround5Dot0;
            break;
        case 6:
            config = QAudioFormat::ChannelConfigSurround5Dot1;
            break;
        case 7:
            config = QAudioFormat::ChannelConfigSurround7Dot0;
            break;
        case 8:
            config = QAudioFormat::ChannelConfigSurround7Dot1;
            break;
        default:
            // give up, simply use the first n channels
            config = QAudioFormat::ChannelConfig((1 << (m_format.channelCount() + 1)) - 1);
        }
    }
    AVSampleFormat requiredFormat = QFFmpegMediaFormatInfo::avSampleFormat(m_format.sampleFormat());
    qCDebug(qLcAudioDecoder) << "init resampler" << requiredFormat << audioStream->codecpar->channels;
    resampler = swr_alloc_set_opts(nullptr,  // we're allocating a new context
                                   QFFmpegMediaFormatInfo::avChannelLayout(config),  // out_ch_layout
                                   requiredFormat,    // out_sample_fmt
                                   m_format.sampleRate(),                // out_sample_rate
                                   audioStream->codecpar->channel_layout, // in_ch_layout
                                   AVSampleFormat(audioStream->codecpar->format),   // in_sample_fmt
                                   audioStream->codecpar->sample_rate,                // in_sample_rate
                                   0,                    // log_offset
                                   nullptr);
    swr_init(resampler);
}

void SteppingAudioRenderer::loop()
{
    if (!streamDecoder) {
        qCDebug(qLcAudioDecoder) << "no stream";
        timeOut = 10; // ### Fixme, this is to avoid 100% CPU load before play()
        return;
    }

    Frame frame = streamDecoder->takeFrame();
    if (!frame.isValid()) {
        timeOut = 10;
        return;
    }
    qCDebug(qLcAudioDecoder) << "    got frame";

    doneStep();

    if (!resampler)
        createResampler(frame.codec());

    qint64 startTime = frame.pts();

    int outSamples = frame.avFrame()->nb_samples;
    QByteArray samples(m_format.bytesForFrames(outSamples), Qt::Uninitialized);
    const uint8_t **in = (const uint8_t **)frame.avFrame()->extended_data;
    uint8_t *out = (uint8_t *)samples.data();
    int out_samples = swr_convert(resampler, &out, outSamples,
                                  in, frame.avFrame()->nb_samples);
    if (out_samples != outSamples)
        samples.resize(m_format.bytesForFrames(outSamples));

    QAudioBuffer buffer(samples, m_format, startTime);
    emit m_decoder->newAudioBuffer(buffer);

    paused = true;
    timeOut = -1;
}

}


QFFmpegAudioDecoder::QFFmpegAudioDecoder(QAudioDecoder *parent)
    : QPlatformAudioDecoder(parent)
{
}

QFFmpegAudioDecoder::~QFFmpegAudioDecoder()
{
    delete decoder;
}

QUrl QFFmpegAudioDecoder::source() const
{
    return m_url;
}

void QFFmpegAudioDecoder::setSource(const QUrl &fileName)
{
    stop();
    m_sourceDevice = nullptr;

    if (m_url == fileName)
        return;
    m_url = fileName;

    QByteArray url = fileName.toEncoded(QUrl::PreferLocalFile);
    AVFormatContext *context = nullptr;
    int ret = avformat_open_input(&context, url.constData(), nullptr, nullptr);
    if (ret < 0) {
        error(QMediaPlayer::AccessDeniedError, QMediaPlayer::tr("Could not open file"));
        return;
    }
    //    decoder = new QFFmpeg::Decoder(this, context);

    emit sourceChanged();
}

QIODevice *QFFmpegAudioDecoder::sourceDevice() const
{
    return m_sourceDevice;
}

void QFFmpegAudioDecoder::setSourceDevice(QIODevice *device)
{
    stop();
    m_url.clear();
    bool isSignalRequired = (m_sourceDevice != device);
    m_sourceDevice = device;
    if (isSignalRequired)
        emit sourceChanged();
}

void QFFmpegAudioDecoder::start()
{
    qCDebug(qLcAudioDecoder) << "start";
    if (decoder)
        delete decoder;
    decoder = new QFFmpeg::AudioDecoder();
    decoder->setUrl(m_url);
    decoder->setup(this, m_audioFormat);
    decoder->play();
    decoder->nextBuffer();
}

void QFFmpegAudioDecoder::stop()
{
    qCDebug(qLcAudioDecoder) << ">>>>> stop";
    if (decoder)
        decoder->stop();

    m_position = 0;
    emit positionChanged(0);
    setIsDecoding(false);
}

QAudioFormat QFFmpegAudioDecoder::audioFormat() const
{
    return m_audioFormat;
}

void QFFmpegAudioDecoder::setAudioFormat(const QAudioFormat &format)
{
    if (m_audioFormat == format)
        return;

    m_audioFormat = format;
    emit formatChanged(m_audioFormat);
}

QAudioBuffer QFFmpegAudioDecoder::read()
{
    auto b = m_audioBuffer;
    qCDebug(qLcAudioDecoder) << "reading buffer" << b.startTime();
    m_audioBuffer = {};
    bufferAvailableChanged(false);
    decoder->nextBuffer();
    return b;
}

bool QFFmpegAudioDecoder::bufferAvailable() const
{
    return m_audioBuffer.isValid();
}

qint64 QFFmpegAudioDecoder::position() const
{
    return m_position;
}

qint64 QFFmpegAudioDecoder::duration() const
{
    return m_duration;
}

void QFFmpegAudioDecoder::newAudioBuffer(const QAudioBuffer &b)
{
    qCDebug(qLcAudioDecoder) << "new audio buffer" << b.startTime();
    m_audioBuffer = b;
    m_position = b.startTime() + b.duration();
    emit positionChanged(m_position);
    bufferAvailableChanged(true);
    bufferReady();
}

QT_END_NAMESPACE

#include "qffmpegaudiodecoder.moc"
