// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpipewire_screencapturehelper_p.h"

#include "qpipewire_instance_p.h"

#include <QtCore/qdebug.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qmutex.h>
#include <QtCore/qrandom.h>
#include <QtCore/qurlquery.h>
#include <QtCore/quuid.h>
#include <QtCore/qvariantmap.h>
#include <QtCore/private/qcore_unix_p.h>
#include <QtDBus/qdbusconnection.h>
#include <QtDBus/qdbusinterface.h>
#include <QtDBus/qdbusmessage.h>
#include <QtDBus/qdbuspendingcall.h>
#include <QtDBus/qdbuspendingreply.h>
#include <QtDBus/qdbusreply.h>
#include <QtDBus/qdbusunixfiledescriptor.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/qpa/qplatformintegration.h>
#include <QtGui/qscreen.h>
#include <QtGui/qwindow.h>
#include <QtGui/private/qdesktopunixservices_p.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtMultimedia/qabstractvideobuffer.h>
#include <QtMultimedia/private/qvideoframe_p.h>
#include <QtMultimedia/private/qcapturablewindow_p.h>
#include <QtMultimedia/private/qmemoryvideobuffer_p.h>
#include <QtMultimedia/private/qvideoframeconversionhelper_p.h>

#include <fcntl.h>

// pipewire's macros tend to emit unused value warnings
QT_WARNING_PUSH
QT_WARNING_DISABLE_CLANG("-Wunused-value")

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

Q_STATIC_LOGGING_CATEGORY(qLcPipeWireCapture, "qt.multimedia.pipewire.capture");
Q_STATIC_LOGGING_CATEGORY(qLcPipeWireCaptureMore, "qt.multimedia.pipewire.capture.more");

namespace QtPipeWire {

struct PipeWireCaptureGlobalState
{
    PipeWireCaptureGlobalState() {
        QDBusConnection bus = QDBusConnection::sessionBus();
        QDBusInterface *interface = new QDBusInterface(
                u"org.freedesktop.portal.Desktop"_s, u"/org/freedesktop/portal/desktop"_s,
                u"org.freedesktop.DBus.Properties"_s, bus, qGuiApp);

        QList<QVariant> args;
        args << u"org.freedesktop.portal.ScreenCast"_s << u"version"_s;

        QDBusMessage reply = interface->callWithArgumentList(QDBus::Block, u"Get"_s, args);
        qCDebug(qLcPipeWireCapture) << "v1=" << reply.type()
                                    << "v2=" << reply.arguments().size()
                                    << "v3=" << reply.arguments().at(0).toUInt();
        if (reply.type() == QDBusMessage::ReplyMessage
            && reply.arguments().size() == 1
            // && reply.arguments().at(0).toUInt() >= 2
            ) {
            hasScreenCastPortal = true;
        }
        qCDebug(qLcPipeWireCapture) << Q_FUNC_INFO << "hasScreenCastPortal=" << hasScreenCastPortal;
    }

