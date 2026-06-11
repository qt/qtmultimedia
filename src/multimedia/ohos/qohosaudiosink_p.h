// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSAUDIOSINK_P_H
#define QOHOSAUDIOSINK_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qaudio_platform_implementation_support_p.h>

#include "qohaudiostream_p.h"

#include <memory>

QT_BEGIN_NAMESPACE

namespace QtOHAudio {

class QOhosAudioSink;

class QOhosAudioSinkStream final : public std::enable_shared_from_this<QOhosAudioSinkStream>,
                                   public QtMultimediaPrivate::QPlatformAudioSinkStream
{
    using QPlatformAudioSinkStream = QtMultimediaPrivate::QPlatformAudioSinkStream;
    using AudioEndpointRole = QtMultimediaPrivate::AudioEndpointRole;

public:
    explicit QOhosAudioSinkStream(QAudioDevice device, const QAudioFormat &format,
                                  std::optional<qsizetype> ringbufferSize,
                                  QOhosAudioSink *parent, float volume,
                                  std::optional<QtMultimediaPrivate::NativePeriodFrames> hardwareBufferFrames,
                                  AudioEndpointRole role);
    Q_DISABLE_COPY_MOVE(QOhosAudioSinkStream)

    bool open();

    bool start(QIODevice *device);
    QIODevice *start();
    bool start(AudioCallback cb);

    void suspend();
    void resume();
    void stop(ShutdownPolicy policy);

    using QPlatformAudioSinkStream::bytesFree;
    using QPlatformAudioSinkStream::processedDuration;
    using QPlatformAudioSinkStream::ringbufferSizeInBytes;
    using QPlatformAudioSinkStream::setVolume;

private:
    void stop();
    void reset();

    void updateStreamIdle(bool arg) override;

    QSpan<std::byte> getHostSpan(void *audioData, int32_t numBytes) const noexcept Q_DECL_NONBLOCKING_FUNCTION;
    OH_AudioData_Callback_Result processRingbuffer(QSpan<std::byte> audioSpan,
                                                   int32_t numBytes) noexcept Q_DECL_NONBLOCKING_FUNCTION;
    OH_AudioData_Callback_Result processCallback(QSpan<std::byte> audioSpan) noexcept Q_DECL_NONBLOCKING_FUNCTION;

    QOhosAudioSink *m_parent{ nullptr };
    std::shared_ptr<QOhosAudioSinkStream> m_self;

    std::optional<AudioCallback> m_audioCallback;
    AudioEndpointRole m_role;

    std::unique_ptr<QtOHAudio::Stream> m_stream;
    std::optional<QAudioFormat> m_hostFormat;
};

class QOhosAudioSink final
    : public QtMultimediaPrivate::QPlatformAudioSinkImplementation<QOhosAudioSinkStream,
                                                                   QOhosAudioSink>
{
    using BaseClass = QtMultimediaPrivate::QPlatformAudioSinkImplementation<QOhosAudioSinkStream,
                                                                            QOhosAudioSink>;

public:
    QOhosAudioSink(QAudioDevice device, const QAudioFormat &format, QObject *parent);
    ~QOhosAudioSink() override;
};

} // namespace QtOHAudio

QT_END_NAMESPACE

#endif // QOHOSAUDIOSINK_P_H
