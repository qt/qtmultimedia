// Copyright (C) 2016 Jolla Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qgstvideorenderersink_p.h"

#include <QtMultimedia/qvideoframe.h>
#include <QtMultimedia/qvideosink.h>
#include <QtCore/private/qfactoryloader_p.h>
#include <QtCore/private/quniquehandle_p.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qdebug.h>
#include <QtCore/qdebug.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qmap.h>
#include <QtCore/qthread.h>
#include <QtGui/qevent.h>

#include <common/qgst_debug_p.h>
#include <common/qgstreamermetadata_p.h>
#include <common/qgstreamervideosink_p.h>
#include <common/qgstutils_p.h>
#include <common/qgstvideobuffer_p.h>

#include <private/qvideoframe_p.h>

#include <gst/video/video.h>
#include <gst/video/gstvideometa.h>


#include <rhi/qrhi.h>
#if QT_CONFIG(gstreamer_gl)
#include <gst/gl/gl.h>
#endif // #if QT_CONFIG(gstreamer_gl)

// DMA support
#if QT_CONFIG(gstreamer_gl_egl) && QT_CONFIG(linux_dmabuf)
#  include <gst/allocators/gstdmabuf.h>
#endif

Q_STATIC_LOGGING_CATEGORY(qLcGstVideoRenderer, "qt.multimedia.gstvideorenderer");

QT_BEGIN_NAMESPACE

QGstVideoRenderer::QGstVideoRenderer(QGstreamerVideoSink *sink)
    : m_sink(sink), m_surfaceCaps(createSurfaceCaps(sink))
{
    QObject::connect(
            sink, &QGstreamerVideoSink::aboutToBeDestroyed, this,
            [this] {
                QMutexLocker locker(&m_sinkMutex);
                m_sink = nullptr;
            },
            Qt::DirectConnection);
}

QGstVideoRenderer::~QGstVideoRenderer() = default;

QGstCaps QGstVideoRenderer::createSurfaceCaps([[maybe_unused]] QGstreamerVideoSink *sink)
{
    QGstCaps caps = QGstCaps::create();

    // All the formats that both we and gstreamer support
    auto formats = QList<QVideoFrameFormat::PixelFormat>()
                   << QVideoFrameFormat::Format_YUV420P
                   << QVideoFrameFormat::Format_YUV422P
                   << QVideoFrameFormat::Format_YV12
                   << QVideoFrameFormat::Format_UYVY
                   << QVideoFrameFormat::Format_YUYV
                   << QVideoFrameFormat::Format_NV12
                   << QVideoFrameFormat::Format_NV21
                   << QVideoFrameFormat::Format_AYUV
                   << QVideoFrameFormat::Format_P010
                   << QVideoFrameFormat::Format_XRGB8888
                   << QVideoFrameFormat::Format_XBGR8888
                   << QVideoFrameFormat::Format_RGBX8888
                   << QVideoFrameFormat::Format_BGRX8888
                   << QVideoFrameFormat::Format_ARGB8888
                   << QVideoFrameFormat::Format_ABGR8888
                   << QVideoFrameFormat::Format_RGBA8888
                   << QVideoFrameFormat::Format_BGRA8888
                   << QVideoFrameFormat::Format_Y8
                   << QVideoFrameFormat::Format_Y16
        ;
#if QT_CONFIG(gstreamer_gl)
    QRhi *rhi = sink->rhi();
    if (rhi && rhi->backend() == QRhi::OpenGLES2) {
        caps.addPixelFormats(formats, GST_CAPS_FEATURE_MEMORY_GL_MEMORY);
#  if QT_CONFIG(gstreamer_gl_egl) && QT_CONFIG(linux_dmabuf)
        if (sink->eglDisplay() && sink->eglImageTargetTexture2D()) {
            // We currently do not handle planar DMA buffers, as it's somewhat unclear how to
            // convert the planar EGLImage into something we can use from OpenGL
            auto singlePlaneFormats = QList<QVideoFrameFormat::PixelFormat>()
                           << QVideoFrameFormat::Format_UYVY
                           << QVideoFrameFormat::Format_YUYV
                           << QVideoFrameFormat::Format_AYUV
                           << QVideoFrameFormat::Format_XRGB8888
                           << QVideoFrameFormat::Format_XBGR8888
                           << QVideoFrameFormat::Format_RGBX8888
                           << QVideoFrameFormat::Format_BGRX8888
                           << QVideoFrameFormat::Format_ARGB8888
                           << QVideoFrameFormat::Format_ABGR8888
                           << QVideoFrameFormat::Format_RGBA8888
                           << QVideoFrameFormat::Format_BGRA8888
                           << QVideoFrameFormat::Format_Y8
                           << QVideoFrameFormat::Format_Y16
                ;
            caps.addPixelFormats(singlePlaneFormats, GST_CAPS_FEATURE_MEMORY_DMABUF);
        }
#  endif
    }
#endif
    caps.addPixelFormats(formats);
    return caps;
}

