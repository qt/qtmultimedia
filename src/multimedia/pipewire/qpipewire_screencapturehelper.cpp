// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpipewire_screencapturehelper_p.h"

#include <QtMultimedia/private/qpipewire_instance_p.h>
#include <QtMultimedia/private/qpipewire_videoformat_support_p.h>
#include <QtMultimedia/private/qmemoryvideobuffer_p.h>
#include <QtMultimedia/private/qpipewire_async_support_p.h>
#include <QtMultimedia/private/qvideoframe_p.h>
#include <QtGui/private/qdesktopunixservices_p.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/qpa/qplatformintegration.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qrandom.h>
#include <QtCore/quuid.h>
#include <QtCore/qvariantmap.h>
#include <QtDBus/qdbusconnection.h>
#include <QtDBus/qdbusinterface.h>
#include <QtDBus/qdbusmessage.h>
#include <QtDBus/qdbusreply.h>
#include <QtDBus/qdbusunixfiledescriptor.h>

#include <spa/debug/types.h>
#include <spa/param/video/format-utils.h>
#include <spa/param/video/type-info.h>

#include <mutex>

// pipewire's macros tend to emit unused value warnings
QT_WARNING_PUSH
QT_WARNING_DISABLE_CLANG("-Wunused-value")

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;
using namespace std::chrono_literals;
using namespace std::string_view_literals;

Q_STATIC_LOGGING_CATEGORY(qLcPipeWireCapture, "qt.multimedia.pipewire.capture");
Q_STATIC_LOGGING_CATEGORY(qLcPipeWireCaptureMore, "qt.multimedia.pipewire.capture.more");

