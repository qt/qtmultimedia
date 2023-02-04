// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwasmaudiosink_p.h"


#include <emscripten.h>
#include <AL/al.h>
#include <AL/alc.h>
#include <QDebug>
#include <QtMath>
#include <QIODevice>

// non native al formats (AL_EXT_float32)
#define AL_FORMAT_MONO_FLOAT32                   0x10010
#define AL_FORMAT_STEREO_FLOAT32                 0x10011

constexpr unsigned int DEFAULT_BUFFER_DURATION = 50'000;

class ALData {
public:
    ALCcontext *context = nullptr;
    ALCdevice *device = nullptr;
    ALuint source;
    ALuint *buffers = nullptr;
    ALuint *buffer = nullptr;
    ALenum format;
};

QT_BEGIN_NAMESPACE

class QWasmAudioSinkDevice : public QIODevice {

    QWasmAudioSink *m_out;

public:
    QWasmAudioSinkDevice(QWasmAudioSink *parent);

protected:
    qint64 readData(char *data, qint64 maxlen) override;
    qint64 writeData(const char *data, qint64 len) override;
};

QWasmAudioSink::QWasmAudioSink(const QByteArray &device, QObject *parent)
    : QPlatformAudioSink(parent),
      m_name(device),
      m_timer(new QTimer(this))
{
    m_timer->setSingleShot(false);
    aldata = new ALData();
    connect(m_timer, &QTimer::timeout, this, [this](){
        if (m_pullMode)
            nextALBuffers();
        else {
            unloadALBuffers();
            m_device->write(nullptr, 0);
            updateState();
        }
    });
}

QWasmAudioSink::~QWasmAudioSink()
{
    delete aldata;
    if (m_tmpData)
        delete[] m_tmpData;
}

void QWasmAudioSink::start(QIODevice *device)
{
    Q_ASSERT(device);
    Q_ASSERT(device->openMode().testFlag(QIODevice::ReadOnly));
    m_device = device;
    start(true);
}

QIODevice *QWasmAudioSink::start()
{
    m_device = new QWasmAudioSinkDevice(this);
    m_device->open(QIODevice::WriteOnly);
    start(false);
    return m_device;
}

void QWasmAudioSink::start(bool mode)
{
    auto formatError = [this](){
        qWarning() << "Unsupported audio format " << m_format;
        setError(QAudio::OpenError);
    };
    switch (m_format.sampleFormat()) {
    case QAudioFormat::UInt8:
        switch (m_format.channelCount()) {
        case 1:
            aldata->format = AL_FORMAT_MONO8;
            break;
        case 2:
            aldata->format = AL_FORMAT_STEREO8;
            break;
        default:
            return formatError();
        }
        break;
    case QAudioFormat::Int16:
        switch (m_format.channelCount()) {
        case 1:
            aldata->format = AL_FORMAT_MONO16;
            break;
        case 2:
            aldata->format = AL_FORMAT_STEREO16;
            break;
        default:
            return formatError();
        }
        break;
    case QAudioFormat::Float:
        switch (m_format.channelCount()) {
        case 1:
            aldata->format = AL_FORMAT_MONO_FLOAT32;
            break;
        case 2:
            aldata->format = AL_FORMAT_STEREO_FLOAT32;
            break;
        default:
            return formatError();
        }
        break;
    default:
        return formatError();
    }

    alGetError();
    aldata->device = alcOpenDevice(m_name.data());
    if (!aldata->device) {
        qWarning() << "Failed to open audio device" << alGetString(alGetError());
        return setError(QAudio::OpenError);
    }
    ALint attrlist[] = {ALC_FREQUENCY, m_format.sampleRate(), 0};
    aldata->context = alcCreateContext(aldata->device, attrlist);

    if (!aldata->context) {
        qWarning() << "Failed to create audio context" << alGetString(alGetError());
        return setError(QAudio::OpenError);
    }
    alcMakeContextCurrent(aldata->context);

    alGenSources(1, &aldata->source);

    if (m_bufferSize > 0)
        m_bufferFragmentsCount = qMax(2,qCeil((qreal)m_bufferSize/(m_bufferFragmentSize)));
    m_bufferSize = m_bufferFragmentsCount * m_bufferFragmentSize;
    aldata->buffers = new ALuint[m_bufferFragmentsCount];
    aldata->buffer = aldata->buffers;
    alGenBuffers(m_bufferFragmentsCount, aldata->buffers);
    m_processed = 0;
    m_tmpDataOffset = 0;
    m_pullMode = mode;
    alSourcef(aldata->source, AL_GAIN, m_volume);
    if (m_pullMode)
        loadALBuffers();
    m_timer->setInterval(DEFAULT_BUFFER_DURATION / 3000);
    m_timer->start();
    if (m_pullMode)
        alSourcePlay(aldata->source);
    m_running = true;
    m_elapsedTimer.start();
    updateState();
}

