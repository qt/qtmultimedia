// Copyright (C) 2016 Research In Motion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqnxaudiosource_p.h"

#include <private/qaudiohelpers_p.h>

#include <QDebug>

QT_BEGIN_NAMESPACE

QQnxAudioSource::QQnxAudioSource(QAudioDevice device, const QAudioFormat &format, QObject *parent)
    : QPlatformAudioSource(std::move(device), format, parent),
      m_audioSource(0),
      m_pcmNotifier(0),
      m_state(QAudio::StoppedState),
      m_bytesRead(0),
      m_elapsedTimeOffset(0),
      m_totalTimeValue(0),
      m_bytesAvailable(0),
      m_bufferSize(0),
      m_periodSize(0),
      m_pullMode(true)
{
}

QQnxAudioSource::~QQnxAudioSource()
{
    close();
}

void QQnxAudioSource::start(QIODevice *device)
{
    if (m_state != QAudio::StoppedState)
        close();

    if (!m_pullMode && m_audioSource)
        delete m_audioSource;

    m_pullMode = true;
    m_audioSource = device;

    if (open())
        changeState(QAudio::ActiveState, QAudio::NoError);
    else
        changeState(QAudio::StoppedState, QAudio::OpenError);
}

QIODevice *QQnxAudioSource::start()
{
    if (m_state != QAudio::StoppedState)
        close();

    if (!m_pullMode && m_audioSource)
        delete m_audioSource;

    m_pullMode = false;
    m_audioSource = new InputPrivate(this);
    m_audioSource->open(QIODevice::ReadOnly | QIODevice::Unbuffered);

    if (open()) {
        changeState(QAudio::IdleState, QAudio::NoError);
    } else {
        delete m_audioSource;
        m_audioSource = 0;

        changeState(QAudio::StoppedState, QAudio::OpenError);
    }

    return m_audioSource;
}

void QQnxAudioSource::stop()
{
    if (m_state == QAudio::StoppedState)
        return;

    changeState(QAudio::StoppedState, QAudio::NoError);
    close();
}

void QQnxAudioSource::reset()
{
    stop();
    m_bytesAvailable = 0;
}

void QQnxAudioSource::suspend()
{
    if (m_state == QAudio::StoppedState)
        return;

    snd_pcm_capture_pause(m_pcmHandle.get());

    if (m_pcmNotifier)
        m_pcmNotifier->setEnabled(false);

    changeState(QAudio::SuspendedState, QAudio::NoError);
}

void QQnxAudioSource::resume()
{
    if (m_state == QAudio::StoppedState)
        return;

    snd_pcm_capture_resume(m_pcmHandle.get());

    if (m_pcmNotifier)
        m_pcmNotifier->setEnabled(true);

    if (m_pullMode)
        changeState(QAudio::ActiveState, QAudio::NoError);
    else
        changeState(QAudio::IdleState, QAudio::NoError);
}

qsizetype QQnxAudioSource::bytesReady() const
{
    return qMax(m_bytesAvailable, 0);
}

void QQnxAudioSource::setBufferSize(qsizetype bufferSize)
{
    m_bufferSize = bufferSize;
}

qsizetype QQnxAudioSource::bufferSize() const
{
    return m_bufferSize;
}

qint64 QQnxAudioSource::processedUSecs() const
{
    return qint64(1000000) * m_format.framesForBytes(m_bytesRead) / m_format.sampleRate();
}

QAudio::State QQnxAudioSource::state() const
{
    return m_state;
}

void QQnxAudioSource::userFeed()
{
    if (m_state == QAudio::StoppedState || m_state == QAudio::SuspendedState)
        return;

    deviceReady();
}

bool QQnxAudioSource::deviceReady()
{
    if (m_pullMode) {
        // reads some audio data and writes it to QIODevice
        read(0, 0);
    } else {
        m_bytesAvailable = m_periodSize;

        // emits readyRead() so user will call read() on QIODevice to get some audio data
        if (m_audioSource != 0) {
            InputPrivate *input = qobject_cast<InputPrivate*>(m_audioSource);
            input->trigger();
        }
    }

    if (m_state != QAudio::ActiveState)
        return true;

    return true;
}

