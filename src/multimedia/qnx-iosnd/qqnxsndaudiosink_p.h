// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

#ifndef QQNXSNDAUDIOSINK_P_H
#define QQNXSNDAUDIOSINK_P_H

#include <alsa/asoundlib.h>

#include <QtCore/qiodevice.h>
#include <QtCore/qthread.h>
#include <QtCore/qtclasshelpermacros.h>
#include <QtMultimedia/qaudio.h>
#include <QtMultimedia/qaudiodevice.h>
#include <QtMultimedia/private/qaudiosystem_p.h>
#include <QtMultimedia/private/qaudiosystem_platform_stream_support_p.h>
#include <QtMultimedia/private/qaudio_platform_implementation_support_p.h>

#include "qqnxsndhelpers_p.h"

#include <atomic>
#include <memory>
#include <optional>

QT_BEGIN_NAMESPACE

class QQnxSndAudioSink;

struct QQnxSndAudioSinkStream final
    : std::enable_shared_from_this<QQnxSndAudioSinkStream>,
      QtMultimediaPrivate::QPlatformAudioSinkStream
{
    using SinkType = QQnxSndAudioSink;
    using AudioCallback = QtMultimediaPrivate::QPlatformAudioSinkStream::AudioCallback;
    using ShutdownPolicy = QtMultimediaPrivate::QPlatformAudioIOStream::ShutdownPolicy;
    using AudioEndpointRole = QtMultimediaPrivate::AudioEndpointRole;
    using NativePeriodFrames = QtMultimediaPrivate::NativePeriodFrames;

    enum class StreamType : uint8_t {
        Ringbuffer,
        Callback,
    };

    QQnxSndAudioSinkStream(QAudioDevice, const QAudioFormat &,
                           std::optional<qsizetype> ringbufferSize, QQnxSndAudioSink *parent,
                           float volume, std::optional<NativePeriodFrames> nativePeriodFrames,
                           AudioEndpointRole);
    Q_DISABLE_COPY_MOVE(QQnxSndAudioSinkStream)
    ~QQnxSndAudioSinkStream();

    bool open();

    using QtMultimediaPrivate::QPlatformAudioSinkStream::bytesFree;
    using QtMultimediaPrivate::QPlatformAudioSinkStream::processedDuration;
    using QtMultimediaPrivate::QPlatformAudioSinkStream::ringbufferSizeInBytes;
    using QtMultimediaPrivate::QPlatformAudioSinkStream::setVolume;

    bool start(QIODevice *);
    QIODevice *start();
    bool start(AudioCallback);

    void suspend();
    void resume();
    void stop(ShutdownPolicy);

    void updateStreamIdle(bool) override;

private:
    bool openPcmDevice();
    int recoverFromXrun(int err);
    void startWorker(StreamType);
    void joinWorkerThread();
    void runProcessLoop(StreamType);
    bool processOnePeriod(StreamType);
    void handleSndPcmError(int err = 0);
    void closePcmDevice();

    snd_pcm_t *m_handle = nullptr;
    snd_pcm_uframes_t m_periodFrames = 0;
    // Device-native sample format chosen at open (best-native + convert); the
    // host buffer written to io-snd is in this format, converted from the
    // application format by the base-class process()/runAudioCallback helpers.
    QAudioHelperInternal::NativeSampleFormat m_nativeFormat =
            QAudioHelperInternal::NativeSampleFormat::int16_t;

    std::atomic_bool m_suspended = false;
    std::atomic<ShutdownPolicy> m_shutdownPolicy = ShutdownPolicy::DiscardRingbuffer;
    QtPrivate::QAutoResetEvent m_ringbufferDrained;

    std::unique_ptr<QThread> m_workerThread;
    // Wakes the worker out of poll() for stop/suspend/resume (see runProcessLoop).
    QnxSndHelpers::WakePipe m_wakePipe;
    // Optional (matching the source) so the Callback path can guard on it being
    // set rather than relying on runAudioCallback's Q_PRESUME(valid).
    std::optional<AudioCallback> m_audioCallback = std::nullopt;

    // Read on the worker thread inside updateStreamIdle and inside lambdas
    // posted via invokeOnAppThread; written on the app thread from stop().
    // Atomic to avoid the read/write race; the front class is responsible
    // for joining the worker before its own destruction so the pointer
    // cannot become dangling once read.
    std::atomic<QQnxSndAudioSink *> m_parent{ nullptr };
};

class QQnxSndAudioSink final
    : public QtMultimediaPrivate::QPlatformAudioSinkImplementation<QQnxSndAudioSinkStream,
                                                                   QQnxSndAudioSink>
{
    using BaseClass = QtMultimediaPrivate::QPlatformAudioSinkImplementation<QQnxSndAudioSinkStream,
                                                                            QQnxSndAudioSink>;

public:
    QQnxSndAudioSink(QAudioDevice, const QAudioFormat &, QObject *parent);
    ~QQnxSndAudioSink() override;
};

QT_END_NAMESPACE

#endif // QQNXSNDAUDIOSINK_P_H
