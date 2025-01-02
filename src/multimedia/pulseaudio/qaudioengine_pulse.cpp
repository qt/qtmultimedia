// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/qdebug.h>

#include <qaudiodevice.h>
#include <QGuiApplication>
#include <QIcon>
#include <QTimer>
#include "qaudioengine_pulse_p.h"
#include "qpulseaudiodevice_p.h"
#include "qpulsehelpers_p.h"
#include <sys/types.h>
#include <unistd.h>
#include <mutex> // for lock_guard

QT_BEGIN_NAMESPACE

template<typename Info>
static bool updateDevicesMap(QReadWriteLock &lock, QByteArray defaultDeviceId,
                             QMap<int, QAudioDevice> &devices, QAudioDevice::Mode mode,
                             const Info &info)
{
    QWriteLocker locker(&lock);

    bool isDefault = defaultDeviceId == info.name;
    auto newDeviceInfo = std::make_unique<QPulseAudioDeviceInfo>(info.name, info.description, isDefault, mode);
    newDeviceInfo->channelConfiguration = QPulseAudioInternal::channelConfigFromMap(info.channel_map);
    newDeviceInfo->preferredFormat = QPulseAudioInternal::sampleSpecToAudioFormat(info.sample_spec);
    newDeviceInfo->preferredFormat.setChannelConfig(newDeviceInfo->channelConfiguration);

    auto &device = devices[info.index];
    if (device.handle() && *newDeviceInfo == *device.handle())
        return false;

    device = newDeviceInfo.release()->create();
    return true;
}

static bool updateDevicesMap(QReadWriteLock &lock, QByteArray defaultDeviceId,
                             QMap<int, QAudioDevice> &devices)
{
    QWriteLocker locker(&lock);

    bool result = false;

    for (QAudioDevice &device : devices) {
        auto deviceInfo = device.handle();
        const auto isDefault = deviceInfo->id == defaultDeviceId;
        if (deviceInfo->isDefault != isDefault) {
            Q_ASSERT(dynamic_cast<const QPulseAudioDeviceInfo *>(deviceInfo));
            auto newDeviceInfo = std::make_unique<QPulseAudioDeviceInfo>(
                    *static_cast<const QPulseAudioDeviceInfo *>(deviceInfo));
            newDeviceInfo->isDefault = isDefault;
            device = newDeviceInfo.release()->create();
            result = true;
        }
    }

    return result;
};

static void serverInfoCallback(pa_context *context, const pa_server_info *info, void *userdata)
{
    if (!info) {
        qWarning() << QString::fromLatin1("Failed to get server information: %s").arg(QString::fromUtf8(pa_strerror(pa_context_errno(context))));
        return;
    }

#ifdef DEBUG_PULSE
    char ss[PA_SAMPLE_SPEC_SNPRINT_MAX], cm[PA_CHANNEL_MAP_SNPRINT_MAX];

    pa_sample_spec_snprint(ss, sizeof(ss), &info->sample_spec);
    pa_channel_map_snprint(cm, sizeof(cm), &info->channel_map);

    qDebug() << QString("User name: %1\n"
             "Host Name: %2\n"
             "Server Name: %3\n"
             "Server Version: %4\n"
             "Default Sample Specification: %5\n"
             "Default Channel Map: %6\n"
             "Default Sink: %7\n"
             "Default Source: %8\n").arg(
           info->user_name,
           info->host_name,
           info->server_name,
           info->server_version,
           ss,
           cm,
           info->default_sink_name,
           info->default_source_name);
#endif

    QPulseAudioEngine *pulseEngine = static_cast<QPulseAudioEngine*>(userdata);

    bool defaultSinkChanged = false;
    bool defaultSourceChanged = false;

    {
        QWriteLocker locker(&pulseEngine->m_serverLock);

        if (pulseEngine->m_defaultSink != info->default_sink_name) {
            pulseEngine->m_defaultSink = info->default_sink_name;
            defaultSinkChanged = true;
        }

        if (pulseEngine->m_defaultSource != info->default_source_name) {
            pulseEngine->m_defaultSource = info->default_source_name;
            defaultSourceChanged = true;
        }
    }

    if (defaultSinkChanged
        && updateDevicesMap(pulseEngine->m_sinkLock, pulseEngine->m_defaultSink,
                            pulseEngine->m_sinks))
        emit pulseEngine->audioOutputsChanged();

    if (defaultSourceChanged
        && updateDevicesMap(pulseEngine->m_sourceLock, pulseEngine->m_defaultSource,
                            pulseEngine->m_sources))
        emit pulseEngine->audioInputsChanged();

    pa_threaded_mainloop_signal(pulseEngine->mainloop(), 0);
}

