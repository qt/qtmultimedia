// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPIPEWIRE_AUDIOSOURCE_P_H
#define QPIPEWIRE_AUDIOSOURCE_P_H

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

class QPipewireAudioSource;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// LATER:
// ideally the ringbuffer should fill a buffer that can grow via a worker thread on which we can
// allocate.
struct QPipewireAudioSourceStream final : QPipewireAudioStream, QPlatformAudioSourceStream
{
    using SourceType = QPipewireAudioSource;
    using SampleFormat = QAudioFormat::SampleFormat;

    QPipewireAudioSourceStream(QAudioDevice, const QAudioFormat &,
                               std::optional<qsizetype> ringbufferSize,
                               QPipewireAudioSource *parent, float volume,
                               std::optional<int32_t> hardwareBufferFrames);
    Q_DISABLE_COPY_MOVE(QPipewireAudioSourceStream)
    ~QPipewireAudioSourceStream();

    bool open() { return true; }
    bool start(QIODevice *device);
    QIODevice *start();
    bool start(AudioCallback &&);
    void stop(ShutdownPolicy);

    using QPlatformAudioSourceStream::bytesReady;
    using QPlatformAudioSourceStream::deviceIsRingbufferReader;
    using QPlatformAudioSourceStream::processedDuration;
    using QPlatformAudioSourceStream::ringbufferSizeInBytes;
    using QPlatformAudioSourceStream::setVolume;

    void updateStreamIdle(bool idle) override;

private:
    void createStream(QPipewireAudioStream::StreamType);
    std::optional<ObjectSerial> findSourceNodeSerial();

    using QPlatformAudioSourceStream::m_format;

    void processRingbuffer() noexcept QT_MM_NONBLOCKING override;
    void processCallback() noexcept QT_MM_NONBLOCKING override;
    void handleDeviceRemoved() override;

    void stateChanged(pw_stream_state old, pw_stream_state state, const char *error) override;
    void disconnectStream();

    QSemaphore m_streamDisconnected;

    // xrun detection
    void xrunOccurred(int /*xrunCount*/) override { m_xrunOccurred.set(); }
    QtPrivate::QAutoResetEvent m_xrunOccurred;
    QMetaObject::Connection m_xrunNotification;

    std::optional<AudioCallback> m_audioCallback;

    QPipewireAudioSource *m_parent;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class QPipewireAudioSource final
    : public QPlatformAudioSourceImplementationWithCallback<QPipewireAudioSourceStream,
                                                            QPipewireAudioSource>
{
    using BaseClass = QPlatformAudioSourceImplementationWithCallback<QPipewireAudioSourceStream,
                                                                     QPipewireAudioSource>;

public:
    QPipewireAudioSource(QAudioDevice, const QAudioFormat &, QObject *parent);
    void reportXRuns(int);
};

} // namespace QtPipeWire

QT_END_NAMESPACE

#endif
