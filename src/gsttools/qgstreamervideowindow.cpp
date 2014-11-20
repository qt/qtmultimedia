/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qgstreamervideowindow_p.h"
#include <private/qgstutils_p.h>

#include <QtCore/qdebug.h>

#include <gst/gst.h>

#if !GST_CHECK_VERSION(1,0,0)
#include <gst/interfaces/xoverlay.h>
#include <gst/interfaces/propertyprobe.h>
#else
#include <gst/video/videooverlay.h>
#endif


QGstreamerVideoWindow::QGstreamerVideoWindow(QObject *parent, const char *elementName)
    : QVideoWindowControl(parent)
    , QGstreamerBufferProbe(QGstreamerBufferProbe::ProbeCaps)
    , m_videoSink(0)
    , m_windowId(0)
    , m_aspectRatioMode(Qt::KeepAspectRatio)
    , m_fullScreen(false)
    , m_colorKey(QColor::Invalid)
{
    if (elementName) {
        m_videoSink = gst_element_factory_make(elementName, NULL);
    } else {
        m_videoSink = gst_element_factory_make("xvimagesink", NULL);
    }

    if (m_videoSink) {
        qt_gst_object_ref_sink(GST_OBJECT(m_videoSink)); //Take ownership

        GstPad *pad = gst_element_get_static_pad(m_videoSink, "sink");
        addProbeToPad(pad);
        gst_object_unref(GST_OBJECT(pad));
    }
    else
        qDebug() << "No m_videoSink available!";
}

QGstreamerVideoWindow::~QGstreamerVideoWindow()
{
    if (m_videoSink) {
        GstPad *pad = gst_element_get_static_pad(m_videoSink,"sink");
        removeProbeFromPad(pad);
        gst_object_unref(GST_OBJECT(pad));
        gst_object_unref(GST_OBJECT(m_videoSink));
    }
}

WId QGstreamerVideoWindow::winId() const
{
    return m_windowId;
}

void QGstreamerVideoWindow::setWinId(WId id)
{
    if (m_windowId == id)
        return;

    WId oldId = m_windowId;

    m_windowId = id;
#if GST_CHECK_VERSION(1,0,0)
    if (m_videoSink && GST_IS_VIDEO_OVERLAY(m_videoSink)) {
        gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY(m_videoSink), m_windowId);
    }
#else
    if (m_videoSink && GST_IS_X_OVERLAY(m_videoSink)) {
        gst_x_overlay_set_xwindow_id(GST_X_OVERLAY(m_videoSink), m_windowId);
    }
#endif
    if (!oldId)
        emit readyChanged(true);

    if (!id)
        emit readyChanged(false);
}

bool QGstreamerVideoWindow::processSyncMessage(const QGstreamerMessage &message)
{
    GstMessage* gm = message.rawMessage();
#if GST_CHECK_VERSION(1,0,0)
    const GstStructure *s = gst_message_get_structure(gm);
    if ((GST_MESSAGE_TYPE(gm) == GST_MESSAGE_ELEMENT) &&
            gst_structure_has_name(s, "prepare-window-handle") &&
            m_videoSink && GST_IS_VIDEO_OVERLAY(m_videoSink)) {

        gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY(m_videoSink), m_windowId);

        return true;
    }
#else
    if ((GST_MESSAGE_TYPE(gm) == GST_MESSAGE_ELEMENT) &&
            gst_structure_has_name(gm->structure, "prepare-xwindow-id") &&
            m_videoSink && GST_IS_X_OVERLAY(m_videoSink)) {

        gst_x_overlay_set_xwindow_id(GST_X_OVERLAY(m_videoSink), m_windowId);

        return true;
    }
#endif
    return false;
}

QRect QGstreamerVideoWindow::displayRect() const
{
    return m_displayRect;
}

