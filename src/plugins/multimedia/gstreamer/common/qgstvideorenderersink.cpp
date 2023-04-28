// Copyright (C) 2016 Jolla Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qvideoframe.h>
#include <qvideosink.h>
#include <QDebug>
#include <QMap>
#include <QThread>
#include <QEvent>
#include <QCoreApplication>

#include <private/qfactoryloader_p.h>
#include "qgstvideobuffer_p.h"
#include "qgstreamervideosink_p.h"

#include "qgstvideorenderersink_p.h"

#include <gst/video/video.h>
#include <gst/video/gstvideometa.h>
#include <qloggingcategory.h>
#include <qdebug.h>

#include "qgstutils_p.h"

#include <rhi/qrhi.h>
#if QT_CONFIG(gstreamer_gl)
#include <gst/gl/gl.h>
#endif // #if QT_CONFIG(gstreamer_gl)

// DMA support
#if QT_CONFIG(linux_dmabuf)
#include <gst/allocators/gstdmabuf.h>
#endif

//#define DEBUG_VIDEO_SURFACE_SINK

static Q_LOGGING_CATEGORY(qLcGstVideoRenderer, "qt.multimedia.gstvideorenderer")

QT_BEGIN_NAMESPACE

QGstVideoRenderer::QGstVideoRenderer(QGstreamerVideoSink *sink)
    : m_sink(sink)
{
    createSurfaceCaps();
}

QGstVideoRenderer::~QGstVideoRenderer()
{
}

