// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/qcoreapplication.h>
#include <QtCore/qdebug.h>
#include <QtCore/qmath.h>
#include <private/qaudiohelpers_p.h>

#include "qpulseaudiosource_p.h"
#include "qaudioengine_pulse_p.h"
#include "qpulseaudiodevice_p.h"
#include "qpulsehelpers_p.h"
#include <sys/types.h>
#include <unistd.h>
#include <mutex> // for lock_guard

QT_BEGIN_NAMESPACE

const int SourcePeriodTimeMs = 50;

static void inputStreamReadCallback(pa_stream *stream, size_t length, void *userdata)
{
    Q_UNUSED(userdata);
    Q_UNUSED(length);
    Q_UNUSED(stream);
    QPulseAudioEngine *pulseEngine = QPulseAudioEngine::instance();
    pa_threaded_mainloop_signal(pulseEngine->mainloop(), 0);
}

static void inputStreamStateCallback(pa_stream *stream, void *userdata)
{
    using namespace QPulseAudioInternal;

    Q_UNUSED(userdata);
    pa_stream_state_t state = pa_stream_get_state(stream);
    qCDebug(qLcPulseAudioIn) << "Stream state: " << state;
    switch (state) {
    case PA_STREAM_CREATING:
        break;
    case PA_STREAM_READY:
        if (Q_UNLIKELY(qLcPulseAudioIn().isEnabled(QtDebugMsg))) {
            QPulseAudioSource *audioInput = static_cast<QPulseAudioSource *>(userdata);
            const pa_buffer_attr *buffer_attr = pa_stream_get_buffer_attr(stream);
            qCDebug(qLcPulseAudioIn) << "*** maxlength: " << buffer_attr->maxlength;
            qCDebug(qLcPulseAudioIn) << "*** prebuf: " << buffer_attr->prebuf;
            qCDebug(qLcPulseAudioIn) << "*** fragsize: " << buffer_attr->fragsize;
            qCDebug(qLcPulseAudioIn) << "*** minreq: " << buffer_attr->minreq;
            qCDebug(qLcPulseAudioIn) << "*** tlength: " << buffer_attr->tlength;

            pa_sample_spec spec =
                    QPulseAudioInternal::audioFormatToSampleSpec(audioInput->format());
            qCDebug(qLcPulseAudioIn)
                    << "*** bytes_to_usec: " << pa_bytes_to_usec(buffer_attr->fragsize, &spec);
        }
        break;
    case PA_STREAM_TERMINATED:
        break;
    case PA_STREAM_FAILED:
    default:
        qWarning() << "Stream error: " << currentError(stream);
        QPulseAudioEngine *pulseEngine = QPulseAudioEngine::instance();
        pa_threaded_mainloop_signal(pulseEngine->mainloop(), 0);
        break;
    }
}

static void inputStreamUnderflowCallback(pa_stream *stream, void *userdata)
{
    Q_UNUSED(userdata);
    Q_UNUSED(stream);
    qWarning() << "Got a buffer underflow!";
}

static void inputStreamOverflowCallback(pa_stream *stream, void *userdata)
{
    Q_UNUSED(stream);
    Q_UNUSED(userdata);
    qWarning() << "Got a buffer overflow!";
}

static void inputStreamSuccessCallback(pa_stream *stream, int success, void *userdata)
{
    Q_UNUSED(stream);
    Q_UNUSED(userdata);
    Q_UNUSED(success);

    // if (!success)
    // TODO: Is cork success?  i->operation_success = success;

    QPulseAudioEngine *pulseEngine = QPulseAudioEngine::instance();
    pa_threaded_mainloop_signal(pulseEngine->mainloop(), 0);
}