namespace QtPipeWire {

QPipeWireCaptureHelper::QPipeWireCaptureHelper(QPipeWireCapture &capture,
                                               std::shared_ptr<QPipeWireInstance> instance)
    : QSurfaceCaptureGrabber(CreateGrabbingThread),
      m_pwInstance(std::move(instance)),
      m_requestTokenPrefix(QUuid::createUuid().toString(QUuid::WithoutBraces).left(8))
{
    Q_ASSERT(m_pwInstance);

    addFrameCallback(&capture, &QPipeWireCapture::newVideoFrame);
    connect(this, &QSurfaceCaptureGrabber::errorUpdated, &capture, &QPipeWireCapture::updateError);
}

QPipeWireCaptureHelper::~QPipeWireCaptureHelper()
{
    stop();
}

QVideoFrame QPipeWireCaptureHelper::grabFrame()
{
    return std::move(m_currentFrame);
}

void QPipeWireCaptureHelper::initializeGrabbingContext()
{
    QSurfaceCaptureGrabber::initializeGrabbingContext();
    if (m_state == NoState)
        createInterface();
}

void QPipeWireCaptureHelper::finalizeGrabbingContext()
{
    if (m_state != NoState)
        destroy();
    QSurfaceCaptureGrabber::finalizeGrabbingContext();
}

QVideoFrameFormat QPipeWireCaptureHelper::frameFormat() const
{
    return m_videoFrameFormat;
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
        selectSources(QDBusObjectPath{ map[u"session_handle"_s].toString() });
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

static int generateRequestToken()
{
    return QRandomGenerator::global()->bounded(1, 25600);
}

QString QPipeWireCaptureHelper::getRequestToken()
{
    if (m_requestToken <= 0)
        m_requestToken = generateRequestToken();
    return u"u%1%2"_s.arg(m_requestTokenPrefix).arg(m_requestToken);
}

void QPipeWireCaptureHelper::createInterface()
{
    Q_ASSERT(m_pwInstance->hasScreenCastPortal());

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
                    QPlatformSurfaceCapture::Error::InternalError,
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
        updateError(QPlatformSurfaceCapture::Error::InternalError,
                    u"Failed to create session for org.freedesktop.portal.ScreenCast. Error: "_s
                            + reply.errorName() + u": "_s + reply.errorMessage());
        return;
    }

    m_operationState = CreateSession;
}

void QPipeWireCaptureHelper::selectSources(const QDBusObjectPath &sessionHandle)
{
    if (!m_screenCastInterface)
        return;

    m_sessionHandle = sessionHandle;
    m_sessionInterface = std::make_unique<QDBusInterface>(
            u"org.freedesktop.portal.Desktop"_s, sessionHandle.path(),
            u"org.freedesktop.portal.Session"_s, QDBusConnection::sessionBus());
    m_sessionInterface->connection().connect(
            u"org.freedesktop.portal.Desktop"_s, sessionHandle.path(),
            u"org.freedesktop.portal.Session"_s, u"Closed"_s, this, SLOT(sessionClosed()));

    QVariantMap options{
        { u"handle_token"_s, getRequestToken() },
        { u"types"_s, (uint)1 },
        { u"multiple"_s, false },
        { u"cursor_mode"_s, (uint)1 },
        { u"persist_mode"_s, (uint)0 },
    };
    QDBusMessage reply = m_screenCastInterface->call(u"SelectSources"_s, sessionHandle, options);
    if (!reply.errorMessage().isEmpty()) {
        updateError(QPlatformSurfaceCapture::Error::InternalError,
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

    auto *unixServices = dynamic_cast<QDesktopUnixServices *>(QGuiApplicationPrivate::platformIntegration()->services());
    const QString parentWindow = QGuiApplication::focusWindow() && unixServices
            ? unixServices->portalWindowIdentifier(QGuiApplication::focusWindow())
            : QString();
    QDBusMessage reply = m_screenCastInterface->call("Start"_L1, QDBusObjectPath(m_sessionHandle),
                                                     parentWindow, options);
    if (!reply.errorMessage().isEmpty()) {
        updateError(QPlatformSurfaceCapture::Error::InternalError,
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
            const auto position = properties[u"position"_s].value<QDBusArgument>();
            position.beginStructure();
            position >> x;
            position >> y;
            position.endStructure();
        }

        qint32 width = 0;
        qint32 height = 0;
        if (properties.contains(u"size"_s)) {
            const auto size = properties[u"size"_s].value<QDBusArgument>();
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
    QDBusReply<QDBusUnixFileDescriptor> reply =
            m_screenCastInterface->call(u"OpenPipeWireRemote"_s, m_sessionHandle, options);
    if (!reply.isValid()) {
        updateError(
                QPlatformSurfaceCapture::Error::InternalError,
                u"Failed to open pipewire remote for org.freedesktop.portal.ScreenCast. Error: name="_s
                        + reply.error().name() + u", message="_s + reply.error().message());
        return;
    }

    bool ok = open(reply.value());
    qCDebug(qLcPipeWireCapture) << "open() result=" << ok;
    if (!ok) {
        updateError(QPlatformSurfaceCapture::Error::InternalError,
                    u"Failed to open pipewire remote file descriptor"_s);
        return;
    }

    m_operationState = OpenPipeWireRemote;
}

bool QPipeWireCaptureHelper::open(QDBusUnixFileDescriptor portalFd)
{
    if (m_streams.isEmpty())
        return false;

    Q_ASSERT(m_pwInstance->hasScreenCastPortal());

    std::unique_lock pwLock = m_pwInstance->eventLoopLock();
    auto connection = m_pwInstance->connectToPortal(portalFd.takeFileDescriptor());
    if (!connection) {
        m_err = true;
        pwLock.unlock();
        updateError(QPlatformSurfaceCapture::Error::InternalError,
                    u"QPipeWireCaptureHelper failed at pw_context_connect_fd()."_s);
        return false;
    }
    m_core = std::move(*connection);

    m_registry = PwRegistryHandle{
        pw_core_get_registry(m_core.get(), PW_VERSION_REGISTRY, 0),
    };
    if (!m_registry) {
        m_err = true;
        pwLock.unlock();
        updateError(QPlatformSurfaceCapture::Error::InternalError,
                    u"QPipeWireCaptureHelper failed at pw_core_get_registry()."_s);
        return false;
    }

    struct RegistryCallbackContext
    {
        bool hasSource{};
    };

    spa_hook registryListener = {};
    const pw_registry_events registryEvents{
        .version = PW_VERSION_REGISTRY_EVENTS,
        .global =
                [](void *data, uint32_t /*id*/, uint32_t /*permissions*/, const char *type,
                   uint32_t /*version*/, const spa_dict *props) {
        if (type != std::string_view(PW_TYPE_INTERFACE_Node))
            return;

        const char *media_class = spa_dict_lookup(props, PW_KEY_MEDIA_CLASS);
        if (!media_class)
            return;

        if (media_class != "Stream/Output/Video"sv && media_class != "Video/Source"sv)
            return;
        auto *registryContext = reinterpret_cast<RegistryCallbackContext *>(data);
        registryContext->hasSource = true;
    },
        .global_remove = nullptr,
    };

    RegistryCallbackContext registryContext;
    pw_registry_add_listener(m_registry.get(), &registryListener, &registryEvents,
                             &registryContext);

    CoreEventDoneListener doneListener;
    bool initDone = false;
    doneListener.asyncWait(m_core.get(), [&] {
        initDone = true;
        m_pwInstance->pwEventLoop().signal(false);
    });

    while (!initDone) {
        if (m_pwInstance->pwEventLoop().wait_for(2s) != 0)
            break;
    }

    spa_hook_remove(&registryListener);

    if (registryContext.hasSource)
        recreateStream();

    return initDone && registryContext.hasSource;
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
    const struct std::array<spa_dict_item, 3> items{
        SPA_DICT_ITEM_INIT(PW_KEY_MEDIA_TYPE, "Video"),
        SPA_DICT_ITEM_INIT(PW_KEY_MEDIA_CATEGORY, "Capture"),
        SPA_DICT_ITEM_INIT(PW_KEY_MEDIA_ROLE, "Screen"),
    };
    const struct spa_dict info = SPA_DICT_INIT(items.data(), items.size());
    auto *props = pw_properties_new_dict(&info);

    std::unique_lock locker = m_pwInstance->eventLoopLock();

    m_stream = PwStreamHandle{
        pw_stream_new(m_core.get(), "video-capture", props),
    };
    if (!m_stream) {
        m_err = true;
        locker.unlock();
        updateError(QPlatformSurfaceCapture::Error::InternalError,
                    u"QPipeWireCaptureHelper failed at pw_stream_new()."_s);
        return;
    }

    m_streamListener = {};
    pw_stream_add_listener(m_stream.get(), &m_streamListener, &streamEvents, this);

    QT_WARNING_PUSH
    // QTBUG-129587: libpipewire=1.2.5 warning
    QT_WARNING_DISABLE_GCC("-Wmissing-field-initializers")
    QT_WARNING_DISABLE_CLANG("-Wmissing-field-initializers")

    std::array<uint8_t, 4096> buffer;
    struct spa_pod_builder builder = SPA_POD_BUILDER_INIT(buffer.data(), sizeof(buffer));
    struct spa_rectangle defsize = SPA_RECTANGLE(quint32(streamInfo.rect.width()), quint32(streamInfo.rect.height()));
    struct spa_rectangle maxsize = SPA_RECTANGLE(4096, 4096);
    struct spa_rectangle minsize = SPA_RECTANGLE(1,1);

    // Considering the framerate as always variable rate, but with our target set as maximum.
    struct spa_fraction maxrate = SPA_FRACTION(1000, 1);
    struct spa_fraction minrate = SPA_FRACTION(0, 1);
    auto rate = rateFromFps(frameRate());

    std::array<const struct spa_pod *, 1> params{ static_cast<const spa_pod*>(spa_pod_builder_add_object(
            &builder,
            SPA_TYPE_OBJECT_Format,     SPA_PARAM_EnumFormat,
            SPA_FORMAT_mediaType,          SPA_POD_Id(SPA_MEDIA_TYPE_video),
            SPA_FORMAT_mediaSubtype,       SPA_POD_Id(SPA_MEDIA_SUBTYPE_raw),
            SPA_FORMAT_VIDEO_format,       SPA_POD_CHOICE_ENUM_Id(6,
                                               SPA_VIDEO_FORMAT_RGB,
                                               SPA_VIDEO_FORMAT_BGR,
                                               SPA_VIDEO_FORMAT_RGBA,
                                               SPA_VIDEO_FORMAT_BGRA,
                                               SPA_VIDEO_FORMAT_RGBx,
                                               SPA_VIDEO_FORMAT_BGRx),
            SPA_FORMAT_VIDEO_size,         SPA_POD_CHOICE_RANGE_Rectangle(
                                            &defsize, &minsize, &maxsize),
            SPA_FORMAT_VIDEO_framerate,    SPA_POD_CHOICE_RANGE_Fraction(
                                            &rate.frac, &minrate, &maxrate),
            SPA_FORMAT_VIDEO_maxFramerate, SPA_POD_Fraction(&rate.frac))
    )};
    QT_WARNING_POP

    const int connectErr = pw_stream_connect(
            m_stream.get(), PW_DIRECTION_INPUT, streamInfo.nodeId,
            static_cast<pw_stream_flags>(PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS),
            params.data(), 1);
    if (connectErr != 0) {
        m_err = true;
        locker.unlock();
        updateError(QPlatformSurfaceCapture::Error::InternalError,
                    u"QPipeWireCaptureHelper failed at pw_stream_connect()."_s);
        return;
    }
}
void QPipeWireCaptureHelper::destroyStream(bool waitForStreamEnd)
{
    if (!m_stream)
        return;

    std::unique_lock locker(m_pwInstance->eventLoopLock());
    m_ignoreStateChange = true;
    pw_stream_disconnect(m_stream.get());
    m_stream = {};
    m_ignoreStateChange = false;

    m_requestToken = -1;

    if (waitForStreamEnd) {
        while (m_streamState > PW_STREAM_STATE_UNCONNECTED && !m_err) {
            if (m_pwInstance->pwEventLoop().wait_for(1s) != 0)
                break;
        }
    }
}

void QPipeWireCaptureHelper::signalLoop(bool onProcessDone, bool err)
{
    if (err)
        m_err = true;
    if (onProcessDone)
        m_processed = true;
    m_pwInstance->pwEventLoop().signal(false);
}

void QPipeWireCaptureHelper::onStateChanged(pw_stream_state old, pw_stream_state state, const char *error)
{
    Q_UNUSED(old)
    Q_UNUSED(error)

    m_streamState = state;

    switch (state) {
    case PW_STREAM_STATE_UNCONNECTED:
        signalLoop(false, true);
        break;
    case PW_STREAM_STATE_PAUSED:
        signalLoop(false, false);
        break;
    case PW_STREAM_STATE_STREAMING:
        signalLoop(false, false);
        break;
    default:
        break;
    }
}
void QPipeWireCaptureHelper::onProcess()
{
    struct pw_buffer *pwBuffer = pw_stream_dequeue_buffer(m_stream.get());
    if (!pwBuffer) {
        updateError(QPlatformSurfaceCapture::Error::InternalError,
                    u"Out of buffers in pipewire stream dequeue."_s);
        return;
    }

    struct spa_buffer *buf = pwBuffer->buffer;
    void *sdata = buf->datas[0].data;
    if (!sdata)
        return;

    int sstride = buf->datas[0].chunk->stride;
    if (sstride == 0)
        sstride = int(buf->datas[0].chunk->size / m_size.height());
    qsizetype size = buf->datas[0].chunk->size;

    if (m_videoFrameFormat.frameSize() != m_size || m_videoFrameFormat.pixelFormat() != m_pixelFormat)
        m_videoFrameFormat = QVideoFrameFormat(m_size, m_pixelFormat);

    m_currentFrame = QVideoFramePrivate::createFrame(
            std::make_unique<QMemoryVideoBuffer>(QByteArray(static_cast<const char *>(sdata), size), sstride),
            m_videoFrameFormat);
    qCDebug(qLcPipeWireCaptureMore) << "got a frame of size " << buf->datas[0].chunk->size;

    pw_stream_queue_buffer(m_stream.get(), pwBuffer);

    signalLoop(true, false);
}

void QPipeWireCaptureHelper::destroy()
{
    Q_ASSERT(m_pwInstance->hasScreenCastPortal());

    m_state = Stopping;
    destroyStream(false);

    m_pwInstance->runWithEventLoopLock([&] {
        m_registry = {};
        m_core = {};
    });

    closeSession();

    m_state = NoState;
}

void QPipeWireCaptureHelper::closeSession()
{
    if (m_sessionHandle.path().isEmpty())
        return;

    if (m_sessionInterface) {
        m_sessionInterface->connection().disconnect(
                u"org.freedesktop.portal.Desktop"_s, m_sessionHandle.path(),
                u"org.freedesktop.portal.Session"_s, u"Closed"_s, this, SLOT(sessionClosed()));
        m_sessionInterface->asyncCall(u"Close"_s);
        m_sessionInterface.reset();
    }

    m_sessionHandle = {};
}

void QPipeWireCaptureHelper::sessionClosed()
{
    if (m_sessionHandle.path().isEmpty())
        return;

    qCWarning(qLcPipeWireCapture) << "org.freedesktop.portal.Session was closed externally";
    m_sessionHandle = {};
    m_sessionInterface.reset();
    updateError(QPlatformSurfaceCapture::Error::InternalError,
                u"PipeWire screen capture session was closed"_s);
    stop();
}

void QPipeWireCaptureHelper::onParamChanged(uint32_t id, const struct spa_pod *param)
{
    if (param == nullptr || id != SPA_PARAM_Format)
        return;

    if (spa_format_parse(param, &m_format.media_type, &m_format.media_subtype) < 0)
        return;

    if (m_format.media_type != SPA_MEDIA_TYPE_video
        || m_format.media_subtype != SPA_MEDIA_SUBTYPE_raw)
        return;

    if (spa_format_video_raw_parse(param, &m_format.info.raw) < 0)
        return;

    qCDebug(qLcPipeWireCapture) << "got video format:";
    qCDebug(qLcPipeWireCapture) << "  format: " << m_format.info.raw.format
                                << " (" << spa_debug_type_find_name(spa_type_video_format, m_format.info.raw.format) << ")";
    qCDebug(qLcPipeWireCapture) << "  size: " << toQSize(m_format.info.raw.size);
    qCDebug(qLcPipeWireCapture) << "  framerate: " << m_format.info.raw.framerate.num
                                << " / " << m_format.info.raw.framerate.denom;

    m_size = toQSize(m_format.info.raw.size);
    m_pixelFormat = toQtPixelFormat(m_format.info.raw.format);
    qCDebug(qLcPipeWireCapture) << "m_pixelFormat=" << m_pixelFormat;
}

} // namespace QtPipeWire

QT_END_NAMESPACE

QT_WARNING_POP
