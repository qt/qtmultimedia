// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqnxsndaudiosink_p.h"
#include "qqnxsndhelpers_p.h"

#include <QtCore/qspan.h>
#include <QtCore/qthread.h>
#include <QLoggingCategory>

#include <algorithm>

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(lcQnxSndOutput, "qt.multimedia.qnxsnd.output")

using QtMultimediaPrivate::QPlatformAudioSinkStream;
using QtMultimediaPrivate::QPlatformAudioIOStream;
using QtMultimediaPrivate::runAudioCallback;
using QtMultimediaPrivate::withTemporaryBuffer;

QQnxSndAudioSinkStream::QQnxSndAudioSinkStream(QAudioDevice device, const QAudioFormat &format,
                                               std::optional<qsizetype> ringbufferSize,
                                               QQnxSndAudioSink *parent, float volume,
                                               std::optional<NativePeriodFrames> nativePeriodFrames,
                                               AudioEndpointRole /*role*/)
    : QPlatformAudioSinkStream{
          std::move(device),
          format,
          ringbufferSize,
          nativePeriodFrames,
          volume,
      },
      m_parent{ parent }
{
}

QQnxSndAudioSinkStream::~QQnxSndAudioSinkStream()
{
    // Defensive: the front class's stop() should already have torn the
    // worker down, but if a partial-construction unwind or a future
    // refactor drops the shared_ptr without calling stop(), make sure
    // we do not leak the worker thread or the SALSA handle.
    joinWorkerThread();
    closePcmDevice();
}

bool QQnxSndAudioSinkStream::open()
{
    return openPcmDevice();
}

int QQnxSndAudioSinkStream::recoverFromXrun(int err)
{
    return QnxSndHelpers::recoverFromXrun(m_handle, err);
}

bool QQnxSndAudioSinkStream::openPcmDevice()
{
    QnxSndHelpers::PcmOpenConfig cfg = {
        .direction         = SND_PCM_STREAM_PLAYBACK,
        .deviceId          = m_audioDevice.id(),
        .format            = m_format,
        .category          = lcQnxSndOutput(),
        .streamLabel       = "Playback",
        .periodCountEnvVar = "QT_QNXSND_OUTPUT_PERIODS",
        .periodFrames      = m_nativePeriodFrames
                ? std::optional<uint32_t>{ qToUnderlying(*m_nativePeriodFrames) }
                : std::nullopt,
    };
    auto r = QnxSndHelpers::openConfiguredPcm(cfg);
    if (!r)
        return false;

    m_handle       = r.handle;
    m_periodFrames = r.periodFrames;
    m_nativeFormat = r.nativeFormat;
    return true;
}

void QQnxSndAudioSinkStream::closePcmDevice()
{
    // The worker thread uses m_handle directly; closing it while the worker
    // is still running would create a use-after-free window. Every caller
    // path joins the worker first; assert the invariant.
    Q_ASSERT(!m_workerThread || !m_workerThread->isRunning());
    if (m_handle) {
        // Negative returns here typically mean the device was already lost
        // (-ENODEV) or invalidated by a prior drop (-EBADFD); log so the
        // case isn't silent in CI but proceed with cleanup either way.
        if (const int err = snd_pcm_close(m_handle); err < 0) {
            qCWarning(lcQnxSndOutput) << "snd_pcm_close failed:"
                                      << snd_strerror(err) << "(" << err << ")";
        }
        m_handle = nullptr;
    }
}

bool QQnxSndAudioSinkStream::start(QIODevice *ioDevice)
{
    setQIODevice(ioDevice);
    createQIODeviceConnections(ioDevice);
    pullFromQIODevice();

    startWorker(StreamType::Ringbuffer);
    return true;
}

QIODevice *QQnxSndAudioSinkStream::start()
{
    QIODevice *ioDevice = createRingbufferWriterDevice();
    setQIODevice(ioDevice);
    createQIODeviceConnections(ioDevice);

    startWorker(StreamType::Ringbuffer);
    return ioDevice;
}

bool QQnxSndAudioSinkStream::start(AudioCallback audioCallback)
{
    m_audioCallback = std::move(audioCallback);
    startWorker(StreamType::Callback);
    return true;
}

