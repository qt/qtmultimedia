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

#ifndef QQNXSNDAUDIOSOURCE_P_H
#define QQNXSNDAUDIOSOURCE_P_H

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

class QQnxSndAudioSource;

struct QQnxSndAudioSourceStream final
    : std::enable_shared_from_this<QQnxSndAudioSourceStream>,
      QtMultimediaPrivate::QPlatformAudioSourceStream
{
    using SourceType = QQnxSndAudioSource;
    using AudioCallback = QtMultimediaPrivate::QPlatformAudioSourceStream::AudioCallback;
    using ShutdownPolicy = QtMultimediaPrivate::QPlatformAudioIOStream::ShutdownPolicy;
    using NativePeriodFrames = QtMultimediaPrivate::NativePeriodFrames;

    QQnxSndAudioSourceStream(QAudioDevice, const QAudioFormat &,
                             std::optional<qsizetype> ringbufferSize, QQnxSndAudioSource *parent,
                             float volume, std::optional<NativePeriodFrames> nativePeriodFrames);
    Q_DISABLE_COPY_MOVE(QQnxSndAudioSourceStream)
    ~QQnxSndAudioSourceStream();

    using QtMultimediaPrivate::QPlatformAudioSourceStream::bytesReady;
    using QtMultimediaPrivate::QPlatformAudioSourceStream::deviceIsRingbufferReader;
    using QtMultimediaPrivate::QPlatformAudioSourceStream::processedDuration;
    using QtMultimediaPrivate::QPlatformAudioSourceStream::ringbufferSizeInBytes;
    using QtMultimediaPrivate::QPlatformAudioSourceStream::setVolume;

    bool open();
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
    void startWorker();
    void joinWorkerThread();
    void runProcessLoop();
    bool processOnePeriod();
    void handleSndPcmError(int err = 0);
    void closePcmDevice();

    snd_pcm_t *m_handle = nullptr;
    snd_pcm_uframes_t m_periodFrames = 0;
    // Device-native sample format chosen at open (best-native + convert); the
    // host buffer read from io-snd is in this format, converted to the
    // application format by the base-class process()/runAudioCallback helpers.
    QAudioHelperInternal::NativeSampleFormat m_nativeFormat =
            QAudioHelperInternal::NativeSampleFormat::int16_t;

    std::atomic_bool m_suspended = false;

    std::unique_ptr<QThread> m_workerThread;
    // Wakes the worker out of poll() for stop/suspend/resume (see runProcessLoop).
    QnxSndHelpers::WakePipe m_wakePipe;
    std::optional<AudioCallback> m_audioCallback = std::nullopt;

    // See QQnxSndAudioSinkStream::m_parent: atomic to bridge worker reads
    // and app-thread writes safely.
    std::atomic<QQnxSndAudioSource *> m_parent{ nullptr };
};

class QQnxSndAudioSource final
    : public QtMultimediaPrivate::QPlatformAudioSourceImplementationWithCallback<
              QQnxSndAudioSourceStream, QQnxSndAudioSource>
{
    using BaseClass = QtMultimediaPrivate::QPlatformAudioSourceImplementationWithCallback<
            QQnxSndAudioSourceStream, QQnxSndAudioSource>;

public:
    QQnxSndAudioSource(QAudioDevice, const QAudioFormat &, QObject *parent);
    ~QQnxSndAudioSource() override;
};

QT_END_NAMESPACE

#endif // QQNXSNDAUDIOSOURCE_P_H