QPulseAudioSource::QPulseAudioSource(const QByteArray &device, QObject *parent)
    : QPlatformAudioSource(parent),
      m_totalTimeValue(0),
      m_audioSource(nullptr),
      m_volume(qreal(1.0f)),
      m_pullMode(true),
      m_opened(false),
      m_bufferSize(0),
      m_periodSize(0),
      m_periodTime(SourcePeriodTimeMs),
      m_stream(nullptr),
      m_device(device),
      m_stateMachine(*this)
{
}

QPulseAudioSource::~QPulseAudioSource()
{
    // TODO: Investigate draining the stream
    if (auto notifier = m_stateMachine.stop())
        close();
}

QAudio::Error QPulseAudioSource::error() const
{
    return m_stateMachine.error();
}

QAudio::State QPulseAudioSource::state() const
{
    return m_stateMachine.state();
}

void QPulseAudioSource::setFormat(const QAudioFormat &format)
{
    if (!m_stateMachine.isActiveOrIdle())
        m_format = format;
}

QAudioFormat QPulseAudioSource::format() const
{
    return m_format;
}

void QPulseAudioSource::start(QIODevice *device)
{
    reset();

    if (!open())
        return;

    m_pullMode = true;
    m_audioSource = device;

    m_stateMachine.start();
}

QIODevice *QPulseAudioSource::start()
{
    reset();

    if (!open())
        return nullptr;

    m_pullMode = false;
    m_audioSource = new PulseInputPrivate(this);
    m_audioSource->open(QIODevice::ReadOnly | QIODevice::Unbuffered);

    m_stateMachine.start(false);

    return m_audioSource;
}

void QPulseAudioSource::stop()
{
    if (auto notifier = m_stateMachine.stop())
        close();
}