void QQnxSndAudioSinkStream::suspend()
{
    // Gating the worker on m_suspended lets the hardware buffer drain
    // naturally. Calling snd_pcm_drain or snd_pcm_drop here would race with
    // the worker thread blocked in snd_pcm_writei and force it into xrun
    // recovery, occasionally tripping handleSndPcmError -> StoppedState.
    m_suspended.store(true, std::memory_order_release);
    m_wakePipe.wake(); // break the worker out of poll() so it stops writing
}

void QQnxSndAudioSinkStream::resume()
{
    // After resume the worker's next snd_pcm_writei may return -EPIPE if the
    // hardware underran while we were suspended; processOnePeriod handles
    // that via recoverFromXrun, so no explicit prepare/start is needed here.
    m_suspended.store(false, std::memory_order_release);
    m_wakePipe.wake(); // wake the worker out of its suspended poll-wait
}

void QQnxSndAudioSinkStream::stop(ShutdownPolicy shutdownPolicy)
{
    // Draining is meaningless on a suspended stream — downgrade so we
    // can shut down synchronously instead of waiting on a drain that
    // can never complete.
    if (m_suspended.load(std::memory_order_acquire))
        shutdownPolicy = ShutdownPolicy::DiscardRingbuffer;

    m_shutdownPolicy.store(shutdownPolicy, std::memory_order_release);
    requestStop();

    // Sever the source-QIODevice signals on every shutdown path. Otherwise
    // pullFromQIODevice() can fire mid-teardown and refill the ringbuffer
    // we are tearing down. Matches the pipewire backend's pattern.
    disconnectQIODeviceConnections();

    switch (shutdownPolicy) {
    case ShutdownPolicy::DiscardRingbuffer:
        // joinWorkerThread() issues snd_pcm_drop itself to break the
        // worker out of any blocking snd_pcm_writei.
        joinWorkerThread();
        closePcmDevice();
        m_parent.store(nullptr, std::memory_order_release);
        return;
    case ShutdownPolicy::DrainRingbuffer:
        // Clear m_parent now, synchronously on the app thread, before this call
        // returns. The drain completion runs asynchronously and keeps the stream
        // alive via the captured shared_ptr, but the front class (m_parent) may
        // be destroyed as soon as stop() returns. Nulling it here guarantees any
        // worker-originated callback fired during the drain reads nullptr rather
        // than a dangling pointer (the completion lambda re-stores it for
        // defence-in-depth).
        m_parent.store(nullptr, std::memory_order_release);
        m_ringbufferDrained.callOnActivated([self = shared_from_this()]() mutable {
            self->joinWorkerThread();
            self->closePcmDevice();
            self->m_parent.store(nullptr, std::memory_order_release);
            self = {};
        });
        return;
    }
}

void QQnxSndAudioSinkStream::updateStreamIdle(bool streamIsIdle)
{
    // Read once into a local: the app thread may null m_parent between the
    // check and the call. The front class joins the worker before clearing
    // m_parent, so a non-null read here is guaranteed to point at a live
    // object for the duration of this call.
    if (auto *parent = m_parent.load(std::memory_order_acquire))
        parent->updateStreamIdle(streamIsIdle);
}

void QQnxSndAudioSinkStream::startWorker(StreamType streamType)
{
    // Create the wake pipe before the worker starts so suspend()/resume()/stop()
    // can signal it; on failure, stop still breaks the worker via snd_pcm_drop.
    if (!m_wakePipe.open())
        qCWarning(lcQnxSndOutput) << "wake pipe creation failed; worker wakeups degraded";

    m_workerThread.reset(QThread::create([this, streamType] {
        runProcessLoop(streamType);
    }));
    m_workerThread->setObjectName(u"QQnxSndAudioSinkStream");
    // The worker raises itself to a SCHED_FIFO priority in the 11-23 band at the
    // top of runProcessLoop (setWorkerRealtimePriority): above normal app threads
    // (10) so a busy application cannot preempt the audio feed, but strictly below
    // io-snd's own data/helper threads (24/25) — raising into that band wedges the
    // driver (the worker ends up REPLY-blocked on io-snd after preempting it).
    m_workerThread->start();
}

