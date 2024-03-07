// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qgstreamervideosink_p.h"
#include "qgstvideorenderersink_p.h"
#include "qgstsubtitlesink_p.h"
#include <qgst_debug_p.h>
#include <qgstutils_p.h>
#include <rhi/qrhi.h>

#if QT_CONFIG(gstreamer_gl)
#include <QGuiApplication>
#include <QtGui/qopenglcontext.h>
#include <QWindow>
#include <qpa/qplatformnativeinterface.h>
#include <gst/gl/gstglconfig.h>

#if GST_GL_HAVE_WINDOW_X11 && __has_include("X11/Xlib-xcb.h")
#    include <gst/gl/x11/gstgldisplay_x11.h>
#endif
#if GST_GL_HAVE_PLATFORM_EGL
#    include <gst/gl/egl/gstgldisplay_egl.h>
#    include <EGL/egl.h>
#    include <EGL/eglext.h>
#endif
#if GST_GL_HAVE_WINDOW_WAYLAND && __has_include("wayland-client.h")
#    include <gst/gl/wayland/gstgldisplay_wayland.h>
#endif
#endif // #if QT_CONFIG(gstreamer_gl)

#include <QtCore/qdebug.h>

#include <QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

QGstreamerVideoSink::QGstreamerVideoSink(QVideoSink *parent)
    : QPlatformVideoSink(parent)
{
    sinkBin = QGstBin::create("videoSinkBin");

    // This is a hack for some iMX and NVidia platforms. These require the use of a special video
    // conversion element in the pipeline before the video sink, as they unfortunately
    // output some proprietary format from the decoder even though it's sometimes marked as
    // a regular supported video/x-raw format.
    //
    // To fix this, simply insert the element into the pipeline if it's available. Otherwise
    // we simply use an identity element.
    gstQueue = QGstElement::createFromFactory("queue", "videoSinkQueue");

    QGstElementFactoryHandle factory = QGstElementFactoryHandle{
        gst_element_factory_find("imxvideoconvert_g2d"),
    };

    if (!factory)
        factory = QGstElementFactoryHandle{
            gst_element_factory_find("nvvidconv"),
        };

    if (factory) {
        gstPreprocess = QGstElement{
            gst_element_factory_create(factory.get(), "preprocess"),
            QGstElement::NeedsRef,
        };
    }

    bool disablePixelAspectRatio =
            qEnvironmentVariableIsSet("QT_MULTIMEDIA_GSTREAMER_DISABLE_PIXEL_ASPECT_RATIO");
    if (disablePixelAspectRatio) {
        // Enabling the pixel aspect ratio may expose a gstreamer bug on cameras that don't expose a
        // pixel-aspect-ratio via `VIDIOC_CROPCAP`. This can cause the caps negotiation to fail.
        // Using the QT_MULTIMEDIA_GSTREAMER_DISABLE_PIXEL_ASPECT_RATIO environment variable, on can
        // disable pixel-aspect-ratio handling
        //
        // compare: https://gitlab.freedesktop.org/gstreamer/gstreamer/-/merge_requests/6242
        gstCapsFilter =
                QGstElement::createFromFactory("identity", "nullPixelAspectRatioCapsFilter");
    } else {
        gstCapsFilter = QGstElement::createFromFactory("capsfilter", "pixelAspectRatioCapsFilter");
        QGstCaps capsFilterCaps{
            gst_caps_new_simple("video/x-raw", "pixel-aspect-ratio", GST_TYPE_FRACTION, 1, 1, NULL),
            QGstCaps::HasRef,
        };
        g_object_set(gstCapsFilter.element(), "caps", capsFilterCaps.release(), NULL);
    }

    if (gstPreprocess) {
        sinkBin.add(gstQueue, gstPreprocess, gstCapsFilter);
        qLinkGstElements(gstQueue, gstPreprocess, gstCapsFilter);
    } else {
        sinkBin.add(gstQueue, gstCapsFilter);
        qLinkGstElements(gstQueue, gstCapsFilter);
    }
    sinkBin.addGhostPad(gstQueue, "sink");

    gstSubtitleSink = QGstElement(GST_ELEMENT(QGstSubtitleSink::createSink(this)));
}

QGstreamerVideoSink::~QGstreamerVideoSink()
{
    unrefGstContexts();

    setPipeline(QGstPipeline());
}

QGstElement QGstreamerVideoSink::gstSink()
{
    updateSinkElement();
    return sinkBin;
}

void QGstreamerVideoSink::setPipeline(QGstPipeline pipeline)
{
    gstPipeline = pipeline;
}

bool QGstreamerVideoSink::inStoppedState() const
{
    if (gstPipeline.isNull())
        return true;
    return gstPipeline.inStoppedState();
}

void QGstreamerVideoSink::setRhi(QRhi *rhi)
{
    if (rhi && rhi->backend() != QRhi::OpenGLES2)
        rhi = nullptr;
    if (m_rhi == rhi)
        return;

    m_rhi = rhi;
    updateGstContexts();
    if (!gstQtSink.isNull()) {
        // force creation of a new sink with proper caps
        createQtSink();
        updateSinkElement();
    }
}