    bool hasScreenCastPortal = false;
};

Q_GLOBAL_STATIC(PipeWireCaptureGlobalState, globalState)

bool QPipeWireCaptureHelper::setActiveInternal(bool active)
{
    if (isSupported()) {
        if (active && m_state == NoState)
            createInterface();
        if (!active && m_state == Streaming)
            destroy();

        return true;
    }

    updateError(QPlatformSurfaceCapture::InternalError,
                u"There is no ScreenCast service available in org.freedesktop.portal!"_s);

    return false;
}

void QPipeWireCaptureHelper::updateError(QPlatformSurfaceCapture::Error error,
                                         const QString &description)
{
    m_capture.updateError(error, description);
}

bool QPipeWireCapture::isSupported()
{
    if (!QPipeWireInstance::isLoaded())
        return false;

    return QPipeWireCaptureHelper::isSupported();
}

QPipeWireCaptureHelper::QPipeWireCaptureHelper(QPipeWireCapture &capture)
    : m_capture(capture),
      m_requestTokenPrefix(QUuid::createUuid().toString(QUuid::WithoutBraces).left(8))
{
}

QPipeWireCaptureHelper::~QPipeWireCaptureHelper()
{
    if (m_state != NoState)
        destroy();
}

QVideoFrameFormat QPipeWireCaptureHelper::frameFormat() const
{
    return m_videoFrameFormat;
}

bool QPipeWireCaptureHelper::isSupported()
{
    if (!globalState)
        return false;

    return globalState->hasScreenCastPortal;
}

void QPipeWireCaptureHelper::gotRequestResponse(uint result, const QVariantMap &map)
{
    Q_UNUSED(map);
    qCDebug(qLcPipeWireCapture) << Q_FUNC_INFO << "result=" << result << "map=" << map;
    if (result != 0) {
        m_operationState = NoOperation;
        qWarning() << "Failed to capture screen via pipewire, perhaps because user cancelled the operation.";
        m_requestToken = -1;
        return;
    }

    switch (m_operationState) {
    case CreateSession:
        selectSources(map[u"session_handle"_s].toString());
        break;
    case SelectSources:
        startStream();
        break;
    case StartStream:
        updateStreams(map[u"streams"_s].value<QDBusArgument>());
        openPipeWireRemote();
        m_operationState = NoOperation;
        m_state = Streaming;
        break;
    case OpenPipeWireRemote:
        m_operationState = NoOperation;
        break;
    default:
        break;
    }
}

QString QPipeWireCaptureHelper::getRequestToken()
{
    if (m_requestToken <= 0)
        m_requestToken = generateRequestToken();
    return u"u%1%2"_s.arg(m_requestTokenPrefix).arg(m_requestToken);
}

int QPipeWireCaptureHelper::generateRequestToken()
{
    return QRandomGenerator::global()->bounded(1, 25600);
}

void QPipeWireCaptureHelper::createInterface()
{
    if (!globalState)
        return;
    if (!globalState->hasScreenCastPortal)
        return;

    m_operationState = NoOperation;

    if (!m_screenCastInterface) {
        m_screenCastInterface = std::make_unique<QDBusInterface>(
                u"org.freedesktop.portal.Desktop"_s, u"/org/freedesktop/portal/desktop"_s,
                u"org.freedesktop.portal.ScreenCast"_s, QDBusConnection::sessionBus());
        bool ok = m_screenCastInterface->connection().connect(
                u"org.freedesktop.portal.Desktop"_s, u""_s, u"org.freedesktop.portal.Request"_s,
                u"Response"_s, this, SLOT(gotRequestResponse(uint,QVariantMap)));

        if (!ok) {
            updateError(
                    QPlatformSurfaceCapture::InternalError,
                    u"Failed to connect to org.freedesktop.portal.ScreenCast dbus interface."_s);
            return;
        }
    }
    createSession();
}

void QPipeWireCaptureHelper::createSession()
{
    if (!m_screenCastInterface)
        return;

    QVariantMap options{
        //{u"handle_token"_s        , getRequestToken()},
        { u"session_handle_token"_s, getRequestToken() },
    };
    QDBusMessage reply = m_screenCastInterface->call(u"CreateSession"_s, options);
    if (!reply.errorMessage().isEmpty()) {
        updateError(QPlatformSurfaceCapture::InternalError,
                    u"Failed to create session for org.freedesktop.portal.ScreenCast. Error: "_s
                            + reply.errorName() + u": "_s + reply.errorMessage());
        return;
    }

    m_operationState = CreateSession;
}

void QPipeWireCaptureHelper::selectSources(const QString &sessionHandle)
{
    if (!m_screenCastInterface)
        return;

    m_sessionHandle = sessionHandle;
    QVariantMap options{
        { u"handle_token"_s, getRequestToken() },
        { u"types"_s, (uint)1 },
        { u"multiple"_s, false },
        { u"cursor_mode"_s, (uint)1 },
        { u"persist_mode"_s, (uint)0 },
    };
    QDBusMessage reply = m_screenCastInterface->call(u"SelectSources"_s,
                                                     QDBusObjectPath(sessionHandle), options);
    if (!reply.errorMessage().isEmpty()) {
        updateError(QPlatformSurfaceCapture::InternalError,
                    u"Failed to select sources for org.freedesktop.portal.ScreenCast. Error: "_s
                            + reply.errorName() + u": "_s + reply.errorMessage());
        return;
    }

    m_operationState = SelectSources;
}

void QPipeWireCaptureHelper::startStream()
{
    if (!m_screenCastInterface)
        return;

    QVariantMap options{
        { u"handle_token"_s, getRequestToken() },
    };

    const auto unixServices = dynamic_cast<QDesktopUnixServices *>(QGuiApplicationPrivate::platformIntegration()->services());
    const QString parentWindow = QGuiApplication::focusWindow() && unixServices
            ? unixServices->portalWindowIdentifier(QGuiApplication::focusWindow())
            : QString();
    QDBusMessage reply = m_screenCastInterface->call("Start"_L1, QDBusObjectPath(m_sessionHandle),
                                                     parentWindow, options);
    if (!reply.errorMessage().isEmpty()) {
        updateError(QPlatformSurfaceCapture::InternalError,
                    u"Failed to start stream for org.freedesktop.portal.ScreenCast. Error: "_s
                            + reply.errorName() + u": "_s + reply.errorMessage());
        return;
    }

    m_operationState = StartStream;
}

void QPipeWireCaptureHelper::updateStreams(const QDBusArgument &streamsInfo)
{
    m_streams.clear();

    streamsInfo.beginStructure();
    streamsInfo.beginArray();

    while (!streamsInfo.atEnd()) {
        quint32 nodeId = 0;
        streamsInfo >> nodeId;
        QMap<QString, QVariant> properties;
        streamsInfo >> properties;

        qint32 x = 0;
        qint32 y = 0;
        if (properties.contains(u"position"_s)) {
            const QDBusArgument position = properties[u"position"_s].value<QDBusArgument>();
            position.beginStructure();
            position >> x;
            position >> y;
            position.endStructure();
        }

        qint32 width = 0;
        qint32 height = 0;
        if (properties.contains(u"size"_s)) {
            const QDBusArgument size = properties[u"size"_s].value<QDBusArgument>();
            size.beginStructure();
            size >> width;
            size >> height;
            size.endStructure();
        }

        uint sourceType = 0;
        if (properties.contains(u"source_type"_s))
            sourceType = properties[u"source_type"_s].toUInt();

        StreamInfo streamInfo;
        streamInfo.nodeId = nodeId;
        streamInfo.sourceType = sourceType;
        streamInfo.rect = {x, y, width, height};
        m_streams << streamInfo;
    }

    streamsInfo.endArray();
    streamsInfo.endStructure();

}

void QPipeWireCaptureHelper::openPipeWireRemote()
{
    if (!m_screenCastInterface)
        return;

    QVariantMap options;
    QDBusReply<QDBusUnixFileDescriptor> reply = m_screenCastInterface->call(
            u"OpenPipeWireRemote"_s, QDBusObjectPath(m_sessionHandle), options);
    if (!reply.isValid()) {
        updateError(
                QPlatformSurfaceCapture::InternalError,
                u"Failed to open pipewire remote for org.freedesktop.portal.ScreenCast. Error: name="_s
                        + reply.error().name() + u", message="_s + reply.error().message());
        return;
    }

    m_pipewireFd = reply.value().fileDescriptor();
    bool ok = open(m_pipewireFd);
    qCDebug(qLcPipeWireCapture) << "open(" << m_pipewireFd << ") result=" << ok;
    if (!ok) {
        updateError(QPlatformSurfaceCapture::InternalError,
                    u"Failed to open pipewire remote file descriptor"_s);
        return;
    }

    m_operationState = OpenPipeWireRemote;
}

namespace {
class LoopLocker
{
public:
    LoopLocker(pw_thread_loop *threadLoop)
        : m_threadLoop(threadLoop) {
        lock();
    }
    ~LoopLocker() {
        unlock();
    }

