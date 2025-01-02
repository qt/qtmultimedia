// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/qcoreapplication.h>
#include <QtCore/qdebug.h>
#include <QtCore/qmath.h>
#include <private/qaudiohelpers_p.h>

#include "qpulseaudiosink_p.h"
#include "qpulseaudiodevice_p.h"
#include "qaudioengine_pulse_p.h"
#include "qpulsehelpers_p.h"
#include <sys/types.h>
#include <unistd.h>
#include <mutex> // for std::lock_guard

QT_BEGIN_NAMESPACE

static constexpr int SinkPeriodTimeMs = 20;

#define LOW_LATENCY_CATEGORY_NAME "game"

static void  outputStreamWriteCallback(pa_stream *stream, size_t length, void *userdata)
{
    Q_UNUSED(stream);
    Q_UNUSED(userdata);
    qCDebug(qLcPulseAudioOut) << "Write callback:" << length;
    QPulseAudioEngine *pulseEngine = QPulseAudioEngine::instance();
    pa_threaded_mainloop_signal(pulseEngine->mainloop(), 0);
}

static void outputStreamStateCallback(pa_stream *stream, void *userdata)
{
    Q_UNUSED(userdata);
    pa_stream_state_t state = pa_stream_get_state(stream);
    qCDebug(qLcPulseAudioOut) << "Stream state callback:" << state;
    switch (state) {
        case PA_STREAM_CREATING:
        case PA_STREAM_READY:
        case PA_STREAM_TERMINATED:
            break;

        case PA_STREAM_FAILED:
        default:
            qWarning() << QString::fromLatin1("Stream error: %1").arg(QString::fromUtf8(pa_strerror(pa_context_errno(pa_stream_get_context(stream)))));
            QPulseAudioEngine *pulseEngine = QPulseAudioEngine::instance();
            pa_threaded_mainloop_signal(pulseEngine->mainloop(), 0);
            break;
    }
}

static void outputStreamUnderflowCallback(pa_stream *stream, void *userdata)
{
    Q_UNUSED(stream);
    qCDebug(qLcPulseAudioOut) << "Buffer underflow";
    if (userdata)
        static_cast<QPulseAudioSink *>(userdata)->streamUnderflowCallback();
}

static void outputStreamOverflowCallback(pa_stream *stream, void *userdata)
{
    Q_UNUSED(stream);
    Q_UNUSED(userdata);
    qCDebug(qLcPulseAudioOut) << "Buffer overflow";
}

static void outputStreamLatencyCallback(pa_stream *stream, void *userdata)
{
    Q_UNUSED(stream);
    Q_UNUSED(userdata);

    if (Q_UNLIKELY(qLcPulseAudioOut().isEnabled(QtDebugMsg))) {
        const pa_timing_info *info = pa_stream_get_timing_info(stream);

        qCDebug(qLcPulseAudioOut) << "Latency callback:";
        qCDebug(qLcPulseAudioOut) << "\tWrite index corrupt: " << info->write_index_corrupt;
        qCDebug(qLcPulseAudioOut) << "\tWrite index: " << info->write_index;
        qCDebug(qLcPulseAudioOut) << "\tRead index corrupt: " << info->read_index_corrupt;
        qCDebug(qLcPulseAudioOut) << "\tRead index: " << info->read_index;
        qCDebug(qLcPulseAudioOut) << "\tSink usec: " << info->sink_usec;
        qCDebug(qLcPulseAudioOut) << "\tConfigured sink usec: " << info->configured_sink_usec;
    }
}

static void outputStreamSuccessCallback(pa_stream *stream, int success, void *userdata)
{
    Q_UNUSED(stream);
    Q_UNUSED(userdata);

    qCDebug(qLcPulseAudioOut) << "Stream successful:" << success;
    QPulseAudioEngine *pulseEngine = QPulseAudioEngine::instance();
    pa_threaded_mainloop_signal(pulseEngine->mainloop(), 0);
}