static void sinkInfoCallback(pa_context *context, const pa_sink_info *info, int isLast, void *userdata)
{
    QPulseAudioEngine *pulseEngine = static_cast<QPulseAudioEngine*>(userdata);

    if (isLast < 0) {
        qWarning() << QString::fromLatin1("Failed to get sink information: %s").arg(QString::fromUtf8(pa_strerror(pa_context_errno(context))));
        return;
    }

    if (isLast) {
        pa_threaded_mainloop_signal(pulseEngine->mainloop(), 0);
        return;
    }

    Q_ASSERT(info);

#ifdef DEBUG_PULSE
    static const QMap<pa_sink_state, QString> stateMap{
        { PA_SINK_INVALID_STATE, "n/a" }, { PA_SINK_RUNNING, "RUNNING" },
        { PA_SINK_IDLE, "IDLE" },         { PA_SINK_SUSPENDED, "SUSPENDED" },
        { PA_SINK_UNLINKED, "UNLINKED" },
    };

    qDebug() << QString("Sink #%1\n"
             "\tState: %2\n"
             "\tName: %3\n"
             "\tDescription: %4\n"
            ).arg(QString::number(info->index),
                  stateMap.value(info->state),
                  info->name,
                  info->description);
#endif

    if (updateDevicesMap(pulseEngine->m_sinkLock, pulseEngine->m_defaultSink, pulseEngine->m_sinks,
                         QAudioDevice::Output, *info))
        emit pulseEngine->audioOutputsChanged();
}

static void sourceInfoCallback(pa_context *context, const pa_source_info *info, int isLast, void *userdata)
{
    Q_UNUSED(context);
    QPulseAudioEngine *pulseEngine = static_cast<QPulseAudioEngine*>(userdata);

    if (isLast) {
        pa_threaded_mainloop_signal(pulseEngine->mainloop(), 0);
        return;
    }

    Q_ASSERT(info);

#ifdef DEBUG_PULSE
    static const QMap<pa_source_state, QString> stateMap{ { PA_SOURCE_INVALID_STATE, "n/a" },
                                                          { PA_SOURCE_RUNNING, "RUNNING" },
                                                          { PA_SOURCE_IDLE, "IDLE" },
                                                          { PA_SOURCE_SUSPENDED, "SUSPENDED" },
                                                          { PA_SOURCE_UNLINKED, "UNLINKED" } };

    qDebug() << QString("Source #%1\n"
         "\tState: %2\n"
         "\tName: %3\n"
         "\tDescription: %4\n"
        ).arg(QString::number(info->index),
              stateMap.value(info->state),
              info->name,
              info->description);
#endif

    // skip monitor channels
    if (info->monitor_of_sink != PA_INVALID_INDEX)
        return;

    if (updateDevicesMap(pulseEngine->m_sourceLock, pulseEngine->m_defaultSource,
                         pulseEngine->m_sources, QAudioDevice::Input, *info))
        emit pulseEngine->audioInputsChanged();
}

static void event_cb(pa_context* context, pa_subscription_event_type_t t, uint32_t index, void* userdata)
{
    QPulseAudioEngine *pulseEngine = static_cast<QPulseAudioEngine*>(userdata);

    int type = t & PA_SUBSCRIPTION_EVENT_TYPE_MASK;
    int facility = t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK;

    switch (type) {
    case PA_SUBSCRIPTION_EVENT_NEW:
    case PA_SUBSCRIPTION_EVENT_CHANGE:
        switch (facility) {
        case PA_SUBSCRIPTION_EVENT_SERVER: {
            PAOperationUPtr op(pa_context_get_server_info(context, serverInfoCallback, userdata));
            if (!op)
                qWarning("PulseAudioService: failed to get server info");
            break;
        }
        case PA_SUBSCRIPTION_EVENT_SINK: {
            PAOperationUPtr op(
                    pa_context_get_sink_info_by_index(context, index, sinkInfoCallback, userdata));
            if (!op)
                qWarning("PulseAudioService: failed to get sink info");
            break;
        }
        case PA_SUBSCRIPTION_EVENT_SOURCE: {
            PAOperationUPtr op(pa_context_get_source_info_by_index(context, index,
                                                                   sourceInfoCallback, userdata));
            if (!op)
                qWarning("PulseAudioService: failed to get source info");
            break;
        }
        default:
            break;
        }
        break;
    case PA_SUBSCRIPTION_EVENT_REMOVE:
        switch (facility) {
        case PA_SUBSCRIPTION_EVENT_SINK: {
            QWriteLocker locker(&pulseEngine->m_sinkLock);
            pulseEngine->m_sinks.remove(index);
            break;
        }
        case PA_SUBSCRIPTION_EVENT_SOURCE: {
            QWriteLocker locker(&pulseEngine->m_sourceLock);
            pulseEngine->m_sources.remove(index);
            break;
        }
        default:
            break;
        }
        break;
    default:
        break;
    }
}

