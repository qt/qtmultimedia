// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <common/qgstreamervideosink_p.h>
#include <common/qgstvideorenderersink_p.h>
#include <common/qgst_debug_p.h>
#include <common/qgstutils_p.h>
#include <rhi/qrhi.h>

#include <QtCore/qdebug.h>
#include <QtCore/qloggingcategory.h>

#if QT_CONFIG(gstreamer_gl)
#  include <QtGui/qguiapplication.h>
#  include <QtGui/qopenglcontext.h>
#  include <QtGui/qwindow.h>
#  include <QtGui/qpa/qplatformnativeinterface.h>
#  include <gst/gl/gstglconfig.h>
#  include <gst/gl/gstgldisplay.h>

#  if QT_CONFIG(gstreamer_gl_x11)
#    include <gst/gl/x11/gstgldisplay_x11.h>
#  endif
#  if QT_CONFIG(gstreamer_gl_egl)
#    include <gst/gl/egl/gstgldisplay_egl.h>
#    include <EGL/egl.h>
#    include <EGL/eglext.h>
#  endif
#  if QT_CONFIG(gstreamer_gl_wayland)
#    include <gst/gl/wayland/gstgldisplay_wayland.h>
#  endif
#endif // #if QT_CONFIG(gstreamer_gl)

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(qLcGstVideoSink, "qt.multimedia.gstvideosink");

QGstreamerVideoSink::QGstreamerVideoSink(QVideoSink *parent)
    : QPlatformVideoSink{
          parent,
      },
      m_sinkBin{
          QGstBin::create("videoSinkBin"),
      }
{
    // This is a hack for some iMX and NVidia platforms. These require the use of a special video
    // conversion element in the pipeline before the video sink, as they unfortunately
    // output some proprietary format from the decoder even though it's sometimes marked as
    // a regular supported video/x-raw format.
    //
    // To fix this, simply insert the element into the pipeline if it's available. Otherwise
    // we simply use an identity element.
    QGstElementFactoryHandle factory;

    // QT_GSTREAMER_OVERRIDE_VIDEO_CONVERSION_ELEMENT allows users to override the
    // conversion element. Ideally we construct the element programatically, though.
    QByteArray preprocessOverride = qgetenv("QT_GSTREAMER_OVERRIDE_VIDEO_CONVERSION_ELEMENT");
    if (!preprocessOverride.isEmpty()) {
        qCDebug(qLcGstVideoSink) << "requesting conversion element from environment:"
                                 << preprocessOverride;

        m_gstPreprocess = QGstBin::createFromPipelineDescription(preprocessOverride, nullptr,
                                                                 /*ghostUnlinkedPads=*/true);
        if (!m_gstPreprocess)
            qCWarning(qLcGstVideoSink) << "Cannot create conversion element:" << preprocessOverride;
    }

    if (!m_gstPreprocess) {
        // This is a hack for some iMX and NVidia platforms. These require the use of a special
        // video conversion element in the pipeline before the video sink, as they unfortunately
        // output some proprietary format from the decoder even though it's sometimes marked as
        // a regular supported video/x-raw format.
        static constexpr auto decodersToTest = {
            "imxvideoconvert_g2d",
            "nvvidconv",
        };

        for (const char *decoder : decodersToTest) {
            factory = QGstElement::findFactory(decoder);
            if (factory)
                break;
        }

        if (factory) {
            qCDebug(qLcGstVideoSink)
                    << "instantiating conversion element:"
                    << g_type_name(gst_element_factory_get_element_type(factory.get()));

            m_gstPreprocess = QGstElement::createFromFactory(factory, "preprocess");
        }
    }

    bool disablePixelAspectRatio =
            qEnvironmentVariableIsSet("QT_GSTREAMER_DISABLE_PIXEL_ASPECT_RATIO");
    if (disablePixelAspectRatio) {
        // Enabling the pixel aspect ratio may expose a gstreamer bug on cameras that don't expose a
        // pixel-aspect-ratio via `VIDIOC_CROPCAP`. This can cause the caps negotiation to fail.
        // Using the QT_GSTREAMER_DISABLE_PIXEL_ASPECT_RATIO environment variable, one can disable
        // pixel-aspect-ratio handling
        //
        // compare: https://gitlab.freedesktop.org/gstreamer/gstreamer/-/merge_requests/6242
        m_gstCapsFilter =
                QGstElement::createFromFactory("identity", "nullPixelAspectRatioCapsFilter");
    } else {
        m_gstCapsFilter =
                QGstElement::createFromFactory("capsfilter", "pixelAspectRatioCapsFilter");
        QGstCaps capsFilterCaps{
            gst_caps_new_simple("video/x-raw", "pixel-aspect-ratio", GST_TYPE_FRACTION, 1, 1, NULL),
            QGstCaps::HasRef,
        };
        g_object_set(m_gstCapsFilter.element(), "caps", capsFilterCaps.caps(), NULL);
    }

    if (m_gstPreprocess) {
        m_sinkBin.add(m_gstPreprocess, m_gstCapsFilter);
        qLinkGstElements(m_gstPreprocess, m_gstCapsFilter);
        m_sinkBin.addGhostPad(m_gstPreprocess, "sink");
    } else {
        m_sinkBin.add(m_gstCapsFilter);
        m_sinkBin.addGhostPad(m_gstCapsFilter, "sink");
    }
}

