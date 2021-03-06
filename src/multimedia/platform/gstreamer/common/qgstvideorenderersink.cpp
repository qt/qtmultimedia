/****************************************************************************
**
** Copyright (C) 2016 Jolla Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qvideoframe.h>
#include <qvideosink.h>
#include <QDebug>
#include <QMap>
#include <QThread>
#include <QEvent>
#include <QCoreApplication>

#include <private/qfactoryloader_p.h>
#include "qgstvideobuffer_p.h"

#include "qgstvideorenderersink_p.h"

#include <gst/video/video.h>
#include <gst/video/gstvideometa.h>

#include "qgstutils_p.h"

#if QT_CONFIG(gstreamer_gl)
#include <QtGui/private/qrhi_p.h>
#include <QtGui/private/qrhigles2_p.h>
#include <QGuiApplication>
#include <QtGui/qopenglcontext.h>
#include <QWindow>
#include <qpa/qplatformnativeinterface.h>

#include <gst/gl/gstglconfig.h>

#if GST_GL_HAVE_WINDOW_X11
#    include <gst/gl/x11/gstgldisplay_x11.h>
#endif
#if GST_GL_HAVE_PLATFORM_EGL
#    include <gst/gl/egl/gstgldisplay_egl.h>
#endif
#if GST_GL_HAVE_WINDOW_WAYLAND
#    include <gst/gl/wayland/gstgldisplay_wayland.h>
#endif
#endif // #if QT_CONFIG(gstreamer_gl)

//#define DEBUG_VIDEO_SURFACE_SINK

QT_BEGIN_NAMESPACE

QGstVideoRenderer::QGstVideoRenderer(QVideoSink *sink)
    : m_sink(sink)
{
}

QGstVideoRenderer::~QGstVideoRenderer() = default;

QGstMutableCaps QGstVideoRenderer::getCaps()
{
    QGstMutableCaps caps;
    caps.create();

    // All the formats that both we and gstreamer support
#if QT_CONFIG(gstreamer_gl)
    QRhi *rhi = m_sink->rhi();
    if (rhi && rhi->backend() == QRhi::OpenGLES2) {
        auto formats = QList<QVideoFrameFormat::PixelFormat>()
//                       << QVideoFrameFormat::Format_YUV420P
//                       << QVideoFrameFormat::Format_YUV422P
//                       << QVideoFrameFormat::Format_YV12
                       << QVideoFrameFormat::Format_UYVY
                       << QVideoFrameFormat::Format_YUYV
//                       << QVideoFrameFormat::Format_NV12
//                       << QVideoFrameFormat::Format_NV21
                       << QVideoFrameFormat::Format_AYUV444
//                       << QVideoFrameFormat::Format_P010
                       << QVideoFrameFormat::Format_RGB32
                       << QVideoFrameFormat::Format_BGR32
                       << QVideoFrameFormat::Format_ARGB32
                       << QVideoFrameFormat::Format_ABGR32
                       << QVideoFrameFormat::Format_BGRA32
//                       << QVideoFrameFormat::Format_Y8
//                       << QVideoFrameFormat::Format_Y16
            ;
        // Even if the surface does not support gl textures,
        // glupload will be added to the pipeline and GLMemory will be requested.
        // This will lead to upload data to gl textures
        // and download it when the buffer will be used within rendering.
        caps.addPixelFormats(formats, GST_CAPS_FEATURE_MEMORY_GL_MEMORY);
        // ### needs more work, somehow casting the GstBuffer to texture upload meta
        // fails. In addition, the texture upload seems to be broken for VAAPI decoders
//        caps.addPixelFormats(formats, GST_CAPS_FEATURE_META_GST_VIDEO_GL_TEXTURE_UPLOAD_META);
    }
#endif
    auto formats = QList<QVideoFrameFormat::PixelFormat>()
                   << QVideoFrameFormat::Format_YUV420P
                   << QVideoFrameFormat::Format_YUV422P
                   << QVideoFrameFormat::Format_YV12
                   << QVideoFrameFormat::Format_UYVY
                   << QVideoFrameFormat::Format_YUYV
                   << QVideoFrameFormat::Format_NV12
                   << QVideoFrameFormat::Format_NV21
                   << QVideoFrameFormat::Format_AYUV444
#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
                   << QVideoFrameFormat::Format_P010
#endif
                   << QVideoFrameFormat::Format_RGB32
                   << QVideoFrameFormat::Format_BGR32
                   << QVideoFrameFormat::Format_ARGB32
                   << QVideoFrameFormat::Format_ABGR32
                   << QVideoFrameFormat::Format_BGRA32
                   << QVideoFrameFormat::Format_Y8
                   << QVideoFrameFormat::Format_Y16
        ;
    caps.addPixelFormats(formats);
    return caps;
}

bool QGstVideoRenderer::start(GstCaps *caps)
{
    m_flushed = true;
    m_format = QGstUtils::formatForCaps(caps, &m_videoInfo);
//    qDebug() << "START:" << QGstCaps(caps).toString();
//    qDebug() << m_format;

    auto *features = gst_caps_get_features(caps, 0);
#if QT_CONFIG(gstreamer_gl)
    if (gst_caps_features_contains(features, GST_CAPS_FEATURE_MEMORY_GL_MEMORY))
        bufferFormat = QGstVideoBuffer::GLTexture;
    else
#endif
    if (gst_caps_features_contains(features, GST_CAPS_FEATURE_META_GST_VIDEO_GL_TEXTURE_UPLOAD_META))
        bufferFormat = QGstVideoBuffer::VideoGLTextureUploadMeta;
    else if (gst_caps_features_contains(features, "memory:DMABuf"))
        bufferFormat = QGstVideoBuffer::DMABuf;

    return m_format.isValid();
}

void QGstVideoRenderer::stop()
{
    m_flushed = true;
}

bool QGstVideoRenderer::present(QVideoSink *sink, GstBuffer *buffer)
{
    m_flushed = false;

    QGstVideoBuffer *videoBuffer = new QGstVideoBuffer(buffer, m_videoInfo, m_sink->rhi(), bufferFormat);

    auto meta = gst_buffer_get_video_crop_meta (buffer);
    if (meta) {
        QRect vp(meta->x, meta->y, meta->width, meta->height);
        if (m_format.viewport() != vp) {
#ifdef DEBUG_VIDEO_SURFACE_SINK
            qDebug() << Q_FUNC_INFO << " Update viewport on Metadata: [" << meta->height << "x" << meta->width << " | " << meta->x << "x" << meta->y << "]";
#endif
            //Update viewport if data is not the same
            m_format.setViewport(vp);
        }
    }

    QVideoFrame frame(videoBuffer, m_format);
    QGstUtils::setFrameTimeStamps(&frame, buffer);

    sink->newVideoFrame(frame);
    return true;
}

void QGstVideoRenderer::flush(QVideoSink *surface)
{
    if (surface && !m_flushed)
        surface->newVideoFrame(QVideoFrame());
    m_flushed = true;
}

bool QGstVideoRenderer::proposeAllocation(GstQuery *)
{
    return true;
}

QVideoSurfaceGstDelegate::QVideoSurfaceGstDelegate(QVideoSink *sink)
    : m_sink(sink)
{
    m_renderer = new QGstVideoRenderer(sink);
    updateSupportedFormats();
}

QVideoSurfaceGstDelegate::~QVideoSurfaceGstDelegate()
{
    delete m_renderer;

#if QT_CONFIG(gstreamer_gl)
    if (m_gstGLDisplayContext)
        gst_object_unref(m_gstGLDisplayContext);
#endif
}

QGstMutableCaps QVideoSurfaceGstDelegate::caps()
{
    QMutexLocker locker(&m_mutex);

    return m_surfaceCaps;
}

bool QVideoSurfaceGstDelegate::start(GstCaps *caps)
{
//    qDebug() << "QVideoSurfaceGstDelegate::start" << QGstCaps(caps).toString();
    QMutexLocker locker(&m_mutex);

    if (m_activeRenderer) {
        m_flush = true;
        m_stop = true;
    }

    m_startCaps = QGstMutableCaps(caps, QGstMutableCaps::NeedsRef);

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

    return m_activeRenderer != nullptr;
}

void QVideoSurfaceGstDelegate::stop()
{
    QMutexLocker locker(&m_mutex);

    if (!m_activeRenderer)
        return;

    m_flush = true;
    m_stop = true;

    m_startCaps = {};

    waitForAsyncEvent(&locker, &m_setupCondition, 500);
}

void QVideoSurfaceGstDelegate::unlock()
{
    QMutexLocker locker(&m_mutex);

    m_setupCondition.wakeAll();
    m_renderCondition.wakeAll();
}

bool QVideoSurfaceGstDelegate::proposeAllocation(GstQuery *query)
{
    QMutexLocker locker(&m_mutex);

    if (QGstVideoRenderer *pool = m_activeRenderer) {
        locker.unlock();

        return pool->proposeAllocation(query);
    }

    return false;
}

void QVideoSurfaceGstDelegate::flush()
{
    QMutexLocker locker(&m_mutex);

    m_flush = true;
    m_renderBuffer = nullptr;
    m_renderCondition.wakeAll();

    notify();
}

GstFlowReturn QVideoSurfaceGstDelegate::render(GstBuffer *buffer)
{
    QMutexLocker locker(&m_mutex);

    m_renderReturn = GST_FLOW_OK;
    m_renderBuffer = buffer;

    waitForAsyncEvent(&locker, &m_renderCondition, 300);

    m_renderBuffer = nullptr;

    return m_renderReturn;
}

#if QT_CONFIG(gstreamer_gl)
static GstGLContext *gstGLDisplayContext(QVideoSink *sink)
{
    QRhi *rhi = sink->rhi();
    if (!rhi || rhi->backend() != QRhi::OpenGLES2)
        return nullptr;

    auto *nativeHandles = static_cast<const QRhiGles2NativeHandles *>(rhi->nativeHandles());
    auto glContext = nativeHandles->context;
    if (!glContext)
        return nullptr;

    GstGLDisplay *display = nullptr;
    const QString platform = QGuiApplication::platformName();
    const char *contextName = "eglcontext";
    GstGLPlatform glPlatform = GST_GL_PLATFORM_EGL;
    QPlatformNativeInterface *pni = QGuiApplication::platformNativeInterface();

#if GST_GL_HAVE_WINDOW_X11
    if (platform == QLatin1String("xcb")) {
        if (QOpenGLContext::openGLModuleType() == QOpenGLContext::LibGL) {
            contextName = "glxcontext";
            glPlatform = GST_GL_PLATFORM_GLX;
        }

        display = (GstGLDisplay *)gst_gl_display_x11_new_with_display(
            (Display *)pni->nativeResourceForIntegration("display"));
    }
#endif

#if GST_GL_HAVE_PLATFORM_EGL
    if (!display && platform == QLatin1String("eglfs")) {
        display = (GstGLDisplay *)gst_gl_display_egl_new_with_egl_display(
            pni->nativeResourceForIntegration("egldisplay"));
    }
#endif

#if GST_GL_HAVE_WINDOW_WAYLAND
    if (!display && platform.startsWith(QLatin1String("wayland"))) {
        const char *displayName = (platform == QLatin1String("wayland"))
            ? "display" : "egldisplay";

        display = (GstGLDisplay *)gst_gl_display_wayland_new_with_display(
            (struct wl_display *)pni->nativeResourceForIntegration(displayName));
    }
#endif

    if (!display) {
        qWarning() << "Could not create GstGLDisplay";
        return nullptr;
    }

    void *nativeContext = pni->nativeResourceForContext(contextName, glContext);
    if (!nativeContext)
        qWarning() << "Could not find resource for" << contextName;

    GstGLContext *appContext = gst_gl_context_new_wrapped(display, (guintptr)nativeContext, glPlatform, GST_GL_API_ANY);
    if (!appContext)
        qWarning() << "Could not create wrappped context for platform:" << glPlatform;

    GstGLContext *displayContext = nullptr;
    GError *error = nullptr;
    gst_gl_display_create_context(display, appContext, &displayContext, &error);
    if (error) {
        qWarning() << "Could not create display context:" << error->message;
        g_clear_error(&error);
    }

    if (appContext)
        gst_object_unref(appContext);

    gst_object_unref(display);

    return displayContext;
}
#endif // #if QT_CONFIG(gstreamer_gl)

bool QVideoSurfaceGstDelegate::query(GstQuery *query)
{
#if QT_CONFIG(gstreamer_gl)
    if (GST_QUERY_TYPE(query) == GST_QUERY_CONTEXT) {
        const gchar *type;
        gst_query_parse_context_type(query, &type);

        if (strcmp(type, "gst.gl.local_context") != 0)
            return false;

        if (!m_gstGLDisplayContext)
            m_gstGLDisplayContext = gstGLDisplayContext(m_sink);

        // No context yet.
        if (!m_gstGLDisplayContext)
            return false;

        GstContext *context = nullptr;
        gst_query_parse_context(query, &context);
        context = context ? gst_context_copy(context) : gst_context_new(type, FALSE);
        GstStructure *structure = gst_context_writable_structure(context);
        gst_structure_set(structure, "context", GST_TYPE_GL_CONTEXT, m_gstGLDisplayContext, nullptr);
        gst_query_set_context(query, context);
        gst_context_unref(context);

        return true;
    }
#else
    Q_UNUSED(query);
#endif
    return false;
}

bool QVideoSurfaceGstDelegate::event(QEvent *event)
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

bool QVideoSurfaceGstDelegate::handleEvent(QMutexLocker<QMutex> *locker)
{
    if (m_flush) {
        m_flush = false;
        if (m_activeRenderer) {
            locker->unlock();

            m_activeRenderer->flush(m_sink);
        }
    } else if (m_stop) {
        m_stop = false;

        if (QGstVideoRenderer * const activePool = m_activeRenderer) {
            m_activeRenderer = nullptr;
            locker->unlock();

            activePool->stop();

            locker->relock();
        }
    } else if (!m_startCaps.isNull()) {
        Q_ASSERT(!m_activeRenderer);

        auto startCaps = m_startCaps;
        m_startCaps = nullptr;

        if (m_renderer && m_sink) {
            locker->unlock();

            const bool started = m_renderer->start(startCaps.get());

            locker->relock();

            m_activeRenderer = started
                    ? m_renderer
                    : nullptr;
        } else if (QGstVideoRenderer * const activePool = m_activeRenderer) {
            m_activeRenderer = nullptr;
            locker->unlock();

            activePool->stop();

            locker->relock();
        }

    } else if (m_renderBuffer) {
        GstBuffer *buffer = m_renderBuffer;
        m_renderBuffer = nullptr;
        m_renderReturn = GST_FLOW_ERROR;

        if (m_activeRenderer && m_sink) {
            gst_buffer_ref(buffer);

            locker->unlock();

            const bool rendered = m_activeRenderer->present(m_sink, buffer);

            gst_buffer_unref(buffer);

            locker->relock();

            if (rendered)
                m_renderReturn = GST_FLOW_OK;
        }

        m_renderCondition.wakeAll();
    } else {
        m_setupCondition.wakeAll();

        return false;
    }
    return true;
}

void QVideoSurfaceGstDelegate::notify()
{
    if (!m_notified) {
        m_notified = true;
        QCoreApplication::postEvent(this, new QEvent(QEvent::UpdateRequest));
    }
}

bool QVideoSurfaceGstDelegate::waitForAsyncEvent(
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

void QVideoSurfaceGstDelegate::updateSupportedFormats()
{
    m_surfaceCaps = m_renderer->getCaps();
}

static GstVideoSinkClass *sink_parent_class;
static thread_local QVideoSink *current_sink;

#define VO_SINK(s) QGstVideoRendererSink *sink(reinterpret_cast<QGstVideoRendererSink *>(s))

QGstVideoRendererSink *QGstVideoRendererSink::createSink(QVideoSink *sink)
{
    setSink(sink);
    QGstVideoRendererSink *gstSink = reinterpret_cast<QGstVideoRendererSink *>(
            g_object_new(QGstVideoRendererSink::get_type(), nullptr));

    g_signal_connect(G_OBJECT(gstSink), "notify::show-preroll-frame", G_CALLBACK(handleShowPrerollChange), gstSink);

    return gstSink;
}

void QGstVideoRendererSink::setSink(QVideoSink *sink)
{
    current_sink = sink;
    get_type();
}

GType QGstVideoRendererSink::get_type()
{
    static GType type = 0;

    if (type == 0) {
        static const GTypeInfo info =
        {
            sizeof(QGstVideoRendererSinkClass),                    // class_size
            base_init,                                         // base_init
            nullptr,                                           // base_finalize
            class_init,                                        // class_init
            nullptr,                                           // class_finalize
            nullptr,                                           // class_data
            sizeof(QGstVideoRendererSink),                         // instance_size
            0,                                                 // n_preallocs
            instance_init,                                     // instance_init
            nullptr                                                  // value_table
        };

        type = g_type_register_static(
                GST_TYPE_VIDEO_SINK, "QGstVideoRendererSink", &info, GTypeFlags(0));

        // Register the sink type to be used in custom piplines.
        // When surface is ready the sink can be used.
        gst_element_register(nullptr, "qtvideosink", GST_RANK_PRIMARY, type);
    }

    return type;
}

void QGstVideoRendererSink::class_init(gpointer g_class, gpointer class_data)
{
    Q_UNUSED(class_data);

    sink_parent_class = reinterpret_cast<GstVideoSinkClass *>(g_type_class_peek_parent(g_class));

    GstVideoSinkClass *video_sink_class = reinterpret_cast<GstVideoSinkClass *>(g_class);
    video_sink_class->show_frame = QGstVideoRendererSink::show_frame;

    GstBaseSinkClass *base_sink_class = reinterpret_cast<GstBaseSinkClass *>(g_class);
    base_sink_class->get_caps = QGstVideoRendererSink::get_caps;
    base_sink_class->set_caps = QGstVideoRendererSink::set_caps;
    base_sink_class->propose_allocation = QGstVideoRendererSink::propose_allocation;
    base_sink_class->stop = QGstVideoRendererSink::stop;
    base_sink_class->unlock = QGstVideoRendererSink::unlock;
    base_sink_class->query = QGstVideoRendererSink::query;

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

    Q_ASSERT(current_sink);

    sink->delegate = new QVideoSurfaceGstDelegate(current_sink);
    sink->delegate->moveToThread(current_sink->thread());
    current_sink = nullptr;
}

void QGstVideoRendererSink::finalize(GObject *object)
{
    VO_SINK(object);

    delete sink->delegate;

    // Chain up
    G_OBJECT_CLASS(sink_parent_class)->finalize(object);
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
            sink->delegate->flush();
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
        sink->delegate->flush();

    return GST_ELEMENT_CLASS(sink_parent_class)->change_state(element, transition);
}

GstCaps *QGstVideoRendererSink::get_caps(GstBaseSink *base, GstCaps *filter)
{
    VO_SINK(base);

    QGstMutableCaps caps = sink->delegate->caps();
    if (filter)
        caps = gst_caps_intersect(caps.get(), filter);

    gst_caps_ref(caps.get());
    return caps.get();
}

gboolean QGstVideoRendererSink::set_caps(GstBaseSink *base, GstCaps *caps)
{
    VO_SINK(base);

#ifdef DEBUG_VIDEO_SURFACE_SINK
    qDebug() << "set_caps:";
    qDebug() << caps;
#endif

    if (!caps) {
        sink->delegate->stop();

        return TRUE;
    } else if (sink->delegate->start(caps)) {
        return TRUE;
    } else {
        return FALSE;
    }
}

gboolean QGstVideoRendererSink::propose_allocation(GstBaseSink *base, GstQuery *query)
{
    VO_SINK(base);
    return sink->delegate->proposeAllocation(query);
}

gboolean QGstVideoRendererSink::stop(GstBaseSink *base)
{
    VO_SINK(base);
    sink->delegate->stop();
    return TRUE;
}

gboolean QGstVideoRendererSink::unlock(GstBaseSink *base)
{
    VO_SINK(base);
    sink->delegate->unlock();
    return TRUE;
}

GstFlowReturn QGstVideoRendererSink::show_frame(GstVideoSink *base, GstBuffer *buffer)
{
    VO_SINK(base);
    return sink->delegate->render(buffer);
}

gboolean QGstVideoRendererSink::query(GstBaseSink *base, GstQuery *query)
{
    VO_SINK(base);
    if (sink->delegate->query(query))
        return TRUE;

    return GST_BASE_SINK_CLASS(sink_parent_class)->query(base, query);
}

QT_END_NAMESPACE
