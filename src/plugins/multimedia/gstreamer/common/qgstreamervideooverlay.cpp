// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qgstreamervideooverlay_p.h"

#include <QtGui/qguiapplication.h>
#include <QtMultimedia/private/qtmultimediaglobal_p.h>

#include <common/qglist_helper_p.h>
#include <common/qgst_p.h>
#include <common/qgstreamermessage_p.h>
#include <common/qgstreamervideosink_p.h>
#include <common/qgstutils_p.h>

#include <gst/video/videooverlay.h>

QT_BEGIN_NAMESPACE

struct ElementMap
{
    QStringView qtPlatform;
    const char *gstreamerElement = nullptr;
};

// Ordered by descending priority
static constexpr ElementMap elementMap[] = {
    { u"xcb", "xvimagesink" },
    { u"xcb", "ximagesink" },

    // wayland
    { u"wayland", "waylandsink" },
};

static bool qt_gst_element_is_functioning(QGstElement element)
{
    GstStateChangeReturn ret = element.setState(GST_STATE_READY);
    if (ret == GST_STATE_CHANGE_SUCCESS) {
        element.setState(GST_STATE_NULL);
        return true;
    }

    return false;
}

static QGstElement findBestVideoSink()
{
    using namespace Qt::StringLiterals;
    QString platform = QGuiApplication::platformName();

    // First, try some known video sinks, depending on the Qt platform plugin in use.
    for (const auto &i : elementMap) {
        if (platform != i.qtPlatform)
            continue;
        QGstElement choice = QGstElement::createFromFactory(i.gstreamerElement, i.gstreamerElement);
        if (choice.isNull())
            continue;

        if (qt_gst_element_is_functioning(choice))
            return choice;
    }

    // We need a native window ID to use the GstVideoOverlay interface.
    // Bail out if the Qt platform plugin in use cannot provide a sensible WId.
    if (platform != QStringView{ u"xcb" } && platform != QStringView{ u"wayland" })
        return {};

    QGstElement choice;
    // If none of the known video sinks are available, try to find one that implements the
    // GstVideoOverlay interface and has autoplugging rank.
    GList *list = qt_gst_video_sinks();
    for (GstElementFactory *f : QGstUtils::GListRangeAdaptor<GstElementFactory *>(list)) {
        if (!gst_element_factory_has_interface(f, "GstVideoOverlay"))
            continue;

        choice = QGstElement::createFromFactory(f, nullptr);
        if (choice.isNull())
            continue;

        if (qt_gst_element_is_functioning(choice))
            break;
        choice = {};
    }

    gst_plugin_feature_list_free(list);
    if (choice.isNull())
        qWarning() << "Could not find a valid windowed video sink";

    return choice;
}

QGstreamerVideoOverlay::QGstreamerVideoOverlay(QGstreamerVideoSink *parent, const QByteArray &elementName)
    : QObject(parent)
    , QGstreamerBufferProbe(QGstreamerBufferProbe::ProbeCaps)
    , m_gstreamerVideoSink(parent)
{
    QGstElement sink;
    if (!elementName.isEmpty())
        sink = QGstElement::createFromFactory(elementName.constData());
    else
        sink = findBestVideoSink();

    setVideoSink(sink);
}

QGstreamerVideoOverlay::~QGstreamerVideoOverlay()
{
    if (!m_videoSink.isNull()) {
        QGstPad pad = m_videoSink.staticPad("sink");
        removeProbeFromPad(pad.pad());
    }
}

QGstElement QGstreamerVideoOverlay::videoSink() const
{
    return m_videoSink;
}

void QGstreamerVideoOverlay::setVideoSink(QGstElement sink)
{
    if (sink.isNull())
        return;

    m_videoSink = std::move(sink);

    QGstPad pad = m_videoSink.staticPad("sink");
    addProbeToPad(pad.pad());

    auto *klass = G_OBJECT_GET_CLASS(m_videoSink.object());
    m_hasForceAspectRatio = g_object_class_find_property(klass, "force-aspect-ratio");
    m_hasFullscreen = g_object_class_find_property(klass, "fullscreen");
}

QSize QGstreamerVideoOverlay::nativeVideoSize() const
{
    return m_nativeVideoSize;
}

void QGstreamerVideoOverlay::setWindowHandle(WId id)
{
    m_windowId = id;

    if (!m_videoSink.isNull() && GST_IS_VIDEO_OVERLAY(m_videoSink.object())) {
        applyRenderRect();

        // Properties need to be reset when changing the winId.
        setAspectRatioMode(m_aspectRatioMode);
        setFullScreen(m_fullScreen);
        applyRenderRect();
    }
}

void QGstreamerVideoOverlay::setRenderRectangle(const QRect &rect)
{
    renderRect = rect;
    applyRenderRect();
}

void QGstreamerVideoOverlay::applyRenderRect()
{
    if (!m_windowId)
        return;

    int x = -1;
    int y = -1;
    int w = -1;
    int h = -1;

    if (!renderRect.isEmpty()) {
        x = renderRect.x();
        y = renderRect.y();
        w = renderRect.width();
        h = renderRect.height();
        QSize scaledVideo = m_nativeVideoSize.scaled(w, h, m_aspectRatioMode);
        x += (w - scaledVideo.width())/2;
        y += (h - scaledVideo.height())/2;
        w = scaledVideo.width();
        h = scaledVideo.height();
    }

    if (!m_videoSink.isNull() && GST_IS_VIDEO_OVERLAY(m_videoSink.object()))
        gst_video_overlay_set_render_rectangle(GST_VIDEO_OVERLAY(m_videoSink.object()), x, y, w, h);
}

void QGstreamerVideoOverlay::probeCaps(GstCaps *caps)
{
    QSize size = QGstCaps(caps, QGstCaps::NeedsRef).at(0).resolution();
    if (size != m_nativeVideoSize) {
        m_nativeVideoSize = size;
        m_gstreamerVideoSink->setNativeSize(m_nativeVideoSize);
        applyRenderRect();
    }
}

void QGstreamerVideoOverlay::setAspectRatioMode(Qt::AspectRatioMode mode)
{
    m_aspectRatioMode = mode;
    if (m_hasForceAspectRatio)
        m_videoSink.set("force-aspect-ratio", (mode == Qt::KeepAspectRatio));
}

void QGstreamerVideoOverlay::setFullScreen(bool fullscreen)
{
    m_fullScreen = fullscreen;
    if (m_hasFullscreen)
        m_videoSink.set("fullscreen", fullscreen);
}

bool QGstreamerVideoOverlay::processSyncMessage(const QGstreamerMessage &message)
{
    if (!gst_is_video_overlay_prepare_window_handle_message(message.message()))
        return false;
    gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY(m_videoSink.object()), m_windowId);
    return true;
}

QT_END_NAMESPACE

#include "moc_qgstreamervideooverlay_p.cpp"
