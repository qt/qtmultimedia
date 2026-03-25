// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqnxsndaudiosource_p.h"
#include "qqnxsndhelpers_p.h"

#include <QtCore/qspan.h>
#include <QtCore/qthread.h>
#include <QLoggingCategory>

#include <algorithm>

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(lcQnxSndInput, "qt.multimedia.qnxsnd.input")

using QtMultimediaPrivate::QPlatformAudioSourceStream;
using QtMultimediaPrivate::QPlatformAudioIOStream;
using QtMultimediaPrivate::runAudioCallback;
using QtMultimediaPrivate::withTemporaryBuffer;

QQnxSndAudioSourceStream::QQnxSndAudioSourceStream(QAudioDevice device, const QAudioFormat &format,
                                                   std::optional<qsizetype> ringbufferSize,
                                                   QQnxSndAudioSource *parent, float volume,
                                                   std::optional<NativePeriodFrames> nativePeriodFrames)
    : QPlatformAudioSourceStream{
          std::move(device),
          format,
          ringbufferSize,
          nativePeriodFrames,
          volume,
      },
      m_parent{ parent }
{
}

QQnxSndAudioSourceStream::~QQnxSndAudioSourceStream()
{
    // Defensive: the front class's stop() should already have torn the
    // worker down, but if a partial-construction unwind or a future
    // refactor drops the shared_ptr without calling stop(), make sure
    // we do not leak the worker thread or the SALSA handle.
    joinWorkerThread();
    closePcmDevice();
}

bool QQnxSndAudioSourceStream::open()
{
    return openPcmDevice();
}

int QQnxSndAudioSourceStream::recoverFromXrun(int err)
{
    return QnxSndHelpers::recoverFromXrun(m_handle, err);
}

