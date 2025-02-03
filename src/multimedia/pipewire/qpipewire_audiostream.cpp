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

QT_BEGIN_NAMESPACE

namespace QtPipeWire {

QPipewireAudioStream::QPipewireAudioStream(const QAudioFormat &format) : m_format{ format }
{
    prepareParameters();
}

QPipewireAudioStream::~QPipewireAudioStream()
{
    QAudioContextManager::withEventLoopLock([&] {
        m_stream = {};
    });
}

void QPipewireAudioStream::prepareParameters()
{
    struct spa_pod_builder b =
            SPA_POD_BUILDER_INIT(parameterBuffer.data(), uint32_t(parameterBuffer.size()));

    spa_audio_info_raw audioInfo = asSpaAudioInfoRaw(m_format);

    params[0] = spa_format_audio_raw_build(&b, SPA_PARAM_EnumFormat, &audioInfo);
}

void QPipewireAudioStream::createStream(QSpan<spa_dict_item> extraProperties,
                                        std::optional<qsizetype> hardwareBufferSize,
                                        const char *streamName)
{
    stream_events.version = PW_VERSION_STREAM_EVENTS;
    stream_events.process = [](void *userData) {
        reinterpret_cast<QPipewireAudioStream *>(userData)->process();
    };

    stream_events.state_changed = [](void *userData, pw_stream_state old, pw_stream_state state,
                                     const char *error) {
        reinterpret_cast<QPipewireAudioStream *>(userData)->stateChanged(old, state, error);
    };

    std::vector<spa_dict_item> properties{
        { PW_KEY_MEDIA_TYPE, "Audio" },
    };
    properties.insert(properties.end(), extraProperties.begin(), extraProperties.end());

    if (hardwareBufferSize)
        properties.push_back({
                PW_KEY_NODE_FORCE_QUANTUM,
                QString::number(*hardwareBufferSize).toStdString().data(),
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

} // namespace QtPipeWire

QT_END_NAMESPACE
