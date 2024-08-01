// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "audiodecoder.h"

#include <QtCore/qfile.h>
#include <QtCore/qdebug.h>

AudioDecoder::AudioDecoder(bool isPlayback, bool isDelete, const QString &targetFileName)
    : m_cout(stdout, QIODevice::WriteOnly), m_targetFilename(targetFileName)
{
    m_isPlayback = isPlayback;
    m_isDelete = isDelete;

    connect(&m_decoder, &QAudioDecoder::bufferReady, this, &AudioDecoder::bufferReady);
    connect(&m_decoder, QOverload<QAudioDecoder::Error>::of(&QAudioDecoder::error), this,
            QOverload<QAudioDecoder::Error>::of(&AudioDecoder::error));
    connect(&m_decoder, &QAudioDecoder::isDecodingChanged, this, &AudioDecoder::isDecodingChanged);
    connect(&m_decoder, &QAudioDecoder::finished, this, &AudioDecoder::finished);
    connect(&m_decoder, &QAudioDecoder::positionChanged, this, &AudioDecoder::updateProgress);
    connect(&m_decoder, &QAudioDecoder::durationChanged, this, &AudioDecoder::updateProgress);

    connect(&m_soundEffect, &QSoundEffect::statusChanged, this,
            &AudioDecoder::playbackStatusChanged);
    connect(&m_soundEffect, &QSoundEffect::playingChanged, this, &AudioDecoder::playingChanged);

    m_progress = -1.0;
}

AudioDecoder::~AudioDecoder()
{
    delete m_waveDecoder;
}

void AudioDecoder::setSource(const QString &fileName)
{
    m_decoder.setSource(QUrl::fromLocalFile(fileName));
}

void AudioDecoder::start()
{
    m_decoder.start();
}

void AudioDecoder::stop()
{
    m_decoder.stop();
}

QAudioDecoder::Error AudioDecoder::getError()
{
    return m_decoder.error();
}

void AudioDecoder::setTargetFilename(const QString &fileName)
{
    m_targetFilename = fileName;
}

void AudioDecoder::bufferReady()
{
    // read a buffer from audio decoder
    QAudioBuffer buffer = m_decoder.read();
    if (!buffer.isValid())
        return;

    if (!m_waveDecoder) {
        QIODevice *target = new QFile(m_targetFilename, this);
        if (!target->open(QIODevice::WriteOnly)) {
            qWarning() << "target file is not writable";
            m_decoder.stop();
            return;
        }
        m_waveDecoder = new QWaveDecoder(target, buffer.format());
    }

    if (!m_waveDecoder
        || (!m_waveDecoder->isOpen() && !m_waveDecoder->open(QIODevice::WriteOnly))) {
        m_decoder.stop();
        return;
    }

    m_waveDecoder->write(buffer.constData<char>(), buffer.byteCount());
}

void AudioDecoder::error(QAudioDecoder::Error error)
{
    switch (error) {
    case QAudioDecoder::NoError:
        return;
    case QAudioDecoder::ResourceError:
        m_cout << "Resource error\n";
        break;
    case QAudioDecoder::FormatError:
        m_cout << "Format error\n";
        break;
    case QAudioDecoder::AccessDeniedError:
        m_cout << "Access denied error\n";
        break;
    case QAudioDecoder::NotSupportedError:
        m_cout << "Service missing error\n";
        break;
    }

    emit done();
}

void AudioDecoder::isDecodingChanged(bool isDecoding)
{
    if (isDecoding)
        m_cout << "Decoding...\n";
    else
        m_cout << "Decoding stopped\n";
}

void AudioDecoder::finished()
{
    m_waveDecoder->close();
    m_cout << "Decoding finished\n";

    if (m_isPlayback) {
        m_cout << "Starting playback\n";
        m_soundEffect.setSource(QUrl::fromLocalFile(m_targetFilename));
        m_soundEffect.play();
    } else {
        emit done();
    }
}

void AudioDecoder::playbackStatusChanged()
{
    if (m_soundEffect.status() == QSoundEffect::Error) {
        m_cout << "Playback error\n";
        emit done();
    }
}

void AudioDecoder::playingChanged()
{
    if (!m_soundEffect.isPlaying()) {
        m_cout << "Playback finished\n";
        if (m_isDelete)
            QFile::remove(m_targetFilename);
        emit done();
    }
}

void AudioDecoder::updateProgress()
{
    qint64 position = m_decoder.position();
    qint64 duration = m_decoder.duration();
    qreal progress = m_progress;
    if (position >= 0 && duration > 0)
        progress = position / (qreal)duration;

    if (progress > m_progress + 0.1) {
        m_cout << "Decoding progress: " << (int)(progress * 100.0) << "%\n";
        m_progress = progress;
    }
}

