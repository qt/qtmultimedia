// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef AUDIORECORDER_H
#define AUDIORECORDER_H

#include <QIODevice>
#include <QAudioSource>

#define DR_WAV_IMPLEMENTATION
#define DRWAV_API static
#define DRWAV_PRIVATE static
#define DR_WAV_NO_CONVERSION_API
#define DR_WAV_NO_WCHAR
#include "dr_libs/dr_wav.h"

class AudioRecorder
{
public:
    AudioRecorder(const QAudioFormat &format, QString fileName);
    virtual ~AudioRecorder();

    [[nodiscard]] virtual int progress(int maximum) const;
    [[nodiscard]] virtual bool isDone() const;

protected:
    qint64 writeBytesToFile(QSpan<const std::byte> buffer);
    [[nodiscard]] virtual quint64 bytesWritten() const = 0;

    quint64 m_bytesExpected;

private:
    drwav m_wav;
    QString m_fileName;
};

// Recorder in pull mode
class PullRecorder : public AudioRecorder
{
public:
    using AudioRecorder::AudioRecorder;

    // Call writeSpan from main thread
    // This writes directly to file
    void writeSpan(QSpan<const std::byte> data);

protected:
    quint64 bytesWritten() const override;

private:
    quint64 m_bytesWritten = 0;
};

// Recorder in push mode
class PushRecorder : public QIODevice, public AudioRecorder
{
public:
    using AudioRecorder::AudioRecorder;

protected:
    qint64 readData(char *, qint64) override { return 0; };
    qint64 writeData(const char *data, qint64 len) override;

    quint64 bytesWritten() const override;

private:
    quint64 m_bytesWritten = 0;
};

// Recorder in callback mode
class CallbackRecorder : public AudioRecorder
{
public:
    CallbackRecorder(const QAudioFormat &format, QString fileName);
    ~CallbackRecorder();

    // Call writeSpan from audio callback thread
    // This writes data to a pre-allocated buffer
    void writeSpan(QSpan<const std::byte> data) noexcept
#if defined(__has_cpp_attribute) && __has_cpp_attribute(clang::nonblocking)
            [[clang::nonblocking]]
#endif
            ;

protected:
    [[nodiscard]] quint64 bytesWritten() const override;

private:
    std::vector<std::byte> m_recordingBuffer;
    std::vector<std::byte>::iterator m_recordingIterator;
    std::atomic<quint64> m_bytesWritten = 0;
};

#endif
