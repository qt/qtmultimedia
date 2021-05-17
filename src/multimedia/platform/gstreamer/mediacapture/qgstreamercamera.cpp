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

#include <qcamerainfo.h>

#include "qgstreamercamera_p.h"
#include "qgstreamercameraimagecapture_p.h"
#include <private/qgstreamermediadevices_p.h>
#include <private/qgstreamerintegration_p.h>
#include <private/qgstreamercameraexposure_p.h>
#include <private/qgstreamercameraimageprocessing_p.h>
#include <qmediacapturesession.h>

#include <QtCore/qdebug.h>

QGstreamerCamera::QGstreamerCamera(QCamera *camera)
    : QPlatformCamera(camera),
      gstCameraBin("camerabin")
{
    gstCamera = QGstElement("videotestsrc");
    gstDecode = QGstElement("identity");
    gstVideoConvert = QGstElement("videoconvert", "videoConvert");
    gstVideoScale = QGstElement("videoscale", "videoScale");
    gstCameraBin.add(gstCamera, gstDecode, gstVideoConvert, gstVideoScale);
    gstCamera.link(gstDecode, gstVideoConvert, gstVideoScale);

    gstCameraBin.addGhostPad(gstVideoScale, "src");

    imageProcessing = new QGstreamerImageProcessing(this);
}

QGstreamerCamera::~QGstreamerCamera() = default;

bool QGstreamerCamera::isActive() const
{
    return m_active;
}

void QGstreamerCamera::setActive(bool active)
{
    if (m_active == active)
        return;
    if (m_cameraInfo.isNull() && active)
        return;

    m_active = active;

    statusChanged(m_active ? QCamera::ActiveStatus : QCamera::InactiveStatus);
    emit activeChanged(active);
}

void QGstreamerCamera::setCamera(const QCameraInfo &camera)
{
    if (m_cameraInfo == camera)
        return;
    qDebug() << "setCamera" << camera;

    m_cameraInfo = camera;

    bool havePipeline = !gstPipeline.isNull();

    auto state = havePipeline ? gstPipeline.state() : GST_STATE_NULL;
    if (havePipeline)
        gstPipeline.setStateSync(GST_STATE_PAUSED);

    Q_ASSERT(!gstCamera.isNull());

    gstCamera.setStateSync(GST_STATE_NULL);
    gstCameraBin.remove(gstCamera);

    if (camera.isNull()) {
        gstCamera = QGstElement("videotestsrc");
    } else {
        auto *devices = static_cast<QGstreamerMediaDevices *>(QGstreamerIntegration::instance()->devices());
        auto *device = devices->videoDevice(camera.id());
        gstCamera = gst_device_create_element(device, "camerasrc");
        QGstStructure properties = gst_device_get_properties(device);
        if (properties.name() == "v4l2deviceprovider")
            m_v4l2Device = QString::fromUtf8(properties["device.path"].toString());
    }

    gstCameraBin.add(gstCamera);
    // set the camera up with a decent format
    setCameraFormatInternal({});

    gstCamera.setStateSync(state == GST_STATE_PLAYING ? GST_STATE_PAUSED : state);

    if (havePipeline) {
        gstPipeline.dumpGraph("setCamera");
        gstPipeline.setStateSync(state);
    }

    //m_session->cameraChanged();
    imageProcessing->update();
}

void QGstreamerCamera::setCameraFormatInternal(const QCameraFormat &format)
{
    QCameraFormat f = format;
    if (f.isNull())
        f = findBestCameraFormat(m_cameraInfo);

    // add jpeg decoder where required
    gstDecode.setStateSync(GST_STATE_NULL);
    gstCameraBin.remove(gstDecode);

    if (f.pixelFormat() == QVideoFrameFormat::Format_Jpeg) {
        qDebug() << "    enabling jpeg decoder";
        gstDecode = QGstElement("jpegdec");
    } else {
        qDebug() << "    camera delivers raw video";
        gstDecode = QGstElement("identity");
    }
    gstCameraBin.add(gstDecode);
    gstDecode.link(gstVideoConvert);

    auto caps = QGstMutableCaps::fromCameraFormat(f);
    if (!gstCamera.linkFiltered(gstDecode, caps))
        qWarning() << "linking failed";
}

bool QGstreamerCamera::setCameraFormat(const QCameraFormat &format)
{
    if (!m_cameraInfo.videoFormats().contains(format))
        return false;
    bool havePipeline = !gstPipeline.isNull();
    auto state = havePipeline ? gstPipeline.state() : GST_STATE_NULL;
    if (havePipeline)
        gstPipeline.setStateSync(GST_STATE_PAUSED);
    setCameraFormatInternal(format);
    if (havePipeline)
        gstPipeline.setStateSync(state);
    return true;
}

void QGstreamerCamera::setCaptureSession(QPlatformMediaCaptureSession *session)
{
    QGstreamerMediaCapture *captureSession = static_cast<QGstreamerMediaCapture *>(session);
    if (m_session == captureSession)
        return;

    m_session = captureSession;
    // is this enough?
}

QPlatformCameraImageProcessing *QGstreamerCamera::imageProcessingControl()
{
    return imageProcessing;
}

QPlatformCameraExposure *QGstreamerCamera::exposureControl()
{
    return exposure;
}

void QGstreamerCamera::setFocusMode(QCamera::FocusMode mode)
{
    if (mode == focusMode())
        return;

#if QT_CONFIG(gstreamer_photography)
    auto p = photography();
    if (p) {
        GstPhotographyFocusMode photographyMode = GST_PHOTOGRAPHY_FOCUS_MODE_CONTINUOUS_NORMAL;

        switch (mode) {
        case QCamera::FocusModeAutoNear:
            photographyMode = GST_PHOTOGRAPHY_FOCUS_MODE_MACRO;
            break;
        case QCamera::FocusModeAutoFar:
            // not quite, but hey :)
            Q_FALLTHROUGH();
        case QCamera::FocusModeHyperfocal:
            photographyMode = GST_PHOTOGRAPHY_FOCUS_MODE_HYPERFOCAL;
            break;
        case QCamera::FocusModeInfinity:
            photographyMode = GST_PHOTOGRAPHY_FOCUS_MODE_INFINITY;
            break;
        case QCamera::FocusModeManual:
            photographyMode = GST_PHOTOGRAPHY_FOCUS_MODE_MANUAL;
            break;
        default: // QCamera::FocusModeAuto:
            break;
        }

        if (gst_photography_set_focus_mode(p, photographyMode))
            focusModeChanged(mode);
    }
#endif
}

bool QGstreamerCamera::isFocusModeSupported(QCamera::FocusMode mode) const
{
    Q_UNUSED(mode);

#if QT_CONFIG(gstreamer_photography)
    if (photography())
        return true;
#endif

    return false;
}

GstColorBalance *QGstreamerCamera::colorBalance() const
{
    if (!gstCamera.isNull() && GST_IS_COLOR_BALANCE(gstCamera.element()))
        return GST_COLOR_BALANCE(gstCamera.element());
    // ### Add support for manual/SW color balancing using the gstreamer colorbalance element
    return nullptr;
}

#if QT_CONFIG(gstreamer_photography)
GstPhotography *QGstreamerCamera::photography() const
{
    if (!gstCamera.isNull() && GST_IS_PHOTOGRAPHY(gstCamera.element()))
        return GST_PHOTOGRAPHY(gstCamera.element());
    return nullptr;
}
#endif
