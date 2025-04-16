// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwasmaudiosource_p.h"

#include <emscripten.h>
#include <AL/al.h>
#include <AL/alc.h>
#include <QDataStream>
#include <QDebug>
#include <QtMath>
#include <private/qaudiohelpers_p.h>
#include <QIODevice>

QT_BEGIN_NAMESPACE

#define AL_FORMAT_MONO_FLOAT32                   0x10010
#define AL_FORMAT_STEREO_FLOAT32                 0x10011

constexpr unsigned int DEFAULT_BUFFER_DURATION = 50'000;

class QWasmAudioSourceDevice : public QIODevice
{
    QWasmAudioSource *m_in;

public:
    explicit QWasmAudioSourceDevice(QWasmAudioSource *in);

protected:
    qint64 readData(char *data, qint64 maxlen) override;
    qint64 writeData(const char *data, qint64 len) override;
};

class ALData {
public:
    ALCdevice *device = nullptr;
    ALCcontext *context = nullptr;
};

void QWasmAudioSource::writeBuffer()
{
    int samples = 0;
    alcGetIntegerv(aldata->device, ALC_CAPTURE_SAMPLES, 1, &samples);
    samples = qMin(samples, m_format.framesForBytes(m_bufferSize));
    auto bytes = m_format.bytesForFrames(samples);
    auto err = alcGetError(aldata->device);
    alcCaptureSamples(aldata->device, m_tmpData, samples);
    err = alcGetError(aldata->device);
    if (err) {
        qWarning() << alcGetString(aldata->device, err);
        return setError(QAudio::FatalError);
    }
    if (volume() < 1)
        QAudioHelperInternal::qMultiplySamples(volume(), m_format, m_tmpData, m_tmpData, bytes);
    m_processed += bytes;
    m_device->write(m_tmpData,bytes);
}

QWasmAudioSource::QWasmAudioSource(QAudioDevice device, const QAudioFormat &fmt, QObject *parent)
    : QPlatformAudioSource(std::move(device), fmt, parent), m_timer(new QTimer(this))
{
    if (device.id().contains("Emscripten")) {
        aldata = new ALData();
        connect(m_timer, &QTimer::timeout, this, [this](){
            Q_ASSERT(m_running);
            if (m_pullMode)
                writeBuffer();
            else if (bytesReady() > 0)
                emit m_device->readyRead();
        });
    }
    m_bufferSize = m_format.bytesForDuration(DEFAULT_BUFFER_DURATION);
}

void QWasmAudioSource::start(QIODevice *device)
{
    m_device = device;
    start(true);
}

QIODevice *QWasmAudioSource::start()
{
    m_device = new QWasmAudioSourceDevice(this);
    m_device->open(QIODevice::ReadOnly);
    start(false);
    return m_device;
}

void QWasmAudioSource::start(bool mode)
{
    m_pullMode = mode;
    auto formatError = [this](){
        qWarning() << "Unsupported audio format " << m_format;
        setError(QAudio::OpenError);
    };
    ALCenum format;
    switch (m_format.sampleFormat()) {
    case QAudioFormat::UInt8:
        switch (m_format.channelCount()) {
        case 1:
            format = AL_FORMAT_MONO8;
            break;
        case 2:
            format = AL_FORMAT_STEREO8;
            break;
        default:
            return formatError();
        }
        break;
    case QAudioFormat::Int16:
        switch (m_format.channelCount()) {
        case 1:
            format = AL_FORMAT_MONO16;
            break;
        case 2:
            format = AL_FORMAT_STEREO16;
            break;
        default:
            return formatError();
        }
        break;
    case QAudioFormat::Float:
        switch (m_format.channelCount()) {
        case 1:
            format = AL_FORMAT_MONO_FLOAT32;
            break;
        case 2:
            format = AL_FORMAT_STEREO_FLOAT32;
            break;
        default:
            return formatError();
        }
        break;
    default:
        return formatError();
    }
    if (m_tmpData)
        delete[] m_tmpData;
    if (m_pullMode)
        m_tmpData = new char[m_bufferSize];
    else
        m_tmpData = nullptr;
    m_timer->setInterval(m_format.durationForBytes(m_bufferSize) / 3000);
    m_timer->start();

    alcGetError(aldata->device); // clear error state
    aldata->device = alcCaptureOpenDevice(m_audioDevice.id().data(), m_format.sampleRate(), format,
                                          m_format.framesForBytes(m_bufferSize));

    auto err = alcGetError(aldata->device);
    if (err) {
        qWarning() << "alcCaptureOpenDevice" << alcGetString(aldata->device, err);
        return setError(QAudio::OpenError);
    }
    alcCaptureStart(aldata->device);
    m_elapsedTimer.start();
    auto cerr = alcGetError(aldata->device);
    if (cerr) {
        qWarning() << "alcCaptureStart" << alcGetString(aldata->device, cerr);
        return setError(QAudio::OpenError);
    }
    m_processed = 0;
    m_running = true;
}