    void lock() {
        if (m_threadLoop)
            pw_thread_loop_lock(m_threadLoop);
    }

    void unlock() {
        if (m_threadLoop) {
            pw_thread_loop_unlock(m_threadLoop);
            m_threadLoop = nullptr;
        }
    }

private:
    pw_thread_loop *m_threadLoop = nullptr;
};
} // namespace

bool QPipeWireCaptureHelper::open(int pipewireFd)
{
    if (m_streams.isEmpty())
        return false;

    if (!globalState)
        return false;

    if (!m_instance)
        m_instance = QPipeWireInstance::instance();

    static const pw_core_events coreEvents = {
        .version = PW_VERSION_CORE_EVENTS,
        .info = [](void *data, const struct pw_core_info *info) {
            Q_UNUSED(data)
            Q_UNUSED(info)
        },
        .done = [](void *object, uint32_t id, int seq) {
            reinterpret_cast<QPipeWireCaptureHelper *>(object)->onCoreEventDone(id, seq);
        },
        .ping = [](void *data, uint32_t id, int seq) {
            Q_UNUSED(data)
            Q_UNUSED(id)
            Q_UNUSED(seq)
        },
        .error = [](void *data, uint32_t id, int seq, int res, const char *message) {
            Q_UNUSED(data)
            Q_UNUSED(id)
            Q_UNUSED(seq)
            Q_UNUSED(res)
            Q_UNUSED(message)
        },
        .remove_id = [](void *data, uint32_t id) {
            Q_UNUSED(data)
            Q_UNUSED(id)
        },
        .bound_id = [](void *data, uint32_t id, uint32_t global_id) {
            Q_UNUSED(data)
            Q_UNUSED(id)
            Q_UNUSED(global_id)
        },
        .add_mem = [](void *data, uint32_t id, uint32_t type, int fd, uint32_t flags) {
            Q_UNUSED(data)
            Q_UNUSED(id)
            Q_UNUSED(type)
            Q_UNUSED(fd)
            Q_UNUSED(flags)
        },
        .remove_mem = [](void *data, uint32_t id) {
            Q_UNUSED(data)
            Q_UNUSED(id)
        },
#if defined(PW_CORE_EVENT_BOUND_PROPS)
        .bound_props = [](void *data, uint32_t id, uint32_t global_id, const struct spa_dict *props) {
            Q_UNUSED(data)
            Q_UNUSED(id)
            Q_UNUSED(global_id)
            Q_UNUSED(props)
        },
#endif // PW_CORE_EVENT_BOUND_PROPS
    };

    static const pw_registry_events registryEvents = {
        .version = PW_VERSION_REGISTRY_EVENTS,
        .global = [](void *object, uint32_t id, uint32_t permissions, const char *type, uint32_t version, const spa_dict *props) {
            reinterpret_cast<QPipeWireCaptureHelper *>(object)->onRegistryEventGlobal(id, permissions, type, version, props);
        },
        .global_remove = [](void *data, uint32_t id) {
            Q_UNUSED(data)
            Q_UNUSED(id)
        },
    };

    m_threadLoop = PwThreadLoopHandle{
        pw_thread_loop_new("qt-multimedia-pipewire-loop", nullptr),
    };
    if (!m_threadLoop) {
        m_err = true;
        updateError(QPlatformSurfaceCapture::InternalError,
                    u"QPipeWireCaptureHelper failed at pw_thread_loop_new()."_s);
        return false;
    }

    m_context = PwContextHandle{
        pw_context_new(pw_thread_loop_get_loop(m_threadLoop.get()), nullptr, 0),
    };
    if (!m_context) {
        m_err = true;
        updateError(QPlatformSurfaceCapture::InternalError,
                    u"QPipeWireCaptureHelper failed at pw_context_new()."_s);
        return false;
    }

    m_core = PwCoreConnectionHandle{
        pw_context_connect_fd(m_context.get(), fcntl(pipewireFd, F_DUPFD_CLOEXEC, 5), nullptr, 0),
    };
    if (!m_core) {
        m_err = true;
        updateError(QPlatformSurfaceCapture::InternalError,
                    u"QPipeWireCaptureHelper failed at pw_context_connect_fd()."_s);
        return false;
    }

    pw_core_add_listener(m_core.get(), &m_coreListener, &coreEvents, this);

    m_registry = PwRegistryHandle{
        pw_core_get_registry(m_core.get(), PW_VERSION_REGISTRY, 0),
    };
    if (!m_registry) {
        m_err = true;
        updateError(QPlatformSurfaceCapture::InternalError,
                    u"QPipeWireCaptureHelper failed at pw_core_get_registry()."_s);
        return false;
    }
    pw_registry_add_listener(m_registry.get(), &m_registryListener, &registryEvents, this);

    updateCoreInitSeq();

    if (pw_thread_loop_start(m_threadLoop.get()) != 0) {
        m_err = true;
        updateError(QPlatformSurfaceCapture::InternalError,
                    u"QPipeWireCaptureHelper failed at pw_thread_loop_start()."_s);
        return false;
    }

    LoopLocker locker(m_threadLoop.get());
    while (!m_initDone) {
        if (pw_thread_loop_timed_wait(m_threadLoop.get(), 2) != 0)
            break;
    }

    return m_initDone && m_hasSource;
}

void QPipeWireCaptureHelper::updateCoreInitSeq()
{
    m_coreInitSeq = pw_core_sync(m_core.get(), PW_ID_CORE, m_coreInitSeq);
}

void QPipeWireCaptureHelper::onCoreEventDone(uint32_t id, int seq)
{
    if (id == PW_ID_CORE && seq == m_coreInitSeq) {
        spa_hook_remove(&m_registryListener);
        spa_hook_remove(&m_coreListener);

        m_initDone = true;
        pw_thread_loop_signal(m_threadLoop.get(), false);
    }
}

void QPipeWireCaptureHelper::onRegistryEventGlobal(uint32_t id, uint32_t permissions, const char *type, uint32_t version, const spa_dict *props)
{
    Q_UNUSED(id)
    Q_UNUSED(permissions)
    Q_UNUSED(version)

    if (qstrcmp(type, PW_TYPE_INTERFACE_Node) != 0)
        return;

    auto media_class = spa_dict_lookup(props, PW_KEY_MEDIA_CLASS);
    if (!media_class)
        return;

    if (qstrcmp(media_class, "Stream/Output/Video") != 0
        && qstrcmp(media_class, "Video/Source") != 0)
        return;

    m_hasSource = true;

    updateCoreInitSeq();

    recreateStream();
}

void QPipeWireCaptureHelper::recreateStream()
{
    static const pw_stream_events streamEvents = {
        .version = PW_VERSION_STREAM_EVENTS,
        .destroy = [](void *data) {
            Q_UNUSED(data)
        },
        .state_changed = [](void *data, pw_stream_state old, pw_stream_state state, const char *error) {
            reinterpret_cast<QPipeWireCaptureHelper *>(data)->onStateChanged(old, state, error);
        },
        .control_info = [](void *data, uint32_t id, const struct pw_stream_control *control) {
            Q_UNUSED(data)
            Q_UNUSED(id)
            Q_UNUSED(control)
        },
        .io_changed = [](void *data, uint32_t id, void *area, uint32_t size) {
            Q_UNUSED(data)
            Q_UNUSED(id)
            Q_UNUSED(area)
            Q_UNUSED(size)
        },
        .param_changed = [](void *data, uint32_t id, const struct spa_pod *param) {
            reinterpret_cast<QPipeWireCaptureHelper *>(data)->onParamChanged(id, param);
        },
        .add_buffer = [](void *data, struct pw_buffer *buffer) {
            Q_UNUSED(data)
            Q_UNUSED(buffer)
        },
        .remove_buffer = [](void *data, struct pw_buffer *buffer) {
            Q_UNUSED(data)
            Q_UNUSED(buffer)
        },
        .process = [](void *data) {
            reinterpret_cast<QPipeWireCaptureHelper *>(data)->onProcess();
        },
        .drained = [](void *data) {
            Q_UNUSED(data)
        },
#if PW_VERSION_STREAM_EVENTS >= 1
        .command = [](void *data, const struct spa_command *command) {
            Q_UNUSED(data)
            Q_UNUSED(command)
        },
#endif
#if PW_VERSION_STREAM_EVENTS >= 2
        .trigger_done = [](void *data) {
            Q_UNUSED(data)
        },
#endif
    };

    destroyStream(true);

    auto streamInfo = m_streams[0];
    struct spa_dict_item items[4];
    struct spa_dict info;
    items[0] = SPA_DICT_ITEM_INIT(PW_KEY_MEDIA_TYPE, "Video");
    items[1] = SPA_DICT_ITEM_INIT(PW_KEY_MEDIA_CATEGORY, "Capture");
    items[2] = SPA_DICT_ITEM_INIT(PW_KEY_MEDIA_ROLE, "Screen");
    info = SPA_DICT_INIT(items, 3);
    auto props = pw_properties_new_dict(&info);

    LoopLocker locker(m_threadLoop.get());

    m_stream = PwStreamHandle{
        pw_stream_new(m_core.get(), "video-capture", props),
    };
    if (!m_stream) {
        m_err = true;
        locker.unlock();
        updateError(QPlatformSurfaceCapture::InternalError,
                    u"QPipeWireCaptureHelper failed at pw_stream_new()."_s);
        return;
    }

    m_streamListener = {};
    pw_stream_add_listener(m_stream.get(), &m_streamListener, &streamEvents, this);

    QT_WARNING_PUSH
    // QTBUG-129587: libpipewire=1.2.5 warning
    QT_WARNING_DISABLE_GCC("-Wmissing-field-initializers")
    QT_WARNING_DISABLE_CLANG("-Wmissing-field-initializers")

    uint8_t buffer[4096];
    struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));
    const struct spa_pod *params[1];
    struct spa_rectangle defsize = SPA_RECTANGLE(quint32(streamInfo.rect.width()), quint32(streamInfo.rect.height()));
    struct spa_rectangle maxsize = SPA_RECTANGLE(4096, 4096);
    struct spa_rectangle minsize = SPA_RECTANGLE(1,1);
    struct spa_fraction defrate  = SPA_FRACTION(25, 1);
    struct spa_fraction maxrate  = SPA_FRACTION(1000, 1);
    struct spa_fraction minrate  = SPA_FRACTION(0, 1);

    params[0] = static_cast<const spa_pod*>(spa_pod_builder_add_object(
            &b,
            SPA_TYPE_OBJECT_Format,     SPA_PARAM_EnumFormat,
            SPA_FORMAT_mediaType,       SPA_POD_Id(SPA_MEDIA_TYPE_video),
            SPA_FORMAT_mediaSubtype,    SPA_POD_Id(SPA_MEDIA_SUBTYPE_raw),
            SPA_FORMAT_VIDEO_format,    SPA_POD_CHOICE_ENUM_Id(6,
                                            SPA_VIDEO_FORMAT_RGB,
                                            SPA_VIDEO_FORMAT_BGR,
                                            SPA_VIDEO_FORMAT_RGBA,
                                            SPA_VIDEO_FORMAT_BGRA,
                                            SPA_VIDEO_FORMAT_RGBx,
                                            SPA_VIDEO_FORMAT_BGRx),
            SPA_FORMAT_VIDEO_size,      SPA_POD_CHOICE_RANGE_Rectangle(
                                            &defsize, &minsize, &maxsize),
            SPA_FORMAT_VIDEO_framerate, SPA_POD_CHOICE_RANGE_Fraction(
                                            &defrate, &minrate, &maxrate))
    );
    QT_WARNING_POP

    const int connectErr = pw_stream_connect(
            m_stream.get(), PW_DIRECTION_INPUT, streamInfo.nodeId,
            static_cast<pw_stream_flags>(PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS),
            params, 1);
    if (connectErr != 0) {
        m_err = true;
        locker.unlock();
        updateError(QPlatformSurfaceCapture::InternalError,
                    u"QPipeWireCaptureHelper failed at pw_stream_connect()."_s);
        return;
    }
}
void QPipeWireCaptureHelper::destroyStream(bool forceDrain)
{
    if (!m_stream)
        return;

    if (forceDrain) {
        LoopLocker locker(m_threadLoop.get());
        while (!m_streamPaused && !m_silence && !m_err) {
            if (pw_thread_loop_timed_wait(m_threadLoop.get(), 1) != 0)
                break;
        }
    }

    LoopLocker locker(m_threadLoop.get());
    m_ignoreStateChange = true;
    pw_stream_disconnect(m_stream.get());
    m_stream = {};
    m_ignoreStateChange = false;

    m_stream = nullptr;
    m_requestToken = -1;
}

