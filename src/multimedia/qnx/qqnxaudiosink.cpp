// Copyright (C) 2016 Research In Motion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqnxaudiosink_p.h"

#include <private/qaudiohelpers_p.h>
#include <sys/asoundlib.h>
#include <sys/asound_common.h>

#include <algorithm>
#include <limits>

#pragma GCC diagnostic ignored "-Wvla"

QT_BEGIN_NAMESPACE

QQnxAudioSink::QQnxAudioSink(const QAudioDevice &deviceInfo, QObject *parent)
    : QPlatformAudioSink(parent)
    , m_source(0)
    , m_pushSource(false)
    , m_timer(new QTimer(this))
    , m_error(QAudio::NoError)
    , m_state(QAudio::StoppedState)
    , m_suspendedInState(QAudio::SuspendedState)
    , m_volume(1.0)
    , m_periodSize(0)
    , m_bytesWritten(0)
    , m_requestedBufferSize(0)
    , m_deviceInfo(deviceInfo)
    , m_pcmNotifier(0)
{
    m_timer->setSingleShot(false);
    m_timer->setInterval(20);
    connect(m_timer, &QTimer::timeout, this, &QQnxAudioSink::pullData);

    const std::optional<snd_pcm_channel_info_t> info = QnxAudioUtils::pcmChannelInfo(
            m_deviceInfo.id(), QAudioDevice::Output);

    if (info)
        m_requestedBufferSize = info->max_fragment_size;
}

QQnxAudioSink::~QQnxAudioSink()
{
    stop();
}

void QQnxAudioSink::start(QIODevice *source)
{
    if (m_state != QAudio::StoppedState)
        stop();

    m_source = source;
    m_pushSource = false;

    if (open()) {
        changeState(QAudio::ActiveState, QAudio::NoError);
        m_timer->start();
    } else {
        changeState(QAudio::StoppedState, QAudio::OpenError);
    }
}

QIODevice *QQnxAudioSink::start()
{
    if (m_state != QAudio::StoppedState)
        stop();

    m_source = new QnxPushIODevice(this);
    m_source->open(QIODevice::WriteOnly|QIODevice::Unbuffered);
    m_pushSource = true;

    if (open()) {
        changeState(QAudio::IdleState, QAudio::NoError);
    } else {
        changeState(QAudio::StoppedState, QAudio::OpenError);
    }

    return m_source;
}

void QQnxAudioSink::stop()
{
    if (m_state == QAudio::StoppedState)
        return;

    changeState(QAudio::StoppedState, QAudio::NoError);

    close();
}

void QQnxAudioSink::reset()
{
    if (m_pcmHandle)
#if SND_PCM_VERSION < SND_PROTOCOL_VERSION('P',3,0,2)
        snd_pcm_playback_drain(m_pcmHandle.get());
#else
        snd_pcm_channel_drain(m_pcmHandle.get(), SND_PCM_CHANNEL_PLAYBACK);
#endif
    stop();
}

void QQnxAudioSink::suspend()
{
    if (!m_pcmHandle)
        return;

    snd_pcm_playback_pause(m_pcmHandle.get());
    suspendInternal(QAudio::SuspendedState);
}

void QQnxAudioSink::resume()
{
    if (!m_pcmHandle)
        return;

    snd_pcm_playback_resume(m_pcmHandle.get());
    resumeInternal();
}

void QQnxAudioSink::setBufferSize(qsizetype bufferSize)
{
    m_requestedBufferSize = std::clamp<qsizetype>(bufferSize, 0, std::numeric_limits<int>::max());
}

qsizetype QQnxAudioSink::bufferSize() const
{
    const std::optional<snd_pcm_channel_setup_t> setup = m_pcmHandle
        ? QnxAudioUtils::pcmChannelSetup(m_pcmHandle.get(), QAudioDevice::Output)
        : std::nullopt;

    return setup ? setup->buf.block.frag_size : m_requestedBufferSize;
}

