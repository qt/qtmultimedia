// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QDARWINAUDIOSINK_P_H
#define QDARWINAUDIOSINK_P_H

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

#include <AudioUnit/AudioUnit.h>
#include <QtMultimedia/private/qcoreaudioutils_p.h>
#ifdef Q_OS_MACOS
#  include <QtMultimedia/private/qmacosaudiodatautils_p.h>
#else
#  include <QtMultimedia/private/qcoreaudiosessionmanager_p.h>
#endif

QT_BEGIN_NAMESPACE

class QDarwinAudioSink;

class QCoreAudioSinkStream final : public std::enable_shared_from_this<QCoreAudioSinkStream>,
                                   QtMultimediaPrivate::QPlatformAudioSinkStream
{
    using QPlatformAudioSinkStream = QtMultimediaPrivate::QPlatformAudioSinkStream;
    using AudioEndpointRole = QtMultimediaPrivate::AudioEndpointRole;

public:
    using SinkType = QDarwinAudioSink;

    explicit QCoreAudioSinkStream(QAudioDevice, const QAudioFormat &,
                                  std::optional<qsizetype> ringbufferSize, QDarwinAudioSink *parent,
                                  float volume, std::optional<int32_t> hardwareBufferFrames,
                                  AudioEndpointRole);
    Q_DISABLE_COPY_MOVE(QCoreAudioSinkStream)

    bool open();
    bool start(QIODevice *device);
    QIODevice *start();
    bool start(AudioCallback cb);
    void stop(ShutdownPolicy policy);
    void stopStreamWhenBufferDrained();
    void stopStream();

    void suspend();
    void resume();

    using QPlatformAudioSinkStream::bytesFree;
    using QPlatformAudioSinkStream::processedDuration;
    using QPlatformAudioSinkStream::ringbufferSizeInBytes;
    using QPlatformAudioSinkStream::setVolume;

    void resumeIfNecessary();

private:
    OSStatus processRingbuffer(uint32_t numberOfFrames,
                               AudioBufferList *ioData) noexcept QT_MM_NONBLOCKING;
    OSStatus processAudioCallback(uint32_t numberOfFrames,
                                  AudioBufferList *ioData) noexcept QT_MM_NONBLOCKING;

    void updateStreamIdle(bool arg) override;
    void stopAudioUnit();

#ifdef Q_OS_MACOS
    bool addDisconnectListener(AudioObjectID id);
    void removeDisconnectListener();

    QCoreAudioUtils::DeviceDisconnectMonitor m_disconnectMonitor;
    QFuture<void> m_stopOnDisconnected;
#endif

    std::unique_ptr<QIODevice> m_reader;
    QCoreAudioUtils::AudioUnitHandle m_audioUnit;
    bool m_audioUnitRunning{};

    QDarwinAudioSink *m_parent;

    std::optional<AudioCallback> m_audioCallback;
};

class QDarwinAudioSink final
    : public QtMultimediaPrivate::QPlatformAudioSinkImplementation<QCoreAudioSinkStream,
                                                                   QDarwinAudioSink>
{
    using BaseClass = QtMultimediaPrivate::QPlatformAudioSinkImplementation<QCoreAudioSinkStream,
                                                                            QDarwinAudioSink>;

public:
    QDarwinAudioSink(QAudioDevice device, const QAudioFormat &format, QObject *parent);
    void resumeStreamIfNecessary();
};

QT_END_NAMESPACE

#endif // QDARWINAUDIOSINK_P_H
