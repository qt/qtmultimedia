// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpipewire_videostream_p.h"

#include <QtMultimedia/private/qmemoryvideobuffer_p.h>
#include <QtMultimedia/private/qpipewire_propertydict_p.h>
#include <QtMultimedia/private/qpipewire_videoformat_support_p.h>
#include <QtMultimedia/private/qvideoframe_p.h>
#include <QtCore/qloggingcategory.h>

#include <spa/param/video/format-utils.h>
#include <spa/pod/builder.h>

#if __has_include(<spa/param/buffers.h>)
#  include <spa/param/buffers.h>
#else
#  include <QtMultimedia/private/qpipewire_spa_compat_p.h>
#endif

#if QT_PIPEWIRE_HAS_DMABUF_CAMERA
#  include <QtMultimedia/private/qcaptureslotregistry_p.h>
#  include <QtMultimedia/private/qdmabufallocator_p.h>
#  include <QtMultimedia/private/qdmabufsource_p.h>
#  include <QtMultimedia/private/qdmabufvideobuffer_p.h>
#  include <QtMultimedia/private/qmaterializingvideobuffer_p.h>
#  include <QtMultimedia/private/qmultimedia_dmabuf_support_p.h>
#  include <QtMultimedia/private/qvideotexturehelper_p.h>
#endif

#include <array>
#ifndef QT_NO_DEBUG
#  include <atomic>
#endif
#include <iterator>
#include <optional>

QT_BEGIN_NAMESPACE

