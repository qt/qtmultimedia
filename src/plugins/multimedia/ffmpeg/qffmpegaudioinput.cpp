// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qffmpegaudioinput_p.h"

#include <QtCore/qatomic.h>
#include <QtCore/qdebug.h>
#include <QtCore/qiodevice.h>
#include <QtCore/qmetaobject.h>
#include <QtMultimedia/qaudiobuffer.h>
#include <QtMultimedia/qaudiosource.h>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

class AudioSourceIO : public QIODevice
{
    Q_OBJECT
public:
    AudioSourceIO(QFFmpegAudioInput *audioInput) : m_input(audioInput)
    {
        m_muted = m_input->muted;
        m_volume = m_input->volume;
        updateVolume();
        open(QIODevice::WriteOnly);
    }

    ~AudioSourceIO() override
    {
        // QAudioSource may invoke QIODevice::writeData in the destructor.
        // Let's reset the audio source to get around the case.
        if (m_audioSource)
            m_audioSource->reset();
    }

    void setDevice(const QAudioDevice &device)
    {
        Q_ASSERT(!thread()->isCurrentThread());
        QMutexLocker locker(&m_mutex);
        if (m_device == device)
            return;
        m_device = device;
        QMetaObject::invokeMethod(this, [this] {
            QMutexLocker locker(&m_mutex);
            updateSource(locker);
        });
    }
    void setBufferSize(int bufferSize)
    {
        m_bufferSize.storeRelease((bufferSize > 0 && m_format.isValid())
                                          ? m_format.bytesForFrames(bufferSize)
                                          : DefaultAudioInputBufferSize);
    }
    void setRunning(bool r) {
        Q_ASSERT(!thread()->isCurrentThread());
        QMutexLocker locker(&m_mutex);
        if (m_running == r)
            return;
        m_running = r;
        QMetaObject::invokeMethod(this, &AudioSourceIO::updateRunning);
    }

    void setVolume(float vol) {
        Q_ASSERT(!thread()->isCurrentThread());
        QMutexLocker locker(&m_mutex);
        m_volume = vol;
        QMetaObject::invokeMethod(this, &AudioSourceIO::updateVolume);
    }
    void setMuted(bool muted) {
        Q_ASSERT(!thread()->isCurrentThread());
        QMutexLocker locker(&m_mutex);
        m_muted = muted;
        QMetaObject::invokeMethod(this, &AudioSourceIO::updateVolume);
    }

    int bufferSize() const { return m_bufferSize.loadAcquire(); }

protected:
    qint64 readData(char *, qint64) override
    {
        return 0;
    }
    qint64 writeData(const char *data, qint64 len) override
    {
        Q_ASSERT(m_audioSource);

        int l = len;
        while (len > 0) {
            const auto bufferSize = m_bufferSize.loadAcquire();

            while (m_pcm.size() > bufferSize) {
                // bufferSize has been reduced. Send data until m_pcm
                // can hold more data.
                sendBuffer(m_pcm.first(bufferSize));
                m_pcm.remove(0, bufferSize);
            }

            // Size of m_pcm is always <= bufferSize
            int toAppend = qMin(len, bufferSize - m_pcm.size());
            m_pcm.append(data, toAppend);
            data += toAppend;
            len -= toAppend;
            if (m_pcm.size() == bufferSize) {
                sendBuffer(m_pcm);
                m_pcm.clear();
            }
        }

        return l;
    }

private Q_SLOTS:
    void updateVolume()
    {
        if (m_audioSource)
            m_audioSource->setVolume(m_muted ? 0. : m_volume);
    }
    void updateRunning()
    {
        QMutexLocker locker(&m_mutex);
        if (m_running) {
            if (!m_audioSource)
                updateSource(locker);
            else
                m_audioSource->start(this);
        } else {
            m_audioSource->stop();
        }
    }

private:
    void updateSource(const QMutexLocker<QMutex> &guard)
    {
        Q_ASSERT(guard.mutex() == &m_mutex);
        m_format = m_device.preferredFormat();
        if (std::exchange(m_audioSource, nullptr))
            m_pcm.clear();

        m_audioSource = std::make_unique<QAudioSource>(m_device, m_format);
        updateVolume();
        if (m_running)
            m_audioSource->start(this);
    }