bool QQnxSndAudioSourceStream::openPcmDevice()
{
    QnxSndHelpers::PcmOpenConfig cfg = {
        .direction         = SND_PCM_STREAM_CAPTURE,
        .deviceId          = m_audioDevice.id(),
        .format            = m_format,
        .category          = lcQnxSndInput(),
        .streamLabel       = "Capture",
        .periodCountEnvVar = "QT_QNXSND_INPUT_PERIODS",
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

void QQnxSndAudioSourceStream::closePcmDevice()
{
    // See QQnxSndAudioSinkStream::closePcmDevice — same m_handle-lifetime
    // invariant on the capture side.
    Q_ASSERT(!m_workerThread || !m_workerThread->isRunning());
    if (m_handle) {
        // See QQnxSndAudioSinkStream::closePcmDevice — log negative returns
        // (typically -ENODEV after hot-unplug or -EBADFD after a prior drop)
        // but proceed with cleanup.
        if (const int err = snd_pcm_close(m_handle); err < 0) {
            qCWarning(lcQnxSndInput) << "snd_pcm_close failed:"
                                     << snd_strerror(err) << "(" << err << ")";
        }
        m_handle = nullptr;
    }
}

bool QQnxSndAudioSourceStream::start(QIODevice *ioDevice)
{
    setQIODevice(ioDevice);
    createQIODeviceConnections(ioDevice);
    startWorker();
    return true;
}

QIODevice *QQnxSndAudioSourceStream::start()
{
    QIODevice *ioDevice = createRingbufferReaderDevice();
    setQIODevice(ioDevice);
    createQIODeviceConnections(ioDevice);

    startWorker();
    return ioDevice;
}

bool QQnxSndAudioSourceStream::start(AudioCallback audioCallback)
{
    m_audioCallback = std::move(audioCallback);
    startWorker();
    return true;
}

void QQnxSndAudioSourceStream::suspend()
{
    // See QQnxSndAudioSinkStream::suspend — touching the device here races
    // with the worker thread blocked in snd_pcm_readi.
    m_suspended.store(true, std::memory_order_release);
    m_wakePipe.wake(); // break the worker out of poll() so it stops reading
}

void QQnxSndAudioSourceStream::resume()
{
    // The first readi after resume may return -EPIPE if hardware overran
    // while suspended; processOnePeriod recovers via snd_pcm_prepare.
    m_suspended.store(false, std::memory_order_release);
    m_wakePipe.wake(); // wake the worker out of its suspended poll-wait
}

void QQnxSndAudioSourceStream::stop(ShutdownPolicy shutdownPolicy)
{
    requestStop();
    disconnectQIODeviceConnections();

    // joinWorkerThread() issues snd_pcm_drop itself to break the worker
    // out of any blocking snd_pcm_readi.
    joinWorkerThread();
    closePcmDevice();
    m_parent.store(nullptr, std::memory_order_release);

    finalizeQIODevice(shutdownPolicy);
    if (shutdownPolicy == ShutdownPolicy::DiscardRingbuffer)
        emptyRingbuffer();
}

void QQnxSndAudioSourceStream::updateStreamIdle(bool streamIsIdle)
{
    // See QQnxSndAudioSinkStream::updateStreamIdle — read once into a local
    // to avoid a TOCTOU with stop() nulling m_parent.
    if (auto *parent = m_parent.load(std::memory_order_acquire))
        parent->updateStreamIdle(streamIsIdle);
}

void QQnxSndAudioSourceStream::startWorker()
{
    // See QQnxSndAudioSinkStream::startWorker — the wake pipe lets
    // suspend()/resume()/stop() break the worker out of poll().
    if (!m_wakePipe.open())
        qCWarning(lcQnxSndInput) << "wake pipe creation failed; worker wakeups degraded";

    m_workerThread.reset(QThread::create([this] {
        runProcessLoop();
    }));
    m_workerThread->setObjectName(u"QQnxSndAudioSourceStream");
    // The worker raises itself into the SCHED_FIFO 11-23 band at the top of
    // runProcessLoop (setWorkerRealtimePriority) — see QQnxSndAudioSinkStream::startWorker.
    m_workerThread->start();
}

void QQnxSndAudioSourceStream::joinWorkerThread()
{
    requestStop();
    // Break the worker out of poll(); it then sees the stop request and exits.
    m_wakePipe.wake();
    if (m_handle) {
        // See QQnxSndAudioSinkStream::joinWorkerThread — drop discards queued
        // capture and lets any in-flight snd_pcm_readi return; surface failures.
        if (const int err = snd_pcm_drop(m_handle); err < 0) {
            qCWarning(lcQnxSndInput) << "snd_pcm_drop failed:"
                                     << snd_strerror(err) << "(" << err << ")";
        }
    }
    if (m_workerThread) {
        m_workerThread->wait();
        m_workerThread = {};
    }
    m_wakePipe.close();
}

void QQnxSndAudioSourceStream::runProcessLoop()
{
    QnxSndHelpers::setWorkerRealtimePriority(lcQnxSndInput());

    // openConfiguredPcm only prepares the stream; start capture explicitly here
    // (the capture analogue of the sink's prefill-then-start).
    if (const int err = QnxSndHelpers::startPcm(m_handle)) {
        handleSndPcmError(err);
        return;
    }

    for (;;) {
        if (isStopRequested(std::memory_order_acquire))
            return;

        if (m_suspended.load(std::memory_order_acquire)) {
            // Block on the wake pipe only — never touch the device while
            // suspended. resume()/stop() call wake() to release us.
            m_wakePipe.waitForWake();
            continue;
        }

        // Wait until io-snd has a period of capture data, or a stop/suspend wakes us.
        switch (QnxSndHelpers::pollPcm(m_handle, m_wakePipe)) {
        case QnxSndHelpers::PollOutcome::Woken:
            continue; // re-evaluate stop/suspend at the loop top
        case QnxSndHelpers::PollOutcome::Error:
            handleSndPcmError();
            return;
        case QnxSndHelpers::PollOutcome::Ready:
            break;
        }

        if (!processOnePeriod()) {
            if (!isStopRequested(std::memory_order_acquire))
                return;
        }
    }
}

bool QQnxSndAudioSourceStream::processOnePeriod()
{
    // Read exactly what io-snd has captured (capped at one period so the stack
    // scratch stays bounded). poll() wakes us with avail_min == one period ready.
    // NB: snd_pcm_avail() (the hwsync variant) returns -ENOTSUP on QNX SALSA;
    // snd_pcm_avail_update() reflects the just-polled state and is supported.
    const snd_pcm_sframes_t avail = snd_pcm_avail_update(m_handle);
    if (avail < 0) {
        if (isStopRequested(std::memory_order_acquire))
            return false;
        // Overrun/suspend surfaced via avail; recover and let the loop re-poll.
        if (const int err = recoverFromXrun(static_cast<int>(avail)); err < 0) {
            handleSndPcmError(err);
            return false;
        }
        return true;
    }
    if (avail == 0)
        return true; // poll() raced ahead of the data; re-poll rather than spin.

    const snd_pcm_uframes_t framesToRead =
            std::min<snd_pcm_uframes_t>(static_cast<snd_pcm_uframes_t>(avail), m_periodFrames);
    // io-snd delivers samples in the device-native format, so size the host buffer
    // in native bytes; process()/runAudioCallback convert to the application format.
    const int nativeBytesPerFrame = static_cast<int>(m_format.channelCount()
            * QAudioHelperInternal::bytesPerSample(m_nativeFormat));
    const size_t hostBytes = static_cast<size_t>(framesToRead) * nativeBytesPerFrame;

    return withTemporaryBuffer(hostBytes, [&](QSpan<std::byte> hostBufferSpan) -> bool {
        const snd_pcm_sframes_t framesRead =
                snd_pcm_readi(m_handle, hostBufferSpan.data(), framesToRead);
        if (framesRead == -EAGAIN)
            return true; // raced poll(); the outer loop will poll again.
        if (framesRead < 1) {
            if (isStopRequested(std::memory_order_acquire))
                return false;
            if (const int err = recoverFromXrun(static_cast<int>(framesRead)); err < 0) {
                handleSndPcmError(err);
                return false;
            }
            return true;
        }

        // framesRead >= 1 with a positive nativeBytesPerFrame, so the byte count is
        // always valid (no snd_pcm_frames_to_bytes stale-handle guard needed).
        const qsizetype bytesRead = static_cast<qsizetype>(framesRead) * nativeBytesPerFrame;
        QSpan<const std::byte> filled = hostBufferSpan.first(bytesRead);

        if (m_audioCallback) {
            runAudioCallback(*m_audioCallback, filled, m_format, volume(), m_nativeFormat);
        } else {
            const uint64_t framesWritten =
                    QPlatformAudioSourceStream::process(filled, framesRead, m_nativeFormat);
            if (framesWritten != static_cast<uint64_t>(framesRead)) {
                invokeOnAppThread([self = shared_from_this()] {
                    // updateStreamIdle reads m_parent atomically and no-ops on null.
                    self->updateStreamIdle(true);
                });
            }
        }
        return true;
    });
}

void QQnxSndAudioSourceStream::handleSndPcmError(int err)
{
    requestStop();
    // Read m_parent at fire-time inside the lambda; see QQnxSndAudioSinkStream
    // counterpart for the lifetime argument.
    invokeOnAppThread([self = shared_from_this(), err] {
        qCWarning(lcQnxSndInput) << "audio input error, stopping stream:"
                                 << (err ? snd_strerror(err) : "I/O error");
        if (auto *parent = self->m_parent.load(std::memory_order_acquire))
            self->handleIOError(parent);
    });
}

///////////////////////////////////////////////////////////////////////////////////////////////////

QQnxSndAudioSource::QQnxSndAudioSource(QAudioDevice device, const QAudioFormat &format,
                                       QObject *parent)
    : BaseClass{ std::move(device), format, parent }
{
}

QQnxSndAudioSource::~QQnxSndAudioSource()
{
    // See QQnxSndAudioSink counterpart. Discard joins the worker
    // synchronously and clears m_parent before the front class is freed.
    if (m_stream) {
        m_stream->stop(ShutdownPolicy::DiscardRingbuffer);
        m_stream = {};
    }
}

QT_END_NAMESPACE
