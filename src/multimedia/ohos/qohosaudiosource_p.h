// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSAUDIOSOURCE_P_H
#define QOHOSAUDIOSOURCE_P_H

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

class QOhosAudioSource;

class QOhosAudioSourceStream final : public QtMultimediaPrivate::QPlatformAudioSourceStream
{
    using QPlatformAudioSourceStream = QtMultimediaPrivate::QPlatformAudioSourceStream;

public:
    explicit QOhosAudioSourceStream(QAudioDevice device, const QAudioFormat &format,
                                    std::optional<int> ringbufferSize,
                                    QOhosAudioSource *parent, float volume,
                                    std::optional<QtMultimediaPrivate::NativePeriodFrames> hardwareBufferFrames);
    Q_DISABLE_COPY_MOVE(QOhosAudioSourceStream)
    ~QOhosAudioSourceStream() = default;

    bool open();

    bool start(QIODevice *);
    QIODevice *start();
    bool start(AudioCallback &&);

    void suspend();
    void resume();
    void stop(ShutdownPolicy);

    using QPlatformAudioSourceStream::bytesReady;
    using QPlatformAudioSourceStream::deviceIsRingbufferReader;
    using QPlatformAudioSourceStream::processedDuration;
    using QPlatformAudioSourceStream::ringbufferSizeInBytes;
    using QPlatformAudioSourceStream::setVolume;

private:
    void updateStreamIdle(bool idle) override;

    QSpan<const std::byte> getHostSpan(void *audioData,
                                       int32_t numBytes) const noexcept Q_DECL_NONBLOCKING_FUNCTION;
    int32_t processRingbuffer(QSpan<const std::byte> audioSpan,
                              int32_t numBytes) noexcept Q_DECL_NONBLOCKING_FUNCTION;
    int32_t processCallback(QSpan<const std::byte> audioSpan) noexcept Q_DECL_NONBLOCKING_FUNCTION;

    QOhosAudioSource *m_parent;
    std::optional<AudioCallback> m_audioCallback;
    std::unique_ptr<QtOHAudio::Stream> m_stream;
    std::optional<QAudioFormat> m_hostFormat;
};

class QOhosAudioSource final
    : public QtMultimediaPrivate::QPlatformAudioSourceImplementationWithCallback<
              QOhosAudioSourceStream, QOhosAudioSource>
{
    using BaseClass = QtMultimediaPrivate::QPlatformAudioSourceImplementationWithCallback<
            QOhosAudioSourceStream, QOhosAudioSource>;

public:
    QOhosAudioSource(QAudioDevice device, const QAudioFormat &format, QObject *parent);
    ~QOhosAudioSource() override;
};

} // namespace QtOHAudio

QT_END_NAMESPACE

#endif // QOHOSAUDIOSOURCE_P_H
