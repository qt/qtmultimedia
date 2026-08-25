// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qsyntheticpipewirevideosource_p.h"

#include <QtCore/qdebug.h>
#include <QtCore/qhash.h>
#include <QtCore/qmutex.h>

#include <spa/param/video/format-utils.h>
#include <spa/pod/builder.h>
#include <spa/utils/result.h>
#include <pipewire/pipewire.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <mutex>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;
using namespace std::chrono_literals;

namespace {

// Deliberately generous: the consumer picks its own pacing, we just need to be
// at least as fast as the fastest frame rate a test asks for.
constexpr spa_fraction FrameRate = SPA_FRACTION(60, 1);

constexpr auto NodeIdTimeout = 5s;

int strideForWidth(int width)
{
    return SPA_ROUND_UP_N(width * 4, 4);
}

} // namespace

QSyntheticPipeWireVideoSourceConfig QSyntheticPipeWireVideoSourceConfig::screenCast()
{
    return {};
}

QSyntheticPipeWireVideoSourceConfig QSyntheticPipeWireVideoSourceConfig::camera(int index)
{
    const QByteArray suffix = QByteArray::number(index);
    QSyntheticPipeWireVideoSourceConfig config;
    config.loopName = "qt-fake-cam"_ba + suffix;
    config.nodeName = "qt-fake-camera-"_ba + suffix;
    config.nodeDescription = "Qt Multimedia synthetic camera "_ba + suffix;
    config.mediaClass = "Video/Source"_ba;
    config.mediaRole = "Camera"_ba;
    return config;
}

class QSyntheticPipeWireVideoSource::Private
{
public:
    using ObjectId = QtPipeWire::ObjectId;

    // --- pipewire thread-loop callbacks -------------------------------------
    static void onStateChanged(void *userData, pw_stream_state, pw_stream_state state,
                               const char *error);
    static void onParamChanged(void *userData, uint32_t id, const spa_pod *param);
    static void onProcess(void *userData);
    static void onRegistryGlobal(void *userData, uint32_t id, uint32_t permissions,
                                 const char *type, uint32_t version, const spa_dict *props);

    void linkTo(ObjectId inputPort);
    void considerPort(ObjectId port, uint32_t consumerNode);

    QSyntheticPipeWireVideoSourceConfig config;

    std::optional<QtPipeWire::PWThreadedEventLoop> threadLoop;
    QtPipeWire::PwContextHandle context;
    QtPipeWire::PwCoreConnectionHandle core;
    QtPipeWire::PwRegistryHandle registry;
    spa_hook registryListener = {};
    QtPipeWire::PwStreamHandle stream;
    spa_hook streamListener = {};

    ObjectId nodeId{ SPA_ID_INVALID };
    ObjectId outputPort{ SPA_ID_INVALID };
    // Input ports seen before their node's properties arrived; globals can come in
    // either order. Keyed by the node the port belongs to.
    std::unordered_map<uint32_t, std::vector<ObjectId>> pendingInputPorts;
    // QHash::value() gives a default-constructed (empty) QByteArray for an unknown
    // node without a separate lookup, which is exactly what considerPort() wants.
    QHash<uint32_t, QByteArray> consumerRoles;
    std::unordered_set<uint32_t> mismatchWarned;
    std::vector<QtPipeWire::PwProxyHandle> links;

    QMutex sceneMutex;
    SyntheticVideoScene scene;
    QImage frame; // guarded by sceneMutex
    unsigned tick = 0; // guarded by sceneMutex

    bool started = false;
};

void QSyntheticPipeWireVideoSource::Private::onStateChanged(void *userData, pw_stream_state,
                                                            pw_stream_state state,
                                                            const char *error)
{
    auto *self = static_cast<Private *>(userData);
    if (error)
        qWarning() << "QSyntheticPipeWireVideoSource: stream error:" << error;

    if (state == PW_STREAM_STATE_PAUSED || state == PW_STREAM_STATE_STREAMING) {
        // The node exists on the server from PAUSED onwards, so this is the
        // earliest point the id can be reported to a portal client.
        const ObjectId id{ pw_stream_get_node_id(self->stream.get()) };
        if (id.value != SPA_ID_INVALID && self->nodeId != id) {
            self->nodeId = id;
            self->threadLoop->signal(false);
        }
    }
}

