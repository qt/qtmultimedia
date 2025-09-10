// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPULSEAUDIOSINK_P_H
#define QPULSEAUDIOSINK_P_H

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

#include <QtCore/qtclasshelpermacros.h>

#include <QtMultimedia/qaudio.h>
#include <QtMultimedia/qaudiodevice.h>
#include <QtMultimedia/private/qaudio_platform_implementation_support_p.h>
#include <QtMultimedia/private/qaudiosystem_p.h>
#include <QtMultimedia/private/qpulsehelpers_p.h>

QT_BEGIN_NAMESPACE

namespace QPulseAudioInternal {

using namespace QtMultimediaPrivate;
class QPulseAudioSink;

struct QPulseAudioSinkStream final : QPlatformAudioSinkStream
{
    using SinkType = QPulseAudioSink;

    QPulseAudioSinkStream(QAudioDevice, const QAudioFormat &format,
                          std::optional<qsizetype> ringbufferSize, QPulseAudioSink *parent,
                          float volume, std::optional<int32_t> hardwareBufferSize,
                          AudioEndpointRole);
    ~QPulseAudioSinkStream();

    using QPlatformAudioSinkStream::bytesFree;
    using QPlatformAudioSinkStream::processedDuration;
    using QPlatformAudioSinkStream::ringbufferSizeInBytes;
    using QPlatformAudioSinkStream::setVolume;

    bool start(QIODevice *device);
    bool start(AudioCallback &&);
    QIODevice *start();
    void stop(ShutdownPolicy);
    void stop() { stop(ShutdownPolicy::DrainRingbuffer); }
    void reset() { stop(ShutdownPolicy::DiscardRingbuffer); }
    void suspend();
    void resume();

    bool open() const;

private:
    enum class StreamType : uint8_t
    {
        Ringbuffer,
        Callback,
    };

    void installCallbacks(StreamType);
    void uninstallCallbacks();

    bool startStream(StreamType);

    void updateStreamIdle(bool) override;

    // PulseAudio callbacks
    void underflowCallback() { }
    void overflowCallback() { }
    void stateCallback() { }
    void writeCallbackRingbuffer(size_t requestedBytes);
    void writeCallbackAudioCallback(size_t requestedBytes);
    void latencyUpdateCallback() { }

    QPulseAudioSink *m_parent;
    PAStreamHandle m_stream;

    std::optional<AudioCallback> m_audioCallback;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class QPulseAudioSink final
    : public QPlatformAudioSinkImplementation<QPulseAudioSinkStream, QPulseAudioSink>
{
    using BaseClass = QPlatformAudioSinkImplementation<QPulseAudioSinkStream, QPulseAudioSink>;

public:
    QPulseAudioSink(QAudioDevice, const QAudioFormat &, QObject *parent);

    void start(QIODevice *device) override;
    QIODevice *start() override;
    void start(AudioCallback &&) override;

private:
    bool validatePulseaudio();
};

} // namespace QPulseAudioInternal

QT_END_NAMESPACE

#endif // QPULSEAUDIOSINK_P_H