static void contextStateCallbackInit(pa_context *context, void *userdata)
{
    Q_UNUSED(context);
#ifdef DEBUG_PULSE
    qDebug() << QPulseAudioInternal::stateToQString(pa_context_get_state(context));
#endif
    QPulseAudioEngine *pulseEngine = reinterpret_cast<QPulseAudioEngine*>(userdata);
    pa_threaded_mainloop_signal(pulseEngine->mainloop(), 0);
}

static void contextStateCallback(pa_context *c, void *userdata)
{
    QPulseAudioEngine *self = reinterpret_cast<QPulseAudioEngine*>(userdata);
    pa_context_state_t state = pa_context_get_state(c);

#ifdef DEBUG_PULSE
    qDebug() << QPulseAudioInternal::stateToQString(state);
#endif

    if (state == PA_CONTEXT_FAILED)
        QMetaObject::invokeMethod(self, "onContextFailed", Qt::QueuedConnection);
}

Q_GLOBAL_STATIC(QPulseAudioEngine, pulseEngine);

QPulseAudioEngine::QPulseAudioEngine(QObject *parent)
    : QObject(parent)
    , m_mainLoopApi(nullptr)
    , m_context(nullptr)
    , m_prepared(false)
{
    prepare();
}

QPulseAudioEngine::~QPulseAudioEngine()
{
    if (m_prepared)
        release();
}

void QPulseAudioEngine::prepare()
{
    bool keepGoing = true;
    bool ok = true;

    m_mainLoop = pa_threaded_mainloop_new();
    if (m_mainLoop == nullptr) {
        qWarning("PulseAudioService: unable to create pulseaudio mainloop");
        return;
    }

    if (pa_threaded_mainloop_start(m_mainLoop) != 0) {
        qWarning("PulseAudioService: unable to start pulseaudio mainloop");
        pa_threaded_mainloop_free(m_mainLoop);
        m_mainLoop = nullptr;
        return;
    }

    m_mainLoopApi = pa_threaded_mainloop_get_api(m_mainLoop);

    lock();

    pa_proplist *proplist = pa_proplist_new();
    if (!QGuiApplication::applicationDisplayName().isEmpty())
        pa_proplist_sets(proplist, PA_PROP_APPLICATION_NAME, qUtf8Printable(QGuiApplication::applicationDisplayName()));
    if (!QGuiApplication::desktopFileName().isEmpty())
        pa_proplist_sets(proplist, PA_PROP_APPLICATION_ID, qUtf8Printable(QGuiApplication::desktopFileName()));
    if (const QString windowIconName = QGuiApplication::windowIcon().name(); !windowIconName.isEmpty())
        pa_proplist_sets(proplist, PA_PROP_WINDOW_ICON_NAME, qUtf8Printable(windowIconName));

    m_context = pa_context_new_with_proplist(m_mainLoopApi, nullptr, proplist);
    pa_proplist_free(proplist);

    if (m_context == nullptr) {
        qWarning("PulseAudioService: Unable to create new pulseaudio context");
        pa_threaded_mainloop_unlock(m_mainLoop);
        pa_threaded_mainloop_free(m_mainLoop);
        m_mainLoop = nullptr;
        onContextFailed();
        return;
    }

    pa_context_set_state_callback(m_context, contextStateCallbackInit, this);

    if (pa_context_connect(m_context, nullptr, (pa_context_flags_t)0, nullptr) < 0) {
        qWarning("PulseAudioService: pa_context_connect() failed");
        pa_context_unref(m_context);
        pa_threaded_mainloop_unlock(m_mainLoop);
        pa_threaded_mainloop_free(m_mainLoop);
        m_mainLoop = nullptr;
        m_context = nullptr;
        return;
    }

    pa_threaded_mainloop_wait(m_mainLoop);

    while (keepGoing) {
        switch (pa_context_get_state(m_context)) {
            case PA_CONTEXT_CONNECTING:
            case PA_CONTEXT_AUTHORIZING:
            case PA_CONTEXT_SETTING_NAME:
                break;

            case PA_CONTEXT_READY:
#ifdef DEBUG_PULSE
                qDebug("Connection established.");
#endif
                keepGoing = false;
                break;

            case PA_CONTEXT_TERMINATED:
                qCritical("PulseAudioService: Context terminated.");
                keepGoing = false;
                ok = false;
                break;

            case PA_CONTEXT_FAILED:
            default:
                qCritical() << QString::fromLatin1("PulseAudioService: Connection failure: %1")
                                .arg(QString::fromUtf8(pa_strerror(pa_context_errno(m_context))));
                keepGoing = false;
                ok = false;
        }

        if (keepGoing)
            pa_threaded_mainloop_wait(m_mainLoop);
    }

    if (ok) {
        pa_context_set_state_callback(m_context, contextStateCallback, this);

        pa_context_set_subscribe_callback(m_context, event_cb, this);
        PAOperationUPtr op(pa_context_subscribe(
                m_context,
                pa_subscription_mask_t(PA_SUBSCRIPTION_MASK_SINK | PA_SUBSCRIPTION_MASK_SOURCE
                                       | PA_SUBSCRIPTION_MASK_SERVER),
                nullptr, nullptr));
        if (!op)
            qWarning("PulseAudioService: failed to subscribe to context notifications");
    } else {
        pa_context_unref(m_context);
        m_context = nullptr;
    }

    unlock();

    if (ok) {
        updateDevices();
        m_prepared = true;
    } else {
        pa_threaded_mainloop_free(m_mainLoop);
        m_mainLoop = nullptr;
        onContextFailed();
    }
}