void QGstreamerVideoSink::createQtSink()
{
    if (gstQtSink)
        gstQtSink.setStateSync(GST_STATE_NULL);

    gstQtSink = QGstElement(reinterpret_cast<GstElement *>(QGstVideoRendererSink::createSink(this)));
}

void QGstreamerVideoSink::updateSinkElement()
{
    QGstElement newSink;
    if (gstQtSink.isNull())
        createQtSink();
    newSink = gstQtSink;

    if (newSink == gstVideoSink)
        return;

    gstPipeline.modifyPipelineWhileNotRunning([&] {
        if (!gstVideoSink.isNull())
            sinkBin.stopAndRemoveElements(gstVideoSink);

        gstVideoSink = newSink;
        sinkBin.add(gstVideoSink);
        qLinkGstElements(gstCapsFilter, gstVideoSink);
        gstVideoSink.setState(GST_STATE_PAUSED);
    });

    gstPipeline.dumpGraph("updateVideoSink");
}

void QGstreamerVideoSink::unrefGstContexts()
{
    m_gstGlDisplayContext.close();
    m_gstGlLocalContext.close();
    m_eglDisplay = nullptr;
    m_eglImageTargetTexture2D = nullptr;
}

void QGstreamerVideoSink::updateGstContexts()
{
    unrefGstContexts();

#if QT_CONFIG(gstreamer_gl)
    if (!m_rhi || m_rhi->backend() != QRhi::OpenGLES2)
        return;

    auto *nativeHandles = static_cast<const QRhiGles2NativeHandles *>(m_rhi->nativeHandles());
    auto glContext = nativeHandles->context;
    Q_ASSERT(glContext);

    const QString platform = QGuiApplication::platformName();
    QPlatformNativeInterface *pni = QGuiApplication::platformNativeInterface();
    m_eglDisplay = pni->nativeResourceForIntegration("egldisplay");
//    qDebug() << "platform is" << platform << m_eglDisplay;

    QGstGLDisplayHandle gstGlDisplay;

    const char *contextName = "eglcontext";
    GstGLPlatform glPlatform = GST_GL_PLATFORM_EGL;
    // use the egl display if we have one
    if (m_eglDisplay) {
#if GST_GL_HAVE_PLATFORM_EGL
        gstGlDisplay.reset(
                GST_GL_DISPLAY_CAST(gst_gl_display_egl_new_with_egl_display(m_eglDisplay)));
        m_eglImageTargetTexture2D = eglGetProcAddress("glEGLImageTargetTexture2DOES");
#endif
    } else {
        auto display = pni->nativeResourceForIntegration("display");

        if (display) {
#if GST_GL_HAVE_WINDOW_X11 && __has_include("X11/Xlib-xcb.h")
            if (platform == QLatin1String("xcb")) {
                contextName = "glxcontext";
                glPlatform = GST_GL_PLATFORM_GLX;

                gstGlDisplay.reset(GST_GL_DISPLAY_CAST(
                        gst_gl_display_x11_new_with_display(reinterpret_cast<Display *>(display))));
            }
#endif
#if GST_GL_HAVE_WINDOW_WAYLAND && __has_include("wayland-client.h")
            if (platform.startsWith(QLatin1String("wayland"))) {
                Q_ASSERT(!gstGlDisplay);
                gstGlDisplay.reset(GST_GL_DISPLAY_CAST(gst_gl_display_wayland_new_with_display(
                        reinterpret_cast<struct wl_display *>(display))));
            }
#endif
        }
    }

    if (!gstGlDisplay) {
        qWarning() << "Could not create GstGLDisplay";
        return;
    }

    void *nativeContext = pni->nativeResourceForContext(contextName, glContext);
    if (!nativeContext)
        qWarning() << "Could not find resource for" << contextName;

    GstGLAPI glApi = QOpenGLContext::openGLModuleType() == QOpenGLContext::LibGL ? GST_GL_API_OPENGL : GST_GL_API_GLES2;
    QGstGLContextHandle appContext{
        gst_gl_context_new_wrapped(gstGlDisplay.get(), guintptr(nativeContext), glPlatform, glApi),
    };
    if (!appContext)
        qWarning() << "Could not create wrappped context for platform:" << glPlatform;

    QUniqueGErrorHandle error;
    QGstGLContextHandle displayContext;
    gst_gl_display_create_context(gstGlDisplay.get(), appContext.get(), &displayContext, &error);
    if (error)
        qWarning() << "Could not create display context:" << error;

    appContext.close();

    m_gstGlDisplayContext.reset(gst_context_new(GST_GL_DISPLAY_CONTEXT_TYPE, false));
    gst_context_set_gl_display(m_gstGlDisplayContext.get(), gstGlDisplay.get());

    m_gstGlLocalContext.reset(gst_context_new("gst.gl.local_context", false));
    GstStructure *structure = gst_context_writable_structure(m_gstGlLocalContext.get());
    gst_structure_set(structure, "context", GST_TYPE_GL_CONTEXT, displayContext.get(), nullptr);
    displayContext.close();

    if (!gstPipeline.isNull())
        gst_element_set_context(gstPipeline.element(), m_gstGlLocalContext.get());
#endif // #if QT_CONFIG(gstreamer_gl)
}

QT_END_NAMESPACE

#include "moc_qgstreamervideosink_p.cpp"