void QWasmAudioSource::stop()
{
    if (m_pullMode)
        writeBuffer();
    alcCaptureStop(aldata->device);
    alcCaptureCloseDevice(aldata->device);
    m_elapsedTimer.invalidate();
    if (m_tmpData) {
        delete[] m_tmpData;
        m_tmpData = nullptr;
    }
    if (!m_pullMode)
        m_device->deleteLater();
    m_timer->stop();
    m_running = false;
}

void QWasmAudioSource::reset()
{
    stop();
    if (m_tmpData) {
        delete[] m_tmpData;
        m_tmpData = nullptr;
    }
    m_running = false;
    m_processed = 0;
    setError(QAudio::NoError);
}

void QWasmAudioSource::suspend()
{
    if (!m_running)
        return;

    m_suspended = true;
    alcCaptureStop(aldata->device);
}

void QWasmAudioSource::resume()
{
    if (!m_running)
        return;

    m_suspended = false;
    alcCaptureStart(aldata->device);
}

qsizetype QWasmAudioSource::bytesReady() const
{
    if (!m_running)
        return 0;
    int samples;
    alcGetIntegerv(aldata->device, ALC_CAPTURE_SAMPLES, 1, &samples);
    return m_format.bytesForFrames(samples);
}

void QWasmAudioSource::setBufferSize(qsizetype value)
{
    if (!m_running)
        return;
    m_bufferSize = value;
}

qsizetype QWasmAudioSource::bufferSize() const
{
    return m_bufferSize;
}

qint64 QWasmAudioSource::processedUSecs() const
{
    return m_format.durationForBytes(m_processed);
}

QAudio::State QWasmAudioSource::state() const
{
    if (m_running)
        return QAudio::ActiveState;
    else
        return QAudio::StoppedState;
}

QWasmAudioSourceDevice::QWasmAudioSourceDevice(QWasmAudioSource *in) : QIODevice(in), m_in(in)
{

}

qint64 QWasmAudioSourceDevice::readData(char *data, qint64 maxlen)
{
    int samples;
    alcGetIntegerv(m_in->aldata->device, ALC_CAPTURE_SAMPLES, 1, &samples);
    samples = qMin(samples, m_in->m_format.framesForBytes(maxlen));
    auto bytes = m_in->m_format.bytesForFrames(samples);
    alcGetError(m_in->aldata->device);
    alcCaptureSamples(m_in->aldata->device, data, samples);
    if (m_in->volume() < 1)
        QAudioHelperInternal::qMultiplySamples(m_in->volume(), m_in->m_format, data, data, bytes);
    auto err = alcGetError(m_in->aldata->device);
    if (err) {
        qWarning() << alcGetString(m_in->aldata->device, err);
        m_in->setError(QAudio::FatalError);
        return 0;
    }
    m_in->m_processed += bytes;
    return bytes;
}

qint64 QWasmAudioSourceDevice::writeData(const char *data, qint64 len)
{
    Q_UNREACHABLE();
    Q_UNUSED(data);
    Q_UNUSED(len);
    return 0;
}

QT_END_NAMESPACE
