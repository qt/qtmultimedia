// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpulseaudio_contextmanager_p.h"

#include <QtCore/qdebug.h>
#include <QtCore/qtimer.h>
#include <QtCore/private/qflatmap_p.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/qicon.h>
#include <QtMultimedia/qaudiodevice.h>
#include <QtMultimedia/private/qaudiodevice_p.h>
#include <QtMultimedia/private/qpulsehelpers_p.h>
#include <QtMultimedia/private/qpulseaudiodevice_p.h>

#include <sys/types.h>
#include <unistd.h>
#include <mutex> // for lock_guard

QT_BEGIN_NAMESPACE

using PAOperationHandle = QPulseAudioInternal::PAOperationHandle;

static std::unique_ptr<QAudioDevicePrivate>
makeQAudioDevicePrivate(const char *device, const char *desc, bool isDef, QAudioDevice::Mode mode,
                        const pa_channel_map &map, const pa_sample_spec &spec)
{
    using namespace QPulseAudioInternal;

    auto deviceInfo = std::make_unique<QPulseAudioDevicePrivate>(device, mode, QString::fromUtf8(desc));
    QAudioFormat::ChannelConfig channelConfig = channelConfigFromMap(map);

    deviceInfo->isDefault = isDef;
    deviceInfo->channelConfiguration = channelConfig;

    deviceInfo->minimumChannelCount = 1;
    deviceInfo->maximumChannelCount = PA_CHANNELS_MAX;
    deviceInfo->minimumSampleRate = 1;
    deviceInfo->maximumSampleRate = PA_RATE_MAX;

    constexpr bool isBigEndian = QSysInfo::ByteOrder == QSysInfo::BigEndian;

    constexpr struct
    {
        pa_sample_format pa_fmt;
        QAudioFormat::SampleFormat qt_fmt;
    } formatMap[] = {
        { PA_SAMPLE_U8, QAudioFormat::UInt8 },
        { isBigEndian ? PA_SAMPLE_S16BE : PA_SAMPLE_S16LE, QAudioFormat::Int16 },
        { isBigEndian ? PA_SAMPLE_S32BE : PA_SAMPLE_S32LE, QAudioFormat::Int32 },
        { isBigEndian ? PA_SAMPLE_FLOAT32BE : PA_SAMPLE_FLOAT32LE, QAudioFormat::Float },
    };

    for (const auto &f : formatMap) {
        if (pa_sample_format_valid(f.pa_fmt) != 0)
            deviceInfo->supportedSampleFormats.append(f.qt_fmt);
    }

    QAudioFormat preferredFormat = sampleSpecToAudioFormat(spec);
    if (!preferredFormat.isValid()) {
        preferredFormat.setChannelCount(spec.channels ? spec.channels : 2);
        preferredFormat.setSampleRate(spec.rate ? spec.rate : 48000);

        Q_ASSERT(spec.format != PA_SAMPLE_INVALID);
        if (!deviceInfo->supportedSampleFormats.contains(preferredFormat.sampleFormat()))
            preferredFormat.setSampleFormat(QAudioFormat::Float);
    }

    deviceInfo->preferredFormat = preferredFormat;
    deviceInfo->preferredFormat.setChannelConfig(channelConfig);
    Q_ASSERT(deviceInfo->preferredFormat.isValid());

    return deviceInfo;
}

template<typename Info>
static bool updateDevicesMap(QReadWriteLock &lock, const QByteArray &defaultDeviceId,
                             QMap<int, QAudioDevice> &devices, QAudioDevice::Mode mode,
                             const Info &info)
{
    QWriteLocker locker(&lock);

    bool isDefault = defaultDeviceId == info.name;
    auto newDeviceInfo = makeQAudioDevicePrivate(info.name, info.description, isDefault, mode,
                                                 info.channel_map, info.sample_spec);

    auto &device = devices[info.index];
    QAudioDevicePrivateAllMembersEqual compare;
    const QAudioDevicePrivate *handle = QAudioDevicePrivate::handle(device);
    if (handle && compare(*newDeviceInfo, *handle))
        return false;

    device = QAudioDevicePrivate::createQAudioDevice(std::move(newDeviceInfo));
    return true;
}