void QGstVideoRenderer::createSurfaceCaps()
{
    QRhi *rhi = m_sink->rhi();
    Q_UNUSED(rhi);

    auto caps = QGstCaps::create();

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
    if (rhi && rhi->backend() == QRhi::OpenGLES2) {
        caps.addPixelFormats(formats, GST_CAPS_FEATURE_MEMORY_GL_MEMORY);
#if QT_CONFIG(linux_dmabuf)
        if (m_sink->eglDisplay() && m_sink->eglImageTargetTexture2D()) {
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
#endif
    }
#endif
    caps.addPixelFormats(formats);

    m_surfaceCaps = caps;
}

QGstCaps QGstVideoRenderer::caps()
{
    QMutexLocker locker(&m_mutex);

    return m_surfaceCaps;
}

bool QGstVideoRenderer::start(const QGstCaps& caps)
{
    qCDebug(qLcGstVideoRenderer) << "QGstVideoRenderer::start" << caps.toString();
    QMutexLocker locker(&m_mutex);

    m_frameMirrored = false;
    m_frameRotationAngle = QVideoFrame::Rotation0;

    if (m_active) {
        m_flush = true;
        m_stop = true;
    }

    m_startCaps = caps;

    /*
    Waiting for start() to be invoked in the main thread may block
    if gstreamer blocks the main thread until this call is finished.
    This situation is rare and usually caused by setState(Null)
    while pipeline is being prerolled.

    The proper solution to this involves controlling gstreamer pipeline from
    other thread than video surface.

    Currently start() fails if wait() timed out.
    */
    if (!waitForAsyncEvent(&locker, &m_setupCondition, 1000) && !m_startCaps.isNull()) {
        qWarning() << "Failed to start video surface due to main thread blocked.";
        m_startCaps = {};
    }

    return m_active;
}

void QGstVideoRenderer::stop()
{
    QMutexLocker locker(&m_mutex);

    if (!m_active)
        return;

    m_flush = true;
    m_stop = true;

    m_startCaps = {};

    waitForAsyncEvent(&locker, &m_setupCondition, 500);
}

void QGstVideoRenderer::unlock()
{
    QMutexLocker locker(&m_mutex);

    m_setupCondition.wakeAll();
    m_renderCondition.wakeAll();
}

bool QGstVideoRenderer::proposeAllocation(GstQuery *query)
{
    Q_UNUSED(query);
    QMutexLocker locker(&m_mutex);
    return m_active;
}

void QGstVideoRenderer::flush()
{
    QMutexLocker locker(&m_mutex);

    m_flush = true;
    m_renderBuffer = nullptr;
    m_renderCondition.wakeAll();

    notify();
}

GstFlowReturn QGstVideoRenderer::render(GstBuffer *buffer)
{
    QMutexLocker locker(&m_mutex);
    qCDebug(qLcGstVideoRenderer) << "QGstVideoRenderer::render";

    m_renderReturn = GST_FLOW_OK;
    m_renderBuffer = buffer;

    waitForAsyncEvent(&locker, &m_renderCondition, 300);

    m_renderBuffer = nullptr;

    return m_renderReturn;
}

bool QGstVideoRenderer::query(GstQuery *query)
{
#if QT_CONFIG(gstreamer_gl)
    if (GST_QUERY_TYPE(query) == GST_QUERY_CONTEXT) {
        const gchar *type;
        gst_query_parse_context_type(query, &type);

        if (strcmp(type, "gst.gl.local_context") != 0)
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
    if (GST_EVENT_TYPE(event) != GST_EVENT_TAG)
        return;

    GstTagList *taglist = nullptr;
    gst_event_parse_tag(event, &taglist);
    if (!taglist)
        return;

    gchar *value = nullptr;
    if (!gst_tag_list_get_string(taglist, GST_TAG_IMAGE_ORIENTATION, &value))
        return;

    constexpr const char rotate[] = "rotate-";
    constexpr const char flipRotate[] = "flip-rotate-";
    constexpr size_t rotateLen = sizeof(rotate) - 1;
    constexpr size_t flipRotateLen = sizeof(flipRotate) - 1;

    bool mirrored = false;
    int rotationAngle = 0;

    if (!strncmp(rotate, value, rotateLen)) {
        rotationAngle = atoi(value + rotateLen);
    } else if (!strncmp(flipRotate, value, flipRotateLen)) {
        // To flip by horizontal axis is the same as to mirror by vertical axis
        // and rotate by 180 degrees.
        mirrored = true;
        rotationAngle = (180 + atoi(value + flipRotateLen)) % 360;
    }

    QMutexLocker locker(&m_mutex);
    m_frameMirrored = mirrored;
    switch (rotationAngle) {
    case 0: m_frameRotationAngle = QVideoFrame::Rotation0; break;
    case 90: m_frameRotationAngle = QVideoFrame::Rotation90; break;
    case 180: m_frameRotationAngle = QVideoFrame::Rotation180; break;
    case 270: m_frameRotationAngle = QVideoFrame::Rotation270; break;
    default: m_frameRotationAngle = QVideoFrame::Rotation0;
    }
}

bool QGstVideoRenderer::event(QEvent *event)
{
    if (event->type() == QEvent::UpdateRequest) {
        QMutexLocker locker(&m_mutex);

        if (m_notified) {
            while (handleEvent(&locker)) {}
            m_notified = false;
        }
        return true;
    }

    return QObject::event(event);
}

bool QGstVideoRenderer::handleEvent(QMutexLocker<QMutex> *locker)
{
    if (m_flush) {
        m_flush = false;
        if (m_active) {
            locker->unlock();

            if (m_sink && !m_flushed)
                m_sink->setVideoFrame(QVideoFrame());
            m_flushed = true;
            locker->relock();
        }
    } else if (m_stop) {
        m_stop = false;

        if (m_active) {
            m_active = false;
            m_flushed = true;
        }
    } else if (!m_startCaps.isNull()) {
        Q_ASSERT(!m_active);

        auto startCaps = m_startCaps;
        m_startCaps = {};

        if (m_sink) {
            locker->unlock();

            m_flushed = true;
            m_format = startCaps.formatForCaps(&m_videoInfo);
            memoryFormat = startCaps.memoryFormat();

            locker->relock();
            m_active = m_format.isValid();
        } else if (m_active) {
            m_active = false;
            m_flushed = true;
        }

    } else if (m_renderBuffer) {
        GstBuffer *buffer = m_renderBuffer;
        m_renderBuffer = nullptr;
        m_renderReturn = GST_FLOW_ERROR;

        qCDebug(qLcGstVideoRenderer) << "QGstVideoRenderer::handleEvent(renderBuffer)" << m_active << m_sink;
        if (m_active && m_sink) {
            gst_buffer_ref(buffer);

            locker->unlock();

            m_flushed = false;

            auto meta = gst_buffer_get_video_crop_meta (buffer);
            if (meta) {
                QRect vp(meta->x, meta->y, meta->width, meta->height);
                if (m_format.viewport() != vp) {
                    qCDebug(qLcGstVideoRenderer) << Q_FUNC_INFO << " Update viewport on Metadata: [" << meta->height << "x" << meta->width << " | " << meta->x << "x" << meta->y << "]";
                    // Update viewport if data is not the same
                    m_format.setViewport(vp);
                }
            }

            if (m_sink->inStoppedState()) {
                qCDebug(qLcGstVideoRenderer) << "    sending empty video frame";
                m_sink->setVideoFrame(QVideoFrame());
            } else {
                QGstVideoBuffer *videoBuffer = new QGstVideoBuffer(buffer, m_videoInfo, m_sink, m_format, memoryFormat);
                QVideoFrame frame(videoBuffer, m_format);
                QGstUtils::setFrameTimeStamps(&frame, buffer);
                frame.setMirrored(m_frameMirrored);
                frame.setRotationAngle(m_frameRotationAngle);

                qCDebug(qLcGstVideoRenderer) << "    sending video frame";
                m_sink->setVideoFrame(frame);
            }

            gst_buffer_unref(buffer);

            locker->relock();

            m_renderReturn = GST_FLOW_OK;
        }

        m_renderCondition.wakeAll();
    } else {
        m_setupCondition.wakeAll();

        return false;
    }
    return true;
}

void QGstVideoRenderer::notify()
{
    if (!m_notified) {
        m_notified = true;
        QCoreApplication::postEvent(this, new QEvent(QEvent::UpdateRequest));
    }
}

bool QGstVideoRenderer::waitForAsyncEvent(
        QMutexLocker<QMutex> *locker, QWaitCondition *condition, unsigned long time)
{
    if (QThread::currentThread() == thread()) {
        while (handleEvent(locker)) {}
        m_notified = false;

        return true;
    }

    notify();

    return condition->wait(&m_mutex, time);
}

static GstVideoSinkClass *gvrs_sink_parent_class;
static thread_local QGstreamerVideoSink *gvrs_current_sink;

#define VO_SINK(s) QGstVideoRendererSink *sink(reinterpret_cast<QGstVideoRendererSink *>(s))

QGstVideoRendererSink *QGstVideoRendererSink::createSink(QGstreamerVideoSink *sink)
{
    setSink(sink);
    QGstVideoRendererSink *gstSink = reinterpret_cast<QGstVideoRendererSink *>(
            g_object_new(QGstVideoRendererSink::get_type(), nullptr));

    g_signal_connect(G_OBJECT(gstSink), "notify::show-preroll-frame", G_CALLBACK(handleShowPrerollChange), gstSink);

    return gstSink;
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

    static const GType type = []() {
        const auto result = g_type_register_static(
                GST_TYPE_VIDEO_SINK, "QGstVideoRendererSink", &info, GTypeFlags(0));

        // Register the sink type to be used in custom piplines.
        // When surface is ready the sink can be used.
        gst_element_register(nullptr, "qtvideosink", GST_RANK_PRIMARY, result);

        return result;
    }();

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

void QGstVideoRendererSink::handleShowPrerollChange(GObject *o, GParamSpec *p, gpointer d)
{
    Q_UNUSED(o);
    Q_UNUSED(p);
    QGstVideoRendererSink *sink = reinterpret_cast<QGstVideoRendererSink *>(d);

    gboolean showPrerollFrame = true; // "show-preroll-frame" property is true by default
    g_object_get(G_OBJECT(sink), "show-preroll-frame", &showPrerollFrame, nullptr);

    if (!showPrerollFrame) {
        GstState state = GST_STATE_VOID_PENDING;
        GstClockTime timeout = 10000000; // 10 ms
        gst_element_get_state(GST_ELEMENT(sink), &state, nullptr, timeout);
        // show-preroll-frame being set to 'false' while in GST_STATE_PAUSED means
        // the QMediaPlayer was stopped from the paused state.
        // We need to flush the current frame.
        if (state == GST_STATE_PAUSED)
            sink->renderer->flush();
    }
}

GstStateChangeReturn QGstVideoRendererSink::change_state(
        GstElement *element, GstStateChange transition)
{
    QGstVideoRendererSink *sink = reinterpret_cast<QGstVideoRendererSink *>(element);

    gboolean showPrerollFrame = true; // "show-preroll-frame" property is true by default
    g_object_get(G_OBJECT(element), "show-preroll-frame", &showPrerollFrame, nullptr);

    // If show-preroll-frame is 'false' when transitioning from GST_STATE_PLAYING to
    // GST_STATE_PAUSED, it means the QMediaPlayer was stopped.
    // We need to flush the current frame.
    if (transition == GST_STATE_CHANGE_PLAYING_TO_PAUSED && !showPrerollFrame)
        sink->renderer->flush();

    return GST_ELEMENT_CLASS(gvrs_sink_parent_class)->change_state(element, transition);
}

GstCaps *QGstVideoRendererSink::get_caps(GstBaseSink *base, GstCaps *filter)
{
    VO_SINK(base);

    QGstCaps caps = sink->renderer->caps();
    if (filter)
        caps = QGstCaps(gst_caps_intersect(caps.get(), filter), QGstCaps::HasRef);

    gst_caps_ref(caps.get());
    return caps.get();
}

gboolean QGstVideoRendererSink::set_caps(GstBaseSink *base, GstCaps *gcaps)
{
    VO_SINK(base);

    auto caps = QGstCaps(gcaps, QGstCaps::NeedsRef);

    qCDebug(qLcGstVideoRenderer) << "set_caps:" << caps.toString();

    if (caps.isNull()) {
        sink->renderer->stop();

        return TRUE;
    } else if (sink->renderer->start(caps)) {
        return TRUE;
    } else {
        return FALSE;
    }
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

QT_END_NAMESPACE

#include "moc_qgstvideorenderersink_p.cpp"