qsizetype QQnxAudioSink::bytesFree() const
{
    if (m_state != QAudio::ActiveState && m_state != QAudio::IdleState)
        return 0;

    const std::optional<snd_pcm_channel_status_t> status = QnxAudioUtils::pcmChannelStatus(
            m_pcmHandle.get(), QAudioDevice::Output);

    return status ? status->free : 0;
}

qint64 QQnxAudioSink::processedUSecs() const
{
    return qint64(1000000) * m_format.framesForBytes(m_bytesWritten) / m_format.sampleRate();
}

QAudio::Error QQnxAudioSink::error() const
{
    return m_error;
}

QAudio::State QQnxAudioSink::state() const
{
    return m_state;
}

void QQnxAudioSink::setFormat(const QAudioFormat &format)
{
    if (m_state == QAudio::StoppedState)
        m_format = format;
}

QAudioFormat QQnxAudioSink::format() const
{
    return m_format;
}

void QQnxAudioSink::setVolume(qreal volume)
{
    m_volume = qBound(qreal(0.0), volume, qreal(1.0));
}

qreal QQnxAudioSink::volume() const
{
    return m_volume;
}

void QQnxAudioSink::updateState()
{
    const std::optional<snd_pcm_channel_status_t> status = QnxAudioUtils::pcmChannelStatus(
            m_pcmHandle.get(), QAudioDevice::Output);

    if (!status)
        return;

    if (state() == QAudio::ActiveState && status->underrun > 0)
        changeState(QAudio::IdleState, QAudio::UnderrunError);
    else if (state() == QAudio::IdleState && status->underrun == 0)
        changeState(QAudio::ActiveState, QAudio::NoError);
}

void QQnxAudioSink::pullData()
{
    if (m_state == QAudio::StoppedState
            || m_state == QAudio::SuspendedState)
        return;

    const int bytesAvailable = bytesFree();

    // skip if we have less than 4ms of data
    if (m_format.durationForBytes(bytesAvailable) < 4000)
        return;

    const int frames = m_format.framesForBytes(bytesAvailable);
    // The buffer is placed on the stack so no more than 64K or 1 frame
    // whichever is larger.
    const int maxFrames = qMax(m_format.framesForBytes(64 * 1024), 1);
    const int bytesRequested = m_format.bytesForFrames(qMin(frames, maxFrames));

    char buffer[bytesRequested];
    const int bytesRead = m_source->read(buffer, bytesRequested);

    // reading can take a while and stream may have been stopped
    if (!m_pcmHandle)
        return;

    if (bytesRead > 0) {
        const qint64 bytesWritten = write(buffer, bytesRead);

        if (bytesWritten <= 0) {
            close();
            changeState(QAudio::StoppedState, QAudio::FatalError);
        } else if (bytesWritten != bytesRead) {
            m_source->seek(m_source->pos()-(bytesRead-bytesWritten));
        }
    } else {
        // We're done
        if (bytesRead == 0)
            changeState(QAudio::IdleState, QAudio::NoError);
        else
            changeState(QAudio::IdleState, QAudio::IOError);
    }
}