namespace QtPipeWire {

using namespace Qt::StringLiterals;

Q_STATIC_LOGGING_CATEGORY(lcPipewireCameraStream, "qt.multimedia.pipewire.camera.stream")

namespace {

// The raw formats a camera realistically produces, offered when the caller did
// not ask for one in particular.
constexpr std::array cameraPixelFormats{
    SPA_VIDEO_FORMAT_NV12, SPA_VIDEO_FORMAT_YUY2, SPA_VIDEO_FORMAT_UYVY,
    SPA_VIDEO_FORMAT_I420, SPA_VIDEO_FORMAT_RGB,  SPA_VIDEO_FORMAT_BGR,
    SPA_VIDEO_FORMAT_RGBA, SPA_VIDEO_FORMAT_BGRA, SPA_VIDEO_FORMAT_GRAY8,
};

enum class CameraMemory : uint8_t { Auto, MemPtr, MemFd, DmaBuf };

CameraMemory cameraMemoryFromEnvironment()
{
    const QByteArray value = qgetenv("QT_PIPEWIRE_CAMERA_MEMORY");
    if (value.isEmpty() || value == "auto")
        return CameraMemory::Auto;
    if (value == "memptr")
        return CameraMemory::MemPtr;
    if (value == "memfd")
        return CameraMemory::MemFd;
    if (value == "dmabuf") {
#if QT_PIPEWIRE_HAS_DMABUF_CAMERA
        return CameraMemory::DmaBuf;
#else
        qCWarning(lcPipewireCameraStream)
                << "QT_PIPEWIRE_CAMERA_MEMORY=dmabuf needs PipeWire 0.3.76 or newer, using auto";
        return CameraMemory::Auto;
#endif
    }

    qCWarning(lcPipewireCameraStream)
            << "unknown QT_PIPEWIRE_CAMERA_MEMORY value" << value << ", using auto";
    return CameraMemory::Auto;
}

// The buffer type an override insists on, or nullopt when any type will do.
[[maybe_unused]] std::optional<spa_data_type> forcedDataType(CameraMemory memory)
{
    switch (memory) {
    case CameraMemory::Auto:
        return std::nullopt;
    case CameraMemory::MemPtr:
        return SPA_DATA_MemPtr;
    case CameraMemory::MemFd:
        return SPA_DATA_MemFd;
    case CameraMemory::DmaBuf:
        return SPA_DATA_DmaBuf;
    }
    Q_UNREACHABLE_RETURN(std::nullopt);
}

// pts is in nanoseconds since an arbitrary epoch.
void setStartTime(QVideoFrame &frame, const spa_buffer &buffer,
                  std::optional<std::chrono::microseconds> &firstFrameTime)
{
    using namespace std::chrono;

    const auto *header = static_cast<const spa_meta_header *>(
            spa_buffer_find_meta_data(&buffer, SPA_META_Header, sizeof(spa_meta_header)));
    const microseconds pts = [&] {
        if (header && header->pts > 0) {
            // pts is in nanoseconds since an arbitrary epoch.
            return round<microseconds>(nanoseconds{ header->pts });
        } else {
            return round<microseconds>(steady_clock::now().time_since_epoch());
        }
    }();

    if (!firstFrameTime)
        firstFrameTime = pts;
    frame.setStartTime(qint64((pts - *firstFrameTime).count()));
}

// Copies the frame out of the buffer. Each frame is copied eagerly, so the
// buffer can be requeued as soon as this returns.
std::optional<QByteArray> copyFrameData(const spa_buffer &buffer,
                                        [[maybe_unused]] bool dmaBufReadable)
{
    const spa_data &data = buffer.datas[0];

#if QT_PIPEWIRE_HAS_DMABUF_CAMERA
    if (data.type == SPA_DATA_DmaBuf) {
        // A single plane, since each plane would otherwise need its own stride. A
        // compressed frame is a byte stream, so only raw frames depend on the layout.
        if (buffer.n_datas != 1 || !dmaBufReadable) {
            qCDebug(lcPipewireCameraStream)
                    << "dropping DMABUF frame: needs one plane in a linear modifier";
            return std::nullopt;
        }

        if (!data.chunk || data.maxsize == 0)
            return std::nullopt;
        const size_t chunkOffset = data.chunk->offset % data.maxsize;
        if (chunkOffset + data.chunk->size > data.maxsize) {
            qCDebug(lcPipewireCameraStream) << "dropping DMABUF frame: chunk exceeds the buffer";
            return std::nullopt;
        }
        return QtMultimediaPrivate::copyDmaBufData(
                data.fd, off_t(data.mapoffset) + off_t(chunkOffset), int(data.chunk->size),
                size_t(data.mapoffset) + data.maxsize);
    }
#endif

    if (!data.data || !data.chunk)
        return std::nullopt;
    return QByteArray(static_cast<const char *>(data.data) + data.chunk->offset,
                      qsizetype(data.chunk->size));
}

} // namespace

#if QT_PIPEWIRE_HAS_DMABUF_CAMERA
namespace {

// PipeWire links at most link.max-buffers, typically 16 (libcamera nodes are limited to 4).
constexpr std::size_t kDmaBufBufferCount = 8;
constexpr std::size_t kDmaBufMinFreeBuffers = 2;

} // namespace

// Owns the DMA-BUFs the camera node fills. A frame holding a pw_buffer keeps it out of the
// stream's queue; releasing the slot queues it back to the node.
class DmaBufBufferRegistry final
    : public QtMultimediaPrivate::CaptureSlotRegistry<DmaBufBufferRegistry, pw_buffer *,
                                                      kDmaBufBufferCount>
{
public:
    using QDmaBuf = QtMultimediaPrivate::QDmaBuf;

    DmaBufBufferRegistry(std::vector<std::shared_ptr<QDmaBuf>> buffers, int stride,
                         QPipeWireInstance &instance, pw_stream *stream)
        : CaptureSlotRegistry(buffers.size(), kDmaBufMinFreeBuffers),
          m_buffers(std::move(buffers)),
          m_stride(stride),
          m_instance(instance),
          m_stream(stream)
    {
        Q_ASSERT(m_buffers.size() == kDmaBufBufferCount);
    }

    void recycleSlot(pw_buffer *buffer) const
    {
#  ifndef QT_NO_DEBUG
        m_leased[bufferIndex(buffer)].store(false);
#  endif
        m_instance.runWithEventLoopLock([&] {
            pw_stream_queue_buffer(m_stream, buffer);
        });
    }

    void markLeased([[maybe_unused]] std::size_t index)
    {
#  ifndef QT_NO_DEBUG
        m_leased[index].store(true);
#  endif
    }

    // Points the buffer at its DMA-BUF. PipeWire re-adds a buffer it already knows when it
    // renegotiates, so a buffer keeps the DMA-BUF it was first given.
    void bindBuffer(pw_buffer *pwBuffer)
    {
#  ifndef QT_NO_DEBUG
        if (pwBuffer->user_data) {
            Q_ASSERT(!m_leased[bufferIndex(pwBuffer)].load()
                     && "PipeWire re-added a buffer that a frame still holds");
        }
#  endif
        if (!pwBuffer->user_data) {
            Q_ASSERT(m_bound < m_buffers.size());
            // One past the index, so that a null user_data means the buffer is unbound.
            pwBuffer->user_data = reinterpret_cast<void *>(++m_bound);
        }

        const QDmaBuf &buf = *m_buffers[bufferIndex(pwBuffer)];

        spa_data &data = pwBuffer->buffer->datas[0];
        data.type = SPA_DATA_DmaBuf;
        data.flags = SPA_DATA_FLAG_READWRITE;
        data.fd = buf.fd();
        data.mapoffset = 0;
        data.maxsize = uint32_t(buf.size());
        data.data = nullptr;
    }

    static std::size_t bufferIndex(const pw_buffer *pwBuffer)
    {
        return reinterpret_cast<std::size_t>(pwBuffer->user_data) - 1;
    }

    std::shared_ptr<QDmaBuf> buffer(std::size_t index) const { return m_buffers[index]; }
    int stride() const { return m_stride; }

private:
    const std::vector<std::shared_ptr<QDmaBuf>> m_buffers;
#  ifndef QT_NO_DEBUG
    mutable std::array<std::atomic<bool>, kDmaBufBufferCount> m_leased{};
#  endif
    const int m_stride;
    QPipeWireInstance &m_instance;
    pw_stream *const m_stream;
    std::size_t m_bound = 0;
};
#endif

QPipeWireVideoStream::QPipeWireVideoStream(QCameraContextManager &context, QByteArray targetObject,

                                           QVideoFrameFormat::PixelFormat requestedPixelFormat,
                                           QSize requestedSize, qreal requestedFrameRate,
                                           bool allowStreamDmaBufs)
    : m_context(context),
      m_targetObject(std::move(targetObject)),
      m_requestedPixelFormat(requestedPixelFormat),
      m_requestedSize(requestedSize),
      m_requestedFrameRate(requestedFrameRate),
      m_allowStreamDmaBufs(allowStreamDmaBufs)
{
}

QPipeWireVideoStream::~QPipeWireVideoStream()
{
#if QT_PIPEWIRE_HAS_DMABUF_CAMERA
    // Outside the loop lock: waits for releases that take it to requeue buffers.
    if (m_bufferRegistry)
        m_bufferRegistry->stop();
#endif

    if (!m_stream)
        return;

    m_context.pwInstance().runWithEventLoopLock([&] {
        pw_stream_disconnect(m_stream.get());
        spa_hook_remove(&m_streamListener);
        m_stream = {};
    });
}

const pw_stream_events &QPipeWireVideoStream::streamEvents()
{
    QT_WARNING_PUSH
    // QTBUG-129587: libpipewire 1.2.5 grew fields we do not set
    QT_WARNING_DISABLE_GCC("-Wmissing-field-initializers")
    QT_WARNING_DISABLE_CLANG("-Wmissing-field-initializers")
    // clang-format off
    static constexpr pw_stream_events events = {
        .version = PW_VERSION_STREAM_EVENTS,
        .state_changed = [](void *data, pw_stream_state o, pw_stream_state s, const char *err) {
            reinterpret_cast<QPipeWireVideoStream *>(data)->onStateChanged(o, s, err);
        },
        .param_changed = [](void *data, uint32_t id, const spa_pod *param) {
            reinterpret_cast<QPipeWireVideoStream *>(data)->onParamChanged(id, param);
        },
#if QT_PIPEWIRE_HAS_DMABUF_CAMERA
        .add_buffer = [](void *data, pw_buffer *buffer) {
            reinterpret_cast<QPipeWireVideoStream *>(data)->onAddBuffer(buffer);
        },
#endif
        .process = [](void *data) {
            reinterpret_cast<QPipeWireVideoStream *>(data)->onProcess();
        },
    };
    // clang-format on
    QT_WARNING_POP

    return events;
}

namespace {

// How the EnumFormat param restricts the DRM modifier to the linear layout, the only one a
// DMABUF buffer can be read through. Optional lets a node omit the modifier altogether.
enum class LinearModifier : uint8_t { Off, Optional, Mandatory };

// The EnumFormat param for the requested pixel format and size.
const spa_pod *buildFormatParam(std::array<uint8_t, 1024> &storage,
                                QVideoFrameFormat::PixelFormat requestedPixelFormat,
                                QSize requestedSize, qreal requestedFrameRate,
                                [[maybe_unused]] LinearModifier linearModifier)
{
    spa_pod_builder builder = SPA_POD_BUILDER_INIT(storage.data(), uint32_t(storage.size()));

    spa_pod_frame frame{};
    spa_pod_builder_push_object(&builder, &frame, SPA_TYPE_OBJECT_Format, SPA_PARAM_EnumFormat);
    spa_pod_builder_add(&builder, SPA_FORMAT_mediaType, SPA_POD_Id(SPA_MEDIA_TYPE_video), 0);

    switch (requestedPixelFormat) {
    case QVideoFrameFormat::Format_Jpeg: {
        spa_pod_builder_add(&builder, SPA_FORMAT_mediaSubtype, SPA_POD_Id(SPA_MEDIA_SUBTYPE_mjpg),
                            0);
        break;
    }
    default: {
        spa_pod_builder_add(&builder, SPA_FORMAT_mediaSubtype, SPA_POD_Id(SPA_MEDIA_SUBTYPE_raw),
                            0);

        const spa_video_format requestedPixelFormatSpa = toSpaVideoFormat(requestedPixelFormat);
        switch (requestedPixelFormatSpa) {
        case SPA_VIDEO_FORMAT_UNKNOWN: {
            // advertise all alternatives, letting PipeWire pick one of the camera's supported formats
            spa_pod_builder_prop(&builder, SPA_FORMAT_VIDEO_format, 0);
            spa_pod_frame choice{};
            spa_pod_builder_push_choice(&builder, &choice, SPA_CHOICE_Enum, 0);
            // The first entry is both the default and one of the alternatives.
            spa_pod_builder_id(&builder, cameraPixelFormats.front());
            for (spa_video_format format : cameraPixelFormats)
                spa_pod_builder_id(&builder, format);
            spa_pod_builder_pop(&builder, &choice);
            break;
        }

        default: {
            spa_pod_builder_add(&builder, SPA_FORMAT_VIDEO_format,
                                SPA_POD_Id(requestedPixelFormatSpa), 0);
            break;
        }
        }
        break;
    }
    }

    if (!requestedSize.isEmpty()) {
        const spa_rectangle exactSize = toSpaRectangle(requestedSize);
        spa_pod_builder_add(&builder, SPA_FORMAT_VIDEO_size, SPA_POD_Rectangle(&exactSize), 0);
    } else {
        constexpr spa_rectangle minSize{ .width = 1, .height = 1 };
        constexpr spa_rectangle maxSize{ .width = 8192, .height = 8192 };

        spa_pod_builder_add(&builder, SPA_FORMAT_VIDEO_size,
                            SPA_POD_CHOICE_RANGE_Rectangle(&maxSize, &minSize, &maxSize), 0);
    }

    // Cameras are treated as variable rate with the requested rate as the
    // upper bound
    constexpr spa_fraction minRate{ .num = 0, .denom = 1 };
    const FrameRate rate = rateFromFps(requestedFrameRate > 0. ? requestedFrameRate : 30.);
    spa_pod_builder_add(&builder, SPA_FORMAT_VIDEO_framerate,
                        SPA_POD_CHOICE_RANGE_Fraction(&rate.frac, &minRate, &rate.frac), 0);

#if QT_PIPEWIRE_HAS_DMABUF_CAMERA
    if (linearModifier != LinearModifier::Off) {
        spa_pod_frame modifierChoice{};
        spa_pod_builder_prop(
                &builder, SPA_FORMAT_VIDEO_modifier,
                linearModifier == LinearModifier::Mandatory ? SPA_POD_PROP_FLAG_MANDATORY : 0);
        spa_pod_builder_push_choice(&builder, &modifierChoice, SPA_CHOICE_Enum, 0);
        spa_pod_builder_long(&builder, int64_t(QtMultimediaPrivate::DrmFormatModifierLinear));
        spa_pod_builder_pop(&builder, &modifierChoice);
    }
#endif

    return static_cast<const spa_pod *>(spa_pod_builder_pop(&builder, &frame));
}

// The Buffers param, which restricts the memory types the stream accepts.
const spa_pod *buildBuffersParam(std::array<uint8_t, 1024> &storage, BufferDataTypes dataTypes)
{
    spa_pod_builder builder = SPA_POD_BUILDER_INIT(storage.data(), uint32_t(storage.size()));
    return static_cast<const spa_pod *>(spa_pod_builder_add_object(
            &builder, SPA_TYPE_OBJECT_ParamBuffers, SPA_PARAM_Buffers, SPA_PARAM_BUFFERS_dataType,
            SPA_POD_CHOICE_FLAGS_Int(uint32_t(dataTypes.to_ulong()))));
}

bool offersDmaBuf(CameraMemory memory)
{
    return QT_PIPEWIRE_HAS_DMABUF_CAMERA
            && (memory == CameraMemory::Auto || memory == CameraMemory::DmaBuf);
}

#if QT_PIPEWIRE_HAS_DMABUF_CAMERA
// The Buffers param for stream-allocated DMA-BUFs. The stride is the one the camera node must
// write frames with, so it is fixed here rather than read back from each frame.
const spa_pod *buildDmaBufBuffersParam(std::array<uint8_t, 1024> &storage, int32_t size,
                                       int32_t stride)
{
    spa_pod_builder builder = SPA_POD_BUILDER_INIT(storage.data(), uint32_t(storage.size()));
    return static_cast<const spa_pod *>(spa_pod_builder_add_object(
            &builder, SPA_TYPE_OBJECT_ParamBuffers, SPA_PARAM_Buffers, SPA_PARAM_BUFFERS_buffers,
            SPA_POD_Int(int32_t(kDmaBufBufferCount)), SPA_PARAM_BUFFERS_blocks, SPA_POD_Int(1),
            SPA_PARAM_BUFFERS_size, SPA_POD_Int(size), SPA_PARAM_BUFFERS_stride,
            SPA_POD_Int(stride), SPA_PARAM_BUFFERS_align, SPA_POD_Int(16),
            SPA_PARAM_BUFFERS_dataType, SPA_POD_CHOICE_FLAGS_Int(1u << SPA_DATA_DmaBuf)));
}

#endif

BufferDataTypes bufferDataTypes(CameraMemory memory)
{
    BufferDataTypes dataTypes;
    if (memory == CameraMemory::Auto || memory == CameraMemory::MemPtr)
        dataTypes.set(SPA_DATA_MemPtr);
    if (memory == CameraMemory::Auto || memory == CameraMemory::MemFd)
        dataTypes.set(SPA_DATA_MemFd);
    if (offersDmaBuf(memory))
        dataTypes.set(SPA_DATA_DmaBuf);
    return dataTypes;
}

} // namespace

// Memory type negotiation with the camera node has three steps:
//
//  1. connect(): the stream offers the formats it accepts, and the buffer memory
//     types it can receive, including DMABUF when it is supported.
//  2. param_changed(Format): once the server has picked a format, the stream
//     decides whether it can read DMABUF frames in that format. If it cannot, it
//     withdraws DMABUF from its offer, and the server falls back to mapped memory.
//  3. process(): frames are now delivered by the server.
//
// With stream-allocated DMA-BUFs, step 1 offers only linear DMABUF and sets
// PW_STREAM_FLAG_ALLOC_BUFFERS. Step 2 allocates the buffers for the negotiated
// size and sends them back as the Buffers param. The node then adds them, and
// onAddBuffer() binds each one to its DMA-BUF.
//
bool QPipeWireVideoStream::start()
{
    Q_ASSERT(!m_stream);

    m_firstFrameTime.reset();

    if (!m_context.isConnected())
        return false;

    const spa_dict_item items[] = {
        SPA_DICT_ITEM_INIT(PW_KEY_MEDIA_TYPE, "Video"),
        SPA_DICT_ITEM_INIT(PW_KEY_MEDIA_CATEGORY, "Capture"),
        SPA_DICT_ITEM_INIT(PW_KEY_MEDIA_ROLE, "Camera"),
        SPA_DICT_ITEM_INIT(PW_KEY_TARGET_OBJECT, m_targetObject.constData()),
    };

    return m_context.pwInstance().runWithEventLoopLock([&] {
        PwPropertiesHandle props = makeProperties(items);

        m_stream = PwStreamHandle{
            pw_stream_new(m_context.coreConnection().get(), "qt-camera", props.release()),
        };
        if (!m_stream) {
            qCWarning(lcPipewireCameraStream) << "pw_stream_new failed";
            return false;
        }

        m_streamListener = {};
        pw_stream_add_listener(m_stream.get(), &m_streamListener, &streamEvents(), this);

        const CameraMemory memory = cameraMemoryFromEnvironment();

        m_bufferDataTypes = bufferDataTypes(memory);

        std::array<uint8_t, 1024> dmaBufFormatStorage{};
        std::array<uint8_t, 1024> formatStorage{};
        std::array<uint8_t, 1024> buffersStorage{};
        std::array<const spa_pod *, 3> params{};
        uint32_t paramCount = 0;
        pw_stream_flags flags =
                pw_stream_flags(PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS);
        bool zeroCopy = false;

#if QT_PIPEWIRE_HAS_DMABUF_CAMERA
        // PipeWire refuses to allocate the stream's DMA-BUFs for MJPEG, so compressed streams
        // keep the server's buffers.
        if (m_allowStreamDmaBufs && offersDmaBuf(memory)
            && m_requestedPixelFormat != QVideoFrameFormat::Format_Jpeg) {
            if (auto allocator = QtMultimediaPrivate::QDmaBufAllocator::create()) {
                m_dmaBufAllocator.emplace(std::move(*allocator));
                zeroCopy = true;
            } else {
                qCWarning(lcPipewireCameraStream)
                        << "no DMA-BUF allocator, using server buffers:" << allocator.error();
            }
        }
        if (zeroCopy) {
            m_bufferDataTypes = BufferDataTypes{};
            m_bufferDataTypes.set(SPA_DATA_DmaBuf);
            flags = pw_stream_flags(flags | PW_STREAM_FLAG_ALLOC_BUFFERS);

            params[paramCount++] =
                    buildFormatParam(dmaBufFormatStorage, m_requestedPixelFormat, m_requestedSize,
                                     m_requestedFrameRate, LinearModifier::Optional);
        }
#endif

        if (!zeroCopy) {
            if (offersDmaBuf(memory)) {
                params[paramCount++] = buildFormatParam(dmaBufFormatStorage, m_requestedPixelFormat,
                                                        m_requestedSize, m_requestedFrameRate,
                                                        LinearModifier::Mandatory);
            }
            if (memory != CameraMemory::DmaBuf) {
                params[paramCount++] =
                        buildFormatParam(formatStorage, m_requestedPixelFormat, m_requestedSize,
                                         m_requestedFrameRate, LinearModifier::Off);
            }
            params[paramCount++] = buildBuffersParam(buffersStorage, m_bufferDataTypes);
        }

        const int status = pw_stream_connect(m_stream.get(), PW_DIRECTION_INPUT, PW_ID_ANY, flags,
                                             params.data(), paramCount);
        if (status < 0) {
            qCWarning(lcPipewireCameraStream)
                    << "pw_stream_connect failed:" << make_error_code(-status).message();
            spa_hook_remove(&m_streamListener);
            m_stream = {};
            return false;
        }

        return true;
    });
}

QVideoFrameFormat QPipeWireVideoStream::frameFormat() const
{
    std::lock_guard guard{ m_formatMutex };
    return m_frameFormat;
}

void QPipeWireVideoStream::onStateChanged(pw_stream_state old, pw_stream_state state,
                                          const char *error)
{
    qCDebug(lcPipewireCameraStream) << "stream state" << pw_stream_state_as_string(old) << "->"
                                    << pw_stream_state_as_string(state)
                                    << (error ? QString::fromUtf8(error) : QString());

    if (state == PW_STREAM_STATE_ERROR) {
        emit errorOccurred(error ? QString::fromUtf8(error)
                                  : u"The PipeWire camera stream failed"_s);
    }
}

void QPipeWireVideoStream::onParamChanged(uint32_t id, const spa_pod *param)
{
    if (!param || id != SPA_PARAM_Format)
        return;

    spa_video_info info{};
    if (spa_format_parse(param, &info.media_type, &info.media_subtype) < 0)
        return;

    if (info.media_type != SPA_MEDIA_TYPE_video)
        return;

    QSize size;
    QVideoFrameFormat::PixelFormat pixelFormat = QVideoFrameFormat::Format_Invalid;
    spa_fraction framerate{};
#if QT_PIPEWIRE_HAS_DMABUF_CAMERA
    bool fixationRequired = false;
#endif

    switch (info.media_subtype) {
    case SPA_MEDIA_SUBTYPE_raw: {
        if (spa_format_video_raw_parse(param, &info.info.raw) < 0)
            return;
        size = QSize(int(info.info.raw.size.width), int(info.info.raw.size.height));
        pixelFormat = toQtPixelFormat(info.info.raw.format);
        framerate = info.info.raw.framerate;

#if QT_PIPEWIRE_HAS_DMABUF_CAMERA
        // Without a modifier, the layout is implicit. Stream-allocated buffers are linear in
        // practice, so they are readable.
        m_dmaBufReadable = (info.info.raw.flags & SPA_VIDEO_FLAG_MODIFIER)
                ? QtMultimediaPrivate::DRMModifier(info.info.raw.modifier)
                        == QtMultimediaPrivate::DrmFormatModifierLinear
                : m_dmaBufAllocator.has_value();

        fixationRequired = info.info.raw.flags & SPA_VIDEO_FLAG_MODIFIER_FIXATION_REQUIRED;
        if (fixationRequired) {
            // Answer with our one supported modifier; triggers another
            // param_changed with the final, fixated format.
            qCDebug(lcPipewireCameraStream)
                    << "modifier fixation requested; confirming linear support";
            confirmDmaBufModifier(info.info.raw.format, info.info.raw.size);
        }
#endif
        break;
    }
    case SPA_MEDIA_SUBTYPE_mjpg: {
        if (spa_format_video_mjpg_parse(param, &info.info.mjpg) < 0)
            return;
        size = QSize(int(info.info.mjpg.size.width), int(info.info.mjpg.size.height));
        pixelFormat = QVideoFrameFormat::Format_Jpeg;
        framerate = info.info.mjpg.framerate;
#if QT_PIPEWIRE_HAS_DMABUF_CAMERA
        m_dmaBufReadable = true;
#endif
        break;
    }
    default:
        return;
    }

#if QT_PIPEWIRE_HAS_DMABUF_CAMERA
    if (!m_dmaBufReadable && !fixationRequired) {
        // A DMABUF that is not linear cannot be read here. Where mapped memory is
        // also on offer, renegotiate without DMABUF rather than dropping every frame.
        const bool mappedOnOffer =
                m_bufferDataTypes.test(SPA_DATA_MemPtr) || m_bufferDataTypes.test(SPA_DATA_MemFd);
        if (m_bufferDataTypes.test(SPA_DATA_DmaBuf) && mappedOnOffer) {
            m_bufferDataTypes.reset(SPA_DATA_DmaBuf);
            std::array<uint8_t, 1024> buffersBuffer{};
            const spa_pod *params[] = {
                buildBuffersParam(buffersBuffer, m_bufferDataTypes),
            };
            pw_stream_update_params(m_stream.get(), params, std::size(params));
        }
    }
#endif

#if QT_PIPEWIRE_HAS_DMABUF_CAMERA
    if (m_dmaBufAllocator && !m_bufferRegistry && !fixationRequired
        && pixelFormat != QVideoFrameFormat::Format_Invalid) {
        allocateDmaBufBuffers(size, pixelFormat);
    }
#endif

    qCDebug(lcPipewireCameraStream)
            << "negotiated" << pixelFormat << size << framerate.num << "/" << framerate.denom;

    if (pixelFormat == QVideoFrameFormat::Format_Invalid) {
        qCWarning(lcPipewireCameraStream)
                << "the server picked a pixel format we cannot represent, media subtype"
                << info.media_subtype;
        Q_EMIT errorOccurred(u"The PipeWire camera stream picked an unsupported pixel format"_s);
    }

    QVideoFrameFormat frameFormat(size, pixelFormat);
    if (framerate.denom != 0)
        frameFormat.setStreamFrameRate(qreal(framerate.num) / qreal(framerate.denom));

    std::lock_guard guard{ m_formatMutex };
    m_frameFormat = std::move(frameFormat);
}

#if QT_PIPEWIRE_HAS_DMABUF_CAMERA
void QPipeWireVideoStream::confirmDmaBufModifier(spa_video_format format, spa_rectangle size)
{
    QT_WARNING_PUSH
    QT_WARNING_DISABLE_GCC("-Wmissing-field-initializers")
    QT_WARNING_DISABLE_CLANG("-Wmissing-field-initializers")

    std::array<uint8_t, 1024> buffer{};
    spa_pod_builder builder = SPA_POD_BUILDER_INIT(buffer.data(), buffer.size());

    spa_pod_frame frame{};
    spa_pod_builder_push_object(&builder, &frame, SPA_TYPE_OBJECT_Format, SPA_PARAM_EnumFormat);

    spa_pod_builder_add(&builder, SPA_FORMAT_mediaType, SPA_POD_Id(SPA_MEDIA_TYPE_video),
                        SPA_FORMAT_mediaSubtype, SPA_POD_Id(SPA_MEDIA_SUBTYPE_raw),
                        SPA_FORMAT_VIDEO_format, SPA_POD_Id(format), SPA_FORMAT_VIDEO_size,
                        SPA_POD_Rectangle(&size), 0);

    // Single-value choice, DONT_FIXATE not set: "of the modifiers you
    // offered, this is the one I can import."
    spa_pod_frame modifierChoiceFrame{};
    spa_pod_builder_prop(&builder, SPA_FORMAT_VIDEO_modifier, SPA_POD_PROP_FLAG_MANDATORY);
    spa_pod_builder_push_choice(&builder, &modifierChoiceFrame, SPA_CHOICE_Enum, 0);
    spa_pod_builder_long(&builder, int64_t(QtMultimediaPrivate::DrmFormatModifierLinear));
    spa_pod_builder_pop(&builder, &modifierChoiceFrame);
    const auto *pod = static_cast<const spa_pod *>(spa_pod_builder_pop(&builder, &frame));

    QT_WARNING_POP

    const spa_pod *params[] = { pod };
    pw_stream_update_params(m_stream.get(), params, std::size(params));
}
#endif

#if QT_PIPEWIRE_HAS_DMABUF_CAMERA
void QPipeWireVideoStream::allocateDmaBufBuffers(QSize size,
                                                 QVideoFrameFormat::PixelFormat pixelFormat)
{
    const auto *desc = QVideoTextureHelper::textureDescription(pixelFormat);
    if (!desc) {
        qCWarning(lcPipewireCameraStream) << "no layout for" << pixelFormat;
        Q_EMIT errorOccurred(u"The PipeWire camera stream picked an unsupported pixel format"_s);
        return;
    }

    const int stride = desc->strideForWidth(size.width());
    const qsizetype bufferSize = desc->bytesForSize(size);

    std::vector<std::shared_ptr<QtMultimediaPrivate::QDmaBuf>> buffers;
    for (std::size_t index = 0; index < kDmaBufBufferCount; ++index) {
        auto buf = m_dmaBufAllocator->allocate(bufferSize);
        if (!buf) {
            qCWarning(lcPipewireCameraStream) << "cannot allocate DMA-BUF:" << buf.error();
            break;
        }
        buffers.push_back(std::move(*buf));
    }
    if (buffers.size() != kDmaBufBufferCount) {
        Q_EMIT errorOccurred(u"The PipeWire camera stream could not allocate DMA-BUF buffers"_s);
        return;
    }

    m_bufferRegistry = std::make_shared<DmaBufBufferRegistry>(
            std::move(buffers), stride, m_context.pwInstance(), m_stream.get());

    std::array<uint8_t, 1024> storage{};
    const spa_pod *params[] = {
        buildDmaBufBuffersParam(storage, int32_t(bufferSize), int32_t(stride)),
    };
    pw_stream_update_params(m_stream.get(), params, std::size(params));
}

void QPipeWireVideoStream::onAddBuffer(pw_buffer *buffer)
{
    if (!m_bufferRegistry) {
        qCWarning(lcPipewireCameraStream) << "buffer added before its DMA-BUF was allocated";
        return;
    }

    m_bufferRegistry->bindBuffer(buffer);
    pw_stream_queue_buffer(m_stream.get(), buffer);
}

bool QPipeWireVideoStream::emitDmaBufFrame(pw_buffer *pwBuffer,
                                           const QVideoFrameFormat &frameFormat)
{
    using namespace QtMultimediaPrivate;

    const std::size_t index = DmaBufBufferRegistry::bufferIndex(pwBuffer);
    const std::shared_ptr<QDmaBuf> buf = m_bufferRegistry->buffer(index);
    m_bufferRegistry->markLeased(index);

    const spa_chunk &chunk = *pwBuffer->buffer->datas[0].chunk;
    const uint32_t chunkSize = std::min(chunk.size, uint32_t(buf->size()));

    const std::optional<DmaBufLayout> layout =
            dmaBufLayoutForFormat(frameFormat.pixelFormat(), frameFormat.frameSize(),
                                  m_bufferRegistry->stride(), uint32_t(buf->size()));

    std::unique_ptr<QAbstractVideoBuffer> videoBuffer;
    if (layout) {
        // Zero-copy: one DmaBufPlane per layout plane, all within the same DMA-BUF fd.
        const auto drmFormats = dmaBufFourccFromPixelFormat(frameFormat.pixelFormat());
        QVarLengthArray<DmaBufPlane, 4> planes;
        for (qsizetype plane = 0; plane < layout->size(); ++plane) {
            const DmaBufPlaneLayout &planeLayout = (*layout)[plane];
            planes.push_back({
                    buf->fd(),
                    planeLayout.offset,
                    planeLayout.pitch,
                    drmFormats[plane],
            });
        }
        const QSpan<const DmaBufPlane> planeSpan(planes.data(), planes.size());

        using Buffer =
                MaterializingVideoBuffer<DmaBufSource, DmaBufBufferRegistry, QDmaBufVideoBuffer>;

        videoBuffer = Buffer::wrapOrMaterialize(*m_bufferRegistry, pwBuffer,
                                                DmaBufSource(buf, *layout), frameFormat,
                                                // QDmaBufVideoBuffer args:
                                                frameFormat, planeSpan, buf);
    } else {
        // No zero-copy format (e.g. a compressed one): wrap the DMA-BUF into a plain
        // MaterializingVideoBuffer without zero-copy machinery.
        const int stride = chunk.stride != 0 ? chunk.stride : m_bufferRegistry->stride();
        const DmaBufLayout plainLayout{
            DmaBufPlaneLayout{ 0, uint32_t(stride), qsizetype(chunkSize) },
        };

        using Buffer = MaterializingVideoBuffer<DmaBufSource, DmaBufBufferRegistry>;

        videoBuffer = Buffer::wrapOrMaterialize(*m_bufferRegistry, pwBuffer,
                                                DmaBufSource(buf, plainLayout), frameFormat);
    }

    QVideoFrame frame = QVideoFramePrivate::createFrame(std::move(videoBuffer), frameFormat);
    setStartTime(frame, *pwBuffer->buffer, m_firstFrameTime);

    Q_EMIT newVideoFrame(frame);
    return true;
}
#endif

void QPipeWireVideoStream::onProcess()
{
    pw_buffer *pwBuffer = pw_stream_dequeue_buffer(m_stream.get());
    if (!pwBuffer) {
        // The producer is ahead of us; dropping is better than blocking the
        // graph.
        qCDebug(lcPipewireCameraStream) << "no buffer available";
        return;
    }

    const QVideoFrameFormat frameFormat = this->frameFormat();

    spa_buffer *buffer = pwBuffer->buffer;

#if QT_PIPEWIRE_HAS_DMABUF_CAMERA
    // A frame that wraps the buffer takes over its requeue, through its capture slot.
    if (m_bufferRegistry && frameFormat.isValid() && buffer->n_datas == 1
        && buffer->datas[0].type == SPA_DATA_DmaBuf && emitDmaBufFrame(pwBuffer, frameFormat)) {
        return;
    }
#endif

    if (frameFormat.isValid() && buffer->n_datas > 0) {
        // Stream-owned buffers that cannot be wrapped are copied, as server buffers are.
        if (std::optional<QByteArray> data = copyFrameData(*buffer, m_dmaBufReadable)) {
            qCDebug(lcPipewireCameraStream)
                    << "frame data type" << spa_data_type(buffer->datas[0].type)
                    << frameFormat.pixelFormat();

            const spa_chunk *chunk = buffer->datas[0].chunk;

            // A compressed format has no fixed stride; guessing one from the
            // chunk size would be meaningless.
            int stride = chunk->stride;
            if (stride == 0 && frameFormat.pixelFormat() != QVideoFrameFormat::Format_Jpeg) {
                const int height = frameFormat.frameHeight();
                if (height > 0)
                    stride = int(chunk->size / quint32(height));
            }

            QVideoFrame frame = QVideoFramePrivate::createFrame(
                    std::make_unique<QMemoryVideoBuffer>(std::move(*data), stride), frameFormat);
            setStartTime(frame, *buffer, m_firstFrameTime);

            emit newVideoFrame(frame);
        }
    }

    pw_stream_queue_buffer(m_stream.get(), pwBuffer);
}

} // namespace QtPipeWire

QT_END_NAMESPACE