const QGstCaps &QGstVideoRenderer::caps()
{
    return m_surfaceCaps;
}

bool QGstVideoRenderer::start(const QGstCaps& caps)
{
    qCDebug(qLcGstVideoRenderer) << "QGstVideoRenderer::start" << caps;

    {
        m_frameRotationAngle = QtVideo::Rotation::None;
        auto optionalFormatAndVideoInfo = caps.formatAndVideoInfo();
        if (optionalFormatAndVideoInfo) {
            std::tie(m_format, m_videoInfo) = std::move(*optionalFormatAndVideoInfo);
        } else {
            m_format = {};
            m_videoInfo = {};
        }
        m_memoryFormat = caps.memoryFormat();
    }

    return true;
}

void QGstVideoRenderer::stop()
{
    qCDebug(qLcGstVideoRenderer) << "QGstVideoRenderer::stop";

    QMetaObject::invokeMethod(this, [this] {
        m_currentState.buffer = {};
        m_currentPipelineFrame = {};
        updateCurrentVideoFrame(m_currentVideoFrame);
    });
}

void QGstVideoRenderer::unlock()
{
    qCDebug(qLcGstVideoRenderer) << "QGstVideoRenderer::unlock";
}

bool QGstVideoRenderer::proposeAllocation(GstQuery *)
{
    qCDebug(qLcGstVideoRenderer) << "QGstVideoRenderer::proposeAllocation";
    return true;
}

GstFlowReturn QGstVideoRenderer::render(GstBuffer *buffer)
{
    qCDebug(qLcGstVideoRenderer) << "QGstVideoRenderer::render";

    GstVideoCropMeta *meta = gst_buffer_get_video_crop_meta(buffer);
    if (meta) {
        QRect vp(meta->x, meta->y, meta->width, meta->height);
        if (m_format.viewport() != vp) {
            qCDebug(qLcGstVideoRenderer)
                    << Q_FUNC_INFO << " Update viewport on Metadata: [" << meta->height << "x"
                    << meta->width << " | " << meta->x << "x" << meta->y << "]";
            // Update viewport if data is not the same
            m_format.setViewport(vp);
        }
    }

    RenderBufferState state{
        .buffer = QGstBufferHandle{ buffer, QGstBufferHandle::NeedsRef },
        .format = m_format,
        .memoryFormat = m_memoryFormat,
        .mirrored = m_frameMirrored,
        .rotationAngle = m_frameRotationAngle,
    };

    qCDebug(qLcGstVideoRenderer) << "    sending video frame";

    QMetaObject::invokeMethod(this, [this, state = std::move(state)]() mutable {
        if (state == m_currentState)
            // same buffer received twice
            return;

        auto videoBuffer = std::make_unique<QGstVideoBuffer>(state.buffer, m_videoInfo, m_sink,
                                                             state.format, state.memoryFormat);
        QVideoFrame frame = QVideoFramePrivate::createFrame(std::move(videoBuffer), state.format);
        QGstUtils::setFrameTimeStampsFromBuffer(&frame, state.buffer.get());
        frame.setMirrored(state.mirrored);
        frame.setRotation(state.rotationAngle);

        m_currentPipelineFrame = std::move(frame);
        m_currentState = std::move(state);

        if (!m_isActive) {
            qCDebug(qLcGstVideoRenderer) << "    showing empty video frame";
            updateCurrentVideoFrame({});
            return;
        }

        updateCurrentVideoFrame(m_currentPipelineFrame);
    });

    return GST_FLOW_OK;
}

bool QGstVideoRenderer::query(GstQuery *query)
{
#if QT_CONFIG(gstreamer_gl)
    if (GST_QUERY_TYPE(query) == GST_QUERY_CONTEXT) {
        const gchar *type;
        gst_query_parse_context_type(query, &type);

        if (strcmp(type, "gst.gl.local_context") != 0)
            return false;

        QMutexLocker locker(&m_sinkMutex);
        if (!m_sink)
            return false;

        auto *gstGlContext = m_sink->gstGlLocalContext();
        if (!gstGlContext)
            return false;

        gst_query_set_context(query, gstGlContext);

        return true;
    }
#else
    Q_UNUSED(query);
#endif
    return false;
}