bool QPulseAudioSource::open()
{
    if (m_opened)
        return true;

    QPulseAudioEngine *pulseEngine = QPulseAudioEngine::instance();

    if (!pulseEngine->context()
        || pa_context_get_state(pulseEngine->context()) != PA_CONTEXT_READY) {
        m_stateMachine.stopOrUpdateError(QAudio::FatalError);
        return false;
    }

    pa_sample_spec spec = QPulseAudioInternal::audioFormatToSampleSpec(m_format);
    pa_channel_map channel_map = QPulseAudioInternal::channelMapForAudioFormat(m_format);
    Q_ASSERT(spec.channels == channel_map.channels);

    if (!pa_sample_spec_valid(&spec)) {
        m_stateMachine.stopOrUpdateError(QAudio::OpenError);
        return false;
    }

    m_spec = spec;

    //if (Q_UNLIKELY(qLcPulseAudioIn().isEnabled(QtDebugMsg)) {
    //    QTime now(QTime::currentTime());
    //    qCDebug(qLcPulseAudioIn) << now.second() << "s " << now.msec() << "ms :open()";
    //}

    if (m_streamName.isNull())
        m_streamName =
                QStringLiteral("QtmPulseStream-%1-%2").arg(::getpid()).arg(quintptr(this)).toUtf8();

    if (Q_UNLIKELY(qLcPulseAudioIn().isEnabled(QtDebugMsg))) {
        qCDebug(qLcPulseAudioIn) << "Format: " << spec.format;
        qCDebug(qLcPulseAudioIn) << "Rate: " << spec.rate;
        qCDebug(qLcPulseAudioIn) << "Channels: " << spec.channels;
        qCDebug(qLcPulseAudioIn) << "Frame size: " << pa_frame_size(&spec);
    }

    pulseEngine->lock();

    m_stream = pa_stream_new(pulseEngine->context(), m_streamName.constData(), &spec, &channel_map);

    pa_stream_set_state_callback(m_stream, inputStreamStateCallback, this);
    pa_stream_set_read_callback(m_stream, inputStreamReadCallback, this);

    pa_stream_set_underflow_callback(m_stream, inputStreamUnderflowCallback, this);
    pa_stream_set_overflow_callback(m_stream, inputStreamOverflowCallback, this);

    m_periodSize = pa_usec_to_bytes(SourcePeriodTimeMs * 1000, &spec);

    int flags = 0;
    pa_buffer_attr buffer_attr;
    buffer_attr.maxlength = static_cast<uint32_t>(-1);
    buffer_attr.prebuf = static_cast<uint32_t>(-1);
    buffer_attr.tlength = static_cast<uint32_t>(-1);
    buffer_attr.minreq = static_cast<uint32_t>(-1);
    flags |= PA_STREAM_ADJUST_LATENCY;

    if (m_bufferSize > 0)
        buffer_attr.fragsize = static_cast<uint32_t>(m_bufferSize);
    else
        buffer_attr.fragsize = static_cast<uint32_t>(m_periodSize);

    flags |= PA_STREAM_AUTO_TIMING_UPDATE | PA_STREAM_INTERPOLATE_TIMING;

    int connectionResult = pa_stream_connect_record(m_stream, m_device.data(), &buffer_attr,
                                                    static_cast<pa_stream_flags_t>(flags));
    if (connectionResult < 0) {
        qWarning() << "pa_stream_connect_record() failed!";
        pa_stream_unref(m_stream);
        m_stream = nullptr;
        pulseEngine->unlock();
        m_stateMachine.stopOrUpdateError(QAudio::OpenError);
        return false;
    }

    //if (Q_UNLIKELY(qLcPulseAudioIn().isEnabled(QtDebugMsg))) {
    //    auto *ss = pa_stream_get_sample_spec(m_stream);
    //    qCDebug(qLcPulseAudioIn) << "connected stream:";
    //    qCDebug(qLcPulseAudioIn) << "    channels" << ss->channels << spec.channels;
    //    qCDebug(qLcPulseAudioIn) << "    format" << ss->format << spec.format;
    //    qCDebug(qLcPulseAudioIn) << "    rate" << ss->rate << spec.rate;
    //}

    while (pa_stream_get_state(m_stream) != PA_STREAM_READY)
        pa_threaded_mainloop_wait(pulseEngine->mainloop());

    const pa_buffer_attr *actualBufferAttr = pa_stream_get_buffer_attr(m_stream);
    m_periodSize = actualBufferAttr->fragsize;
    m_periodTime = pa_bytes_to_usec(m_periodSize, &spec) / 1000;
    if (actualBufferAttr->tlength != static_cast<uint32_t>(-1))
        m_bufferSize = actualBufferAttr->tlength;

    pulseEngine->unlock();

    connect(pulseEngine, &QPulseAudioEngine::contextFailed, this,
            &QPulseAudioSource::onPulseContextFailed);

    m_opened = true;
    m_timer.start(m_periodTime, this);

    m_elapsedTimeOffset = 0;
    m_totalTimeValue = 0;

    return true;
}

void QPulseAudioSource::close()
{
    if (!m_opened)
        return;

    m_timer.stop();

    QPulseAudioEngine *pulseEngine = QPulseAudioEngine::instance();

    if (m_stream) {
        std::lock_guard lock(*pulseEngine);

        pa_stream_set_state_callback(m_stream, nullptr, nullptr);
        pa_stream_set_read_callback(m_stream, nullptr, nullptr);
        pa_stream_set_underflow_callback(m_stream, nullptr, nullptr);
        pa_stream_set_overflow_callback(m_stream, nullptr, nullptr);

        pa_stream_disconnect(m_stream);
        pa_stream_unref(m_stream);
        m_stream = nullptr;
    }

    disconnect(pulseEngine, &QPulseAudioEngine::contextFailed, this,
               &QPulseAudioSource::onPulseContextFailed);

    if (!m_pullMode && m_audioSource) {
        delete m_audioSource;
        m_audioSource = nullptr;
    }
    m_opened = false;
}

