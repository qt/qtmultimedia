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
public:
    explicit QAndroidAudioSourceStream(QAudioDevice device, const QAudioFormat &format,
                                       std::optional<int> ringbufferSize,
                                       QAndroidAudioSource *parent, float volume,
                                       std::optional<int32_t> hardwareBufferFrames);
    Q_DISABLE_COPY_MOVE(QAndroidAudioSourceStream)

    bool open();

    bool start(QIODevice *);
    QIODevice *start();

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

    aaudio_data_callback_result_t process(void *audioData,
                                          int numFrames) noexcept QT_MM_NONBLOCKING;
    void handleError(aaudio_result_t error);

    QAndroidAudioSource *m_parent;

    std::unique_ptr<QtAAudio::Stream> m_stream;

    std::optional<NativeSampleFormat> m_nativeSampleFormat;
};

class QAndroidAudioSource final
    : public QtMultimediaPrivate::QPlatformAudioSourceImplementation<QAndroidAudioSourceStream,
                                                                     QAndroidAudioSource>
{
    using BaseClass =
            QtMultimediaPrivate::QPlatformAudioSourceImplementation<QAndroidAudioSourceStream,
                                                                    QAndroidAudioSource>;

public:
    QAndroidAudioSource(QAudioDevice device, const QAudioFormat &format, QObject *parent);
};

}

QT_END_NAMESPACE

#endif // QANDROIDAUDIOINPUT_H