void QSyntheticPipeWireVideoSource::Private::onParamChanged(void *userData, uint32_t id,
                                                            const spa_pod *param)
{
    auto *self = static_cast<Private *>(userData);
    if (!param || id != SPA_PARAM_Format)
        return;

    spa_video_info info = {};
    if (spa_format_parse(param, &info.media_type, &info.media_subtype) < 0)
        return;
    if (info.media_type != SPA_MEDIA_TYPE_video || info.media_subtype != SPA_MEDIA_SUBTYPE_raw)
        return;
    if (spa_format_video_raw_parse(param, &info.info.raw) < 0)
        return;

    const int stride = strideForWidth(int(info.info.raw.size.width));
    const int size = stride * int(info.info.raw.size.height);

    std::array<uint8_t, 1024> buffer;
    spa_pod_builder builder = SPA_POD_BUILDER_INIT(buffer.data(), buffer.size());
    const spa_pod *params[1];
    params[0] = static_cast<const spa_pod *>(spa_pod_builder_add_object(
            &builder, SPA_TYPE_OBJECT_ParamBuffers, SPA_PARAM_Buffers, SPA_PARAM_BUFFERS_buffers,
            SPA_POD_CHOICE_RANGE_Int(4, 2, 8), SPA_PARAM_BUFFERS_blocks, SPA_POD_Int(1),
            SPA_PARAM_BUFFERS_size, SPA_POD_Int(size), SPA_PARAM_BUFFERS_stride,
            SPA_POD_Int(stride), SPA_PARAM_BUFFERS_dataType,
            SPA_POD_CHOICE_FLAGS_Int((1 << SPA_DATA_MemFd) | (1 << SPA_DATA_MemPtr))));

    pw_stream_update_params(self->stream.get(), params, 1);
}

void QSyntheticPipeWireVideoSource::Private::onProcess(void *userData)
{
    auto *self = static_cast<Private *>(userData);

    pw_buffer *pwBuffer = pw_stream_dequeue_buffer(self->stream.get());
    if (!pwBuffer)
        return; // consumer is not keeping up; simply skip this cycle

    spa_buffer *buffer = pwBuffer->buffer;
    auto *destination = static_cast<uint8_t *>(buffer->datas[0].data);
    if (!destination) {
        pw_stream_queue_buffer(self->stream.get(), pwBuffer);
        return;
    }

    int height = 0;
    int stride = 0;
    {
        QMutexLocker locker(&self->sceneMutex);
        if (self->frame.isNull()) {
            pw_stream_queue_buffer(self->stream.get(), pwBuffer);
            return;
        }
        height = self->frame.height();
        stride = strideForWidth(self->frame.width());

        const qsizetype copyBytes = std::min<qsizetype>(stride, self->frame.bytesPerLine());
        for (int y = 0; y < height; ++y)
            memcpy(destination + qsizetype(y) * stride, self->frame.constScanLine(y), copyBytes);
    }

    buffer->datas[0].chunk->offset = 0;
    buffer->datas[0].chunk->stride = stride;
    buffer->datas[0].chunk->size = uint32_t(stride * height);

    pw_stream_queue_buffer(self->stream.get(), pwBuffer);
}

void QSyntheticPipeWireVideoSource::Private::linkTo(ObjectId inputPort)
{
    if (outputPort.value == SPA_ID_INVALID)
        return;

    const QByteArray out = QByteArray::number(outputPort.value);
    const QByteArray in = QByteArray::number(inputPort.value);

    QtPipeWire::PwPropertiesHandle props{
        pw_properties_new(PW_KEY_LINK_OUTPUT_PORT, out.constData(), PW_KEY_LINK_INPUT_PORT,
                          in.constData(), PW_KEY_OBJECT_LINGER, "false", nullptr),
    };
    // pw_core_create_object() is a macro yielding void *.
    QtPipeWire::PwProxyHandle link{
        static_cast<pw_proxy *>(pw_core_create_object(core.get(), "link-factory",
                                                      PW_TYPE_INTERFACE_Link, PW_VERSION_LINK,
                                                      &props->dict, 0)),
    };

    if (!link) {
        qWarning() << "QSyntheticPipeWireVideoSource: failed to link port" << outputPort.value
                   << "->" << inputPort.value;
        return;
    }
    links.push_back(std::move(link));
}

/*!
    Decides whether \a port, belonging to \a consumerNode, is meant for us, and
    links it if so. Called once the node's properties and the port are both known.
*/
void QSyntheticPipeWireVideoSource::Private::considerPort(ObjectId port, uint32_t consumerNode)
{
    const QByteArray role = consumerRoles.value(consumerNode);
    if (role == config.mediaRole) {
        linkTo(port);
        return;
    }

    // Not ours. Warn once per node: a consumer that never gets linked simply sits
    // in "paused" forever, which is otherwise indistinguishable from a hang.
    if (mismatchWarned.insert(consumerNode).second) {
        qWarning() << "QSyntheticPipeWireVideoSource(" << config.mediaRole
                   << "): not linking consumer node" << consumerNode << "with media.role"
                   << (role.isEmpty() ? "<unset>"_ba : role);
    }
}