    void sendBuffer(const QByteArray &pcmData)
    {
        QAudioFormat fmt = m_audioSource->format();
        qint64 time = fmt.durationForBytes(m_processed);
        QAudioBuffer buffer(pcmData, fmt, time);
        emit m_input->newAudioBuffer(buffer);
        m_processed += pcmData.size();
    }

    QMutex m_mutex;
    QAudioDevice m_device;
    float m_volume = 1.;
    bool m_muted = false;
    bool m_running = false;

    QFFmpegAudioInput *m_input = nullptr;
    std::unique_ptr<QAudioSource> m_audioSource;
    QAudioFormat m_format;
    QAtomicInt m_bufferSize = DefaultAudioInputBufferSize;
    qint64 m_processed = 0;
    QByteArray m_pcm;
};

} // namespace QFFmpeg

QFFmpegAudioInput::QFFmpegAudioInput(QAudioInput *qq)
    : QPlatformAudioInput(qq)
{
    qRegisterMetaType<QAudioBuffer>();

    m_inputThread = std::make_unique<QThread>();
    m_inputThread->setObjectName(QStringLiteral("QFFmpegAudioInputThread"));
    m_audioIO = new QFFmpeg::AudioSourceIO(this);
    m_audioIO->moveToThread(m_inputThread.get());
    m_inputThread->start();
}

QFFmpegAudioInput::~QFFmpegAudioInput()
{
    // Ensure that COM is uninitialized by nested QWindowsResampler
    // on the same thread that initialized it.
    m_audioIO->deleteLater();
    m_inputThread->exit();
    m_inputThread->wait();
}

void QFFmpegAudioInput::setAudioDevice(const QAudioDevice &device)
{
    m_audioIO->setDevice(device);
}

void QFFmpegAudioInput::setMuted(bool muted)
{
    m_audioIO->setMuted(muted);
}

void QFFmpegAudioInput::setVolume(float volume)
{
    m_audioIO->setVolume(volume);
}

void QFFmpegAudioInput::setBufferSize(int bufferSize)
{
    m_audioIO->setBufferSize(bufferSize);
}

int QFFmpegAudioInput::bufferSize() const
{
    return m_audioIO->bufferSize();
}

void QFFmpegAudioInput::connectNotify(const QMetaMethod &signal)
{
    // threading considerations:
    // AudioSourceIO::setRunning doesn't reenter
    // the internal QObject's mutex of the audio input instance
    if (signal == QMetaMethod::fromSignal(&QFFmpegAudioInput::newAudioBuffer))
        m_audioIO->setRunning(true);
}

void QFFmpegAudioInput::disconnectNotify(const QMetaMethod &signal)
{
    if (!signal.isValid()
        || signal == QMetaMethod::fromSignal(&QFFmpegAudioInput::newAudioBuffer)) {
        auto stopIOifNeeded = [this]() {
            // if the signal disconnectNotify is not
            if (!isSignalConnected(QMetaMethod::fromSignal(&QFFmpegAudioInput::newAudioBuffer)))
                m_audioIO->setRunning(false);
        };

        // threading considerations:
        // QMetaObject::invokeMethod doesn't reenter
        // the internal QObject's mutex of the audio input instance.
        // Instead, QMetaObject::invokeMethod locks ThreadData::postEventMutex

        // postpone update to avoid redundant QAudioSource restarts upon reconnection
        QMetaObject::invokeMethod(this, stopIOifNeeded, Qt::QueuedConnection);
    }
}

QT_END_NAMESPACE

#include "moc_qffmpegaudioinput_p.cpp"

#include "qffmpegaudioinput.moc"
