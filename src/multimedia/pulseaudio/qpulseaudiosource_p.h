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

#ifndef QPULSEAUDIOSOURCE_P_H
#define QPULSEAUDIOSOURCE_P_H

#include <QtCore/qtclasshelpermacros.h>

#include <QtMultimedia/qaudio.h>
#include <QtMultimedia/qaudiodevice.h>
#include <QtMultimedia/private/qaudio_platform_implementation_support_p.h>
#include <QtMultimedia/private/qaudiosystem_p.h>
#include <QtMultimedia/private/qpulsehelpers_p.h>

QT_BEGIN_NAMESPACE

namespace QPulseAudioInternal {

class QPulseAudioSource;
using namespace QtMultimediaPrivate;

struct QPulseAudioSourceStream final : QPlatformAudioSourceStream
{
    using SourceType = QPulseAudioSource;

    QPulseAudioSourceStream(QAudioDevice, const QAudioFormat &,
                            std::optional<qsizetype> ringbufferSize, QPulseAudioSource *parent,
                            float volume, std::optional<int32_t> hardwareBufferSize);
    Q_DISABLE_COPY_MOVE(QPulseAudioSourceStream)
    ~QPulseAudioSourceStream();

    using QPlatformAudioSourceStream::bytesReady;
    using QPlatformAudioSourceStream::deviceIsRingbufferReader;
    using QPlatformAudioSourceStream::processedDuration;
    using QPlatformAudioSourceStream::ringbufferSizeInBytes;
    using QPlatformAudioSourceStream::setVolume;

    bool start(QIODevice *device);
    bool start(AudioCallback &&);
    QIODevice *start();
    void stop(ShutdownPolicy);
    void suspend();
    void resume();
    bool open() const;

    void updateStreamIdle(bool idle) override;

private:
    enum class StreamType : uint8_t
    {
        Ringbuffer,
        Callback,
    };

    bool startStream(StreamType);
    void installCallbacks(StreamType);
    void uninstallCallbacks();

    QPulseAudioSource *m_parent;
    PAStreamHandle m_stream;
    std::optional<AudioCallback> m_audioCallback;

    // PulseAudio callbacks
    void underflowCallback() { }
    void overflowCallback() { }
    void stateCallback() { }
    void readCallbackRingbuffer(size_t bytesToRead);
    void readCallbackAudioCallback(size_t bytesToRead);
    void latencyUpdateCallback() { }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class QPulseAudioSource final
    : public QPlatformAudioSourceImplementationWithCallback<QPulseAudioSourceStream,
                                                            QPulseAudioSource>
{
    using BaseClass = QPlatformAudioSourceImplementationWithCallback<QPulseAudioSourceStream,
                                                                     QPulseAudioSource>;

public:
    QPulseAudioSource(QAudioDevice, const QAudioFormat &, QObject *parent);

    void start(QIODevice *device) override;
    void start(AudioCallback &&) override;
    QIODevice *start() override;

private:
    bool validatePulseaudio();
};

} // namespace QPulseAudioInternal

QT_END_NAMESPACE

#endif // QPULSEAUDIOSOURCE_P_H