void QSyntheticPipeWireVideoSource::Private::onRegistryGlobal(void *userData, uint32_t id, uint32_t,
                                                              const char *type, uint32_t,
                                                              const spa_dict *props)
{
    auto *self = static_cast<Private *>(userData);
    if (!props)
        return;

    if (std::string_view(type) == PW_TYPE_INTERFACE_Node) {
        if (ObjectId{ id } == self->nodeId)
            return;

        const char *role = spa_dict_lookup(props, PW_KEY_MEDIA_ROLE);
        self->consumerRoles.insert(id, role ? QByteArray(role) : QByteArray());

        // Drain ports that arrived before we knew what this node was.
        for (ObjectId port : std::exchange(self->pendingInputPorts[id], {}))
            self->considerPort(port, id);
        self->pendingInputPorts.erase(id);
        return;
    }

    if (std::string_view(type) != PW_TYPE_INTERFACE_Port)
        return;

    const char *direction = spa_dict_lookup(props, PW_KEY_PORT_DIRECTION);
    const char *portNodeId = spa_dict_lookup(props, PW_KEY_NODE_ID);
    if (!direction || !portNodeId)
        return;

    const ObjectId portId{ id };
    const ObjectId nodeOfPort{ uint32_t(QByteArray(portNodeId).toUInt()) };

    if (std::string_view(direction) == "out" && nodeOfPort == self->nodeId) {
        self->outputPort = portId;
        // Our own output port may show up after some consumers; reconsider them.
        for (const auto &[node, ports] : std::exchange(self->pendingInputPorts, {})) {
            for (ObjectId port : ports)
                self->considerPort(port, node);
        }
        return;
    }

    if (std::string_view(direction) == "in" && nodeOfPort != self->nodeId) {
        // Wait until both our output port and the consumer's identity are known.
        if (self->outputPort.value == SPA_ID_INVALID
            || !self->consumerRoles.contains(nodeOfPort.value)) {
            self->pendingInputPorts[nodeOfPort.value].push_back(portId);
            return;
        }
        self->considerPort(portId, nodeOfPort.value);
    }
}

// ---------------------------------------------------------------------------

QSyntheticPipeWireVideoSource::QSyntheticPipeWireVideoSource() : d(std::make_unique<Private>()) { }

QSyntheticPipeWireVideoSource::~QSyntheticPipeWireVideoSource()
{
    stop();
}

