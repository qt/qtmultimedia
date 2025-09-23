// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QANDROIDAUDIOOUTPUT_H
#define QANDROIDAUDIOOUTPUT_H

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

#include "qaaudiostream_p.h"

#include <memory>

QT_BEGIN_NAMESPACE

namespace QtAAudio {

class QAndroidAudioSink;

class QAndroidAudioSinkStream final : public std::enable_shared_from_this<QAndroidAudioSinkStream>,
                                      QtMultimediaPrivate::QPlatformAudioSinkStream
{
    using QPlatformAudioSinkStream = QtMultimediaPrivate::QPlatformAudioSinkStream;
    using AudioEndpointRole = QtMultimediaPrivate::AudioEndpointRole;

public:
    explicit QAndroidAudioSinkStream(QAudioDevice, const QAudioFormat &,
                                     std::optional<qsizetype> ringbufferSize,
                                     QAndroidAudioSink *parent, float volume,
                                     std::optional<int32_t> hardwareBufferFrames,
                                     AudioEndpointRole);
    Q_DISABLE_COPY_MOVE(QAndroidAudioSinkStream)

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

    // QPlatformAudioSinkStream overrides
    void updateStreamIdle(bool arg) override;

    QSpan<std::byte> getHostSpan(void *audioData, int numFrames) const noexcept QT_MM_NONBLOCKING;
    aaudio_data_callback_result_t processRingbuffer(void *audioData,
                                                    int numFrames) noexcept QT_MM_NONBLOCKING;
    aaudio_data_callback_result_t processCallback(void *audioData,
                                                  int numFrames) noexcept QT_MM_NONBLOCKING;
    void handleError(aaudio_result_t error);

    QAndroidAudioSink *m_parent{ nullptr };
    std::shared_ptr<QAndroidAudioSinkStream> m_self;

    std::optional<AudioCallback> m_audioCallback;

    AudioEndpointRole m_role;

    std::unique_ptr<QtAAudio::Stream> m_stream;

    std::optional<NativeSampleFormat> m_nativeSampleFormat;
};

class QAndroidAudioSink final
    : public QtMultimediaPrivate::QPlatformAudioSinkImplementation<QAndroidAudioSinkStream,
                                                                   QAndroidAudioSink>
{
    using BaseClass = QtMultimediaPrivate::QPlatformAudioSinkImplementation<QAndroidAudioSinkStream,
                                                                            QAndroidAudioSink>;

public:
    QAndroidAudioSink(QAudioDevice device, const QAudioFormat &format, QObject *parent);
};

}

QT_END_NAMESPACE

#endif // QANDROIDAUDIOOUTPUT_H