void QPipeWireCaptureHelper::signalLoop(bool onProcessDone, bool err)
{
    if (err)
        m_err = true;
    if (onProcessDone)
        m_processed = true;
    pw_thread_loop_signal(m_threadLoop.get(), false);
}

void QPipeWireCaptureHelper::onStateChanged(pw_stream_state old, pw_stream_state state, const char *error)
{
    Q_UNUSED(old)
    Q_UNUSED(error)

    if (m_ignoreStateChange)
        return;

    switch (state)
    {
    case PW_STREAM_STATE_UNCONNECTED:
        signalLoop(false, true);
        break;
    case PW_STREAM_STATE_PAUSED:
        m_streamPaused = true;
        signalLoop(false, false);
        break;
    case PW_STREAM_STATE_STREAMING:
        m_streamPaused = false;
        signalLoop(false, false);
        break;
    default:
        break;
    }
}
void QPipeWireCaptureHelper::onProcess()
{
    struct pw_buffer *b;
    struct spa_buffer *buf;
    int sstride = 0;
    void *sdata;
    qsizetype size = 0;

    if ((b = pw_stream_dequeue_buffer(m_stream.get())) == nullptr) {
        updateError(QPlatformSurfaceCapture::InternalError,
                    u"Out of buffers in pipewire stream dequeue."_s);
        return;
    }

    buf = b->buffer;
    if ((sdata = buf->datas[0].data) == nullptr)
        return;

    sstride = buf->datas[0].chunk->stride;
    if (sstride == 0)
        sstride = buf->datas[0].chunk->size / m_size.height();
    size = buf->datas[0].chunk->size;

    if (m_videoFrameFormat.frameSize() != m_size || m_videoFrameFormat.pixelFormat() != m_pixelFormat)
        m_videoFrameFormat = QVideoFrameFormat(m_size, m_pixelFormat);

    m_currentFrame = QVideoFramePrivate::createFrame(
            std::make_unique<QMemoryVideoBuffer>(QByteArray(static_cast<const char *>(sdata), size), sstride),
            m_videoFrameFormat);
    emit m_capture.newVideoFrame(m_currentFrame);
    qCDebug(qLcPipeWireCaptureMore) << "got a frame of size " << buf->datas[0].chunk->size;

    pw_stream_queue_buffer(m_stream.get(), b);

    signalLoop(true, false);
}