void QWasmAudioSink::stop()
{
    if (!m_running)
        return;
    m_elapsedTimer.invalidate();
    alSourceStop(aldata->source);
    alSourceRewind(aldata->source);
    m_timer->stop();
    m_bufferFragmentsBusyCount = 0;
    alDeleteSources(1, &aldata->source);
    alDeleteBuffers(m_bufferFragmentsCount, aldata->buffers);
    delete[] aldata->buffers;
    alcMakeContextCurrent(nullptr);
    alcDestroyContext(aldata->context);
    alcCloseDevice(aldata->device);
    m_running = false;
    m_processed = 0;
    if (!m_pullMode)
        m_device->deleteLater();
    updateState();
}

void QWasmAudioSink::reset()
{
    stop();
    m_error = QAudio::NoError;
}

void QWasmAudioSink::suspend()
{
    if (!m_running)
        return;

    m_suspendedInState = m_state;
    alSourcePause(aldata->source);
}

void QWasmAudioSink::resume()
{
    if (!m_running)
        return;

    alSourcePlay(aldata->source);
}

int QWasmAudioSink::bytesFree() const
{
    int processed;
    alGetSourcei(aldata->source, AL_BUFFERS_PROCESSED, &processed);
    return m_running ? m_bufferFragmentSize * (m_bufferFragmentsCount - m_bufferFragmentsBusyCount
                                               + processed) : 0;
}

void QWasmAudioSink::setBufferSize(int value)
{
    if (m_running)
        return;

    m_bufferSize = value;
}

int QWasmAudioSink::bufferSize() const
{
    return m_bufferSize;
}

qint64 QWasmAudioSink::processedUSecs() const
{
    int processed;
    alGetSourcei(aldata->source, AL_BUFFERS_PROCESSED, &processed);
    return m_format.durationForBytes(m_processed + m_format.bytesForDuration(
                                         DEFAULT_BUFFER_DURATION * processed));
}

QAudio::Error QWasmAudioSink::error() const
{
    return m_error;
}

QAudio::State QWasmAudioSink::state() const
{
    if (!m_running)
        return QAudio::StoppedState;
    ALint state;
    alGetSourcei(aldata->source, AL_SOURCE_STATE, &state);
    switch (state) {
    case AL_INITIAL:
        return QAudio::IdleState;
    case AL_PLAYING:
        return QAudio::ActiveState;
    case AL_PAUSED:
        return QAudio::SuspendedState;
    case AL_STOPPED:
        return m_running ? QAudio::IdleState : QAudio::StoppedState;
    }
    return QAudio::StoppedState;
}

void QWasmAudioSink::setFormat(const QAudioFormat &fmt)
{
    if (m_running)
        return;
    m_format = fmt;
    if (m_tmpData)
        delete[] m_tmpData;
    m_bufferFragmentSize = m_format.bytesForDuration(DEFAULT_BUFFER_DURATION);
    m_bufferSize = m_bufferFragmentSize * m_bufferFragmentsCount;
    m_tmpData = new char[m_bufferFragmentSize];
}

QAudioFormat QWasmAudioSink::format() const
{
    return m_format;
}

void QWasmAudioSink::setVolume(qreal volume)
{
    if (m_volume == volume)
        return;
    m_volume = volume;
    if (m_running)
        alSourcef(aldata->source, AL_GAIN, volume);
}

qreal QWasmAudioSink::volume() const
{
    return m_volume;
}

void QWasmAudioSink::loadALBuffers()
{
    if (m_bufferFragmentsBusyCount == m_bufferFragmentsCount)
        return;

    if (m_device->bytesAvailable() == 0) {
        return;
    }

    auto size = m_device->read(m_tmpData + m_tmpDataOffset, m_bufferFragmentSize -
                               m_tmpDataOffset);
    m_tmpDataOffset += size;
    if (!m_tmpDataOffset || (m_tmpDataOffset != m_bufferFragmentSize &&
                             m_bufferFragmentsBusyCount >= m_bufferFragmentsCount * 2 / 3))
        return;

    alBufferData(*aldata->buffer, aldata->format, m_tmpData, m_tmpDataOffset,
                 m_format.sampleRate());
    m_tmpDataOffset = 0;
    alGetError();
    alSourceQueueBuffers(aldata->source, 1, aldata->buffer);
    if (alGetError())
        return;

    m_bufferFragmentsBusyCount++;
    m_processed += size;
    if (++aldata->buffer == aldata->buffers + m_bufferFragmentsCount)
        aldata->buffer = aldata->buffers;
}