qsizetype QPulseAudioSource::bytesReady() const
{
    using namespace QPulseAudioInternal;

    if (!m_stateMachine.isActiveOrIdle())
        return 0;

    std::lock_guard lock(*QPulseAudioEngine::instance());

    int bytes = pa_stream_readable_size(m_stream);
    if (bytes < 0) {
        qWarning() << "pa_stream_readable_size() failed:" << currentError(m_stream);
        return 0;
    }

    return static_cast<qsizetype>(bytes);
}

qint64 QPulseAudioSource::read(char *data, qint64 len)
{
    using namespace QPulseAudioInternal;

    Q_ASSERT(data != nullptr || len == 0);

    m_stateMachine.updateActiveOrIdle(true, QAudio::NoError);
    int readBytes = 0;

    if (!m_pullMode && !m_tempBuffer.isEmpty()) {
        readBytes = qMin(static_cast<int>(len), m_tempBuffer.size());
        if (readBytes)
            memcpy(data, m_tempBuffer.constData(), readBytes);
        m_totalTimeValue += readBytes;

        if (readBytes < m_tempBuffer.size()) {
            m_tempBuffer.remove(0, readBytes);
            return readBytes;
        }

        m_tempBuffer.clear();
    }

    while (pa_stream_readable_size(m_stream) > 0) {
        size_t readLength = 0;

        if (Q_UNLIKELY(qLcPulseAudioIn().isEnabled(QtDebugMsg))) {
            auto readableSize = pa_stream_readable_size(m_stream);
            qCDebug(qLcPulseAudioIn) << "QPulseAudioSource::read -- " << readableSize
                                     << " bytes available from pulse audio";
        }

        QPulseAudioEngine *pulseEngine = QPulseAudioEngine::instance();
        pulseEngine->lock();

        const void *audioBuffer;

        // Second and third parameters (audioBuffer and length) to pa_stream_peek are output
        // parameters, the audioBuffer pointer is set to point to the actual pulse audio data, and
        // the length is set to the length of this data.
        if (pa_stream_peek(m_stream, &audioBuffer, &readLength) < 0) {
            qWarning() << "pa_stream_peek() failed:" << currentError(m_stream);
            pulseEngine->unlock();
            return 0;
        }

        qint64 actualLength = 0;
        if (m_pullMode) {
            QByteArray adjusted(readLength, Qt::Uninitialized);
            applyVolume(audioBuffer, adjusted.data(), readLength);
            actualLength = m_audioSource->write(adjusted);

            if (actualLength < qint64(readLength)) {
                pulseEngine->unlock();
                m_stateMachine.updateActiveOrIdle(false, QAudio::UnderrunError);
                return actualLength;
            }
        } else {
            actualLength = qMin(static_cast<int>(len - readBytes), static_cast<int>(readLength));
            applyVolume(audioBuffer, data + readBytes, actualLength);
        }

        qCDebug(qLcPulseAudioIn) << "QPulseAudioSource::read -- wrote " << actualLength
                                 << " to client";

        if (actualLength < qint64(readLength)) {
            int diff = readLength - actualLength;
            int oldSize = m_tempBuffer.size();

            qCDebug(qLcPulseAudioIn) << "QPulseAudioSource::read -- appending " << diff
                                     << " bytes of data to temp buffer";

            m_tempBuffer.resize(m_tempBuffer.size() + diff);
            applyVolume(static_cast<const char *>(audioBuffer) + actualLength,
                        m_tempBuffer.data() + oldSize, diff);
            QMetaObject::invokeMethod(this, "userFeed", Qt::QueuedConnection);
        }

        m_totalTimeValue += actualLength;
        readBytes += actualLength;

        pa_stream_drop(m_stream);
        pulseEngine->unlock();

        if (!m_pullMode && readBytes >= len)
            break;
    }

    qCDebug(qLcPulseAudioIn) << "QPulseAudioSource::read -- returning after reading " << readBytes
                             << " bytes";

    return readBytes;
}

