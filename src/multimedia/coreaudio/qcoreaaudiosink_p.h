// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QCOREAAUDIOSINK_P_H
#define QCOREAAUDIOSINK_P_H

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
#include <QtMultimedia/private/qcoreaudiodevices_p.h>
#ifdef Q_OS_MACOS
#  include <QtMultimedia/private/qmacosaudiodatautils_p.h>
#else
#  include <QtMultimedia/private/qcoreaudiosessionmanager_p.h>
#endif

QT_BEGIN_NAMESPACE

class QCoreAudioSink;

class QCoreAudioSinkStream final : public std::enable_shared_from_this<QCoreAudioSinkStream>,
                                   public QtMultimediaPrivate::QPlatformAudioSinkStream
{
    using QPlatformAudioSinkStream = QtMultimediaPrivate::QPlatformAudioSinkStream;
    using AudioEndpointRole = QtMultimediaPrivate::AudioEndpointRole;

public:
    using SinkType = QCoreAudioSink;

    explicit QCoreAudioSinkStream(QAudioDevice, const QAudioFormat &,
                                  std::optional<qsizetype> ringbufferSize, QCoreAudioSink *parent,
                                  float volume, std::optional<QtMultimediaPrivate::NativePeriodFrames> nativePeriodFrames,
                                  AudioEndpointRole);
    Q_DISABLE_COPY_MOVE(QCoreAudioSinkStream)
    ~QCoreAudioSinkStream();

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
                               AudioBufferList *ioData) noexcept Q_DECL_NONBLOCKING_FUNCTION;
    OSStatus processAudioCallback(uint32_t numberOfFrames,
                                  AudioBufferList *ioData) noexcept Q_DECL_NONBLOCKING_FUNCTION;

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

    QCoreAudioSink *m_parent;

    std::optional<AudioCallback> m_audioCallback;
};

class QCoreAudioSink final
    : public QtMultimediaPrivate::QPlatformAudioSinkImplementation<QCoreAudioSinkStream,
                                                                   QCoreAudioSink>
{
    using BaseClass = QtMultimediaPrivate::QPlatformAudioSinkImplementation<QCoreAudioSinkStream,
                                                                            QCoreAudioSink>;

public:
    QCoreAudioSink(QAudioDevice device, const QAudioFormat &format, QObject *parent);
    ~QCoreAudioSink() override;

    void resumeStreamIfNecessary();
};

QT_END_NAMESPACE

#endif // QCOREAAUDIOSINK_P_H
