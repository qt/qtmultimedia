// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qffmpegaudioinput_p.h"
#include <qiodevice.h>
#include <qaudiosource.h>
#include <qaudiobuffer.h>
#include <qatomic.h>
#include <qdebug.h>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

class AudioSourceIO : public QIODevice
{
    Q_OBJECT
public:
    AudioSourceIO(QFFmpegAudioInput *audioInput) : QIODevice(), m_input(audioInput)
    {
        m_muted = m_input->muted;
        m_volume = m_input->volume;
        updateVolume();
        open(QIODevice::WriteOnly);
    }

    ~AudioSourceIO() override = default;

    void setDevice(const QAudioDevice &device)
    {
        QMutexLocker locker(&m_mutex);
        if (m_device == device)
            return;
        m_device = device;
        QMetaObject::invokeMethod(this, "updateSource");
    }
    void setFrameSize(int frameSize)
    {
        m_bufferSize.storeRelease(frameSize > 0 ? m_format.bytesForFrames(frameSize)
                                                : DefaultAudioInputBufferSize);
    }
    void setRunning(bool r) {
        QMutexLocker locker(&m_mutex);
        if (m_running == r)
            return;
        m_running = r;
        QMetaObject::invokeMethod(this, "updateRunning");
    }

    void setVolume(float vol) {
        QMutexLocker locker(&m_mutex);
        m_volume = vol;
        QMetaObject::invokeMethod(this, "updateVolume");
    }
    void setMuted(bool muted) {
        QMutexLocker locker(&m_mutex);
        m_muted = muted;
        QMetaObject::invokeMethod(this, "updateVolume");
    }

    int bufferSize() const { return m_bufferSize.loadAcquire(); }

protected:
    qint64 readData(char *, qint64) override
    {
        return 0;
    }
    qint64 writeData(const char *data, qint64 len) override
    {
        int l = len;
        while (len > 0) {
            const auto bufferSize = m_bufferSize.loadAcquire();
            int toAppend = qMin(len, bufferSize - m_pcm.size());
            m_pcm.append(data, toAppend);
            data += toAppend;
            len -= toAppend;
            if (m_pcm.size() == bufferSize)
                sendBuffer();
        }

        return l;
    }

private Q_SLOTS:
    void updateSource() {
        QMutexLocker locker(&m_mutex);
        m_format = m_device.preferredFormat();
        if (std::exchange(m_src, nullptr))
            m_pcm.clear();

        m_src = std::make_unique<QAudioSource>(m_device, m_format);
        updateVolume();
        if (m_running)
            m_src->start(this);
    }
    void updateVolume()
    {
        if (m_src)
            m_src->setVolume(m_muted ? 0. : m_volume);
    }
    void updateRunning()
    {
        QMutexLocker locker(&m_mutex);
        if (m_running) {
            if (!m_src)
                updateSource();
            m_src->start(this);
        } else {
            m_src->stop();
        }
    }

private:

    void sendBuffer()
    {
        QAudioFormat fmt = m_src->format();
        qint64 time = fmt.durationForBytes(m_processed);
        QAudioBuffer buffer(m_pcm, fmt, time);
        emit m_input->newAudioBuffer(buffer);
        m_processed += m_pcm.size();
        m_pcm.clear();
    }

    QMutex m_mutex;
    QAudioDevice m_device;
    float m_volume = 1.;
    bool m_muted = false;
    bool m_running = false;

    QFFmpegAudioInput *m_input = nullptr;
    std::unique_ptr<QAudioSource> m_src;
    QAudioFormat m_format;
    QAtomicInt m_bufferSize = DefaultAudioInputBufferSize;
    qint64 m_processed = 0;
    QByteArray m_pcm;
};

}

QFFmpegAudioInput::QFFmpegAudioInput(QAudioInput *qq)
    : QPlatformAudioInput(qq)
{
    qRegisterMetaType<QAudioBuffer>();

    inputThread = std::make_unique<QThread>();
    audioIO = std::make_unique<QFFmpeg::AudioSourceIO>(this);
    audioIO->moveToThread(inputThread.get());
    inputThread->start();
}

QFFmpegAudioInput::~QFFmpegAudioInput()
{
    inputThread->exit();
    inputThread->wait();
}

void QFFmpegAudioInput::setAudioDevice(const QAudioDevice &device)
{
    audioIO->setDevice(device);
}

void QFFmpegAudioInput::setMuted(bool muted)
{
    audioIO->setMuted(muted);
}

void QFFmpegAudioInput::setVolume(float volume)
{
    audioIO->setVolume(volume);
}

void QFFmpegAudioInput::setFrameSize(int s)
{
    audioIO->setFrameSize(s);
}

void QFFmpegAudioInput::setRunning(bool b)
{
    audioIO->setRunning(b);
}

int QFFmpegAudioInput::bufferSize() const
{
    return audioIO->bufferSize();
}

QT_END_NAMESPACE

#include "moc_qffmpegaudioinput_p.cpp"

#include "qffmpegaudioinput.moc"