bool QQnxAudioSource::open()
{
    m_pcmHandle = QnxAudioUtils::openPcmDevice(m_audioDevice.id(), QAudioDevice::Input);

    if (!m_pcmHandle)
        return false;

    int errorCode = 0;

    // Necessary so that bytesFree() which uses the "free" member of the status struct works
    snd_pcm_plugin_set_disable(m_pcmHandle.get(), PLUGIN_MMAP);

    const std::optional<snd_pcm_channel_info_t> info = QnxAudioUtils::pcmChannelInfo(
            m_pcmHandle.get(), QAudioDevice::Input);

    if (!info) {
        close();
        return false;
    }

    snd_pcm_channel_params_t params = QnxAudioUtils::formatToChannelParams(m_format,
            QAudioDevice::Input, info->max_fragment_size);

    if ((errorCode = snd_pcm_plugin_params(m_pcmHandle.get(), &params)) < 0) {
        qWarning("QQnxAudioSource: open error, couldn't set channel params (0x%x)", -errorCode);
        close();
        return false;
    }

    if ((errorCode = snd_pcm_plugin_prepare(m_pcmHandle.get(), SND_PCM_CHANNEL_CAPTURE)) < 0) {
        qWarning("QQnxAudioSource: open error, couldn't prepare channel (0x%x)", -errorCode);
        close();
        return false;
    }

    const std::optional<snd_pcm_channel_setup_t> setup = QnxAudioUtils::pcmChannelSetup(
            m_pcmHandle.get(), QAudioDevice::Input);

    m_periodSize = qMin(2048, setup->buf.block.frag_size);

    m_elapsedTimeOffset = 0;
    m_totalTimeValue = 0;
    m_bytesRead = 0;

    m_pcmNotifier = new QSocketNotifier(snd_pcm_file_descriptor(m_pcmHandle.get(), SND_PCM_CHANNEL_CAPTURE),
                                        QSocketNotifier::Read, this);
    connect(m_pcmNotifier, SIGNAL(activated(QSocketDescriptor)), this, SLOT(userFeed()));

    return true;
}

void QQnxAudioSource::close()
{
    if (m_pcmHandle) {
#if SND_PCM_VERSION < SND_PROTOCOL_VERSION('P',3,0,2)
        snd_pcm_plugin_flush(m_pcmHandle.get(), SND_PCM_CHANNEL_CAPTURE);
#else
        snd_pcm_plugin_drop(m_pcmHandle.get(), SND_PCM_CHANNEL_CAPTURE);
#endif
        m_pcmHandle = nullptr;
    }

    if (m_pcmNotifier) {
        delete m_pcmNotifier;
        m_pcmNotifier = 0;
    }

    if (!m_pullMode && m_audioSource) {
        delete m_audioSource;
        m_audioSource = 0;
    }
}

qint64 QQnxAudioSource::read(char *data, qint64 len)
{
    if (!m_pullMode && m_bytesAvailable == 0)
        return 0;

    int errorCode = 0;
    QByteArray tempBuffer(m_periodSize, 0);

    const int actualRead = snd_pcm_plugin_read(m_pcmHandle.get(), tempBuffer.data(), m_periodSize);
    if (actualRead < 1) {
        snd_pcm_channel_status_t status;
        memset(&status, 0, sizeof(status));
        status.channel = SND_PCM_CHANNEL_CAPTURE;
        if ((errorCode = snd_pcm_plugin_status(m_pcmHandle.get(), &status)) < 0) {
            qWarning("QQnxAudioSource: read error, couldn't get plugin status (0x%x)", -errorCode);
            close();
            changeState(QAudio::StoppedState, QAudio::FatalError);
            return -1;
        }

        if (status.status == SND_PCM_STATUS_READY
            || status.status == SND_PCM_STATUS_OVERRUN) {
            if ((errorCode = snd_pcm_plugin_prepare(m_pcmHandle.get(), SND_PCM_CHANNEL_CAPTURE)) < 0) {
                qWarning("QQnxAudioSource: read error, couldn't prepare plugin (0x%x)", -errorCode);
                close();
                changeState(QAudio::StoppedState, QAudio::FatalError);
                return -1;
            }
        }
    } else {
        changeState(QAudio::ActiveState, QAudio::NoError);
    }

    if (volume() < 1.0f)
        QAudioHelperInternal::qMultiplySamples(volume(), m_format, tempBuffer.data(),
                                               tempBuffer.data(), actualRead);

    m_bytesRead += actualRead;

    if (m_pullMode) {
        m_audioSource->write(tempBuffer.data(), actualRead);
    } else {
        memcpy(data, tempBuffer.data(), qMin(static_cast<qint64>(actualRead), len));
    }

    m_bytesAvailable = 0;

    return actualRead;
}

void QQnxAudioSource::changeState(QAudio::State state, QAudio::Error error)
{
    if (m_state != state) {
        m_state = state;
        emit stateChanged(state);
    }

    setError(error);
}

InputPrivate::InputPrivate(QQnxAudioSource *audio)
    : m_audioDevice(audio)
{
}

qint64 InputPrivate::readData(char *data, qint64 len)
{
    return m_audioDevice->read(data, len);
}

qint64 InputPrivate::writeData(const char *data, qint64 len)
{
    Q_UNUSED(data);
    Q_UNUSED(len);
    return 0;
}

qint64 InputPrivate::bytesAvailable() const
{
    return m_audioDevice->m_bytesAvailable + QIODevice::bytesAvailable();
}

bool InputPrivate::isSequential() const
{
    return true;
}

void InputPrivate::trigger()
{
    emit readyRead();
}

QT_END_NAMESPACE
