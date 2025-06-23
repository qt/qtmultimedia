// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpipewire_audiostream_p.h"

#include "qpipewire_audiocontextmanager_p.h"
#include "qpipewire_spa_pod_support_p.h"

#include <spa/pod/builder.h>

#if __has_include(<spa/param/audio/raw-utils.h>)
#  include <spa/param/audio/raw-utils.h>
#else
#  include "qpipewire_spa_compat_p.h"
#endif


#ifndef PW_KEY_NODE_FORCE_QUANTUM
#  define PW_KEY_NODE_FORCE_QUANTUM "node.force-quantum"
#endif

#if !PW_CHECK_VERSION(0, 3, 50)
extern "C" {
int pw_stream_get_time_n(struct pw_stream *stream, struct pw_time *time, size_t size);
}
#endif

#include <array>

QT_BEGIN_NAMESPACE

namespace QtPipeWire {

QPipewireAudioStream::QPipewireAudioStream(const QAudioFormat &format) : m_format{ format }
{
}

QPipewireAudioStream::~QPipewireAudioStream()
{
    resetStream();
}

void QPipewireAudioStream::createStream(QSpan<spa_dict_item> extraProperties,
                                        std::optional<int32_t> hardwareBufferFrames,
                                        const char *streamName, StreamType type)
{
    stream_events.version = PW_VERSION_STREAM_EVENTS;

    switch (type) {
    case StreamType::Ringbuffer:
        stream_events.process = [](void *userData) {
            reinterpret_cast<QPipewireAudioStream *>(userData)->processRingbuffer();
        };
        break;
    case StreamType::Callback:
        stream_events.process = [](void *userData) {
            reinterpret_cast<QPipewireAudioStream *>(userData)->processCallback();
        };
        break;
    default:
        Q_UNREACHABLE_RETURN();
    };

    stream_events.state_changed = [](void *userData, pw_stream_state old, pw_stream_state state,
                                     const char *error) {
        reinterpret_cast<QPipewireAudioStream *>(userData)->stateChanged(old, state, error);
    };

    std::vector<spa_dict_item> properties{
        { PW_KEY_MEDIA_TYPE, "Audio" },
    };
    properties.insert(properties.end(), extraProperties.begin(), extraProperties.end());

    if (hardwareBufferFrames)
        properties.push_back({
                PW_KEY_NODE_FORCE_QUANTUM,
                std::to_string(*hardwareBufferFrames).data(),
        });

    QAudioContextManager::withEventLoopLock([&] {
        m_stream = PwStreamHandle{
            pw_stream_new_simple(QAudioContextManager::getEventLoop(), streamName,
                                 makeProperties(properties).release(), &stream_events, this),
        };
    });
    if (!m_stream)
        qWarning() << "pw_stream_new_simple failed" << make_error_code().message();
}

bool QPipewireAudioStream::connectStream(ObjectSerial target, spa_direction direction)
{
    int status = QAudioContextManager::withEventLoopLock([&] {
        std::optional<ObjectId> targetNodeId =
                QAudioContextManager::deviceMonitor().findObjectId(target);
        if (!targetNodeId)
            return -ENODEV;

        bool deviceAlreadyRemoved = registerDeviceObserver(target);
        if (!deviceAlreadyRemoved)
            return -ENODEV;

        std::array<uint8_t, 1024> buffer;

        QT_WARNING_PUSH
        QT_WARNING_DISABLE_CLANG("-Wmissing-field-initializers")
        struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer.data(), uint32_t(buffer.size()));
        QT_WARNING_POP
        spa_audio_info_raw audioInfo = asSpaAudioInfoRaw(m_format);

        std::array<const struct spa_pod *, 1> params{
            spa_format_audio_raw_build(&b, SPA_PARAM_EnumFormat, &audioInfo),
        };

        return pw_stream_connect(
                m_stream.get(), direction, targetNodeId->value,
                pw_stream_flags(PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS
                                | PW_STREAM_FLAG_RT_PROCESS | PW_STREAM_FLAG_DONT_RECONNECT),
                params.data(), params.size());
    });

    if (status < 0) {
        qWarning() << "pw_stream_connect failed" << make_error_code(-status).message();
        return false;
    }