void QGstVideoRenderer::gstEvent(GstEvent *event)
{
    switch (GST_EVENT_TYPE(event)) {
    case GST_EVENT_TAG:
        qCDebug(qLcGstVideoRenderer) << "QGstVideoRenderer::gstEvent: Tag";
        return gstEventHandleTag(event);
    case GST_EVENT_EOS:
        qCDebug(qLcGstVideoRenderer) << "QGstVideoRenderer::gstEvent: EOS";
        return gstEventHandleEOS(event);

    default:
        qCDebug(qLcGstVideoRenderer) << "QGstVideoRenderer::gstEvent: unhandled event - " << event;
        return;
    }
}

void QGstVideoRenderer::setActive(bool isActive)
{
    if (isActive == m_isActive)
        return;

    m_isActive = isActive;
    if (isActive)
        updateCurrentVideoFrame(m_currentPipelineFrame);
    else
        updateCurrentVideoFrame({});
}

void QGstVideoRenderer::updateCurrentVideoFrame(QVideoFrame frame)
{
    m_currentVideoFrame = std::move(frame);
    if (m_sink)
        m_sink->setVideoFrame(m_currentVideoFrame);
}

void QGstVideoRenderer::gstEventHandleTag(GstEvent *event)
{
    GstTagList *taglist = nullptr;
    gst_event_parse_tag(event, &taglist);
    if (!taglist)
        return;

    QGString value;
    if (!gst_tag_list_get_string(taglist, GST_TAG_IMAGE_ORIENTATION, &value))
        return;

    RotationResult parsed = parseRotationTag(value.get());

    m_frameRotationAngle = parsed.rotation;
    m_frameMirrored = parsed.flip;
}

void QGstVideoRenderer::gstEventHandleEOS(GstEvent *)
{
    stop();
}

static GstVideoSinkClass *gvrs_sink_parent_class;
static thread_local QGstreamerVideoSink *gvrs_current_sink;

#define VO_SINK(s) QGstVideoRendererSink *sink(reinterpret_cast<QGstVideoRendererSink *>(s))

QGstVideoRendererSinkElement QGstVideoRendererSink::createSink(QGstreamerVideoSink *sink)
{
    setSink(sink);
    QGstVideoRendererSink *gstSink = reinterpret_cast<QGstVideoRendererSink *>(
            g_object_new(QGstVideoRendererSink::get_type(), nullptr));

    return QGstVideoRendererSinkElement{
        gstSink,
        QGstElement::NeedsRef,
    };
}

void QGstVideoRendererSink::setSink(QGstreamerVideoSink *sink)
{
    gvrs_current_sink = sink;
}

GType QGstVideoRendererSink::get_type()
{
    static const GTypeInfo info =
    {
        sizeof(QGstVideoRendererSinkClass),                // class_size
        base_init,                                         // base_init
        nullptr,                                           // base_finalize
        class_init,                                        // class_init
        nullptr,                                           // class_finalize
        nullptr,                                           // class_data
        sizeof(QGstVideoRendererSink),                     // instance_size
        0,                                                 // n_preallocs
        instance_init,                                     // instance_init
        nullptr                                            // value_table
    };

    static const GType type = g_type_register_static(GST_TYPE_VIDEO_SINK, "QGstVideoRendererSink",
                                                     &info, GTypeFlags(0));

    return type;
}

void QGstVideoRendererSink::class_init(gpointer g_class, gpointer class_data)
{
    Q_UNUSED(class_data);

    gvrs_sink_parent_class = reinterpret_cast<GstVideoSinkClass *>(g_type_class_peek_parent(g_class));

    GstVideoSinkClass *video_sink_class = reinterpret_cast<GstVideoSinkClass *>(g_class);
    video_sink_class->show_frame = QGstVideoRendererSink::show_frame;

    GstBaseSinkClass *base_sink_class = reinterpret_cast<GstBaseSinkClass *>(g_class);
    base_sink_class->get_caps = QGstVideoRendererSink::get_caps;
    base_sink_class->set_caps = QGstVideoRendererSink::set_caps;
    base_sink_class->propose_allocation = QGstVideoRendererSink::propose_allocation;
    base_sink_class->stop = QGstVideoRendererSink::stop;
    base_sink_class->unlock = QGstVideoRendererSink::unlock;
    base_sink_class->query = QGstVideoRendererSink::query;
    base_sink_class->event = QGstVideoRendererSink::event;

    GstElementClass *element_class = reinterpret_cast<GstElementClass *>(g_class);
    element_class->change_state = QGstVideoRendererSink::change_state;
    gst_element_class_set_metadata(element_class,
        "Qt built-in video renderer sink",
        "Sink/Video",
        "Qt default built-in video renderer sink",
        "The Qt Company");

    GObjectClass *object_class = reinterpret_cast<GObjectClass *>(g_class);
    object_class->finalize = QGstVideoRendererSink::finalize;
}