void QPipeWireCaptureHelper::destroy()
{
    if (!globalState)
        return;
    m_state = Stopping;
    destroyStream(false);

    pw_thread_loop_stop(m_threadLoop.get());

    m_registry = {};
    m_core = {};
    m_context = {};
    m_threadLoop = {};

    m_state = NoState;
}

void QPipeWireCaptureHelper::onParamChanged(uint32_t id, const struct spa_pod *param)
{
    if (param == nullptr || id != SPA_PARAM_Format)
        return;

    if (spa_format_parse(param,
                         &m_format.media_type,
                         &m_format.media_subtype) < 0)
        return;

    if (m_format.media_type != SPA_MEDIA_TYPE_video
        || m_format.media_subtype != SPA_MEDIA_SUBTYPE_raw)
        return;

    if (spa_format_video_raw_parse(param, &m_format.info.raw) < 0)
        return;

    qCDebug(qLcPipeWireCapture) << "got video format:";
    qCDebug(qLcPipeWireCapture) << "  format: " << m_format.info.raw.format
                                << " (" << spa_debug_type_find_name(spa_type_video_format, m_format.info.raw.format) << ")";
    qCDebug(qLcPipeWireCapture) << "  size: " << m_format.info.raw.size.width
                                << " x " << m_format.info.raw.size.height;
    qCDebug(qLcPipeWireCapture) << "  framerate: " << m_format.info.raw.framerate.num
                                << " / " << m_format.info.raw.framerate.denom;

    m_size = QSize(m_format.info.raw.size.width, m_format.info.raw.size.height);
    m_pixelFormat = QPipeWireCaptureHelper::toQtPixelFormat(m_format.info.raw.format);
    qCDebug(qLcPipeWireCapture) << "m_pixelFormat=" << m_pixelFormat;
}