static bool updateDevicesMap(QReadWriteLock &lock, const QByteArray &defaultDeviceId,
                             QMap<int, QAudioDevice> &devices)
{
    QWriteLocker locker(&lock);

    bool result = false;

    for (QAudioDevice &device : devices) {
        auto deviceInfo = QAudioDevicePrivate::handle<QPulseAudioDevicePrivate>(device);
        const auto isDefault = deviceInfo->id == defaultDeviceId;
        if (deviceInfo->isDefault != isDefault) {
            auto newDeviceInfo = std::make_unique<QPulseAudioDevicePrivate>(*deviceInfo);
            newDeviceInfo->isDefault = isDefault;
            device = QAudioDevicePrivate::createQAudioDevice(std::move(newDeviceInfo));
            result = true;
        }
    }

    return result;
};

void QPulseAudioContextManager::serverInfoCallback(pa_context *context, const pa_server_info *info,
                                                   void *userdata)
{
    using namespace Qt::Literals;
    using namespace QPulseAudioInternal;

    if (!info) {
        qWarning() << "Failed to get server information:" << currentError(context);
        return;
    }

    if (Q_UNLIKELY(qLcPulseAudioEngine().isEnabled(QtDebugMsg))) {
        char ss[PA_SAMPLE_SPEC_SNPRINT_MAX], cm[PA_CHANNEL_MAP_SNPRINT_MAX];

        pa_sample_spec_snprint(ss, sizeof(ss), &info->sample_spec);
        pa_channel_map_snprint(cm, sizeof(cm), &info->channel_map);

        qCDebug(qLcPulseAudioEngine)
                << QStringLiteral("User name: %1\n"
                                  "Host Name: %2\n"
                                  "Server Name: %3\n"
                                  "Server Version: %4\n"
                                  "Default Sample Specification: %5\n"
                                  "Default Channel Map: %6\n"
                                  "Default Sink: %7\n"
                                  "Default Source: %8\n")
                           .arg(QString::fromUtf8(info->user_name),
                                QString::fromUtf8(info->host_name),
                                QString::fromUtf8(info->server_name),
                                QLatin1StringView(info->server_version), QLatin1StringView(ss),
                                QLatin1StringView(cm), QString::fromUtf8(info->default_sink_name),
                                QString::fromUtf8(info->default_source_name));
    }

    QPulseAudioContextManager *pulseEngine = static_cast<QPulseAudioContextManager *>(userdata);

    bool defaultSinkChanged = false;
    bool defaultSourceChanged = false;

    {
        QWriteLocker locker(&pulseEngine->m_serverLock);
        pulseEngine->m_serverName = QString::fromUtf8(info->server_name);

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

void QPulseAudioContextManager::sinkInfoCallback(pa_context *context, const pa_sink_info *info,
                                                 int isLast, void *userdata)
{
    using namespace Qt::Literals;
    using namespace QPulseAudioInternal;

    QPulseAudioContextManager *pulseEngine = static_cast<QPulseAudioContextManager *>(userdata);

    if (isLast < 0) {
        qWarning() << "Failed to get sink information:" << currentError(context);
        return;
    }

    if (isLast) {
        pa_threaded_mainloop_signal(pulseEngine->mainloop(), 0);
        return;
    }

    Q_ASSERT(info);

    if (Q_UNLIKELY(qLcPulseAudioEngine().isEnabled(QtDebugMsg))) {
        static const QFlatMap<pa_sink_state, QStringView> stateMap{
            { PA_SINK_INVALID_STATE, u"n/a" }, { PA_SINK_RUNNING, u"RUNNING" },
            { PA_SINK_IDLE, u"IDLE" },         { PA_SINK_SUSPENDED, u"SUSPENDED" },
            { PA_SINK_UNLINKED, u"UNLINKED" },
        };

        qCDebug(qLcPulseAudioEngine)
                << QStringLiteral("Sink #%1\n"
                                  "\tState: %2\n"
                                  "\tName: %3\n"
                                  "\tDescription: %4\n")
                           .arg(QString::number(info->index), stateMap.value(info->state),
                                QString::fromUtf8(info->name),
                                QString::fromUtf8(info->description));
    }

    if (updateDevicesMap(pulseEngine->m_sinkLock, pulseEngine->m_defaultSink, pulseEngine->m_sinks,
                         QAudioDevice::Output, *info))
        emit pulseEngine->audioOutputsChanged();
}

void QPulseAudioContextManager::sourceInfoCallback(pa_context *context, const pa_source_info *info,
                                                   int isLast, void *userdata)
{
    using namespace Qt::Literals;

    Q_UNUSED(context);
    QPulseAudioContextManager *pulseEngine = static_cast<QPulseAudioContextManager *>(userdata);

    if (isLast) {
        pa_threaded_mainloop_signal(pulseEngine->mainloop(), 0);
        return;
    }

    Q_ASSERT(info);

    if (Q_UNLIKELY(qLcPulseAudioEngine().isEnabled(QtDebugMsg))) {
        static const QFlatMap<pa_source_state, QStringView> stateMap{
            { PA_SOURCE_INVALID_STATE, u"n/a" }, { PA_SOURCE_RUNNING, u"RUNNING" },
            { PA_SOURCE_IDLE, u"IDLE" },         { PA_SOURCE_SUSPENDED, u"SUSPENDED" },
            { PA_SOURCE_UNLINKED, u"UNLINKED" },
        };

        qCDebug(qLcPulseAudioEngine)
                << QStringLiteral("Source #%1\n"
                                  "\tState: %2\n"
                                  "\tName: %3\n"
                                  "\tDescription: %4\n")
                           .arg(QString::number(info->index), stateMap.value(info->state),
                                QString::fromUtf8(info->name),
                                QString::fromUtf8(info->description));
    }

    // skip monitor channels
    if (info->monitor_of_sink != PA_INVALID_INDEX)
        return;

    if (updateDevicesMap(pulseEngine->m_sourceLock, pulseEngine->m_defaultSource,
                         pulseEngine->m_sources, QAudioDevice::Input, *info))
        emit pulseEngine->audioInputsChanged();
}

void QPulseAudioContextManager::eventCallback(pa_context *context, pa_subscription_event_type_t t,
                                              uint32_t index, void *userdata)
{
    QPulseAudioContextManager *pulseEngine = static_cast<QPulseAudioContextManager *>(userdata);

    int type = t & PA_SUBSCRIPTION_EVENT_TYPE_MASK;
    int facility = t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK;

    switch (type) {
    case PA_SUBSCRIPTION_EVENT_NEW:
    case PA_SUBSCRIPTION_EVENT_CHANGE:
        switch (facility) {
        case PA_SUBSCRIPTION_EVENT_SERVER: {
            PAOperationHandle op{
                pa_context_get_server_info(context, serverInfoCallback, userdata),
                PAOperationHandle::HasRef,
            };
            if (!op)
                qWarning() << "PulseAudioService: failed to get server info";
            break;
        }
        case PA_SUBSCRIPTION_EVENT_SINK: {
            PAOperationHandle op{
                pa_context_get_sink_info_by_index(context, index, sinkInfoCallback, userdata),
                PAOperationHandle::HasRef,
            };

            if (!op)
                qWarning() << "PulseAudioService: failed to get sink info";
            break;
        }
        case PA_SUBSCRIPTION_EVENT_SOURCE: {
            PAOperationHandle op{
                pa_context_get_source_info_by_index(context, index, sourceInfoCallback, userdata),
                PAOperationHandle::HasRef,
            };

            if (!op)
                qWarning() << "PulseAudioService: failed to get source info";
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

void QPulseAudioContextManager::contextStateCallbackInit(pa_context *context, void *userdata)
{
    Q_UNUSED(context);

    if (Q_UNLIKELY(qLcPulseAudioEngine().isEnabled(QtDebugMsg)))
        qCDebug(qLcPulseAudioEngine) << pa_context_get_state(context);

    QPulseAudioContextManager *pulseEngine =
            reinterpret_cast<QPulseAudioContextManager *>(userdata);
    pa_threaded_mainloop_signal(pulseEngine->mainloop(), 0);
}

void QPulseAudioContextManager::contextStateCallback(pa_context *c, void *userdata)
{
    QPulseAudioContextManager *self = reinterpret_cast<QPulseAudioContextManager *>(userdata);
    pa_context_state_t state = pa_context_get_state(c);

    if (Q_UNLIKELY(qLcPulseAudioEngine().isEnabled(QtDebugMsg)))
        qCDebug(qLcPulseAudioEngine) << state;

    if (state == PA_CONTEXT_FAILED)
        QMetaObject::invokeMethod(self, &QPulseAudioContextManager::onContextFailed,
                                  Qt::QueuedConnection);
}

Q_GLOBAL_STATIC(QPulseAudioContextManager, pulseEngine);

QPulseAudioContextManager::QPulseAudioContextManager(QObject *parent) : QObject(parent)
{
    prepare();
}

QPulseAudioContextManager::~QPulseAudioContextManager()
{
    release();
}

void QPulseAudioContextManager::prepare()
{
    using namespace QPulseAudioInternal;
    bool keepGoing = true;
    bool ok = true;

    m_mainLoop.reset(pa_threaded_mainloop_new());
    if (m_mainLoop == nullptr) {
        qCritical() << "PulseAudioService: unable to create pulseaudio mainloop";
        return;
    }

    pa_threaded_mainloop_set_name(
            m_mainLoop.get(), "QPulseAudioEngi"); // thread names are limited to 15 chars on linux

    if (pa_threaded_mainloop_start(m_mainLoop.get()) != 0) {
        qCritical() << "PulseAudioService: unable to start pulseaudio mainloop";
        m_mainLoop = {};
        return;
    }

    m_mainLoopApi = pa_threaded_mainloop_get_api(m_mainLoop.get());

    std::unique_lock guard{ *this };

    PAProplistHandle proplist{
        pa_proplist_new(),
    };
    if (!QGuiApplication::applicationDisplayName().isEmpty())
        pa_proplist_sets(proplist.get(), PA_PROP_APPLICATION_NAME,
                         qUtf8Printable(QGuiApplication::applicationDisplayName()));
    if (!QGuiApplication::desktopFileName().isEmpty())
        pa_proplist_sets(proplist.get(), PA_PROP_APPLICATION_ID,
                         qUtf8Printable(QGuiApplication::desktopFileName()));
    if (const QString windowIconName = QGuiApplication::windowIcon().name();
        !windowIconName.isEmpty())
        pa_proplist_sets(proplist.get(), PA_PROP_WINDOW_ICON_NAME, qUtf8Printable(windowIconName));

    m_context = PAContextHandle{
        pa_context_new_with_proplist(m_mainLoopApi, nullptr, proplist.get()),
        PAContextHandle::HasRef,
    };

    if (!m_context) {
        qCritical() << "PulseAudioService: Unable to create new pulseaudio context";
        guard.unlock();
        m_mainLoop = {};
        onContextFailed();
        return;
    }

    pa_context_set_state_callback(m_context.get(), contextStateCallbackInit, this);

    if (pa_context_connect(m_context.get(), nullptr, static_cast<pa_context_flags_t>(0), nullptr)
        < 0) {
        qWarning() << "PulseAudioService: pa_context_connect() failed";
        m_context = {};
        guard.unlock();
        m_mainLoop = {};
        return;
    }

    pa_threaded_mainloop_wait(m_mainLoop.get());

    while (keepGoing) {
        switch (pa_context_get_state(m_context.get())) {
        case PA_CONTEXT_CONNECTING:
        case PA_CONTEXT_AUTHORIZING:
        case PA_CONTEXT_SETTING_NAME:
            break;

        case PA_CONTEXT_READY:
            qCDebug(qLcPulseAudioEngine) << "Connection established.";
            keepGoing = false;
            break;

        case PA_CONTEXT_TERMINATED:
            qCritical("PulseAudioService: Context terminated.");
            keepGoing = false;
            ok = false;
            break;

        case PA_CONTEXT_FAILED:
        default:
            qCritical() << "PulseAudioService: Connection failure:"
                        << currentError(m_context.get());
            keepGoing = false;
            ok = false;
        }

        if (keepGoing)
            pa_threaded_mainloop_wait(m_mainLoop.get());
    }

    if (ok) {
        pa_context_set_state_callback(m_context.get(), contextStateCallback, this);

        pa_context_set_subscribe_callback(m_context.get(), eventCallback, this);
        PAOperationHandle op{
            pa_context_subscribe(m_context.get(),
                                 pa_subscription_mask_t(PA_SUBSCRIPTION_MASK_SINK
                                                        | PA_SUBSCRIPTION_MASK_SOURCE
                                                        | PA_SUBSCRIPTION_MASK_SERVER),
                                 nullptr, nullptr),
            PAOperationHandle::HasRef,
        };

        if (!op)
            qWarning() << "PulseAudioService: failed to subscribe to context notifications";
    } else {
        m_context = {};
    }

    guard.unlock();

    if (ok) {
        updateDevices();
    } else {
        m_mainLoop = {};
        onContextFailed();
    }
}

void QPulseAudioContextManager::release()
{
    if (m_context) {
        std::lock_guard lock{ *this };
        pa_context_disconnect(m_context.get());
        m_context = {};
    }

    if (m_mainLoop) {
        pa_threaded_mainloop_stop(m_mainLoop.get());
        m_mainLoop = {};
    }
}

void QPulseAudioContextManager::updateDevices()
{
    std::lock_guard lock(*this);

    // Get default input and output devices
    bool success = waitForAsyncOperation(
            pa_context_get_server_info(m_context.get(), serverInfoCallback, this));

    if (!success)
        qWarning() << "PulseAudioService: failed to get server info";

    // Get output devices
    success = waitForAsyncOperation(
            pa_context_get_sink_info_list(m_context.get(), sinkInfoCallback, this));

    if (!success)
        qWarning() << "PulseAudioService: failed to get sink info";

    // Get input devices
    success = waitForAsyncOperation(
            pa_context_get_source_info_list(m_context.get(), sourceInfoCallback, this));

    if (!success)
        qWarning() << "PulseAudioService: failed to get source info";
}

void QPulseAudioContextManager::onContextFailed()
{
    release();

    // Try to reconnect later
    QTimer::singleShot(3000, this, &QPulseAudioContextManager::prepare);
}

QPulseAudioContextManager *QPulseAudioContextManager::instance()
{
    return pulseEngine();
}

bool QPulseAudioContextManager::waitForAsyncOperation(pa_operation *op)
{
    PAOperationHandle operation{
        op,
        PAOperationHandle::HasRef,
    };

    if (!operation)
        return false;

    wait(operation);
    return true;
}

QList<QAudioDevice> QPulseAudioContextManager::availableDevices(QAudioDevice::Mode mode) const
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

QByteArray QPulseAudioContextManager::defaultDevice(QAudioDevice::Mode mode) const
{
    return (mode == QAudioDevice::Output) ? m_defaultSink : m_defaultSource;
}

pa_context_state_t QPulseAudioContextManager::getContextState()
{
    auto lock = std::lock_guard{ *this };
    return pa_context_get_state(m_context.get());
}

bool QPulseAudioContextManager::contextIsGood()
{
    return PA_CONTEXT_IS_GOOD(getContextState());
}

QString QPulseAudioContextManager::serverName()
{
    QReadLocker locker(&pulseEngine->m_serverLock);
    return m_serverName;
}

QT_END_NAMESPACE
