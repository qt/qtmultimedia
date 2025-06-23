// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPIPEWIRE_AUDIOSTREAM_P_H
#define QPIPEWIRE_AUDIOSTREAM_P_H

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

#include <QtCore/qglobal.h>
#include <QtMultimedia/qaudioformat.h>
#include <QtMultimedia/private/qaudio_rtsan_support_p.h>

#include "qpipewire_audiodevicemonitor_p.h"
#include "qpipewire_support_p.h"

#include <pipewire/stream.h>

QT_BEGIN_NAMESPACE

namespace QtPipeWire {

struct QPipewireAudioStream : std::enable_shared_from_this<QPipewireAudioStream>
{
protected:
    explicit QPipewireAudioStream(const QAudioFormat &);
    ~QPipewireAudioStream();

    Q_DISABLE_COPY_MOVE(QPipewireAudioStream)

    const QAudioFormat m_format;

    enum class StreamType : uint8_t
    {
        Ringbuffer,
        Callback,
    };

    // stream control
    void createStream(QSpan<spa_dict_item> extraProperties,
                      std::optional<int32_t> hardwareBufferFrames, const char *streamName,
                      StreamType = StreamType::Ringbuffer);
    bool connectStream(ObjectSerial target, spa_direction);
    void disconnectStream();

    void resetStream();

public:
    void suspend();
    void resume();
    bool hasStream() const;

protected:
    // stream callbacks
    virtual void processRingbuffer() = 0;
    virtual void processCallback() = 0;
    virtual void stateChanged(pw_stream_state oldState, pw_stream_state state,
                              const char *error) = 0;

    // stream members
    pw_stream_events stream_events{};
    PwStreamHandle m_stream;

    // device observer
    [[nodiscard]] bool registerDeviceObserver(ObjectSerial);
    void unregisterDeviceObserver();
    virtual void handleDeviceRemoved() = 0;
    SharedObjectRemoveObserver m_deviceRemovalObserver;

    // xrun detector
    // CAVEAT: has to be called at the beginning of a render callback
    // streams will have to increment m_totalNumberOfFrames internally
    void performXRunDetection(uint64_t framesPerBuffer) noexcept QT_MM_NONBLOCKING;
    virtual void xrunOccurred(int xrunCount) = 0;
    uint64_t m_expectedNextTick{};
    std::atomic_bool m_skipNextTickDiscontinuity{ true };
    std::atomic_int m_xrunCount{ 0 };

    // total samples delivered from/sent to the backend
    void addFramesHandled(uint64_t);
    uint64_t m_totalNumberOfFrames{};

    friend class QAudioContextManager; // to access m_self
    std::shared_ptr<QPipewireAudioStream> m_self;
};

} // namespace QtPipeWire

QT_END_NAMESPACE

#endif