static void outputStreamDrainComplete(pa_stream *stream, int success, void *userdata)
{
    Q_UNUSED(stream);

    qCDebug(qLcPulseAudioOut) << "Stream drained:" << bool(success) << userdata;

    if (userdata && success)
        static_cast<QPulseAudioSink *>(userdata)->streamDrainedCallback();
}

static void streamAdjustPrebufferCallback(pa_stream *stream, int success, void *userdata)
{
    Q_UNUSED(stream);
    Q_UNUSED(success);
    Q_UNUSED(userdata);

    qCDebug(qLcPulseAudioOut) << "Prebuffer adjusted:" << bool(success);
}

QPulseAudioSink::QPulseAudioSink(const QByteArray &device, QObject *parent)
    : QPlatformAudioSink(parent), m_device(device), m_stateMachine(*this)
{
}

QPulseAudioSink::~QPulseAudioSink()
{
    if (auto notifier = m_stateMachine.stop())
        close();
}

QAudio::Error QPulseAudioSink::error() const
{
    return m_stateMachine.error();
}

QAudio::State QPulseAudioSink::state() const
{
    return m_stateMachine.state();
}

void QPulseAudioSink::streamUnderflowCallback()
{
    if (m_audioSource && m_audioSource->atEnd()) {
        qCDebug(qLcPulseAudioOut) << "Draining stream at end of buffer";

        exchangeDrainOperation(pa_stream_drain(m_stream, outputStreamDrainComplete, this));
    } else if (!m_resuming) {
        m_stateMachine.updateActiveOrIdle(false, QAudio::UnderrunError);
    }
}

void QPulseAudioSink::streamDrainedCallback()
{
    if (!exchangeDrainOperation(nullptr))
        return;

    m_stateMachine.updateActiveOrIdle(false);
}

void QPulseAudioSink::start(QIODevice *device)
{
    reset();

    m_pullMode = true;
    m_audioSource = device;

    if (!open()) {
        m_audioSource = nullptr;
        return;
    }

    // ensure we only process timing infos that are up to date
    gettimeofday(&lastTimingInfo, nullptr);
    lastProcessedUSecs = 0;

    connect(m_audioSource, &QIODevice::readyRead, this, &QPulseAudioSink::startReading);

    m_stateMachine.start();
}

void QPulseAudioSink::startReading()
{
    if (!m_tickTimer.isActive())
        m_tickTimer.start(m_periodTime, this);
}

QIODevice *QPulseAudioSink::start()
{
    reset();

    m_pullMode = false;

    if (!open())
        return nullptr;

    m_audioSource = new PulseOutputPrivate(this);
    m_audioSource->open(QIODevice::WriteOnly|QIODevice::Unbuffered);

    // ensure we only process timing infos that are up to date
    gettimeofday(&lastTimingInfo, nullptr);
    lastProcessedUSecs = 0;

    m_stateMachine.start(false);

    return m_audioSource;
}

