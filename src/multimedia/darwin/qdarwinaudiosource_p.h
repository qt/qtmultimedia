// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QDARWINAUDIOSOURCE_P_H
#define QDARWINAUDIOSOURCE_P_H

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
#include <QtMultimedia/private/qdarwinaudiodevices_p.h>

#include <AudioUnit/AudioUnit.h>
#include <vector>

typedef struct OpaqueAudioConverter *AudioConverterRef;

QT_BEGIN_NAMESPACE

class QDarwinAudioSource;

class QCoreAudioSourceStream final : public QtMultimediaPrivate::QPlatformAudioSourceStream
{
    using QPlatformAudioSourceStream = QtMultimediaPrivate::QPlatformAudioSourceStream;

public:
    using SourceType = QDarwinAudioSource;

    explicit QCoreAudioSourceStream(QAudioDevice, const QAudioFormat &,
                                    std::optional<int> ringbufferSize, QDarwinAudioSource *parent,
                                    float volume, std::optional<int32_t> hardwareBufferFrames);
    Q_DISABLE_COPY_MOVE(QCoreAudioSourceStream)
    ~QCoreAudioSourceStream();

    bool open();

    bool start(QIODevice *);
    QIODevice *start();
    bool start(AudioCallback &&);
    void stop(ShutdownPolicy);

    void suspend();
    void resume();

    using QPlatformAudioSourceStream::bytesReady;
    using QPlatformAudioSourceStream::deviceIsRingbufferReader;
    using QPlatformAudioSourceStream::processedDuration;
    using QPlatformAudioSourceStream::ringbufferSizeInBytes;
    using QPlatformAudioSourceStream::setVolume;

    void resumeIfNecessary();

private:
    void updateStreamIdle(bool idle) override;
    void stopAudioUnit();

    static OSStatus inputCallback(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags,
                                  const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber,
                                  UInt32 inNumberFrames, AudioBufferList *ioData);

    OSStatus processInput(AudioUnitRenderActionFlags *ioActionFlags,
                          const AudioTimeStamp *timeStamp, UInt32 inBusNumber,
                          UInt32 inNumberFrames,
                          AudioBufferList *ioData) noexcept QT_MM_NONBLOCKING;

    OSStatus processRingbuffer(QSpan<const std::byte> inputSpan,
                               UInt32 inNumberFrames) noexcept QT_MM_NONBLOCKING;
    OSStatus processAudioCallback(QSpan<const std::byte> inputSpan) noexcept QT_MM_NONBLOCKING;

#ifdef Q_OS_MACOS
    bool addDisconnectListener(AudioObjectID id);
    void removeDisconnectListener();

    QCoreAudioUtils::DeviceDisconnectMonitor m_disconnectMonitor;
    QFuture<void> m_stopOnDisconnected;
#endif

    QCoreAudioUtils::AudioUnitHandle m_audioUnit;
    bool m_audioUnitRunning{};

    std::optional<AudioCallback> m_audioCallback;
    QDarwinAudioSource *m_parent;

    AudioBufferList m_bufferList{};

    // for run-time conversions
    AudioConverterRef m_audioConverter{ nullptr };
    std::vector<uint8_t> m_outputBuffer;
    AudioBufferList m_outputBufferList{};
};

class QDarwinAudioSource final
    : public QtMultimediaPrivate::QPlatformAudioSourceImplementationWithCallback<
              QCoreAudioSourceStream, QDarwinAudioSource>
{
    using BaseClass = QtMultimediaPrivate::QPlatformAudioSourceImplementationWithCallback<
            QCoreAudioSourceStream, QDarwinAudioSource>;

public:
    QDarwinAudioSource(QAudioDevice device, const QAudioFormat &format, QObject *parent);
    ~QDarwinAudioSource() override;

    void resumeStreamIfNecessary();
};

QT_END_NAMESPACE

#endif // QDARWINAUDIOSOURCE_P_H