QGstreamerVideoSink::~QGstreamerVideoSink()
{
    emit aboutToBeDestroyed();

    unrefGstContexts();
}

QGstElement QGstreamerVideoSink::gstSink()
{
    if (!m_gstVideoSink) {
        if (!m_gstQtSink)
            createQtSink();

        updateSinkElement(m_gstQtSink);
    }

    return m_sinkBin;
}

void QGstreamerVideoSink::setActive(bool isActive)
{
    if (m_isActive == isActive)
        return;
    m_isActive = isActive;

    if (m_gstQtSink)
        m_gstQtSink.setActive(isActive);
}

void QGstreamerVideoSink::setAsync(bool isAsync)
{
    m_sinkIsAsync = isAsync;
}

void QGstreamerVideoSink::setRhi(QRhi *rhi)
{
    if (rhi && rhi->backend() != QRhi::OpenGLES2)
        rhi = nullptr;
    if (m_rhi == rhi)
        return;

    m_rhi = rhi;
    updateGstContexts();
    if (m_gstQtSink) {
        QGstVideoRendererSinkElement oldSink = std::move(m_gstQtSink);

        // force creation of a new sink with proper caps.
        createQtSink();
        updateSinkElement(m_gstQtSink);
    }
}

void QGstreamerVideoSink::createQtSink()
{
    Q_ASSERT(!m_gstQtSink);

    m_gstQtSink = QGstVideoRendererSink::createSink(this);
    if (!m_sinkIsAsync)
        m_gstQtSink.set("async", false);
    m_gstQtSink.setActive(m_isActive);
}

void QGstreamerVideoSink::updateSinkElement(QGstVideoRendererSinkElement newSink)
{
    if (newSink == m_gstVideoSink)
        return;

    m_gstCapsFilter.src().modifyPipelineInIdleProbe([&] {
        if (m_gstVideoSink)
            m_sinkBin.stopAndRemoveElements(m_gstVideoSink);

        m_gstVideoSink = std::move(newSink);
        m_sinkBin.add(m_gstVideoSink);
        qLinkGstElements(m_gstCapsFilter, m_gstVideoSink);
        GstEvent *event = gst_event_new_reconfigure();
        gst_element_send_event(m_gstVideoSink.element(), event);
        m_gstVideoSink.syncStateWithParent();
    });

    m_sinkBin.dumpPipelineGraph("updateSinkElement");
}

void QGstreamerVideoSink::unrefGstContexts()
{
    m_gstGlDisplayContext.reset();
    m_gstGlLocalContext.reset();
    m_eglDisplay = nullptr;
    m_eglImageTargetTexture2D = nullptr;
}