bool QPulseAudioSink::open()
{
    if (m_opened)
        return true;

    QPulseAudioEngine *pulseEngine = QPulseAudioEngine::instance();

    if (!pulseEngine->context() || pa_context_get_state(pulseEngine->context()) != PA_CONTEXT_READY) {
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
    m_totalTimeValue = 0;

    if (m_streamName.isNull())
        m_streamName = QString(QLatin1String("QtmPulseStream-%1-%2")).arg(::getpid()).arg(quintptr(this)).toUtf8();

    if (Q_UNLIKELY(qLcPulseAudioOut().isEnabled(QtDebugMsg))) {
        qCDebug(qLcPulseAudioOut) << "Opening stream with.";
        qCDebug(qLcPulseAudioOut) << "\tFormat: " << QPulseAudioInternal::sampleFormatToQString(spec.format);
        qCDebug(qLcPulseAudioOut) << "\tRate: " << spec.rate;
        qCDebug(qLcPulseAudioOut) << "\tChannels: " << spec.channels;
        qCDebug(qLcPulseAudioOut) << "\tFrame size: " << pa_frame_size(&spec);
    }

    pulseEngine->lock();


    pa_proplist *propList = pa_proplist_new();
#if 0
    qint64 bytesPerSecond = m_format.sampleRate() * m_format.bytesPerFrame();
    static const char *mediaRoleFromAudioRole[] = {
        nullptr, // UnknownRole
        "music", // MusicRole
        "video", // VideoRole
        "phone", // VoiceCommunicationRole
        "event", // AlarmRole
        "event", // NotificationRole
        "phone", // RingtoneRole
        "a11y", // AccessibilityRole
        nullptr, // SonificationRole
        "game" // GameRole
    };

    const char *r = mediaRoleFromAudioRole[m_role];
    if (r)
        pa_proplist_sets(propList, PA_PROP_MEDIA_ROLE, r);
#endif

    m_stream = pa_stream_new_with_proplist(pulseEngine->context(), m_streamName.constData(), &m_spec, &channel_map, propList);
    pa_proplist_free(propList);

    if (!m_stream) {
        qCWarning(qLcPulseAudioOut) << "QAudioSink: pa_stream_new_with_proplist() failed!";
        pulseEngine->unlock();

        m_stateMachine.stopOrUpdateError(QAudio::OpenError);
        return false;
    }

    pa_stream_set_state_callback(m_stream, outputStreamStateCallback, this);
    pa_stream_set_write_callback(m_stream, outputStreamWriteCallback, this);

    pa_stream_set_underflow_callback(m_stream, outputStreamUnderflowCallback, this);
    pa_stream_set_overflow_callback(m_stream, outputStreamOverflowCallback, this);
    pa_stream_set_latency_update_callback(m_stream, outputStreamLatencyCallback, this);

    pa_buffer_attr requestedBuffer;
    requestedBuffer.fragsize = (uint32_t)-1;
    requestedBuffer.maxlength = (uint32_t)-1;
    requestedBuffer.minreq = (uint32_t)-1;
    requestedBuffer.prebuf = (uint32_t)-1;
    requestedBuffer.tlength = m_bufferSize;

    pa_stream_flags flags = pa_stream_flags(PA_STREAM_AUTO_TIMING_UPDATE|PA_STREAM_ADJUST_LATENCY);
    if (pa_stream_connect_playback(m_stream, m_device.data(), (m_bufferSize > 0) ? &requestedBuffer : nullptr, flags, nullptr, nullptr) < 0) {
        qCWarning(qLcPulseAudioOut) << "pa_stream_connect_playback() failed!";
        pa_stream_unref(m_stream);
        m_stream = nullptr;
        pulseEngine->unlock();
        m_stateMachine.stopOrUpdateError(QAudio::OpenError);
        return false;
    }

    while (pa_stream_get_state(m_stream) != PA_STREAM_READY)
        pa_threaded_mainloop_wait(pulseEngine->mainloop());

    const pa_buffer_attr *buffer = pa_stream_get_buffer_attr(m_stream);
    m_periodTime = SinkPeriodTimeMs;
    m_periodSize = pa_usec_to_bytes(m_periodTime * 1000, &m_spec);
    m_bufferSize = buffer->tlength;
    m_audioBuffer.resize(buffer->maxlength);

    const qint64 streamSize = m_audioSource ? m_audioSource->size() : 0;
    if (m_pullMode && streamSize > 0 && static_cast<qint64>(buffer->prebuf) > streamSize) {
        pa_buffer_attr newBufferAttr;
        newBufferAttr = *buffer;
        newBufferAttr.prebuf = streamSize;
        PAOperationUPtr(pa_stream_set_buffer_attr(m_stream, &newBufferAttr,
                                                  streamAdjustPrebufferCallback, nullptr));
    }

    if (Q_UNLIKELY(qLcPulseAudioOut().isEnabled(QtDebugMsg))) {
        qCDebug(qLcPulseAudioOut) << "Buffering info:";
        qCDebug(qLcPulseAudioOut) << "\tMax length: " << buffer->maxlength;
        qCDebug(qLcPulseAudioOut) << "\tTarget length: " << buffer->tlength;
        qCDebug(qLcPulseAudioOut) << "\tPre-buffering: " << buffer->prebuf;
        qCDebug(qLcPulseAudioOut) << "\tMinimum request: " << buffer->minreq;
        qCDebug(qLcPulseAudioOut) << "\tFragment size: " << buffer->fragsize;
    }

    pulseEngine->unlock();

    connect(pulseEngine, &QPulseAudioEngine::contextFailed, this, &QPulseAudioSink::onPulseContextFailed);

    m_opened = true;

    startReading();

    m_elapsedTimeOffset = 0;

    return true;
}

void QPulseAudioSink::close()
{
    if (!m_opened)
        return;

    m_tickTimer.stop();

    QPulseAudioEngine *pulseEngine = QPulseAudioEngine::instance();

    if (m_stream) {
        std::lock_guard lock(*pulseEngine);

        pa_stream_set_state_callback(m_stream, nullptr, nullptr);
        pa_stream_set_write_callback(m_stream, nullptr, nullptr);
        pa_stream_set_underflow_callback(m_stream, nullptr, nullptr);
        pa_stream_set_overflow_callback(m_stream, nullptr, nullptr);
        pa_stream_set_latency_update_callback(m_stream, nullptr, nullptr);

        if (auto prevOp = exchangeDrainOperation(nullptr))
            // cancel the draining callback that is not relevant already
            pa_operation_cancel(prevOp.get());

        PAOperationUPtr(pa_stream_drain(m_stream, outputStreamDrainComplete, nullptr));

        pa_stream_disconnect(m_stream);
        pa_stream_unref(m_stream);
        m_stream = nullptr;
    }

    disconnect(pulseEngine, &QPulseAudioEngine::contextFailed, this, &QPulseAudioSink::onPulseContextFailed);

    if (m_audioSource) {
        if (m_pullMode) {
            disconnect(m_audioSource, &QIODevice::readyRead, this, nullptr);
        } else {
            delete m_audioSource;
            m_audioSource = nullptr;
        }
    }
    m_opened = false;
    m_resuming = false;
    m_audioBuffer.clear();
}

void QPulseAudioSink::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == m_tickTimer.timerId())
        userFeed();

    QPlatformAudioSink::timerEvent(event);
}