q23::expected<void, QString>
QSyntheticPipeWireVideoSource::start(const SyntheticVideoScene &scene,
                                     const QSyntheticPipeWireVideoSourceConfig &config)
{
    using namespace QtPipeWire;
    if (d->started)
        return q23::unexpected(u"already started"_s);

    d->config = config;
    setScene(scene);

    pw_init(nullptr, nullptr);

    d->threadLoop.emplace(config.loopName.constData());
    if (!*d->threadLoop)
        return q23::unexpected(u"pw_thread_loop_new() failed"_s);

    if (d->threadLoop->start() != 0)
        return q23::unexpected(u"pw_thread_loop_start() failed"_s);

    std::lock_guard threadLoopLock{ *d->threadLoop };
    return [&]() -> q23::expected<void, QString> {
        d->context = PwContextHandle{
            pw_context_new(d->threadLoop->loop(), nullptr, 0),
        };
        if (!d->context)
            return q23::unexpected(u"pw_context_new() failed"_s);

        d->core = PwCoreConnectionHandle{
            pw_context_connect(d->context.get(), nullptr, 0),
        };
        if (!d->core) {
            return q23::unexpected(u"pw_context_connect() failed: "_s
                                   + QString::fromLocal8Bit(strerror(errno)));
        }

        static constexpr pw_registry_events registryEvents = {
            .version = PW_VERSION_REGISTRY_EVENTS,
            .global = &Private::onRegistryGlobal,
            .global_remove = nullptr,
        };
        d->registry = PwRegistryHandle{
            pw_core_get_registry(d->core.get(), PW_VERSION_REGISTRY, 0),
        };
        pw_registry_add_listener(d->registry.get(), &d->registryListener, &registryEvents, d.get());

        // An output stream that sets media.class implements a Source rather than a
        // plain playback stream; see the pw_stream documentation.
        d->stream = PwStreamHandle{
            pw_stream_new(d->core.get(), config.nodeName.constData(),
                          pw_properties_new(PW_KEY_MEDIA_CLASS, config.mediaClass.constData(),
                                            PW_KEY_MEDIA_TYPE, "Video", PW_KEY_MEDIA_CATEGORY,
                                            config.mediaCategory.constData(), PW_KEY_MEDIA_ROLE,
                                            config.mediaRole.constData(), PW_KEY_NODE_NAME,
                                            config.nodeName.constData(), PW_KEY_NODE_DESCRIPTION,
                                            config.nodeDescription.constData(), nullptr)),
        };
        if (!d->stream)
            return q23::unexpected(u"pw_stream_new() failed"_s);

        static constexpr pw_stream_events streamEvents = {
            .version = PW_VERSION_STREAM_EVENTS,
            .destroy = nullptr,
            .state_changed = &Private::onStateChanged,
            .control_info = nullptr,
            .io_changed = nullptr,
            .param_changed = &Private::onParamChanged,
            .add_buffer = nullptr,
            .remove_buffer = nullptr,
            .process = &Private::onProcess,
            .drained = nullptr,
            .command = nullptr,
            .trigger_done = nullptr,
        };
        pw_stream_add_listener(d->stream.get(), &d->streamListener, &streamEvents, d.get());

        std::array<uint8_t, 1024> buffer;
        spa_pod_builder builder = SPA_POD_BUILDER_INIT(buffer.data(), buffer.size());
        spa_rectangle size = SPA_RECTANGLE(uint32_t(scene.frameSize.width()),
                                           uint32_t(scene.frameSize.height()));
        spa_fraction frameRate = FrameRate;
        const spa_pod *params[1];
        params[0] = static_cast<const spa_pod *>(spa_pod_builder_add_object(
                &builder, SPA_TYPE_OBJECT_Format, SPA_PARAM_EnumFormat, SPA_FORMAT_mediaType,
                SPA_POD_Id(SPA_MEDIA_TYPE_video), SPA_FORMAT_mediaSubtype,
                SPA_POD_Id(SPA_MEDIA_SUBTYPE_raw), SPA_FORMAT_VIDEO_format,
                SPA_POD_Id(SPA_VIDEO_FORMAT_BGRx), SPA_FORMAT_VIDEO_size, SPA_POD_Rectangle(&size),
                SPA_FORMAT_VIDEO_framerate, SPA_POD_Fraction(&frameRate)));

        // DRIVER: with no session manager in the graph nothing else would drive it,
        // so we clock it ourselves from renderNextFrame().
        const int error = pw_stream_connect(
                d->stream.get(), PW_DIRECTION_OUTPUT, PW_ID_ANY,
                pw_stream_flags(PW_STREAM_FLAG_MAP_BUFFERS | PW_STREAM_FLAG_DRIVER), params, 1);
        if (error < 0) {
            return q23::unexpected(u"pw_stream_connect() failed: "_s
                                   + QString::fromLocal8Bit(spa_strerror(error)));
        }

        // Wait for the node id: a portal must not answer Start() without it.
        while (d->nodeId.value == SPA_ID_INVALID) {
            if (d->threadLoop->wait_for(NodeIdTimeout) != 0)
                return q23::unexpected(u"timed out waiting for the pipewire node id"_s);
        }

        d->started = true;
        return {};
    }();
}

void QSyntheticPipeWireVideoSource::stop()
{
    if (!d->threadLoop)
        return;

    {
        std::lock_guard threadLoopLock{ *d->threadLoop };
        d->links.clear();
        d->stream.reset();
        d->registry.reset();
        d->core.reset();
    }

    d->threadLoop->stop();
    d->context.reset();
    d->threadLoop.reset();

    d->nodeId = QtPipeWire::ObjectId{ SPA_ID_INVALID };
    d->outputPort = QtPipeWire::ObjectId{ SPA_ID_INVALID };
    d->started = false;
}

QtPipeWire::ObjectId QSyntheticPipeWireVideoSource::nodeId() const
{
    return d->nodeId;
}

void QSyntheticPipeWireVideoSource::setScene(const SyntheticVideoScene &scene)
{
    QMutexLocker locker(&d->sceneMutex);
    d->scene = scene;
    d->tick = 0;
    d->frame = renderSyntheticVideoScene(scene, 0);
}

void QSyntheticPipeWireVideoSource::renderNextFrame()
{
    {
        QMutexLocker locker(&d->sceneMutex);
        if (d->scene.animated)
            d->frame = renderSyntheticVideoScene(d->scene, ++d->tick);
    }

    if (!d->stream)
        return;

    std::lock_guard threadLoopLock{ *d->threadLoop };
    pw_stream_trigger_process(d->stream.get());
}

QT_END_NAMESPACE