bool QQnxAudioSink::open()
{
    if (!m_format.isValid() || m_format.sampleRate() <= 0) {
        if (!m_format.isValid())
            qWarning("QQnxAudioSink: open error, invalid format.");
        else
            qWarning("QQnxAudioSink: open error, invalid sample rate (%d).", m_format.sampleRate());

        return false;
    }


    m_pcmHandle = QnxAudioUtils::openPcmDevice(m_deviceInfo.id(), QAudioDevice::Output);

    if (!m_pcmHandle)
        return false;

    int errorCode = 0;

    if ((errorCode = snd_pcm_nonblock_mode(m_pcmHandle.get(), 0)) < 0) {
        qWarning("QQnxAudioSink: open error, couldn't set non block mode (0x%x)", -errorCode);
        close();
        return false;
    }

    addPcmEventFilter();

    // Necessary so that bytesFree() which uses the "free" member of the status struct works
    snd_pcm_plugin_set_disable(m_pcmHandle.get(), PLUGIN_MMAP);

    const std::optional<snd_pcm_channel_info_t> info = QnxAudioUtils::pcmChannelInfo(
            m_pcmHandle.get(), QAudioDevice::Output);

    if (!info) {
        close();
        return false;
    }

    const int fragmentSize = std::clamp(m_requestedBufferSize,
            info->min_fragment_size, info->max_fragment_size);

    snd_pcm_channel_params_t params = QnxAudioUtils::formatToChannelParams(m_format,
            QAudioDevice::Output, fragmentSize);

    if ((errorCode = snd_pcm_plugin_params(m_pcmHandle.get(), &params)) < 0) {
        qWarning("QQnxAudioSink: open error, couldn't set channel params (0x%x)", -errorCode);
        close();
        return false;
    }

    if ((errorCode = snd_pcm_plugin_prepare(m_pcmHandle.get(), SND_PCM_CHANNEL_PLAYBACK)) < 0) {
        qWarning("QQnxAudioSink: open error, couldn't prepare channel (0x%x)", -errorCode);
        close();
        return false;
    }

    const std::optional<snd_pcm_channel_setup_t> setup = QnxAudioUtils::pcmChannelSetup(
            m_pcmHandle.get(), QAudioDevice::Output);

    if (!setup) {
        close();
        return false;
    }

    m_periodSize = qMin(2048, setup->buf.block.frag_size);
    m_bytesWritten = 0;

    createPcmNotifiers();

    return true;
}

void QQnxAudioSink::close()
{
    if (!m_pushSource)
        m_timer->stop();

    destroyPcmNotifiers();

    if (m_pcmHandle) {
#if SND_PCM_VERSION < SND_PROTOCOL_VERSION('P',3,0,2)
        snd_pcm_plugin_flush(m_pcmHandle.get(), SND_PCM_CHANNEL_PLAYBACK);
#else
        snd_pcm_plugin_drop(m_pcmHandle.get(), SND_PCM_CHANNEL_PLAYBACK);
#endif
        m_pcmHandle = nullptr;
    }

    if (m_pushSource) {
        delete m_source;
        m_source = 0;
    }
}

void QQnxAudioSink::changeState(QAudio::State state, QAudio::Error error)
{
    if (m_state != state) {
        m_state = state;
        emit stateChanged(state);
    }

    if (m_error != error) {
        m_error = error;
        emit errorChanged(error);
    }
}

qint64 QQnxAudioSink::pushData(const char *data, qint64 len)
{
    const QAudio::State s = state();

    if (s == QAudio::StoppedState || s == QAudio::SuspendedState)
        return 0;

    if (s == QAudio::IdleState)
        changeState(QAudio::ActiveState, QAudio::NoError);

    qint64 totalWritten = 0;

    int retry = 0;

    constexpr int maxRetries = 10;

    while (totalWritten < len) {
        const int bytesWritten = write(data + totalWritten, len - totalWritten);

        if (bytesWritten <= 0) {
            ++retry;

            if (retry >= maxRetries) {
                close();
                changeState(QAudio::StoppedState, QAudio::FatalError);

                return totalWritten;
            } else {
                continue;
            }
        }

        retry = 0;

        totalWritten += bytesWritten;
    }

    return totalWritten;
}

qint64 QQnxAudioSink::write(const char *data, qint64 len)
{
    if (!m_pcmHandle)
        return 0;

    // Make sure we're writing (N * frame) worth of bytes
    const int size = m_format.bytesForFrames(qBound(qint64(0), qint64(bytesFree()), len) / m_format.bytesPerFrame());

    if (size == 0)
        return 0;

    int written = 0;

    if (m_volume < 1.0f) {
        char out[size];
        QAudioHelperInternal::qMultiplySamples(m_volume, m_format, data, out, size);
        written = snd_pcm_plugin_write(m_pcmHandle.get(), out, size);
    } else {
        written = snd_pcm_plugin_write(m_pcmHandle.get(), data, size);
    }

    if (written > 0) {
        m_bytesWritten += written;
        return written;
    }

    return 0;
}

