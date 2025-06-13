// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPIPEWIRE_AUDIOSINK_P_H
#define QPIPEWIRE_AUDIOSINK_P_H

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

#include <QtCore/qsemaphore.h>
#include <QtCore/qtclasshelpermacros.h>

#include <QtMultimedia/private/qaudio_platform_implementation_support_p.h>
#include <QtMultimedia/private/qaudiosystem_p.h>
#include <QtMultimedia/private/qaudiosystem_platform_stream_support_p.h>
#include <QtMultimedia/private/qpipewire_audiostream_p.h>

QT_BEGIN_NAMESPACE

namespace QtPipeWire {

using namespace QtMultimediaPrivate;

class QPipewireAudioSink;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct QPipewireAudioSinkStream final : QPipewireAudioStream, QPlatformAudioSinkStream
{
    using SampleFormat = QAudioFormat::SampleFormat;
    using SinkType = QPipewireAudioSink;

    QPipewireAudioSinkStream(QAudioDevice, const QAudioFormat &,
                             std::optional<qsizetype> ringbufferSize, QPipewireAudioSink *parent,
                             float volume, std::optional<int32_t> hardwareBufferFrames,
                             AudioEndpointRole);
    Q_DISABLE_COPY_MOVE(QPipewireAudioSinkStream)
    ~QPipewireAudioSinkStream();

    bool open();

    using QPlatformAudioSinkStream::bytesFree;
    using QPlatformAudioSinkStream::processedDuration;
    using QPlatformAudioSinkStream::ringbufferSizeInBytes;
    using QPlatformAudioSinkStream::setVolume;

    bool start(QIODevice *device);
    QIODevice *start();
    bool start(AudioCallback);

    void stop(ShutdownPolicy);

    void updateStreamIdle(bool idle) override;

private:
    void createStream(QPipewireAudioStream::StreamType);
    std::optional<ObjectSerial> findSinkNodeSerial();

    using QPlatformAudioSinkStream::m_format;

    // QPipewireAudioStream overrides
    void handleDeviceRemoved() override;
    void processRingbuffer() noexcept QT_MM_NONBLOCKING override;
    void processCallback() noexcept QT_MM_NONBLOCKING override;
    template <typename Functor>
    void processHelper(Functor &&f);

    void stateChanged(pw_stream_state /*old*/, pw_stream_state state,
                      const char * /*error*/) override;

    void disconnectStream();
    QSemaphore m_disconnectSemaphore;

    std::atomic<ShutdownPolicy> m_shutdownPolicy{ ShutdownPolicy::DiscardRingbuffer };
    QAutoResetEvent m_ringbufferDrained;

    // process helpers
    void queueBuffer(struct pw_buffer *b, uint64_t samplesWritten) noexcept QT_MM_NONBLOCKING;

    // xrun detection
    void xrunOccurred(int /*xrunCount*/) override { m_xrunOccurred.set(); }
    QtPrivate::QAutoResetEvent m_xrunOccurred;
    QMetaObject::Connection m_xrunNotification;

    [[maybe_unused]] static void fakeXRun();

    AudioEndpointRole m_role;
    std::optional<AudioCallback> m_audioCallback;

    QPipewireAudioSink *m_parent;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class QPipewireAudioSink final
    : public QPlatformAudioSinkImplementation<QPipewireAudioSinkStream, QPipewireAudioSink>
{
    using BaseClass =
            QPlatformAudioSinkImplementation<QPipewireAudioSinkStream, QPipewireAudioSink>;

public:
    QPipewireAudioSink(QAudioDevice, const QAudioFormat &, QObject *parent);

    void reportXRuns(int);
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace QtPipeWire

QT_END_NAMESPACE

#endif
