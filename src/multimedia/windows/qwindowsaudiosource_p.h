// Copyright (C) 2016 The Qt Company Ltd.
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

#ifndef QWINDOWSAUDIOSOURCE_H
#define QWINDOWSAUDIOSOURCE_H

#include <QtCore/qthread.h>
#include <QtCore/qtclasshelpermacros.h>
#include <QtCore/private/qcomptr_p.h>
#include <QtMultimedia/private/qaudiosystem_p.h>
#include <QtMultimedia/private/qaudiosystem_platform_stream_support_p.h>
#include <QtMultimedia/private/qaudio_platform_implementation_support_p.h>
#include <QtMultimedia/private/qwindowsaudioutils_p.h>

#include <atomic>
#include <memory>

struct IMMDevice;
struct IAudioCaptureClient;

QT_BEGIN_NAMESPACE

namespace QtWASAPI {

class QWindowsAudioSource;
using namespace QtMultimediaPrivate;

struct QWASAPIAudioSourceStream final : std::enable_shared_from_this<QWASAPIAudioSourceStream>,
                                        QPlatformAudioSourceStream
{
    using SampleFormat = QAudioFormat::SampleFormat;
    using SourceType = QWindowsAudioSource;

    QWASAPIAudioSourceStream(QAudioDevice, const QAudioFormat &,
                             std::optional<qsizetype> ringbufferSize, QWindowsAudioSource *parent,
                             float volume, std::optional<int32_t> hardwareBufferFrames);
    Q_DISABLE_COPY_MOVE(QWASAPIAudioSourceStream)
    ~QWASAPIAudioSourceStream();

    using QPlatformAudioSourceStream::bytesReady;
    using QPlatformAudioSourceStream::deviceIsRingbufferReader;
    using QPlatformAudioSourceStream::processedDuration;
    using QPlatformAudioSourceStream::ringbufferSizeInBytes;
    using QPlatformAudioSourceStream::setVolume;

    bool open() { return true; }
    bool start(QIODevice *);
    QIODevice *start();

    void suspend();
    void resume();
    void stop(ShutdownPolicy);

    void updateStreamIdle(bool) override;

private:
    bool openAudioClient(ComPtr<IMMDevice> device);
    bool startAudioClient();

    void runProcessLoop();
    bool process() noexcept QT_MM_NONBLOCKING;
    void handleAudioClientError();

    ComPtr<IAudioClient3> m_audioClient;
    ComPtr<IAudioCaptureClient> m_captureClient;

    QWindowsAudioUtils::reference_time m_periodSize;
    qsizetype m_audioClientFrames;

    std::atomic_bool m_suspended{};
    std::atomic<ShutdownPolicy> m_shutdownPolicy{ ShutdownPolicy::DiscardRingbuffer };
    QAutoResetEvent m_ringbufferDrained;

    const QUniqueWin32NullHandle m_wasapiHandle;
    std::unique_ptr<QThread> m_workerThread;

    QWindowsAudioSource *m_parent;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

class QWindowsAudioSource final
    : public QPlatformAudioSourceImplementation<QWASAPIAudioSourceStream, QWindowsAudioSource>
{
    using BaseClass =
            QPlatformAudioSourceImplementation<QWASAPIAudioSourceStream, QWindowsAudioSource>;

public:
    QWindowsAudioSource(QAudioDevice, const QAudioFormat &, QObject *parent);
};

} // namespace QtWASAPI

QT_END_NAMESPACE

#endif