void QPulseAudioSink::userFeed()
{
    if (!m_stateMachine.isActiveOrIdle())
        return;

    m_resuming = false;

    if (m_pullMode) {
        int writableSize = bytesFree();
        int chunks = writableSize / m_periodSize;
        if (chunks == 0) {
            m_stateMachine.activateFromIdle();
            return;
        }

        const int input = std::min(m_periodSize, static_cast<int>(m_audioBuffer.size()));

        Q_ASSERT(!m_audioBuffer.empty());
        int audioBytesPulled = m_audioSource->read(m_audioBuffer.data(), input);
        Q_ASSERT(audioBytesPulled <= input);
        if (audioBytesPulled > 0) {
            if (audioBytesPulled > input) {
                qCWarning(qLcPulseAudioOut)
                        << "Invalid audio data size provided by pull source:" << audioBytesPulled
                        << "should be less than" << input;
                audioBytesPulled = input;
            }
            auto bytesWritten = write(m_audioBuffer.data(), audioBytesPulled);
            if (bytesWritten != audioBytesPulled)
                qWarning() << "Unfinished write should not happen since the data provided is "
                              "less than writableSize:"
                           << bytesWritten << "vs" << audioBytesPulled;

            if (chunks > 1) {
                // PulseAudio needs more data. Ask for it immediately.
                QMetaObject::invokeMethod(this, &QPulseAudioSink::userFeed, Qt::QueuedConnection);
            }
        } else if (audioBytesPulled == 0) {
            m_tickTimer.stop();
            const auto atEnd = m_audioSource->atEnd();
            qCDebug(qLcPulseAudioOut) << "No more data available, source is done:" << atEnd;

            m_stateMachine.updateActiveOrIdle(false,
                                              atEnd ? QAudio::NoError : QAudio::UnderrunError);
        }
    } else {
        if (state() == QAudio::IdleState)
            m_stateMachine.setError(QAudio::UnderrunError);
    }
}

