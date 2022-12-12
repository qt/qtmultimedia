// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qffmpegaudioinput_p.h"
#include <qiodevice.h>
#include <qaudiosource.h>
#include <qaudiobuffer.h>
#include <qdebug.h>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

class AudioSourceIO : public QIODevice
{
    Q_OBJECT
        public:
    AudioSourceIO(QFFmpegAudioInput *audioInput)
        : QIODevice()
        , input(audioInput)
    {
        m_muted = input->muted;
        m_volume = input->volume;
        updateVolume();
        open(QIODevice::WriteOnly);
    }
    ~AudioSourceIO()
    {
        delete m_src;
    }

    void setDevice(const QAudioDevice &device)
    {
        QMutexLocker locker(&mutex);
        if (m_device == device)
            return;
        m_device = device;
        QMetaObject::invokeMethod(this, "updateSource");
    }
    void setFrameSize(int s)
    {
        QMutexLocker locker(&mutex);
        frameSize = s;
        bufferSize = m_format.bytesForFrames(frameSize);
    }
    void setRunning(bool r) {
        QMutexLocker locker(&mutex);
        if (m_running == r)
            return;
        m_running = r;
        QMetaObject::invokeMethod(this, "updateRunning");
    }

    void setVolume(float vol) {
        QMutexLocker locker(&mutex);
        m_volume = vol;
        QMetaObject::invokeMethod(this, "updateVolume");
    }
    void setMuted(bool muted) {
        QMutexLocker locker(&mutex);
        m_muted = muted;
        QMetaObject::invokeMethod(this, "updateVolume");
    }


protected:
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
            if (pcm.size() == bufferSize)
                sendBuffer();
        }

        return l;
    }

private Q_SLOTS:
    void updateSource() {
        QMutexLocker locker(&mutex);
        m_format = m_device.preferredFormat();
        if (m_src) {
            delete m_src;
            pcm.clear();
        }
        m_src = new QAudioSource(m_device, m_format);
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
        QMutexLocker locker(&mutex);
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
        qint64 time = fmt.durationForBytes(processed);
        QAudioBuffer buffer(pcm, fmt, time);
        emit input->newAudioBuffer(buffer);
        processed += bufferSize;
        pcm.clear();
    }

    QMutex mutex;
    QAudioDevice m_device;
    float m_volume = 1.;
    bool m_muted = false;
    bool m_running = false;

    QFFmpegAudioInput *input = nullptr;
    QAudioSource *m_src = nullptr;
    QAudioFormat m_format;
    int frameSize = 0;
    int bufferSize = 0;
    qint64 processed = 0;
    QByteArray pcm;
};

}

QFFmpegAudioInput::QFFmpegAudioInput(QAudioInput *qq)
    : QPlatformAudioInput(qq)
{
    qRegisterMetaType<QAudioBuffer>();

    inputThread = new QThread;
    audioIO = new QFFmpeg::AudioSourceIO(this);
    audioIO->moveToThread(inputThread);
    inputThread->start();
}

QFFmpegAudioInput::~QFFmpegAudioInput()
{
    inputThread->exit();
    inputThread->wait();
    delete inputThread;
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

QT_END_NAMESPACE

#include "moc_qffmpegaudioinput_p.cpp"

#include "qffmpegaudioinput.moc"
