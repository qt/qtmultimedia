// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QCOREAAUDIOSOURCE_P_H
#define QCOREAAUDIOSOURCE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtMultimedia/private/qaudio_platform_implementation_support_p.h>
#include <QtMultimedia/private/qcoreaudioutils_p.h>
#include <QtMultimedia/private/qcoreaudiodevices_p.h>
#ifndef Q_OS_MACOS
#  include <QtMultimedia/private/qavaudiosessionrecovery_p.h>
#endif

#include <AudioUnit/AudioUnit.h>
#include <vector>

typedef struct OpaqueAudioConverter *AudioConverterRef;

QT_BEGIN_NAMESPACE

class QCoreAudioSource;

class QCoreAudioSourceStream final : public QtMultimediaPrivate::QPlatformAudioSourceStream
{
    using QPlatformAudioSourceStream = QtMultimediaPrivate::QPlatformAudioSourceStream;

public:
    using SourceType = QCoreAudioSource;

    explicit QCoreAudioSourceStream(QAudioDevice, const QAudioFormat &,
                                    std::optional<int> ringbufferSize, QCoreAudioSource *parent,
                                    float volume, std::optional<QtMultimediaPrivate::NativePeriodFrames> nativePeriodFrames);
    Q_DISABLE_COPY_MOVE(QCoreAudioSourceStream)
    ~QCoreAudioSourceStream();

    bool open();

    bool start(QIODevice *);
    QIODevice *start();
    bool start(AudioCallback &&);
    void stop(ShutdownPolicy);

    void suspend();
    bool resume();

    using QPlatformAudioSourceStream::bytesReady;
    using QPlatformAudioSourceStream::deviceIsRingbufferReader;
    using QPlatformAudioSourceStream::processedDuration;
    using QPlatformAudioSourceStream::ringbufferSizeInBytes;
    using QPlatformAudioSourceStream::setVolume;

    bool resumeIfNecessary();

    void reportIOError();
    void invalidateAudioUnit();

private:
    void updateStreamIdle(bool idle) override;

    bool startAudioUnit();
    void stopAudioUnit();

    OSStatus processInput(AudioUnitRenderActionFlags *ioActionFlags,
                          const AudioTimeStamp *timeStamp, UInt32 inBusNumber,
                          UInt32 inNumberFrames,
                          AudioBufferList *ioData) noexcept Q_DECL_NONBLOCKING_FUNCTION;

    OSStatus processRingbuffer(QSpan<const std::byte> inputSpan,
                               UInt32 inNumberFrames) noexcept Q_DECL_NONBLOCKING_FUNCTION;
    OSStatus processAudioCallback(QSpan<const std::byte> inputSpan) noexcept Q_DECL_NONBLOCKING_FUNCTION;

#ifdef Q_OS_MACOS
    bool setDisconnectListener(AudioObjectID id);

    QCoreAudioUtils::DeviceDisconnectMonitor m_disconnectMonitor;
    QFuture<void> m_stopOnDisconnected;
#endif

    QCoreAudioUtils::AudioUnitHandle m_audioUnit;

    std::optional<AudioCallback> m_audioCallback;
    QCoreAudioSource *m_parent;

    AudioBufferList m_bufferList{};
    UInt32 m_bufferCapacity{};

    // for run-time conversions
    AudioConverterRef m_audioConverter{ nullptr };
    std::vector<uint8_t> m_outputBuffer;
    AudioBufferList m_outputBufferList{};

#ifndef Q_OS_MACOS
    // Held for as long as this stream wants the shared AVAudioSession active; acquired in
    // start(), released in stop(). suspend()/resume() (interruption handling) must not touch
    // it: the stream still wants the session active across a suspend/resume cycle, only the
    // OS-level session state changed.
    std::optional<QtMultimediaPrivate::QAVAudioSessionManager::ActivationToken> m_sessionActivation;
#endif
};

class QCoreAudioSource final
    : public QtMultimediaPrivate::QPlatformAudioSourceImplementationWithCallback<
              QCoreAudioSourceStream, QCoreAudioSource>
{
    using BaseClass = QtMultimediaPrivate::QPlatformAudioSourceImplementationWithCallback<
            QCoreAudioSourceStream, QCoreAudioSource>;

public:
    QCoreAudioSource(QAudioDevice device, const QAudioFormat &format, QObject *parent);
    ~QCoreAudioSource() override;

    void resumeStreamIfNecessary();

#ifndef Q_OS_MACOS
    void resume() override;

    // Interface required by QAVAudioSessionRecovery.
    QCoreAudioSourceStream *currentStream() const { return m_stream.get(); }
    using BaseClass::updateStreamState;

    void notifyStreamStopped() { m_recovery.streamStopped(); }

private:
    QtMultimediaPrivate::QAVAudioSessionRecovery<QCoreAudioSource> m_recovery{ *this };
#endif
};

QT_END_NAMESPACE

#endif // QCOREAAUDIOSOURCE_P_H