void QGstreamerVideoWindow::setDisplayRect(const QRect &rect)
{
    m_displayRect = rect;
#if GST_CHECK_VERSION(1,0,0)
    if (m_videoSink && GST_IS_VIDEO_OVERLAY(m_videoSink)) {
        if (m_displayRect.isEmpty())
            gst_video_overlay_set_render_rectangle(GST_VIDEO_OVERLAY(m_videoSink), -1, -1, -1, -1);
        else
            gst_video_overlay_set_render_rectangle(GST_VIDEO_OVERLAY(m_videoSink),
                                               m_displayRect.x(),
                                               m_displayRect.y(),
                                               m_displayRect.width(),
                                               m_displayRect.height());
        repaint();
    }
#else
    if (m_videoSink && GST_IS_X_OVERLAY(m_videoSink)) {
#if GST_VERSION_MICRO >= 29
        if (m_displayRect.isEmpty())
            gst_x_overlay_set_render_rectangle(GST_X_OVERLAY(m_videoSink), -1, -1, -1, -1);
        else
            gst_x_overlay_set_render_rectangle(GST_X_OVERLAY(m_videoSink),
                                               m_displayRect.x(),
                                               m_displayRect.y(),
                                               m_displayRect.width(),
                                               m_displayRect.height());
        repaint();
#endif
    }
#endif
}

Qt::AspectRatioMode QGstreamerVideoWindow::aspectRatioMode() const
{
    return m_aspectRatioMode;
}

void QGstreamerVideoWindow::setAspectRatioMode(Qt::AspectRatioMode mode)
{
    m_aspectRatioMode = mode;

    if (m_videoSink) {
        g_object_set(G_OBJECT(m_videoSink),
                     "force-aspect-ratio",
                     (m_aspectRatioMode == Qt::KeepAspectRatio),
                     (const char*)NULL);
    }
}

void QGstreamerVideoWindow::repaint()
{
#if GST_CHECK_VERSION(1,0,0)
    if (m_videoSink && GST_IS_VIDEO_OVERLAY(m_videoSink)) {
        //don't call gst_x_overlay_expose if the sink is in null state
        GstState state = GST_STATE_NULL;
        GstStateChangeReturn res = gst_element_get_state(m_videoSink, &state, NULL, 1000000);
        if (res != GST_STATE_CHANGE_FAILURE && state != GST_STATE_NULL) {
            gst_video_overlay_expose(GST_VIDEO_OVERLAY(m_videoSink));
        }
    }
#else
    if (m_videoSink && GST_IS_X_OVERLAY(m_videoSink)) {
        //don't call gst_x_overlay_expose if the sink is in null state
        GstState state = GST_STATE_NULL;
        GstStateChangeReturn res = gst_element_get_state(m_videoSink, &state, NULL, 1000000);
        if (res != GST_STATE_CHANGE_FAILURE && state != GST_STATE_NULL) {
            gst_x_overlay_expose(GST_X_OVERLAY(m_videoSink));
        }
    }
#endif
}

QColor QGstreamerVideoWindow::colorKey() const
{
    if (!m_colorKey.isValid()) {
        gint colorkey = 0;
        if (m_videoSink && g_object_class_find_property(G_OBJECT_GET_CLASS(m_videoSink), "colorkey"))
            g_object_get(G_OBJECT(m_videoSink), "colorkey", &colorkey, NULL);

        if (colorkey > 0)
            m_colorKey.setRgb(colorkey);
    }

    return m_colorKey;
}

void QGstreamerVideoWindow::setColorKey(const QColor &color)
{
    m_colorKey = color;

    if (m_videoSink && g_object_class_find_property(G_OBJECT_GET_CLASS(m_videoSink), "colorkey"))
        g_object_set(G_OBJECT(m_videoSink), "colorkey", color.rgba(), NULL);
}

bool QGstreamerVideoWindow::autopaintColorKey() const
{
    bool enabled = true;

    if (m_videoSink && g_object_class_find_property(G_OBJECT_GET_CLASS(m_videoSink), "autopaint-colorkey"))
        g_object_get(G_OBJECT(m_videoSink), "autopaint-colorkey", &enabled, NULL);

    return enabled;
}

