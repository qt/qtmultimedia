/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qgstreamervideooverlay_p.h"

#include <QtGui/qguiapplication.h>
#include "qgstutils_p.h"
#include "private/qgst_p.h"
#include "private/qgstreamermessage_p.h"

#include <gst/video/videooverlay.h>

#include <QtMultimedia/private/qtmultimediaglobal_p.h>

QT_BEGIN_NAMESPACE

struct ElementMap
{
   const char *qtPlatform;
   const char *gstreamerElement;
};

// Ordered by descending priority
static constexpr ElementMap elementMap[] =
{
    { "xcb", "vaapisink" },
    { "xcb", "xvimagesink" },
    { "xcb", "ximagesink" },

    // wayland
    { "wayland", "vaapisink" }
};

class QGstreamerSinkProperties
{
public:
    virtual ~QGstreamerSinkProperties() = default;

    virtual bool hasShowPrerollFrame() const = 0;
    virtual void reset() = 0;
    virtual bool setBrightness(float brightness) = 0;
    virtual bool setContrast(float contrast) = 0;
    virtual bool setHue(float hue) = 0;
    virtual bool setSaturation(float saturation) = 0;
    virtual void setAspectRatioMode(Qt::AspectRatioMode mode) = 0;
};

class QXVImageSinkProperties : public QGstreamerSinkProperties
{
public:
    QXVImageSinkProperties(const QGstElement &sink)
        : m_videoSink(sink)
    {
        auto *klass = G_OBJECT_GET_CLASS(m_videoSink.object());
        m_hasForceAspectRatio = g_object_class_find_property(klass, "force-aspect-ratio");
        m_hasBrightness = g_object_class_find_property(klass, "brightness");
        m_hasContrast = g_object_class_find_property(klass, "contrast");
        m_hasHue = g_object_class_find_property(klass, "hue");
        m_hasSaturation = g_object_class_find_property(klass, "saturation");
        m_hasShowPrerollFrame = g_object_class_find_property(klass, "show-preroll-frame");
    }

    bool hasShowPrerollFrame() const override
    {
        return m_hasShowPrerollFrame;
    }

    void reset() override
    {
        setAspectRatioMode(m_aspectRatioMode);
        setBrightness(m_brightness);
        setContrast(m_contrast);
        setHue(m_hue);
        setSaturation(m_saturation);
    }

    bool setBrightness(float brightness) override
    {
        m_brightness = brightness;
        if (m_hasBrightness)
            m_videoSink.set("brightness", brightness * 1000);

        return m_hasBrightness;
    }

    bool setContrast(float contrast) override
    {
        m_contrast = contrast;
        if (m_hasContrast)
            m_videoSink.set("contrast", contrast * 1000);

        return m_hasContrast;
    }

    bool setHue(float hue) override
    {
        m_hue = hue;
        if (m_hasHue)
            m_videoSink.set("hue", hue * 1000);

        return m_hasHue;
    }

    bool setSaturation(float saturation) override
    {
        m_saturation = saturation;
        if (m_hasSaturation)
            m_videoSink.set("saturation", saturation * 1000);

        return m_hasSaturation;
    }

    void setAspectRatioMode(Qt::AspectRatioMode mode) override
    {
        m_aspectRatioMode = mode;
        if (m_hasForceAspectRatio)
            m_videoSink.set("force-aspect-ratio", (mode == Qt::KeepAspectRatio));
    }

protected:

    QGstElement m_videoSink;
    bool m_hasForceAspectRatio = false;
    bool m_hasBrightness = false;
    bool m_hasContrast = false;
    bool m_hasHue = false;
    bool m_hasSaturation = false;
    bool m_hasShowPrerollFrame = false;
    Qt::AspectRatioMode m_aspectRatioMode = Qt::KeepAspectRatio;
    float m_brightness = 0;
    float m_contrast = 0;
    float m_hue = 0;
    float m_saturation = 0;
};

class QVaapiSinkProperties : public QXVImageSinkProperties
{
public:
    QVaapiSinkProperties(QGstElement sink)
        : QXVImageSinkProperties(sink)
    {
        // Set default values.
        m_contrast = 1;
        m_saturation = 1;
    }

    bool setBrightness(float brightness) override
    {
        m_brightness = brightness;
        if (m_hasBrightness) {
            gfloat v = brightness;
            m_videoSink.set("brightness", v);
        }

        return m_hasBrightness;
    }

    bool setContrast(float contrast) override
    {
        m_contrast = contrast;
        if (m_hasContrast) {
            gfloat v = contrast + 1; // [-1, 1] -> [0,2]
            m_videoSink.set("contrast", v);
        }

        return m_hasContrast;
    }

    bool setHue(float hue) override
    {
        m_hue = hue;
        if (m_hasHue) {
            gfloat v = hue * 180; // [-1,1] -> [-180,180]
            m_videoSink.set("hue", v);
        }

        return m_hasHue;
    }

    bool setSaturation(float saturation) override
    {
        m_saturation = saturation;
        if (m_hasSaturation) {
            gfloat v = saturation + 1; // [-100,100] -> [0,2]
            m_videoSink.set("saturation", v);
        }

        return m_hasSaturation;
    }
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
    QString platform = QGuiApplication::platformName();

    // First, try some known video sinks, depending on the Qt platform plugin in use.
    for (auto i : elementMap) {
        if (platform != QLatin1String(i.qtPlatform))
            continue;
        QGstElement choice(i.gstreamerElement, i.gstreamerElement);
        if (choice.isNull())
            continue;

        if (qt_gst_element_is_functioning(choice))
            return choice;
    }

    // We need a native window ID to use the GstVideoOverlay interface.
    // Bail out if the Qt platform plugin in use cannot provide a sensible WId.
    if (platform != QLatin1String("xcb"))
        return {};

