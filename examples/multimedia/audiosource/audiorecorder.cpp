// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "audiorecorder.h"

#include <QDebug>

namespace {

using namespace std::chrono_literals;
constexpr auto recordingDuration = 5s;

} // namespace

AudioRecorder::AudioRecorder(const QAudioFormat &format, QString fileName)
    : m_bytesExpected(
              format.bytesForDuration(std::chrono::microseconds(recordingDuration).count())),
      m_fileName(fileName)
{
    // Prepare WAV file
    drwav_data_format wavFormat;
    wavFormat.container = drwav_container_w64;
    wavFormat.format = (format.sampleFormat() == QAudioFormat::Float) ? DR_WAVE_FORMAT_IEEE_FLOAT
                                                                      : DR_WAVE_FORMAT_PCM;
    wavFormat.channels = format.channelCount();
    wavFormat.sampleRate = format.sampleRate();
    wavFormat.bitsPerSample = format.bytesPerSample() * 8;

    if (drwav_init_file_write(&m_wav, m_fileName.toLatin1().constData(), &wavFormat, nullptr) == 0) {
        qWarning() << "Error opening WAV file for writing:" << m_fileName;
    }
}

AudioRecorder::~AudioRecorder()
{
    auto result = drwav_uninit(&m_wav);
    if (result != DRWAV_SUCCESS)
        qWarning() << "Failed to close WAV file:" << result;
    else
        qInfo() << "Wav file written:" << m_fileName;
}

int AudioRecorder::progress(int maximum) const
{
    return maximum * qMin(bytesWritten(), m_bytesExpected) / m_bytesExpected;
}

bool AudioRecorder::isDone() const
{
    return bytesWritten() >= m_bytesExpected;
}

qint64 AudioRecorder::writeBytesToFile(QSpan<const std::byte> buffer)
{
    qint64 bytesPerFrame = m_wav.channels * m_wav.bitsPerSample / 8;
    if (bytesPerFrame == 0)
        return 0;
    qint64 framesOnBuffer = buffer.size() / bytesPerFrame;
    quint64 framesWritten = drwav_write_pcm_frames(&m_wav, framesOnBuffer, buffer.data());
    return qint64(framesWritten) * bytesPerFrame;
}

void PullRecorder::writeSpan(QSpan<const std::byte> data)
{
    m_bytesWritten += writeBytesToFile(data);
}

quint64 PullRecorder::bytesWritten() const
{
    return m_bytesWritten;
}

qint64 PushRecorder::writeData(const char *data, qint64 len)
{
    auto bytesWritten =
            writeBytesToFile(QSpan{ reinterpret_cast<const std::byte *>(data), (qsizetype)len });
    m_bytesWritten += bytesWritten;
    return bytesWritten;
}

quint64 PushRecorder::bytesWritten() const
{
    return m_bytesWritten;
}

CallbackRecorder::CallbackRecorder(const QAudioFormat &format, QString fileName)
    : AudioRecorder(format, fileName)
{
    // Pre-allocate a 5s buffer
    m_recordingBuffer.resize(m_bytesExpected, std::byte(0));
    m_recordingIterator = m_recordingBuffer.begin();
}

CallbackRecorder::~CallbackRecorder()
{
    writeBytesToFile(m_recordingBuffer);
}

void CallbackRecorder::writeSpan(QSpan<const std::byte> data) noexcept
{
    auto bytesToWrite = m_bytesExpected - bytesWritten();
    auto length = qMin(bytesToWrite, quint64(data.size()));
    if (length == 0)
        return;

    m_recordingIterator = std::copy_n(data.begin(), length, m_recordingIterator);
    m_bytesWritten.store(std::distance(m_recordingBuffer.begin(), m_recordingIterator),
                         std::memory_order_relaxed);
}

quint64 CallbackRecorder::bytesWritten() const
{
    return m_bytesWritten.load(std::memory_order_relaxed);
}