qint64 QPulseAudioSink::write(const char *data, qint64 len)
{
    QPulseAudioEngine *pulseEngine = QPulseAudioEngine::instance();

    pulseEngine->lock();

    size_t nbytes = len;
    void *dest = nullptr;

    if (pa_stream_begin_write(m_stream, &dest, &nbytes) < 0) {
        pulseEngine->unlock();
        qCWarning(qLcPulseAudioOut) << "pa_stream_begin_write error:"
                                    << pa_strerror(pa_context_errno(pulseEngine->context()));
        m_stateMachine.updateActiveOrIdle(false, QAudio::IOError);
        return 0;
    }

    len = qMin(len, qint64(nbytes));

    if (m_volume < 1.0f) {
        // Don't use PulseAudio volume, as it might affect all other streams of the same category
        // or even affect the system volume if flat volumes are enabled
        QAudioHelperInternal::qMultiplySamples(m_volume, m_format, data, dest, len);
    } else {
        memcpy(dest, data, len);
    }

    data = reinterpret_cast<char *>(dest);

    if ((pa_stream_write(m_stream, data, len, nullptr, 0, PA_SEEK_RELATIVE)) < 0) {
        pulseEngine->unlock();
        qCWarning(qLcPulseAudioOut) << "pa_stream_write error:"
                                    << pa_strerror(pa_context_errno(pulseEngine->context()));
        m_stateMachine.updateActiveOrIdle(false, QAudio::IOError);
        return 0;
    }

    pulseEngine->unlock();
    m_totalTimeValue += len;

    m_stateMachine.updateActiveOrIdle(true);
    return len;
}

void QPulseAudioSink::stop()
{
    if (auto notifier = m_stateMachine.stop())
        close();
}

qsizetype QPulseAudioSink::bytesFree() const
{
    if (!m_stateMachine.isActiveOrIdle())
        return 0;

    std::lock_guard lock(*QPulseAudioEngine::instance());
    return pa_stream_writable_size(m_stream);
}

void QPulseAudioSink::setBufferSize(qsizetype value)
{
    m_bufferSize = value;
}

qsizetype QPulseAudioSink::bufferSize() const
{
    return m_bufferSize;
}

static qint64 operator-(timeval t1, timeval t2)
{
    constexpr qint64 secsToUSecs = 1000000;
    return (t1.tv_sec - t2.tv_sec)*secsToUSecs + (t1.tv_usec - t2.tv_usec);
}

qint64 QPulseAudioSink::processedUSecs() const
{
    const auto state = this->state();
    if (!m_stream || state == QAudio::StoppedState)
        return 0;
    if (state == QAudio::SuspendedState)
        return lastProcessedUSecs;

    auto info = pa_stream_get_timing_info(m_stream);
    if (!info)
        return lastProcessedUSecs;

    // if the info changed, update our cached data, and recalculate the average latency
    if (info->timestamp - lastTimingInfo > 0) {
        lastTimingInfo.tv_sec = info->timestamp.tv_sec;
        lastTimingInfo.tv_usec = info->timestamp.tv_usec;
        averageLatency = 0; // also use that as long as we don't have valid data from the timing info

        // Only use timing values when playing, otherwise the latency numbers can be way off
        if (info->since_underrun >= 0 && pa_bytes_to_usec(info->since_underrun, &m_spec) > info->sink_usec) {
            latencyList.append(info->sink_usec);
            // Average over the last X timing infos to keep numbers more stable.
            // 10 seems to be a decent number that keeps values relatively stable but doesn't make the list too big
            const int latencyListMaxSize = 10;
            if (latencyList.size() > latencyListMaxSize)
                latencyList.pop_front();
            for (const auto l : latencyList)
                averageLatency += l;
            averageLatency /= latencyList.size();
            if (averageLatency < 0)
                averageLatency = 0;
        }
    }

    const qint64 usecsRead = info->read_index < 0 ? 0 : pa_bytes_to_usec(info->read_index, &m_spec);
    const qint64 usecsWritten = info->write_index < 0 ? 0 : pa_bytes_to_usec(info->write_index, &m_spec);

    // processed data is the amount read by the server minus its latency
    qint64 usecs = usecsRead - averageLatency;

    timeval tv;
    gettimeofday(&tv, nullptr);

    // and now adjust for the time since the last update
    qint64 timeSinceUpdate = tv - info->timestamp;
    if (timeSinceUpdate > 0)
        usecs += timeSinceUpdate;

    // We can never have processed more than we've written to the sink
    if (usecs > usecsWritten)
        usecs = usecsWritten;

    // make sure timing is monotonic
    if (usecs < lastProcessedUSecs)
        usecs = lastProcessedUSecs;
    else
        lastProcessedUSecs = usecs;

    return usecs;
}

