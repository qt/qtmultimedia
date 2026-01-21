// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QANDROIDAUDIOINPUT_H
#define QANDROIDAUDIOINPUT_H

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

#include <private/qaudio_platform_implementation_support_p.h>

#include <private/qaaudiostream_p.h>

#include <aaudio/AAudio.h>

QT_BEGIN_NAMESPACE

namespace QtAAudio {

class QAndroidAudioSource;

class QAndroidAudioSourceStream final : public QtMultimediaPrivate::QPlatformAudioSourceStream
{
    using QPlatformAudiosourceStream = QtMultimediaPrivate::QPlatformAudioSourceStream;

public:
    explicit QAndroidAudioSourceStream(QAudioDevice device, const QAudioFormat &format,
                                       std::optional<int> ringbufferSize,
                                       QAndroidAudioSource *parent, float volume,
                                       std::optional<int32_t> hardwareBufferFrames);
    Q_DISABLE_COPY_MOVE(QAndroidAudioSourceStream)
    ~QAndroidAudioSourceStream();

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
    // QPlatformAudioSourceStream overrides
    void updateStreamIdle(bool idle) override;

    QSpan<const std::byte> getHostSpan(void *audioData, int numFrames) const noexcept QT_MM_NONBLOCKING;
    aaudio_data_callback_result_t processRingbuffer(QSpan<const std::byte> audioSpan,
                                                    int numFrames) noexcept QT_MM_NONBLOCKING;
    aaudio_data_callback_result_t
    processCallback(QSpan<const std::byte> audioSpan) noexcept QT_MM_NONBLOCKING;
    void handleError(aaudio_result_t error);

    QAndroidAudioSource *m_parent;

    std::optional<AudioCallback> m_audioCallback;

    std::unique_ptr<QtAAudio::Stream> m_stream;

    std::optional<QAudioFormat> m_hostFormat;
};

class QAndroidAudioSource final
    : public QtMultimediaPrivate::QPlatformAudioSourceImplementationWithCallback<
              QAndroidAudioSourceStream, QAndroidAudioSource>
{
    using BaseClass = QtMultimediaPrivate::QPlatformAudioSourceImplementationWithCallback<
            QAndroidAudioSourceStream, QAndroidAudioSource>;

public:
    QAndroidAudioSource(QAudioDevice device, const QAudioFormat &format, QObject *parent);
    ~QAndroidAudioSource() override;
};

}

QT_END_NAMESPACE

#endif // QANDROIDAUDIOINPUT_H