void QGstreamerVideoWindow::setAutopaintColorKey(bool enabled)
{
    if (m_videoSink && g_object_class_find_property(G_OBJECT_GET_CLASS(m_videoSink), "autopaint-colorkey"))
        g_object_set(G_OBJECT(m_videoSink), "autopaint-colorkey", enabled, NULL);
}

int QGstreamerVideoWindow::brightness() const
{
    int brightness = 0;

    if (m_videoSink && g_object_class_find_property(G_OBJECT_GET_CLASS(m_videoSink), "brightness"))
        g_object_get(G_OBJECT(m_videoSink), "brightness", &brightness, NULL);

    return brightness / 10;
}

void QGstreamerVideoWindow::setBrightness(int brightness)
{
    if (m_videoSink && g_object_class_find_property(G_OBJECT_GET_CLASS(m_videoSink), "brightness")) {
        g_object_set(G_OBJECT(m_videoSink), "brightness", brightness * 10, NULL);

        emit brightnessChanged(brightness);
    }
}

int QGstreamerVideoWindow::contrast() const
{
    int contrast = 0;

    if (m_videoSink && g_object_class_find_property(G_OBJECT_GET_CLASS(m_videoSink), "contrast"))
        g_object_get(G_OBJECT(m_videoSink), "contrast", &contrast, NULL);

    return contrast / 10;
}

void QGstreamerVideoWindow::setContrast(int contrast)
{
    if (m_videoSink && g_object_class_find_property(G_OBJECT_GET_CLASS(m_videoSink), "contrast")) {
        g_object_set(G_OBJECT(m_videoSink), "contrast", contrast * 10, NULL);

        emit contrastChanged(contrast);
    }
}

int QGstreamerVideoWindow::hue() const
{
    int hue = 0;

    if (m_videoSink && g_object_class_find_property(G_OBJECT_GET_CLASS(m_videoSink), "hue"))
        g_object_get(G_OBJECT(m_videoSink), "hue", &hue, NULL);

    return hue / 10;
}

void QGstreamerVideoWindow::setHue(int hue)
{
    if (m_videoSink && g_object_class_find_property(G_OBJECT_GET_CLASS(m_videoSink), "hue")) {
        g_object_set(G_OBJECT(m_videoSink), "hue", hue * 10, NULL);

        emit hueChanged(hue);
    }
}

int QGstreamerVideoWindow::saturation() const
{
    int saturation = 0;

    if (m_videoSink && g_object_class_find_property(G_OBJECT_GET_CLASS(m_videoSink), "saturation"))
        g_object_get(G_OBJECT(m_videoSink), "saturation", &saturation, NULL);

    return saturation / 10;
}

void QGstreamerVideoWindow::setSaturation(int saturation)
{
    if (m_videoSink && g_object_class_find_property(G_OBJECT_GET_CLASS(m_videoSink), "saturation")) {
        g_object_set(G_OBJECT(m_videoSink), "saturation", saturation * 10, NULL);

        emit saturationChanged(saturation);
    }
}

bool QGstreamerVideoWindow::isFullScreen() const
{
    return m_fullScreen;
}

void QGstreamerVideoWindow::setFullScreen(bool fullScreen)
{
    emit fullScreenChanged(m_fullScreen = fullScreen);
}

QSize QGstreamerVideoWindow::nativeSize() const
{
    return m_nativeSize;
}

void QGstreamerVideoWindow::probeCaps(GstCaps *caps)
{
    QSize resolution = QGstUtils::capsCorrectedResolution(caps);
    QMetaObject::invokeMethod(
                this,
                "updateNativeVideoSize",
                Qt::QueuedConnection,
                Q_ARG(QSize, resolution));
}

void QGstreamerVideoWindow::updateNativeVideoSize(const QSize &size)
{
    if (m_nativeSize != size) {
        m_nativeSize = size;
        emit nativeSizeChanged();
    }
}

GstElement *QGstreamerVideoWindow::videoSink()
{
    return m_videoSink;
}