void QPulseAudioSource::applyVolume(const void *src, void *dest, int len)
{
    Q_ASSERT((src && dest) || len == 0);
    if (m_volume < 1.f)
        QAudioHelperInternal::qMultiplySamples(m_volume, m_format, src, dest, len);
    else if (len)
        memcpy(dest, src, len);
}

void QPulseAudioSource::resume()
{
    if (auto notifier = m_stateMachine.resume()) {
        {
            QPulseAudioEngine *pulseEngine = QPulseAudioEngine::instance();

            std::lock_guard lock(*pulseEngine);

            PAOperationUPtr operation(
                    pa_stream_cork(m_stream, 0, inputStreamSuccessCallback, nullptr));
            pulseEngine->wait(operation.get());
        }

        m_timer.start(m_periodTime, this);
    }
}

void QPulseAudioSource::setVolume(qreal vol)
{
    if (qFuzzyCompare(m_volume, vol))
        return;

    m_volume = qBound(qreal(0), vol, qreal(1));
}

qreal QPulseAudioSource::volume() const
{
    return m_volume;
}

void QPulseAudioSource::setBufferSize(qsizetype value)
{
    m_bufferSize = value;
}

qsizetype QPulseAudioSource::bufferSize() const
{
    return m_bufferSize;
}

qint64 QPulseAudioSource::processedUSecs() const
{
    if (!m_stream)
        return 0;
    pa_usec_t usecs = 0;
    int result = pa_stream_get_time(m_stream, &usecs);
    Q_UNUSED(result);
    //if (result != 0)
    //    qWarning() << "no timing info from pulse";

    return usecs;
}

void QPulseAudioSource::suspend()
{
    if (auto notifier = m_stateMachine.suspend()) {
        m_timer.stop();

        QPulseAudioEngine *pulseEngine = QPulseAudioEngine::instance();

        std::lock_guard lock(*pulseEngine);

        PAOperationUPtr operation(pa_stream_cork(m_stream, 1, inputStreamSuccessCallback, nullptr));
        pulseEngine->wait(operation.get());
    }
}

void QPulseAudioSource::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == m_timer.timerId())
        userFeed();

    QPlatformAudioSource::timerEvent(event);
}

void QPulseAudioSource::userFeed()
{
    if (!m_stateMachine.isActiveOrIdle())
        return;

    //if (Q_UNLIKELY(qLcPulseAudioIn().isEnabled(QtDebugMsg)) {
    //    QTime now(QTime::currentTime());
    //    qCDebug(qLcPulseAudioIn) << now.second() << "s " << now.msec() << "ms :userFeed() IN";
    //}

    if (m_pullMode) {
        // reads some audio data and writes it to QIODevice
        read(nullptr,0);
    } else if (m_audioSource != nullptr) {
        // emits readyRead() so user will call read() on QIODevice to get some audio data
        PulseInputPrivate *a = qobject_cast<PulseInputPrivate*>(m_audioSource);
        a->trigger();
    }
}

void QPulseAudioSource::reset()
{
    if (auto notifier = m_stateMachine.stopOrUpdateError())
        close();
}

void QPulseAudioSource::onPulseContextFailed()
{
    if (auto notifier = m_stateMachine.stopOrUpdateError(QAudio::FatalError))
        close();
}

PulseInputPrivate::PulseInputPrivate(QPulseAudioSource *audio)
{
    m_audioDevice = qobject_cast<QPulseAudioSource *>(audio);
}

qint64 PulseInputPrivate::readData(char *data, qint64 len)
{
    return m_audioDevice->read(data, len);
}

qint64 PulseInputPrivate::writeData(const char *data, qint64 len)
{
    Q_UNUSED(data);
    Q_UNUSED(len);
    return 0;
}

void PulseInputPrivate::trigger()
{
    emit readyRead();
}

QT_END_NAMESPACE

#include "moc_qpulseaudiosource_p.cpp"