void QQnxSndAudioSinkStream::joinWorkerThread()
{
    requestStop();
    // Break the worker out of poll(); it then sees the stop request and exits.
    m_wakePipe.wake();
    if (m_handle) {
        // Discard whatever is still queued so playback stops promptly and any
        // in-flight snd_pcm_writei returns. Failures (e.g. -ENODEV on hot-unplug)
        // are worth surfacing but do not block teardown.
        if (const int err = snd_pcm_drop(m_handle); err < 0) {
            qCWarning(lcQnxSndOutput) << "snd_pcm_drop failed:"
                                      << snd_strerror(err) << "(" << err << ")";
        }
    }
    if (m_workerThread) {
        m_workerThread->wait();
        m_workerThread = {};
    }
    m_wakePipe.close();
}

void QQnxSndAudioSinkStream::runProcessLoop(StreamType streamType)
{
    QnxSndHelpers::setWorkerRealtimePriority(lcQnxSndOutput());

    // Prime the device with the first period, then start playback explicitly
    // (the qwindowsaudiosink pattern): openConfiguredPcm only prepares the
    // stream, so we control exactly when it starts and avoid a startup underrun.
    if (!processOnePeriod(streamType) && isStopRequested(std::memory_order_acquire))
        return;
    if (const int err = QnxSndHelpers::startPcm(m_handle)) {
        handleSndPcmError(err);
        return;
    }

    for (;;) {
        if (isStopRequested(std::memory_order_acquire)) {
            switch (m_shutdownPolicy.load(std::memory_order_acquire)) {
            case ShutdownPolicy::DiscardRingbuffer:
                return;
            case ShutdownPolicy::DrainRingbuffer: {
                const bool bufferDrained = visitRingbuffer([](const auto &ringbuffer) {
                    return ringbuffer.used() == 0;
                });
                // While suspended we cannot drain (processOnePeriod would
                // block on snd_pcm_writei against a stopped device), so
                // signal the front class regardless and let the callback
                // tear the stream down.
                if (bufferDrained || m_suspended.load(std::memory_order_acquire)) {
                    // We do not call snd_pcm_drain here: in some post-suspend
                    // / post-xrun states it wedges io-snd internally. The
                    // hardware will keep playing trailing samples already in
                    // the ALSA buffer until snd_pcm_close is called.
                    m_ringbufferDrained.set();
                    return;
                }
                break;
            }
            }
        }

        if (m_suspended.load(std::memory_order_acquire)) {
            // Block on the wake pipe only — never touch the device while
            // suspended (snd_pcm_writei against a stopped stream wedges io-snd).
            // resume()/stop() call wake() to release us.
            m_wakePipe.waitForWake();
            continue;
        }

        // Wait until io-snd can accept a period, or a stop/suspend wakes us.
        switch (QnxSndHelpers::pollPcm(m_handle, m_wakePipe)) {
        case QnxSndHelpers::PollOutcome::Woken:
            continue; // re-evaluate stop/suspend at the loop top
        case QnxSndHelpers::PollOutcome::Error:
            handleSndPcmError();
            return;
        case QnxSndHelpers::PollOutcome::Ready:
            break;
        }

        if (!processOnePeriod(streamType)) {
            if (!isStopRequested(std::memory_order_acquire))
                return;
        }
    }
}

