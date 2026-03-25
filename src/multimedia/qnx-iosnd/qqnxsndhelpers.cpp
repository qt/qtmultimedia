// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqnxsndhelpers_p.h"

#include <QtCore/qsysinfo.h>
#include <QtCore/qthread.h>
#include <QtCore/qvarlengtharray.h>

#include <array>
#include <cerrno>

#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>

QT_BEGIN_NAMESPACE

namespace QnxSndHelpers {

WakePipe::~WakePipe()
{
    close();
}

bool WakePipe::open()
{
    if (isOpen())
        return true;
    if (::pipe(m_fds) != 0)
        return false;
    // Non-blocking both ends so wake()/drain() never block.
    for (int fd : m_fds)
        ::fcntl(fd, F_SETFL, ::fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
    return true;
}

void WakePipe::close() noexcept
{
    for (int &fd : m_fds) {
        if (fd >= 0)
            ::close(fd);
        fd = -1;
    }
}

void WakePipe::wake() const noexcept
{
    if (m_fds[1] < 0)
        return;
    const char byte = 1;
    // A full pipe already means "wake pending", so EAGAIN is fine to ignore.
    ssize_t n = ::write(m_fds[1], &byte, 1);
    Q_UNUSED(n);
}

void WakePipe::drain() const noexcept
{
    if (m_fds[0] < 0)
        return;
    char buf[64];
    while (::read(m_fds[0], buf, sizeof(buf)) > 0) { }
}

void WakePipe::waitForWake() const noexcept
{
    if (m_fds[0] < 0)
        return;
    pollfd pfd = { m_fds[0], POLLIN, 0 };
    // The wake byte persists until drained, so a wake() racing just ahead of
    // poll() is not missed.
    if (::poll(&pfd, 1, -1) > 0)
        drain();
}

PollOutcome pollPcm(snd_pcm_t *handle, const WakePipe &wake)
{
    int count = snd_pcm_poll_descriptors_count(handle);
    if (count <= 0)
        return PollOutcome::Error;

    // io-snd reports one descriptor; the fixed buffer holds it plus the
    // wake-pipe fd without a per-period heap allocation. The capacity is sized
    // well above the single descriptor SALSA reports; assert in debug so a
    // future PCM type that needs more is caught rather than silently truncated.
    constexpr int kMaxPollDescriptors = 8; // PCM descriptors + 1 wake-pipe fd
    std::array<pollfd, kMaxPollDescriptors> fds{};
    Q_ASSERT(count <= int(fds.size()) - 1);
    if (count > int(fds.size()) - 1)
        count = int(fds.size()) - 1;
    const int n = snd_pcm_poll_descriptors(handle, fds.data(), count);
    if (n < 0)
        return PollOutcome::Error;

    fds[n].fd = wake.readFd();
    fds[n].events = POLLIN;

    const int rc = ::poll(fds.data(), n + 1, -1);
    if (rc < 0)
        return errno == EINTR ? PollOutcome::Woken : PollOutcome::Error;

    if (fds[n].revents & POLLIN) {
        wake.drain();
        return PollOutcome::Woken;
    }

    unsigned short revents = 0;
    if (snd_pcm_poll_descriptors_revents(handle, fds.data(), n, &revents) < 0)
        return PollOutcome::Error;
    if (revents & (POLLERR | POLLNVAL | POLLHUP))
        return PollOutcome::Error;
    if (revents & (POLLIN | POLLOUT))
        return PollOutcome::Ready;

    // Spurious wakeup: re-evaluate state.
    return PollOutcome::Woken;
}

snd_pcm_format_t mapSampleFormat(QAudioFormat::SampleFormat fmt) noexcept
{
    constexpr bool isBigEndian = QSysInfo::ByteOrder == QSysInfo::BigEndian;

    switch (fmt) {
    case QAudioFormat::UInt8:
        return SND_PCM_FORMAT_U8;
    case QAudioFormat::Int16:
        return isBigEndian ? SND_PCM_FORMAT_S16_BE : SND_PCM_FORMAT_S16_LE;
    case QAudioFormat::Int32:
        return isBigEndian ? SND_PCM_FORMAT_S32_BE : SND_PCM_FORMAT_S32_LE;
    case QAudioFormat::Float:
        return isBigEndian ? SND_PCM_FORMAT_FLOAT_BE : SND_PCM_FORMAT_FLOAT_LE;
    case QAudioFormat::Unknown:
    case QAudioFormat::NSampleFormats:
        break;
    }
    return SND_PCM_FORMAT_UNKNOWN;
}

namespace {

// Map a NativeSampleFormat (incl. the 24-bit formats QAudioFormat cannot
// express) to the corresponding ALSA PCM format for this platform's byte order.
snd_pcm_format_t nativeToPcmFormat(QAudioHelperInternal::NativeSampleFormat fmt) noexcept
{
    using QAudioHelperInternal::NativeSampleFormat;
    constexpr bool isBigEndian = QSysInfo::ByteOrder == QSysInfo::BigEndian;

    switch (fmt) {
    case NativeSampleFormat::uint8_t:
        return SND_PCM_FORMAT_U8;
    case NativeSampleFormat::int16_t:
        return isBigEndian ? SND_PCM_FORMAT_S16_BE : SND_PCM_FORMAT_S16_LE;
    case NativeSampleFormat::int24_t_3b:
        return isBigEndian ? SND_PCM_FORMAT_S24_3BE : SND_PCM_FORMAT_S24_3LE;
    case NativeSampleFormat::int24_t_4b_low:
        return isBigEndian ? SND_PCM_FORMAT_S24_BE : SND_PCM_FORMAT_S24_LE;
    case NativeSampleFormat::int32_t:
        return isBigEndian ? SND_PCM_FORMAT_S32_BE : SND_PCM_FORMAT_S32_LE;
    case NativeSampleFormat::float32_t:
        return isBigEndian ? SND_PCM_FORMAT_FLOAT_BE : SND_PCM_FORMAT_FLOAT_LE;
    }
    return SND_PCM_FORMAT_UNKNOWN;
}

// "Always best-native + convert": probe the device-native formats the opened PCM
// accepts and pick the best one for the requested application format, then set it.
// Returns the chosen NativeSampleFormat (the application/native conversion is done
// by the base-class process()/runAudioCallback helpers). On failure returns the
// snd_pcm error code via err and leaves the result format unset.
int negotiateNativeFormat(snd_pcm_t *handle, snd_pcm_hw_params_t *hwparams,
                          const QAudioFormat &format, const QLoggingCategory &category,
                          QAudioHelperInternal::NativeSampleFormat &chosen)
{
    using QAudioHelperInternal::NativeSampleFormat;
    // Candidates in ascending byte width; bestNativeSampleFormat picks among the
    // subset the device actually supports according to its precision heuristics.
    static constexpr NativeSampleFormat candidates[] = {
        NativeSampleFormat::uint8_t,        NativeSampleFormat::int16_t,
        NativeSampleFormat::int24_t_3b,     NativeSampleFormat::int24_t_4b_low,
        NativeSampleFormat::int32_t,        NativeSampleFormat::float32_t,
    };

    QVarLengthArray<NativeSampleFormat, std::size(candidates)> supported;
    for (NativeSampleFormat nf : candidates) {
        if (snd_pcm_hw_params_test_format(handle, hwparams, nativeToPcmFormat(nf)) == 0)
            supported.append(nf);
    }
    if (supported.isEmpty()) {
        qCWarning(category) << "device supports no usable native sample format";
        return -EINVAL;
    }

    chosen = QAudioHelperInternal::bestNativeSampleFormat(format, supported);
    qCDebug(category) << "native format: requested" << format.sampleFormat()
                      << "device-supported" << supported << "chosen" << chosen;
    return snd_pcm_hw_params_set_format(handle, hwparams, nativeToPcmFormat(chosen));
}

} // namespace

int recoverFromXrun(snd_pcm_t *handle, int err)
{
    int estrpipe = EIO;
#ifdef ESTRPIPE
    estrpipe = ESTRPIPE;
#endif

    if (err == -EPIPE) {
        err = snd_pcm_prepare(handle);
    } else if ((err == -estrpipe) || (err == -EIO)) {
        int count = 0;
        // ALSA cookbook idiom: retry snd_pcm_resume in ~100 ms increments.
        while ((err = snd_pcm_resume(handle)) == -EAGAIN) {
            QThread::msleep(kSuspendResumeRetryDelay.count());
            if (++count > kSuspendResumeRetryLimit)
                break;
        }
        if (err < 0)
            err = snd_pcm_prepare(handle);
    }
    return err;
}

namespace {
// Period (chunk) count from the environment, clamped to [kMinPeriodCount,
// kMaxPeriodCount]. Returns kDefaultPeriodCount when unset or out of range.
unsigned periodCountFromEnv(const char *name, const QLoggingCategory &category)
{
    bool ok = false;
    const int value = qEnvironmentVariableIntValue(name, &ok);
    if (!ok)
        return kDefaultPeriodCount;
    if (value < int(kMinPeriodCount) || value > int(kMaxPeriodCount)) {
        qCWarning(category) << name << "value" << value << "is outside ["
                            << kMinPeriodCount << "," << kMaxPeriodCount
                            << "]; using default" << kDefaultPeriodCount;
        return kDefaultPeriodCount;
    }
    return static_cast<unsigned>(value);
}
} // namespace

int startPcm(snd_pcm_t *handle)
{
    // A prefill write may have already auto-started the stream via the device's
    // default start threshold, so only start it if it is still PREPARED.
    if (snd_pcm_state(handle) != SND_PCM_STATE_PREPARED)
        return 0;
    // -EAGAIN means "will start on first I/O" — not fatal for a non-blocking PCM.
    const int err = snd_pcm_start(handle);
    return (err < 0 && err != -EAGAIN) ? err : 0;
}

void setWorkerRealtimePriority(const QLoggingCategory &category)
{
    int prio = kDefaultWorkerPriority;
    bool ok = false;
    if (const int value = qEnvironmentVariableIntValue("QT_QNXSND_WORKER_PRIO", &ok); ok) {
        if (value < kMinWorkerPriority || value > kMaxWorkerPriority) {
            qCWarning(category) << "QT_QNXSND_WORKER_PRIO value" << value << "is outside ["
                                << kMinWorkerPriority << "," << kMaxWorkerPriority
                                << "]; using default" << kDefaultWorkerPriority;
        } else {
            prio = value;
        }
    }

    sched_param param = {};
    param.sched_priority = prio;
    // Raise the worker into the 11-23 band so a busy application thread (default
    // priority 10) cannot preempt the audio feed, while staying strictly below
    // io-snd's data/helper threads (24/25). On failure keep inherited scheduling
    // rather than aborting the stream.
    if (const int rc = pthread_setschedparam(pthread_self(), SCHED_FIFO, &param); rc != 0) {
        qCWarning(category) << "could not set worker SCHED_FIFO priority" << prio << ":"
                            << qt_error_string(rc) << "- keeping inherited scheduling";
        return;
    }
    qCDebug(category) << "worker thread running SCHED_FIFO at priority" << prio;
}

PcmOpenResult openConfiguredPcm(const PcmOpenConfig &config)
{
    const QLoggingCategory &category = config.category;

    PcmOpenResult result;

    qCDebug(category) << "Opening" << config.streamLabel
                      << "device:" << config.deviceId;

    int dir = 0;
    snd_pcm_t *handle = nullptr;

    // Open non-blocking: a blocking open (flag 0) stalls this (app) thread
    // indefinitely if the device is held exclusively by another client, and the
    // later snd_pcm_nonblock() cannot unblock an open already in progress. The
    // worker gates all subsequent I/O on poll(), so writei/readi return -EAGAIN
    // instead of blocking. Mirrors probeDeviceFormat's open in qqnxsndaudiodevice.
    int err = snd_pcm_open(&handle, config.deviceId.constData(), config.direction,
                           SND_PCM_NONBLOCK);
    if (err < 0 || handle == nullptr) {
        qCWarning(category) << "Failed to open" << config.streamLabel
                            << "device:" << config.deviceId
                            << "error:" << snd_strerror(err);
        return {};
    }

    auto fail = [&](const char *what) -> PcmOpenResult {
        qCWarning(category) << config.streamLabel << ":" << what
                            << "err =" << snd_strerror(err);
        snd_pcm_close(handle);
        return {};
    };

    snd_pcm_hw_params_t *hwparams;
    snd_pcm_hw_params_alloca(&hwparams);

    unsigned int sampleRate = config.format.sampleRate();

    if ((err = snd_pcm_hw_params_any(handle, hwparams)) < 0)
        return fail("snd_pcm_hw_params_any");

    if (snd_pcm_hw_params_set_rate_resample(handle, hwparams, 1) < 0) {
        // QNX SALSA has no plugin/resampling layer (unlike Linux ALSA), so this
        // returns -EINVAL; rate handling is left to the device. Non-fatal.
        qCDebug(category) << "snd_pcm_hw_params_set_rate_resample not supported, continuing";
    }

    if ((err = snd_pcm_hw_params_set_access(handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
        return fail("snd_pcm_hw_params_set_access");

    if ((err = negotiateNativeFormat(handle, hwparams, config.format, category,
                                     result.nativeFormat)) < 0)
        return fail("snd_pcm_hw_params_set_format");

    if ((err = snd_pcm_hw_params_set_channels(
                 handle, hwparams,
                 static_cast<unsigned int>(config.format.channelCount()))) < 0)
        return fail("snd_pcm_hw_params_set_channels");

    if ((err = snd_pcm_hw_params_set_rate_near(handle, hwparams, &sampleRate, 0)) < 0)
        return fail("snd_pcm_hw_params_set_rate_near");

    // Buffer = period * chunks: period frames (NativePeriodFrames override or
    // default) and chunk count, both clamped to the device range by _near.
    snd_pcm_uframes_t periodFrames = config.periodFrames.value_or(kDefaultPeriodFrames);
    if ((err = snd_pcm_hw_params_set_period_size_near(handle, hwparams, &periodFrames, &dir)) < 0)
        return fail("snd_pcm_hw_params_set_period_size_near");

    unsigned int chunks = periodCountFromEnv(config.periodCountEnvVar, category);
    if ((err = snd_pcm_hw_params_set_periods_near(handle, hwparams, &chunks, &dir)) < 0)
        return fail("snd_pcm_hw_params_set_periods_near");

    if ((err = snd_pcm_hw_params(handle, hwparams)) < 0)
        return fail("snd_pcm_hw_params");

    if ((err = snd_pcm_hw_params_get_buffer_size(hwparams, &result.bufferFrames)) < 0)
        return fail("snd_pcm_hw_params_get_buffer_size");
    if ((err = snd_pcm_hw_params_get_period_size(hwparams, &result.periodFrames, &dir)) < 0)
        return fail("snd_pcm_hw_params_get_period_size");
    result.periodBytes = snd_pcm_frames_to_bytes(handle, result.periodFrames);
    if (result.periodFrames == 0 || result.periodBytes <= 0) {
        err = -EINVAL;
        return fail("invalid period geometry");
    }

    snd_pcm_sw_params_t *swparams;
    snd_pcm_sw_params_alloca(&swparams);

    if ((err = snd_pcm_sw_params_current(handle, swparams)) < 0)
        return fail("snd_pcm_sw_params_current");
    // avail_min = one period, so poll() reports ready once a full period of
    // space/data is available. Start/stop thresholds stay at their defaults; the
    // worker starts the stream explicitly (startPcm).
    if ((err = snd_pcm_sw_params_set_avail_min(handle, swparams, result.periodFrames)) < 0)
        return fail("snd_pcm_sw_params_set_avail_min");
    if ((err = snd_pcm_sw_params(handle, swparams)) < 0)
        return fail("snd_pcm_sw_params");

    if ((err = snd_pcm_prepare(handle)) < 0)
        return fail("snd_pcm_prepare");

    result.handle = handle;

    qCDebug(category) << config.streamLabel << "opened:" << config.deviceId
                      << "buffer_frames:" << result.bufferFrames
                      << "period_frames:" << result.periodFrames
                      << "period_bytes:" << result.periodBytes
                      << "chunks:" << chunks;
    return result;
}

} // namespace QnxSndHelpers

QT_END_NAMESPACE