    return true;
}

void QPipewireAudioStream::suspend()
{
    int status = QAudioContextManager::withEventLoopLock([&] {
        return pw_stream_set_active(m_stream.get(), false);
    });
    if (status < 0)
        qWarning() << "pw_stream_set_active failed" << make_error_code(-status).message();
}

void QPipewireAudioStream::resume()
{
    int status = QAudioContextManager::withEventLoopLock([&] {
        m_skipNextTickDiscontinuity = true;
        return pw_stream_set_active(m_stream.get(), true);
    });
    if (status < 0)
        qWarning() << "pw_stream_set_active failed" << make_error_code(-status).message();
}

void QPipewireAudioStream::disconnectStream()
{
    int status = QAudioContextManager::withEventLoopLock([&] {
        return pw_stream_disconnect(m_stream.get());
    });
    if (status < 0)
        qWarning() << "pw_stream_disconnect failed" << make_error_code(-status).message();
}

void QPipewireAudioStream::resetStream()
{
    QAudioContextManager::withEventLoopLock([&] {
        m_stream = {};
        m_self = {};
    });
}

bool QPipewireAudioStream::hasStream() const
{
    return bool(m_stream);
}

bool QPipewireAudioStream::registerDeviceObserver(ObjectSerial nodeSerial)
{
    m_deviceRemovalObserver = std::make_shared<ObjectRemoveObserver>(nodeSerial);
    QObject::connect(m_deviceRemovalObserver.get(), &ObjectRemoveObserver::objectRemoved,
                     m_deviceRemovalObserver.get(), [this] {
        handleDeviceRemoved();
    });

    return QAudioContextManager::deviceMonitor().registerObserver(m_deviceRemovalObserver);
}

void QPipewireAudioStream::unregisterDeviceObserver()
{
    Q_ASSERT(m_deviceRemovalObserver);
    QAudioContextManager::deviceMonitor().unregisterObserver(m_deviceRemovalObserver);
    m_deviceRemovalObserver = {};
}

void QPipewireAudioStream::performXRunDetection(uint64_t framesPerBuffer) noexcept QT_MM_NONBLOCKING
{
    // XRun detection does not work well with pause/resume.
    // disabling for now, since we don't have any public API for notifying the application about
    // xruns
    constexpr bool runXRunDetection = false;
    if (!runXRunDetection)
        return;

    struct pw_time time_info = {};
    int status = pw_stream_get_time_n(m_stream.get(), &time_info, sizeof(pw_time));
    if (status < 0) {
        if (pw_check_library_version(0, 3, 50))
            return; // no xrun detection on ancient pipewire

        qFatal() << "pw_stream_get_time_n failed. This should not happen";
        return;
    }

    if (m_skipNextTickDiscontinuity.load(std::memory_order_relaxed)) {
        // prevent xrun detection to fire after resume, as ticks will continue incrementing
        m_skipNextTickDiscontinuity = false;
    } else if (std::abs(int64_t(m_expectedNextTick) - int64_t(time_info.ticks)) > 1024) {
        m_totalNumberOfFrames = time_info.ticks;
        m_xrunCount += 1;
        xrunOccurred(m_xrunCount);
    }

    // CAVEAT:
    // counts `ticks` in the device rates, which may be different to the rate the stream is
    // running in. We therefore cannot do any precise xrun detection with this technique, but
    // only to a best effort.
    // TODO: can we use profiler events?
    double rateFactor = double(time_info.rate.num) / time_info.rate.denom * m_format.sampleRate();

#if PW_CHECK_VERSION(1, 1, 0)
    if (pw_check_library_version(1, 1, 0)) {
        // LATER: rely on time_info.size, once 1.1 is the minimum required version
        m_expectedNextTick = time_info.ticks + (time_info.size / rateFactor);
        return;
    }
#endif
    m_expectedNextTick = time_info.ticks + (framesPerBuffer / rateFactor);
}

void QPipewireAudioStream::addFramesHandled(uint64_t arg)
{
    m_totalNumberOfFrames += arg;
}

} // namespace QtPipeWire

QT_END_NAMESPACE