void QWasmAudioSink::unloadALBuffers()
{
    int processed;
    alGetSourcei(aldata->source, AL_BUFFERS_PROCESSED, &processed);

    if (processed) {
        auto head = aldata->buffer - m_bufferFragmentsBusyCount;
        if (head < aldata->buffers) {
            if (head + processed > aldata->buffers) {
                auto batch = m_bufferFragmentsBusyCount - (aldata->buffer - aldata->buffers);
                alGetError();
                alSourceUnqueueBuffers(aldata->source, batch, head + m_bufferFragmentsCount);
                if (!alGetError()) {
                    m_bufferFragmentsBusyCount -= batch;
                    m_processed += m_bufferFragmentSize*batch;
                }
                processed -= batch;
                if (!processed)
                    return;
                head = aldata->buffers;
            } else {
                head += m_bufferFragmentsCount;
            }
        }
        alGetError();
        alSourceUnqueueBuffers(aldata->source, processed, head);
        if (!alGetError())
            m_bufferFragmentsBusyCount -= processed;
    }
}

void QWasmAudioSink::nextALBuffers()
{
    updateState();
    unloadALBuffers();
    loadALBuffers();
    ALint state;
    alGetSourcei(aldata->source, AL_SOURCE_STATE, &state);
    if (state != AL_PLAYING && m_error == QAudio::NoError)
        alSourcePlay(aldata->source);
    updateState();
}

void QWasmAudioSink::updateState()
{
    auto current = state();
    if (m_state == current)
        return;
    m_state = current;

    if (m_state == QAudio::IdleState && m_running && m_device->bytesAvailable() == 0)
        setError(QAudio::UnderrunError);

    emit stateChanged(m_state);

}

void QWasmAudioSink::setError(QAudio::Error error)
{
    if (error == m_error)
        return;
    m_error = error;
    if (error != QAudio::NoError) {
        m_timer->stop();
        alSourceRewind(aldata->source);
    }

    emit errorChanged(error);
}

QWasmAudioSinkDevice::QWasmAudioSinkDevice(QWasmAudioSink *parent)
    : QIODevice(parent),
    m_out(parent)
{
}

qint64 QWasmAudioSinkDevice::readData(char *data, qint64 maxlen)
{
    Q_UNUSED(data)
    Q_UNUSED(maxlen)
    return 0;
}


qint64 QWasmAudioSinkDevice::writeData(const char *data, qint64 len)
{
    ALint state;
    alGetSourcei(m_out->aldata->source, AL_SOURCE_STATE, &state);
    if (state != AL_INITIAL)
        m_out->unloadALBuffers();
    if (m_out->m_bufferFragmentsBusyCount < m_out->m_bufferFragmentsCount) {
        bool exceeds = m_out->m_tmpDataOffset + len > m_out->m_bufferFragmentSize;
        bool flush = m_out->m_bufferFragmentsBusyCount < m_out->m_bufferFragmentsCount * 2 / 3 ||
                m_out->m_tmpDataOffset + len >= m_out->m_bufferFragmentSize;
        const char *read;
        char *tmp = m_out->m_tmpData;
        int size = 0;
        if (m_out->m_tmpDataOffset && exceeds) {
            size = m_out->m_tmpDataOffset + len;
            tmp = new char[m_out->m_tmpDataOffset + len];
            std::memcpy(tmp, m_out->m_tmpData, m_out->m_tmpDataOffset);
        }
        if (flush && !m_out->m_tmpDataOffset) {
            read = data;
            size = len;
        } else {
            std::memcpy(tmp + m_out->m_tmpDataOffset, data, len);
            read = tmp;
            if (!exceeds) {
                m_out->m_tmpDataOffset += len;
                size = m_out->m_tmpDataOffset;
            }
        }
        m_out->m_processed += size;
        if (size && flush) {
            alBufferData(*m_out->aldata->buffer, m_out->aldata->format, read, size,
                         m_out->m_format.sampleRate());
            if (tmp && tmp != m_out->m_tmpData)
                delete[] tmp;
            m_out->m_tmpDataOffset = 0;
            alGetError();
            alSourceQueueBuffers(m_out->aldata->source, 1, m_out->aldata->buffer);
            if (alGetError())
                return 0;
            m_out->m_bufferFragmentsBusyCount++;
            if (++m_out->aldata->buffer == m_out->aldata->buffers + m_out->m_bufferFragmentsCount)
                m_out->aldata->buffer = m_out->aldata->buffers;
            if (state != AL_PLAYING)
                alSourcePlay(m_out->aldata->source);
        }
        return len;
    }
    return 0;
}

QT_END_NAMESPACE