// align with qt_videoFormatLookup in src/plugins/multimedia/gstreamer/common/qgst.cpp
// https://docs.pipewire.org/group__spa__param.html#gacb274daea0abcce261955323e7d0b1aa
//     Most of the formats are identical to their GStreamer equivalent.
QVideoFrameFormat::PixelFormat QPipeWireCaptureHelper::toQtPixelFormat(spa_video_format spaVideoFormat)
{
    switch (spaVideoFormat) {
    default:
        break;
    case SPA_VIDEO_FORMAT_I420:
        return QVideoFrameFormat::Format_YUV420P;
    case SPA_VIDEO_FORMAT_Y42B:
        return QVideoFrameFormat::Format_YUV422P;
    case SPA_VIDEO_FORMAT_YV12:
        return QVideoFrameFormat::Format_YV12;
    case SPA_VIDEO_FORMAT_UYVY:
        return QVideoFrameFormat::Format_UYVY;
    case SPA_VIDEO_FORMAT_YUY2:
        return QVideoFrameFormat::Format_YUYV;
    case SPA_VIDEO_FORMAT_NV12:
        return QVideoFrameFormat::Format_NV12;
    case SPA_VIDEO_FORMAT_NV21:
        return QVideoFrameFormat::Format_NV21;
    case SPA_VIDEO_FORMAT_AYUV:
        return QVideoFrameFormat::Format_AYUV;
    case SPA_VIDEO_FORMAT_GRAY8:
        return QVideoFrameFormat::Format_Y8;
    case SPA_VIDEO_FORMAT_xRGB:
        return QVideoFrameFormat::Format_XRGB8888;
    case SPA_VIDEO_FORMAT_xBGR:
        return QVideoFrameFormat::Format_XBGR8888;
    case SPA_VIDEO_FORMAT_RGBx:
        return QVideoFrameFormat::Format_RGBX8888;
    case SPA_VIDEO_FORMAT_BGRx:
        return QVideoFrameFormat::Format_BGRX8888;
    case SPA_VIDEO_FORMAT_ARGB:
        return QVideoFrameFormat::Format_ARGB8888;
    case SPA_VIDEO_FORMAT_ABGR:
        return QVideoFrameFormat::Format_ABGR8888;
    case SPA_VIDEO_FORMAT_RGBA:
        return QVideoFrameFormat::Format_RGBA8888;
    case SPA_VIDEO_FORMAT_BGRA:
        return QVideoFrameFormat::Format_BGRA8888;
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    case SPA_VIDEO_FORMAT_GRAY16_LE:
        return QVideoFrameFormat::Format_Y16;
    case SPA_VIDEO_FORMAT_P010_10LE:
        return QVideoFrameFormat::Format_P010;
#else
    case SPA_VIDEO_FORMAT_GRAY16_BE:
        return QVideoFrameFormat::Format_Y16;
    case SPA_VIDEO_FORMAT_P010_10BE:
        return QVideoFrameFormat::Format_P010;
#endif
    }

    return QVideoFrameFormat::Format_Invalid;
}