    QGstElement choice;
    // If none of the known video sinks are available, try to find one that implements the
    // GstVideoOverlay interface and has autoplugging rank.
    GList *list = qt_gst_video_sinks();
    for (GList *item = list; item != nullptr; item = item->next) {
        GstElementFactory *f = GST_ELEMENT_FACTORY(item->data);

        if (!gst_element_factory_has_interface(f, "GstVideoOverlay"))
            continue;

        choice = QGstElement(gst_element_factory_create(f, nullptr));
        if (choice.isNull())
            continue;

        if (qt_gst_element_is_functioning(choice))
            break;
        choice = {};
    }

    gst_plugin_feature_list_free(list);

    return choice;
}

QGstreamerVideoOverlay::QGstreamerVideoOverlay(QObject *parent, const QByteArray &elementName)
    : QObject(parent)
    , QGstreamerBufferProbe(QGstreamerBufferProbe::ProbeCaps)
{
    QGstElement sink;
    if (!elementName.isEmpty())
        sink = QGstElement(elementName.constData(), nullptr);
    else
        sink = findBestVideoSink();

    setVideoSink(sink);
}

QGstreamerVideoOverlay::~QGstreamerVideoOverlay()
{
    if (!m_videoSink.isNull()) {
        delete m_sinkProperties;
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

    m_videoSink = sink;

    QGstPad pad = m_videoSink.staticPad("sink");
    addProbeToPad(pad.pad());

    QByteArray sinkName(sink.name());
    bool isVaapi = sinkName.startsWith("vaapisink");
    delete m_sinkProperties;
    m_sinkProperties = isVaapi ? new QVaapiSinkProperties(sink) : new QXVImageSinkProperties(sink);

    if (m_sinkProperties->hasShowPrerollFrame())
        g_signal_connect(m_videoSink.object(), "notify::show-preroll-frame",
                         G_CALLBACK(showPrerollFrameChanged), this);
}

QSize QGstreamerVideoOverlay::nativeVideoSize() const
{
    return m_nativeVideoSize;
}

void QGstreamerVideoOverlay::setWindowHandle(WId id)
{
    m_windowId = id;

    if (isActive())
        setWindowHandle_helper(id);
}

void QGstreamerVideoOverlay::setWindowHandle_helper(WId id)
{
    if (!m_videoSink.isNull() && GST_IS_VIDEO_OVERLAY(m_videoSink.object())) {
        gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY(m_videoSink.object()), id);

        // Properties need to be reset when changing the winId.
        m_sinkProperties->reset();
    }
}

void QGstreamerVideoOverlay::setRenderRectangle(const QRect &rect)
{
    int x = -1;
    int y = -1;
    int w = -1;
    int h = -1;

    if (!rect.isEmpty()) {
        x = rect.x();
        y = rect.y();
        w = rect.width();
        h = rect.height();
    }

    if (!m_videoSink.isNull() && GST_IS_VIDEO_OVERLAY(m_videoSink.object()))
        gst_video_overlay_set_render_rectangle(GST_VIDEO_OVERLAY(m_videoSink.object()), x, y, w, h);
}

bool QGstreamerVideoOverlay::processSyncMessage(const QGstreamerMessage &message)
{
    GstMessage* gm = message.rawMessage();

    if (gm && (GST_MESSAGE_TYPE(gm) == GST_MESSAGE_ELEMENT) &&
        gst_structure_has_name(gst_message_get_structure(gm), "prepare-window-handle")) {
        setWindowHandle_helper(m_windowId);
        return true;
    }

    return false;
}

bool QGstreamerVideoOverlay::processBusMessage(const QGstreamerMessage &message)
{
    GstMessage* gm = message.rawMessage();

    if (GST_MESSAGE_TYPE(gm) == GST_MESSAGE_STATE_CHANGED && GST_MESSAGE_SRC(gm) == m_videoSink.object())
        updateIsActive();

    return false;
}

void QGstreamerVideoOverlay::probeCaps(GstCaps *caps)
{
    QSize size = QGstCaps(caps).at(0).resolution();
    if (size != m_nativeVideoSize) {
        m_nativeVideoSize = size;
        emit nativeVideoSizeChanged();
    }
}

bool QGstreamerVideoOverlay::isActive() const
{
    return m_isActive;
}

void QGstreamerVideoOverlay::updateIsActive()
{
    if (m_videoSink.isNull())
        return;

    GstState state = m_videoSink.state();
    gboolean showPreroll = true;

    if (m_sinkProperties->hasShowPrerollFrame())
        showPreroll = m_videoSink.getBool("show-preroll-frame");

    bool newIsActive = (state == GST_STATE_PLAYING || (state == GST_STATE_PAUSED && showPreroll));

    if (newIsActive != m_isActive) {
        m_isActive = newIsActive;
        emit activeChanged();
    }
}

void QGstreamerVideoOverlay::showPrerollFrameChanged(GObject *, GParamSpec *, QGstreamerVideoOverlay *overlay)
{
    overlay->updateIsActive();
}

void QGstreamerVideoOverlay::setAspectRatioMode(Qt::AspectRatioMode mode)
{
    m_sinkProperties->setAspectRatioMode(mode);
}

void QGstreamerVideoOverlay::setBrightness(float brightness)
{
    m_sinkProperties->setBrightness(brightness);
}

void QGstreamerVideoOverlay::setContrast(float contrast)
{
    m_sinkProperties->setContrast(contrast);
}

void QGstreamerVideoOverlay::setHue(float hue)
{
    m_sinkProperties->setHue(hue);
}

void QGstreamerVideoOverlay::setSaturation(float saturation)
{
    m_sinkProperties->setSaturation(saturation);
}

QT_END_NAMESPACE
