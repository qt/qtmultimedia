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

#include <array>
#include <iterator>

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

} // namespace

QPipeWireVideoStream::QPipeWireVideoStream(QCameraContextManager &context, QByteArray targetObject,
                                           QVideoFrameFormat::PixelFormat requestedPixelFormat,
                                           QSize requestedSize, qreal requestedFrameRate)
    : m_context(context),
      m_targetObject(std::move(targetObject)),
      m_requestedPixelFormat(requestedPixelFormat),
      m_requestedSize(requestedSize),
      m_requestedFrameRate(requestedFrameRate)
{
}

QPipeWireVideoStream::~QPipeWireVideoStream()
{
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
        .process = [](void *data) {
            reinterpret_cast<QPipeWireVideoStream *>(data)->onProcess();
        },
    };
    // clang-format on
    QT_WARNING_POP

    return events;
}

namespace {

const spa_pod *buildFormatParam(spa_pod_builder &builder,
                                QVideoFrameFormat::PixelFormat requestedPixelFormat,
                                const QSize &requestedSize, qreal requestedFrameRate)
{
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

    return static_cast<const spa_pod *>(spa_pod_builder_pop(&builder, &frame));
}

const spa_pod *buildBuffersParam(spa_pod_builder &builder, uint32_t dataTypeMask)
{
    return static_cast<const spa_pod *>(spa_pod_builder_add_object(
            &builder,
            // buffer:
            SPA_TYPE_OBJECT_ParamBuffers, SPA_PARAM_Buffers,
            // with
            SPA_PARAM_BUFFERS_dataType, SPA_POD_CHOICE_FLAGS_Int(dataTypeMask)));
}

} // namespace

bool QPipeWireVideoStream::start()
{
    Q_ASSERT(!m_stream);

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

        std::array<uint8_t, 1024> buffer{};
        spa_pod_builder builder = SPA_POD_BUILDER_INIT(buffer.data(), buffer.size());

        constexpr uint32_t dataTypeMask = (1u << SPA_DATA_MemPtr) | (1u << SPA_DATA_MemFd);

        const spa_pod *params[] = {
            buildFormatParam(builder, m_requestedPixelFormat, m_requestedSize,
                             m_requestedFrameRate),
            buildBuffersParam(builder, dataTypeMask),
        };

        const int status = pw_stream_connect(
                m_stream.get(), PW_DIRECTION_INPUT, PW_ID_ANY,
                pw_stream_flags(PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS), params,
                std::size(params));
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

    switch (info.media_subtype) {
    case SPA_MEDIA_SUBTYPE_raw: {
        if (spa_format_video_raw_parse(param, &info.info.raw) < 0)
            return;
        size = QSize(int(info.info.raw.size.width), int(info.info.raw.size.height));
        pixelFormat = toQtPixelFormat(info.info.raw.format);
        framerate = info.info.raw.framerate;
        break;
    }
    case SPA_MEDIA_SUBTYPE_mjpg: {
        if (spa_format_video_mjpg_parse(param, &info.info.mjpg) < 0)
            return;
        size = QSize(int(info.info.mjpg.size.width), int(info.info.mjpg.size.height));
        pixelFormat = QVideoFrameFormat::Format_Jpeg;
        framerate = info.info.mjpg.framerate;
        break;
    }
    default:
        return;
    }

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

    if (frameFormat.isValid() && buffer->n_datas > 0 && buffer->datas[0].data) {
        const spa_chunk *chunk = buffer->datas[0].chunk;

        // A compressed format has no fixed stride; guessing one from the
        // chunk size would be meaningless.
        int stride = chunk->stride;
        if (stride == 0 && frameFormat.pixelFormat() != QVideoFrameFormat::Format_Jpeg) {
            const int height = frameFormat.frameHeight();
            if (height > 0)
                stride = int(chunk->size / quint32(height));
        }

        QByteArray data{ static_cast<const char *>(buffer->datas[0].data) + chunk->offset,
                         qsizetype(chunk->size) };

        QVideoFrame frame = QVideoFramePrivate::createFrame(
                std::make_unique<QMemoryVideoBuffer>(std::move(data), stride), frameFormat);

        if (const auto *header = static_cast<const spa_meta_header *>(
                    spa_buffer_find_meta_data(buffer, SPA_META_Header, sizeof(spa_meta_header)))) {
            // pts is in nanoseconds since an arbitrary epoch.
            frame.setStartTime(qint64(header->pts) / 1000);
        }

        emit newVideoFrame(frame);
    }

    pw_stream_queue_buffer(m_stream.get(), pwBuffer);
}

} // namespace QtPipeWire

QT_END_NAMESPACE