void QQnxAudioSink::suspendInternal(QAudio::State suspendState)
{
    if (!m_pushSource)
        m_timer->stop();

    m_suspendedInState = m_state;
    changeState(suspendState, QAudio::NoError);
}

void QQnxAudioSink::resumeInternal()
{
    changeState(m_suspendedInState, QAudio::NoError);

    m_timer->start();
}

QAudio::State suspendState(const snd_pcm_event_t &event)
{
    Q_ASSERT(event.type == SND_PCM_EVENT_AUDIOMGMT_STATUS);
    Q_ASSERT(event.data.audiomgmt_status.new_status == SND_PCM_STATUS_SUSPENDED);
    return QAudio::SuspendedState;
}

void QQnxAudioSink::addPcmEventFilter()
{
    /* Enable PCM events */
    snd_pcm_filter_t filter;
    memset(&filter, 0, sizeof(filter));
    filter.enable = (1<<SND_PCM_EVENT_AUDIOMGMT_STATUS) |
                    (1<<SND_PCM_EVENT_AUDIOMGMT_MUTE) |
                    (1<<SND_PCM_EVENT_OUTPUTCLASS) |
                    (1<<SND_PCM_EVENT_UNDERRUN);
    snd_pcm_set_filter(m_pcmHandle.get(), SND_PCM_CHANNEL_PLAYBACK, &filter);
}

void QQnxAudioSink::createPcmNotifiers()
{
    // QSocketNotifier::Read for poll based event dispatcher.  Exception for
    // select based event dispatcher.
    m_pcmNotifier = new QSocketNotifier(snd_pcm_file_descriptor(m_pcmHandle.get(),
                                                                SND_PCM_CHANNEL_PLAYBACK),
                                        QSocketNotifier::Read, this);
    connect(m_pcmNotifier, &QSocketNotifier::activated,
            this, &QQnxAudioSink::pcmNotifierActivated);
}

void QQnxAudioSink::destroyPcmNotifiers()
{
    if (m_pcmNotifier) {
        delete m_pcmNotifier;
        m_pcmNotifier = 0;
    }
}

void QQnxAudioSink::pcmNotifierActivated(int socket)
{
    Q_UNUSED(socket);

    snd_pcm_event_t pcm_event;
    memset(&pcm_event, 0, sizeof(pcm_event));
    while (snd_pcm_channel_read_event(m_pcmHandle.get(), SND_PCM_CHANNEL_PLAYBACK, &pcm_event) == 0) {
        if (pcm_event.type == SND_PCM_EVENT_AUDIOMGMT_STATUS) {
            if (pcm_event.data.audiomgmt_status.new_status == SND_PCM_STATUS_SUSPENDED)
                suspendInternal(suspendState(pcm_event));
            else if (pcm_event.data.audiomgmt_status.new_status == SND_PCM_STATUS_RUNNING)
                resumeInternal();
            else if (pcm_event.data.audiomgmt_status.new_status == SND_PCM_STATUS_PAUSED)
                suspendInternal(QAudio::SuspendedState);
        } else if (pcm_event.type == SND_PCM_EVENT_UNDERRUN) {
            updateState();
        }
    }
}

QnxPushIODevice::QnxPushIODevice(QQnxAudioSink *output)
    : QIODevice(output),
      m_output(output)
{
}

QnxPushIODevice::~QnxPushIODevice()
{
}

qint64 QnxPushIODevice::readData(char *data, qint64 len)
{
    Q_UNUSED(data);
    Q_UNUSED(len);
    return 0;
}

qint64 QnxPushIODevice::writeData(const char *data, qint64 len)
{
    return m_output->pushData(data, len);
}

bool QnxPushIODevice::isSequential() const
{
    return true;
}

QT_END_NAMESPACE