spa_video_format QPipeWireCaptureHelper::toSpaVideoFormat(QVideoFrameFormat::PixelFormat pixelFormat)
{
    switch (pixelFormat) {
    default:
        break;
    case QVideoFrameFormat::Format_YUV420P:
        return SPA_VIDEO_FORMAT_I420;
    case QVideoFrameFormat::Format_YUV422P:
        return SPA_VIDEO_FORMAT_Y42B;
    case QVideoFrameFormat::Format_YV12:
        return SPA_VIDEO_FORMAT_YV12;
    case QVideoFrameFormat::Format_UYVY:
        return SPA_VIDEO_FORMAT_UYVY;
    case QVideoFrameFormat::Format_YUYV:
        return SPA_VIDEO_FORMAT_YUY2;
    case QVideoFrameFormat::Format_NV12:
        return SPA_VIDEO_FORMAT_NV12;
    case QVideoFrameFormat::Format_NV21:
        return SPA_VIDEO_FORMAT_NV21;
    case QVideoFrameFormat::Format_AYUV:
        return SPA_VIDEO_FORMAT_AYUV;
    case QVideoFrameFormat::Format_Y8:
        return SPA_VIDEO_FORMAT_GRAY8;
    case QVideoFrameFormat::Format_XRGB8888:
        return SPA_VIDEO_FORMAT_xRGB;
    case QVideoFrameFormat::Format_XBGR8888:
        return SPA_VIDEO_FORMAT_xBGR;
    case QVideoFrameFormat::Format_RGBX8888:
        return SPA_VIDEO_FORMAT_RGBx;
    case QVideoFrameFormat::Format_BGRX8888:
        return SPA_VIDEO_FORMAT_BGRx;
    case QVideoFrameFormat::Format_ARGB8888:
        return SPA_VIDEO_FORMAT_ARGB;
    case QVideoFrameFormat::Format_ABGR8888:
        return SPA_VIDEO_FORMAT_ABGR;
    case QVideoFrameFormat::Format_RGBA8888:
        return SPA_VIDEO_FORMAT_RGBA;
    case QVideoFrameFormat::Format_BGRA8888:
        return SPA_VIDEO_FORMAT_BGRA;
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    case QVideoFrameFormat::Format_Y16:
        return SPA_VIDEO_FORMAT_GRAY16_LE;
    case QVideoFrameFormat::Format_P010:
        return SPA_VIDEO_FORMAT_P010_10LE;
#else
    case QVideoFrameFormat::Format_Y16:
        return SPA_VIDEO_FORMAT_GRAY16_BE;
    case QVideoFrameFormat::Format_P010:
        return SPA_VIDEO_FORMAT_P010_10BE;
#endif
    }

    return SPA_VIDEO_FORMAT_UNKNOWN;
}

} // namespace QtPipeWire

QT_END_NAMESPACE

QT_WARNING_POP
