// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQNXSNDHELPERS_P_H
#define QQNXSNDHELPERS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <alsa/asoundlib.h>

#include <QtCore/qbytearray.h>
#include <QtCore/qloggingcategory.h>
#include <QtMultimedia/qaudioformat.h>
#include <QtMultimedia/private/qaudiohelpers_p.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>

QT_BEGIN_NAMESPACE

namespace QnxSndHelpers {

// snd_pcm_resume retries before falling back to snd_pcm_prepare.
inline constexpr int kSuspendResumeRetryLimit = 5;
// Per-retry delay during suspend/resume recovery (ALSA cookbook idiom).
inline constexpr std::chrono::milliseconds kSuspendResumeRetryDelay{ 100 };
// Number of periods (chunks) per ALSA buffer: buffer = period * chunks. 2 is
// double-buffering, 3 gives a little more slack; overridable via env var.
inline constexpr unsigned kDefaultPeriodCount = 3;
// Acceptable env-var override range for the period count.
inline constexpr unsigned kMinPeriodCount = 2;
inline constexpr unsigned kMaxPeriodCount = 16;
// Default ALSA period size in frames, used unless the caller passes a
// NativePeriodFrames override. ~21 ms at 48 kHz.
inline constexpr snd_pcm_uframes_t kDefaultPeriodFrames = 1024;
// Worker-thread real-time priority band. io-snd runs its own data/helper threads
// at SCHED_RR 24/25 (mixer/irq higher); we stay strictly below 24 so we never
// preempt them (raising into that band REPLY-block-wedges the driver), but above
// the default application priority (10) so a busy app thread cannot preempt the
// audio worker. Overridable within the band via QT_QNXSND_WORKER_PRIO.
inline constexpr int kMinWorkerPriority = 11;
inline constexpr int kMaxWorkerPriority = 23;
inline constexpr int kDefaultWorkerPriority = 20;
// Self-pipe to wake a worker blocked in poll() on a PCM fd: the worker adds
// readFd() to its poll set; another thread calls wake() to return the poll().
// RAII closes both fds. wake()/drain() are const (they touch the pipe, not the
// object's own state).
class WakePipe
{
public:
    WakePipe() = default;
    ~WakePipe();
    Q_DISABLE_COPY_MOVE(WakePipe)

    bool open();              // create a non-blocking pipe; false on failure
    void close() noexcept;
    void wake() const noexcept;   // write one byte (no-op if a wake is pending)
    void drain() const noexcept;  // discard all pending wake bytes
    void waitForWake() const noexcept; // block until wake() fires, then drain
    int readFd() const noexcept { return m_fds[0]; }
    bool isOpen() const noexcept { return m_fds[0] >= 0; }

private:
    int m_fds[2] = { -1, -1 };
};

// Block until the PCM handle is ready for I/O (POLLOUT for playback / POLLIN for
// capture, as set by snd_pcm_poll_descriptors) or the wake pipe is signalled:
//   Ready - the PCM reported its I/O revent; the caller may write/read a period
//   Woken - the wake pipe fired (re-check stop/suspend); the pipe is drained
//   Error - poll() failed or the PCM reported POLLERR/POLLHUP/POLLNVAL
enum class PollOutcome { Ready, Woken, Error };
PollOutcome pollPcm(snd_pcm_t *handle, const WakePipe &wake);

// RAII guard for the void** array returned by snd_device_name_hint.
struct HintsDeleter
{
    void operator()(void **h) const noexcept
    {
        if (h)
            snd_device_name_free_hint(h);
    }
};
using HintsGuard = std::unique_ptr<void *, HintsDeleter>;

// Map a QAudioFormat::SampleFormat to the corresponding ALSA PCM format.
// Returns SND_PCM_FORMAT_UNKNOWN for Unknown / NSampleFormats / unhandled.
snd_pcm_format_t mapSampleFormat(QAudioFormat::SampleFormat) noexcept;

// Recover from xrun / suspend states; returns the resulting errno.
int recoverFromXrun(snd_pcm_t *handle, int err);

// Start a prepared PCM. No-op (returns 0) if it already left PREPARED, so a
// caller can prefill-then-start without racing the device's auto-start.
// Returns the snd_pcm_start errno otherwise.
int startPcm(snd_pcm_t *handle);

// Raise the *calling* (worker) thread to SCHED_FIFO at a priority in the
// [kMinWorkerPriority, kMaxWorkerPriority] band, overridable via the
// QT_QNXSND_WORKER_PRIO environment variable. On failure, logs via the supplied
// category and leaves the thread's inherited scheduling unchanged.
void setWorkerRealtimePriority(const QLoggingCategory &category);

// Inputs for openConfiguredPcm: everything that differs between the
// playback and capture paths.
struct PcmOpenConfig
{
    snd_pcm_stream_t direction;            // SND_PCM_STREAM_PLAYBACK / _CAPTURE
    QByteArray deviceId;
    QAudioFormat format;
    const QLoggingCategory &category;
    const char *streamLabel;               // "Playback" or "Capture", embedded in log strings
    const char *periodCountEnvVar;         // e.g. "QT_QNXSND_OUTPUT_PERIODS"; chunk-count override
    std::optional<uint32_t> periodFrames;  // NativePeriodFrames override; nullopt -> kDefaultPeriodFrames
};

// Outputs from openConfiguredPcm. handle is null on failure (use operator bool).
struct PcmOpenResult
{
    snd_pcm_t *handle = nullptr;
    snd_pcm_uframes_t periodFrames = 0;
    snd_pcm_uframes_t bufferFrames = 0;
    qsizetype periodBytes = 0;
    // The device's chosen native sample format (may be a 24-bit format that
    // QAudioFormat cannot express). The stream converts between this and the
    // application format via the base-class process()/runAudioCallback helpers.
    QAudioHelperInternal::NativeSampleFormat nativeFormat =
            QAudioHelperInternal::NativeSampleFormat::int16_t;

    explicit operator bool() const noexcept { return handle != nullptr; }
};

// Open a PCM device, configure hw/sw params and prepare it. The stream is left
// PREPARED but NOT started — the worker thread starts it (see startPcm), after
// prefilling the first period on the playback side. Returns a populated
// PcmOpenResult on success; on failure the handle is closed internally and the
// result evaluates to false.
PcmOpenResult openConfiguredPcm(const PcmOpenConfig &config);

} // namespace QnxSndHelpers

QT_END_NAMESPACE

#endif // QQNXSNDHELPERS_P_H