bool QQnxSndAudioSinkStream::processOnePeriod(StreamType streamType)
{
    // Ask io-snd how many frames it can accept right now and fill exactly that
    // (capped at one period so the stack scratch stays bounded). In steady state
    // poll() wakes us with avail_min == one period free; on the initial prefill
    // (called before snd_pcm_start, while the stream is PREPARED) avail_update
    // reports the whole buffer free, so we still prime exactly one period.
    // NB: snd_pcm_avail() (the hwsync variant) returns -ENOTSUP on QNX SALSA;
    // snd_pcm_avail_update() reflects the just-polled state and is supported.
    const snd_pcm_sframes_t avail = snd_pcm_avail_update(m_handle);
    if (avail < 0) {
        if (isStopRequested(std::memory_order_acquire))
            return false;
        // Underrun/suspend surfaced via avail; recover and let the loop re-poll.
        if (const int err = recoverFromXrun(static_cast<int>(avail)); err < 0) {
            handleSndPcmError(err);
            return false;
        }
        return true;
    }
    if (avail == 0)
        return true; // poll() raced ahead of free space; re-poll rather than spin.

    const snd_pcm_uframes_t framesToWrite =
            std::min<snd_pcm_uframes_t>(static_cast<snd_pcm_uframes_t>(avail), m_periodFrames);
    // The host buffer handed to io-snd is in the device-native format, so size it
    // in native bytes; process()/runAudioCallback convert from the application
    // format (which may differ, e.g. Float -> 24-bit) while filling it.
    const size_t nativeBytesPerFrame =
            m_format.channelCount() * QAudioHelperInternal::bytesPerSample(m_nativeFormat);
    const size_t hostBytes = static_cast<size_t>(framesToWrite) * nativeBytesPerFrame;

    return withTemporaryBuffer(hostBytes, [&](QSpan<std::byte> hostBufferSpan) -> bool {
        switch (streamType) {
        case StreamType::Ringbuffer:
            QPlatformAudioSinkStream::process(hostBufferSpan, framesToWrite, m_nativeFormat);
            break;
        case StreamType::Callback:
            // StreamType::Callback is only ever set by start(AudioCallback), which
            // assigns m_audioCallback before starting the worker, so it is non-null.
            Q_ASSERT(m_audioCallback);
            runAudioCallback(*m_audioCallback, hostBufferSpan, m_format, volume(), m_nativeFormat);
            break;
        }

        snd_pcm_uframes_t framesPending = framesToWrite;
        auto *cursor = hostBufferSpan.data();
        while (framesPending > 0) {
            if (isStopRequested(std::memory_order_acquire))
                return false;

            snd_pcm_sframes_t written = snd_pcm_writei(m_handle, cursor, framesPending);
            if (written == -EAGAIN || written == 0) {
                // Bounded by avail above, so a short/again write here is rare;
                // wait for space rather than spinning. The loop-top stop check
                // handles a wake caused by teardown.
                if (QnxSndHelpers::pollPcm(m_handle, m_wakePipe)
                    == QnxSndHelpers::PollOutcome::Error) {
                    handleSndPcmError();
                    return false;
                }
                continue;
            }
            if (written < 0) {
                if (const int err = recoverFromXrun(static_cast<int>(written)); err < 0) {
                    handleSndPcmError(err);
                    return false;
                }
                continue;
            }
            framesPending -= static_cast<snd_pcm_uframes_t>(written);
            cursor += written * nativeBytesPerFrame;
        }
        return true;
    });
}

void QQnxSndAudioSinkStream::handleSndPcmError(int err)
{
    requestStop();
    // Re-read m_parent inside the lambda at fire-time. Both teardown paths clear
    // m_parent synchronously on the app thread before the front class can be
    // freed: Discard via joinWorkerThread() (~QQnxSndAudioSink / stop()), Drain
    // at the top of stop()'s Drain branch. So the load sees a live pointer or
    // nullptr — never a dangling one.
    invokeOnAppThread([self = shared_from_this(), err] {
        qCWarning(lcQnxSndOutput) << "audio output error, stopping stream:"
                                  << (err ? snd_strerror(err) : "I/O error");
        if (auto *parent = self->m_parent.load(std::memory_order_acquire))
            self->handleIOError(parent);
    });
}

///////////////////////////////////////////////////////////////////////////////////////////////////

QQnxSndAudioSink::QQnxSndAudioSink(QAudioDevice device, const QAudioFormat &format, QObject *parent)
    : BaseClass{ std::move(device), format, parent }
{
}

QQnxSndAudioSink::~QQnxSndAudioSink()
{
    // The base destructor calls stop() with Drain, which schedules
    // teardown on the app event loop and returns before the worker has
    // joined. That leaves a window where the worker can post lambdas
    // capturing the front-class address, which then fire after we're
    // freed. Force Discard here so joinWorkerThread() runs synchronously
    // and m_parent is nulled before *this is destroyed; queued lambdas
    // then see nullptr at fire-time and no-op.
    if (m_stream) {
        m_stream->stop(ShutdownPolicy::DiscardRingbuffer);
        m_stream = {};
    }
}

QT_END_NAMESPACE