void QPulseAudioSink::resume()
{
    if (auto notifier = m_stateMachine.resume()) {
        m_resuming = true;

        {
            QPulseAudioEngine *pulseEngine = QPulseAudioEngine::instance();

            std::lock_guard lock(*pulseEngine);

            PAOperationUPtr operation(
                    pa_stream_cork(m_stream, 0, outputStreamSuccessCallback, nullptr));
            pulseEngine->wait(operation.get());

            operation.reset(pa_stream_trigger(m_stream, outputStreamSuccessCallback, nullptr));
            pulseEngine->wait(operation.get());
        }

        m_tickTimer.start(m_periodTime, this);
    }
}

void QPulseAudioSink::setFormat(const QAudioFormat &format)
{
    m_format = format;
}

QAudioFormat QPulseAudioSink::format() const
{
    return m_format;
}

void QPulseAudioSink::suspend()
{
    if (auto notifier = m_stateMachine.suspend()) {
        m_tickTimer.stop();

        QPulseAudioEngine *pulseEngine = QPulseAudioEngine::instance();

        std::lock_guard lock(*pulseEngine);

        PAOperationUPtr operation(
                pa_stream_cork(m_stream, 1, outputStreamSuccessCallback, nullptr));
        pulseEngine->wait(operation.get());
    }
}

void QPulseAudioSink::reset()
{
    if (auto notifier = m_stateMachine.stopOrUpdateError())
        close();
}

PulseOutputPrivate::PulseOutputPrivate(QPulseAudioSink *audio)
{
    m_audioDevice = qobject_cast<QPulseAudioSink*>(audio);
}

qint64 PulseOutputPrivate::readData(char *data, qint64 len)
{
    Q_UNUSED(data);
    Q_UNUSED(len);

    return 0;
}

qint64 PulseOutputPrivate::writeData(const char *data, qint64 len)
{
    qint64 written = 0;

    const auto state = m_audioDevice->state();
    if (state == QAudio::ActiveState || state == QAudio::IdleState) {
        while (written < len) {
            int chunk = m_audioDevice->write(data+written, (len-written));
            if (chunk <= 0)
                return written;
            written+=chunk;
        }
    }

    return written;
}

void QPulseAudioSink::setVolume(qreal vol)
{
    if (qFuzzyCompare(m_volume, vol))
        return;

    m_volume = qBound(qreal(0), vol, qreal(1));
}

qreal QPulseAudioSink::volume() const
{
    return m_volume;
}

void QPulseAudioSink::onPulseContextFailed()
{
    if (auto notifier = m_stateMachine.stop(QAudio::FatalError))
        close();
}

PAOperationUPtr QPulseAudioSink::exchangeDrainOperation(pa_operation *newOperation)
{
    return PAOperationUPtr(m_drainOperation.exchange(newOperation));
}

QT_END_NAMESPACE

#include "moc_qpulseaudiosink_p.cpp"