void QPulseAudioEngine::release()
{
    if (!m_prepared)
        return;

    if (m_context) {
        pa_context_disconnect(m_context);
        pa_context_unref(m_context);
        m_context = nullptr;
    }

    if (m_mainLoop) {
        pa_threaded_mainloop_stop(m_mainLoop);
        pa_threaded_mainloop_free(m_mainLoop);
        m_mainLoop = nullptr;
    }

    m_prepared = false;
}

void QPulseAudioEngine::updateDevices()
{
    std::lock_guard lock(*this);

    // Get default input and output devices
    PAOperationUPtr operation(pa_context_get_server_info(m_context, serverInfoCallback, this));
    if (operation) {
        while (pa_operation_get_state(operation.get()) == PA_OPERATION_RUNNING)
            pa_threaded_mainloop_wait(m_mainLoop);
    } else {
        qWarning("PulseAudioService: failed to get server info");
    }

    // Get output devices
    operation.reset(pa_context_get_sink_info_list(m_context, sinkInfoCallback, this));
    if (operation) {
        while (pa_operation_get_state(operation.get()) == PA_OPERATION_RUNNING)
            pa_threaded_mainloop_wait(m_mainLoop);
    } else {
        qWarning("PulseAudioService: failed to get sink info");
    }

    // Get input devices
    operation.reset(pa_context_get_source_info_list(m_context, sourceInfoCallback, this));
    if (operation) {
        while (pa_operation_get_state(operation.get()) == PA_OPERATION_RUNNING)
            pa_threaded_mainloop_wait(m_mainLoop);
    } else {
        qWarning("PulseAudioService: failed to get source info");
    }
}

void QPulseAudioEngine::onContextFailed()
{
    // Give a chance to the connected slots to still use the Pulse main loop before releasing it.
    emit contextFailed();

    release();

    // Try to reconnect later
    QTimer::singleShot(3000, this, SLOT(prepare()));
}

QPulseAudioEngine *QPulseAudioEngine::instance()
{
    return pulseEngine();
}

QList<QAudioDevice> QPulseAudioEngine::availableDevices(QAudioDevice::Mode mode) const
{
    if (mode == QAudioDevice::Output) {
        QReadLocker locker(&m_sinkLock);
        return m_sinks.values();
    }

    if (mode == QAudioDevice::Input) {
        QReadLocker locker(&m_sourceLock);
        return m_sources.values();
    }

    return {};
}

QByteArray QPulseAudioEngine::defaultDevice(QAudioDevice::Mode mode) const
{
    return (mode == QAudioDevice::Output) ? m_defaultSink : m_defaultSource;
}

QT_END_NAMESPACE