void QGstVideoRendererSink::base_init(gpointer g_class)
{
    static GstStaticPadTemplate sink_pad_template = GST_STATIC_PAD_TEMPLATE(
            "sink", GST_PAD_SINK, GST_PAD_ALWAYS, GST_STATIC_CAPS(
                    "video/x-raw, "
                    "framerate = (fraction) [ 0, MAX ], "
                    "width = (int) [ 1, MAX ], "
                    "height = (int) [ 1, MAX ]"));

    gst_element_class_add_pad_template(
            GST_ELEMENT_CLASS(g_class), gst_static_pad_template_get(&sink_pad_template));
}

void QGstVideoRendererSink::instance_init(GTypeInstance *instance, gpointer g_class)
{
    Q_UNUSED(g_class);
    VO_SINK(instance);

    Q_ASSERT(gvrs_current_sink);

    sink->renderer = new QGstVideoRenderer(gvrs_current_sink);
    sink->renderer->moveToThread(gvrs_current_sink->thread());
    gvrs_current_sink = nullptr;
}

void QGstVideoRendererSink::finalize(GObject *object)
{
    VO_SINK(object);

    delete sink->renderer;

    // Chain up
    G_OBJECT_CLASS(gvrs_sink_parent_class)->finalize(object);
}

GstStateChangeReturn QGstVideoRendererSink::change_state(
        GstElement *element, GstStateChange transition)
{
    return GST_ELEMENT_CLASS(gvrs_sink_parent_class)->change_state(element, transition);
}

GstCaps *QGstVideoRendererSink::get_caps(GstBaseSink *base, GstCaps *filter)
{
    VO_SINK(base);

    QGstCaps caps = sink->renderer->caps();
    if (filter)
        caps = QGstCaps(gst_caps_intersect(caps.caps(), filter), QGstCaps::HasRef);

    return caps.release();
}

gboolean QGstVideoRendererSink::set_caps(GstBaseSink *base, GstCaps *gcaps)
{
    VO_SINK(base);
    auto caps = QGstCaps(gcaps, QGstCaps::NeedsRef);

    qCDebug(qLcGstVideoRenderer) << "set_caps:" << caps;

    if (caps.isNull()) {
        sink->renderer->stop();
        return TRUE;
    }

    return sink->renderer->start(caps);
}

gboolean QGstVideoRendererSink::propose_allocation(GstBaseSink *base, GstQuery *query)
{
    VO_SINK(base);
    return sink->renderer->proposeAllocation(query);
}

gboolean QGstVideoRendererSink::stop(GstBaseSink *base)
{
    VO_SINK(base);
    sink->renderer->stop();
    return TRUE;
}

gboolean QGstVideoRendererSink::unlock(GstBaseSink *base)
{
    VO_SINK(base);
    sink->renderer->unlock();
    return TRUE;
}

GstFlowReturn QGstVideoRendererSink::show_frame(GstVideoSink *base, GstBuffer *buffer)
{
    VO_SINK(base);
    return sink->renderer->render(buffer);
}

gboolean QGstVideoRendererSink::query(GstBaseSink *base, GstQuery *query)
{
    VO_SINK(base);
    if (sink->renderer->query(query))
        return TRUE;

    return GST_BASE_SINK_CLASS(gvrs_sink_parent_class)->query(base, query);
}

gboolean QGstVideoRendererSink::event(GstBaseSink *base, GstEvent * event)
{
    VO_SINK(base);
    sink->renderer->gstEvent(event);
    return GST_BASE_SINK_CLASS(gvrs_sink_parent_class)->event(base, event);
}

QGstVideoRendererSinkElement::QGstVideoRendererSinkElement(QGstVideoRendererSink *element,
                                                           RefMode mode)
    : QGstBaseSink{
          qGstCheckedCast<GstBaseSink>(element),
          mode,
      }
{
}

void QGstVideoRendererSinkElement::setActive(bool isActive)
{
    qGstVideoRendererSink()->renderer->setActive(isActive);
}

QGstVideoRendererSink *QGstVideoRendererSinkElement::qGstVideoRendererSink() const
{
    return reinterpret_cast<QGstVideoRendererSink *>(element());
}

QT_END_NAMESPACE
