// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
//#define DEBUG_DECODER

#include "qffmpegaudiodecoder_p.h"
#include "qffmpegdecoder_p.h"
#include "qffmpegmediaformatinfo_p.h"
#include "qffmpegresampler_p.h"
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
    }

    void loop() override;
    AudioDecoder *m_decoder;
    QAudioFormat m_format;
    std::unique_ptr<Resampler> resampler;
    bool atEndEmitted = false;
};

class AudioDecoder : public Decoder
{
    Q_OBJECT
public:
    explicit AudioDecoder(QFFmpegAudioDecoder *audioDecoder)
        : Decoder(audioDecoder)
    {}

    void setup(const QAudioFormat &format)
    {
        connect(this, &AudioDecoder::newAudioBuffer, audioDecoder, &QFFmpegAudioDecoder::newAudioBuffer);
        connect(this, &AudioDecoder::isAtEnd, audioDecoder, &QFFmpegAudioDecoder::done);
        m_format = format;
        audioRenderer = new SteppingAudioRenderer(this, format);
        audioRenderer->start();
        auto *stream = demuxer->addStream(m_currentAVStreamIndex[QPlatformMediaPlayer::AudioStream]);
        audioRenderer->setStream(stream);
    }

    void nextBuffer()
    {
        audioRenderer->setPaused(false);
    }

Q_SIGNALS:
    void newAudioBuffer(const QAudioBuffer &b);
    void isAtEnd();

private:
    QAudioFormat m_format;
};

SteppingAudioRenderer::SteppingAudioRenderer(AudioDecoder *decoder, const QAudioFormat &format)
    : Renderer(QPlatformMediaPlayer::AudioStream)
    , m_decoder(decoder)
    , m_format(format)
{
}


void SteppingAudioRenderer::loop()
{
    if (!streamDecoder) {
        qCDebug(qLcAudioDecoder) << "no stream";
        timeOut = -1; // Avoid CPU load before play()
        return;
    }

    Frame frame = streamDecoder->takeFrame();
    if (!frame.isValid()) {
        if (streamDecoder->isAtEnd()) {
            if (!atEndEmitted)
                emit m_decoder->isAtEnd();
            atEndEmitted = true;
            paused = true;
            doneStep();
            timeOut = -1;
            return;
        }
        timeOut = 10;
        streamDecoder->wake();
        return;
    }
    qCDebug(qLcAudioDecoder) << "    got frame";

    doneStep();

    if (!resampler)
        resampler.reset(new Resampler(frame.codec(), m_format));

    auto buffer = resampler->resample(frame.avFrame());
    paused = true;
    timeOut = -1;

    emit m_decoder->newAudioBuffer(buffer);
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
        sourceChanged();
}

void QFFmpegAudioDecoder::start()
{
    qCDebug(qLcAudioDecoder) << "start";
    delete decoder;
    decoder = new QFFmpeg::AudioDecoder(this);
    decoder->setMedia(m_url, m_sourceDevice);
    if (error() != QAudioDecoder::NoError)
        goto error;

    decoder->setup(m_audioFormat);
    if (error() != QAudioDecoder::NoError)
        goto error;
    decoder->play();
    if (error() != QAudioDecoder::NoError)
        goto error;
    decoder->nextBuffer();
    if (error() != QAudioDecoder::NoError)
        goto error;

    setIsDecoding(true);
    return;

  error:
    durationChanged(-1);
    positionChanged(-1);
    delete decoder;
    decoder = nullptr;

}

void QFFmpegAudioDecoder::stop()
{
    qCDebug(qLcAudioDecoder) << ">>>>> stop";
    if (decoder) {
        decoder->stop();
        done();
    }
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
    formatChanged(m_audioFormat);
}

QAudioBuffer QFFmpegAudioDecoder::read()
{
    auto b = m_audioBuffer;
    qCDebug(qLcAudioDecoder) << "reading buffer" << b.startTime();
    m_audioBuffer = {};
    bufferAvailableChanged(false);
    if (decoder)
        decoder->nextBuffer();
    return b;
}

void QFFmpegAudioDecoder::newAudioBuffer(const QAudioBuffer &b)
{
    qCDebug(qLcAudioDecoder) << "new audio buffer" << b.startTime();
    m_audioBuffer = b;
    const qint64 pos = b.startTime();
    positionChanged(pos/1000);
    bufferAvailableChanged(b.isValid());
    bufferReady();
}

void QFFmpegAudioDecoder::done()
{
    qCDebug(qLcAudioDecoder) << ">>>>> DONE!";
    finished();
}

QT_END_NAMESPACE

#include "qffmpegaudiodecoder.moc"
