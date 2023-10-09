// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qffmpegaudiodecoder_p.h"
#include "qffmpegresampler_p.h"
#include "qaudiobuffer.h"

#include "qffmpegplaybackengine_p.h"
#include "playbackengine/qffmpegrenderer_p.h"

#include <qloggingcategory.h>

static Q_LOGGING_CATEGORY(qLcAudioDecoder, "qt.multimedia.ffmpeg.audioDecoder")

QT_BEGIN_NAMESPACE

namespace QFFmpeg
{

class SteppingAudioRenderer : public Renderer
{
    Q_OBJECT
public:
    SteppingAudioRenderer(const QAudioFormat &format) : Renderer({}), m_format(format) { }

    RenderingResult renderInternal(Frame frame) override
    {
        if (!frame.isValid())
            return {};

        if (!m_resampler)
            m_resampler = std::make_unique<Resampler>(frame.codec(), m_format);

        emit newAudioBuffer(m_resampler->resample(frame.avFrame()));

        return {};
    }

signals:
    void newAudioBuffer(QAudioBuffer);

private:
    QAudioFormat m_format;
    std::unique_ptr<Resampler> m_resampler;
};

class AudioDecoder : public PlaybackEngine
{
    Q_OBJECT
public:
    explicit AudioDecoder(const QAudioFormat &format) : m_format(format) { }

    RendererPtr createRenderer(QPlatformMediaPlayer::TrackType trackType) override
    {
        if (trackType != QPlatformMediaPlayer::AudioStream)
            return RendererPtr{ {}, {} };

        auto result = createPlaybackEngineObject<SteppingAudioRenderer>(m_format);
        m_audioRenderer = result.get();

        connect(result.get(), &SteppingAudioRenderer::newAudioBuffer, this,
                &AudioDecoder::newAudioBuffer);

        return result;
    }

    void nextBuffer()
    {
        Q_ASSERT(m_audioRenderer);
        Q_ASSERT(!m_audioRenderer->isStepForced());

        m_audioRenderer->doForceStep();
        // updateObjectsPausedState();
    }

signals:
    void newAudioBuffer(QAudioBuffer);

private:
    QPointer<Renderer> m_audioRenderer;
    QAudioFormat m_format;
};
}


QFFmpegAudioDecoder::QFFmpegAudioDecoder(QAudioDecoder *parent)
    : QPlatformAudioDecoder(parent)
{
}

QFFmpegAudioDecoder::~QFFmpegAudioDecoder() = default;

QUrl QFFmpegAudioDecoder::source() const
{
    return m_url;
}

void QFFmpegAudioDecoder::setSource(const QUrl &fileName)
{
    stop();
    m_sourceDevice = nullptr;

    if (std::exchange(m_url, fileName) != fileName)
        sourceChanged();
}

QIODevice *QFFmpegAudioDecoder::sourceDevice() const
{
    return m_sourceDevice;
}

void QFFmpegAudioDecoder::setSourceDevice(QIODevice *device)
{
    stop();
    m_url.clear();
    if (std::exchange(m_sourceDevice, device) != device)
        sourceChanged();
}

void QFFmpegAudioDecoder::start()
{
    qCDebug(qLcAudioDecoder) << "start";
    auto checkNoError = [this]() {
        if (error() == QAudioDecoder::NoError)
            return true;

        durationChanged(-1);
        positionChanged(-1);

        m_decoder.reset();

        return false;
    };

    m_decoder = std::make_unique<AudioDecoder>(m_audioFormat);
    connect(m_decoder.get(), &AudioDecoder::errorOccured, this, &QFFmpegAudioDecoder::errorSignal);
    connect(m_decoder.get(), &AudioDecoder::endOfStream, this, &QFFmpegAudioDecoder::done);
    connect(m_decoder.get(), &AudioDecoder::newAudioBuffer, this,
            &QFFmpegAudioDecoder::newAudioBuffer);

    QFFmpeg::MediaDataHolder::Maybe media = QFFmpeg::MediaDataHolder::create(m_url, m_sourceDevice, nullptr);

    if (media)
        m_decoder->setMedia(std::move(*media.value()));
    else {
        auto [code, description] = media.error();
        errorSignal(code, description);
    }

    if (!checkNoError())
        return;

    m_decoder->setState(QMediaPlayer::PausedState);
    if (!checkNoError())
        return;

    m_decoder->nextBuffer();
    if (!checkNoError())
        return;

    durationChanged(m_decoder->duration() / 1000);
    setIsDecoding(true);
}

void QFFmpegAudioDecoder::stop()
{
    qCDebug(qLcAudioDecoder) << ">>>>> stop";
    if (m_decoder) {
        m_decoder.reset();
        done();
    }
}

QAudioFormat QFFmpegAudioDecoder::audioFormat() const
{
    return m_audioFormat;
}

void QFFmpegAudioDecoder::setAudioFormat(const QAudioFormat &format)
{
    if (std::exchange(m_audioFormat, format) != format)
        formatChanged(m_audioFormat);
}

QAudioBuffer QFFmpegAudioDecoder::read()
{
    auto buffer = std::exchange(m_audioBuffer, QAudioBuffer{});
    if (!buffer.isValid())
        return buffer;
    qCDebug(qLcAudioDecoder) << "reading buffer" << buffer.startTime();
    bufferAvailableChanged(false);
    if (m_decoder)
        m_decoder->nextBuffer();
    return buffer;
}

void QFFmpegAudioDecoder::newAudioBuffer(const QAudioBuffer &b)
{
    Q_ASSERT(b.isValid());
    Q_ASSERT(!m_audioBuffer.isValid());
    Q_ASSERT(!bufferAvailable());

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

void QFFmpegAudioDecoder::errorSignal(int err, const QString &errorString)
{
    // unfortunately the error enums for QAudioDecoder and QMediaPlayer aren't identical.
    // Map them.
    switch (QMediaPlayer::Error(err)) {
    case QMediaPlayer::NoError:
        error(QAudioDecoder::NoError, errorString);
        break;
    case QMediaPlayer::ResourceError:
        error(QAudioDecoder::ResourceError, errorString);
        break;
    case QMediaPlayer::FormatError:
        error(QAudioDecoder::FormatError, errorString);
        break;
    case QMediaPlayer::NetworkError:
        // fall through, Network error doesn't exist in QAudioDecoder
    case QMediaPlayer::AccessDeniedError:
        error(QAudioDecoder::AccessDeniedError, errorString);
        break;
    }
}

QT_END_NAMESPACE

#include "moc_qffmpegaudiodecoder_p.cpp"

#include "qffmpegaudiodecoder.moc"