void QGstreamerVideoSink::updateGstContexts()
{
    using namespace Qt::Literals;

    unrefGstContexts();

#if QT_CONFIG(gstreamer_gl)
    if (!m_rhi || m_rhi->backend() != QRhi::OpenGLES2)
        return;

    auto *nativeHandles = static_cast<const QRhiGles2NativeHandles *>(m_rhi->nativeHandles());
    auto glContext = nativeHandles->context;
    Q_ASSERT(glContext);

    const QString platform = QGuiApplication::platformName();
    QPlatformNativeInterface *pni = QGuiApplication::platformNativeInterface();
    m_eglDisplay = pni->nativeResourceForIntegration("egldisplay"_ba);
//    qDebug() << "platform is" << platform << m_eglDisplay;

    QGstGLDisplayHandle gstGlDisplay;

    QByteArray contextName = "eglcontext"_ba;
    GstGLPlatform glPlatform = GST_GL_PLATFORM_EGL;
    // use the egl display if we have one
    if (m_eglDisplay) {
#  if QT_CONFIG(gstreamer_gl_egl)
        gstGlDisplay.reset(
                GST_GL_DISPLAY_CAST(gst_gl_display_egl_new_with_egl_display(m_eglDisplay)),
                QGstGLDisplayHandle::HasRef);
        m_eglImageTargetTexture2D = eglGetProcAddress("glEGLImageTargetTexture2DOES");
#  endif
    } else {
        auto display = pni->nativeResourceForIntegration("display"_ba);

        if (display) {
#  if QT_CONFIG(gstreamer_gl_x11)
            if (platform == QLatin1String("xcb")) {
                contextName = "glxcontext"_ba;
                glPlatform = GST_GL_PLATFORM_GLX;

                gstGlDisplay.reset(GST_GL_DISPLAY_CAST(gst_gl_display_x11_new_with_display(
                                           reinterpret_cast<Display *>(display))),
                                   QGstGLDisplayHandle::HasRef);
            }
#  endif
#  if QT_CONFIG(gstreamer_gl_wayland)
            if (platform.startsWith(QLatin1String("wayland"))) {
                Q_ASSERT(!gstGlDisplay);
                gstGlDisplay.reset(GST_GL_DISPLAY_CAST(gst_gl_display_wayland_new_with_display(
                                           reinterpret_cast<struct wl_display *>(display))),
                                   QGstGLDisplayHandle::HasRef);
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
        QGstGLContextHandle::HasRef,
    };
    if (!appContext)
        qWarning() << "Could not create wrappped context for platform:" << glPlatform;

    gst_gl_context_activate(appContext.get(), true);

    QUniqueGErrorHandle error;
    gst_gl_context_fill_info(appContext.get(), &error);
    if (error) {
        qWarning() << "Could not fill context info:" << error;
        error = {};
    }

    QGstGLContextHandle displayContext;
    gst_gl_display_create_context(gstGlDisplay.get(), appContext.get(), &displayContext, &error);
    if (error)
        qWarning() << "Could not create display context:" << error;

    appContext.reset();

    m_gstGlDisplayContext.reset(gst_context_new(GST_GL_DISPLAY_CONTEXT_TYPE, false),
                                QGstContextHandle::HasRef);
    gst_context_set_gl_display(m_gstGlDisplayContext.get(), gstGlDisplay.get());

    m_gstGlLocalContext.reset(gst_context_new("gst.gl.local_context", false),
                              QGstContextHandle::HasRef);
    GstStructure *structure = gst_context_writable_structure(m_gstGlLocalContext.get());
    gst_structure_set(structure, "context", GST_TYPE_GL_CONTEXT, displayContext.get(), nullptr);
    displayContext.reset();

    // Note: after updating the context, we switch the sink and send gst_event_new_reconfigure()
    // upstream. this will cause the context to be queried again.
#endif // #if QT_CONFIG(gstreamer_gl)
}

QT_END_NAMESPACE

#include "moc_qgstreamervideosink_p.cpp"
