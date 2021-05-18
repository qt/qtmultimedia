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
//    qDebug() << "setCamera" << camera;

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
//        qDebug() << "    enabling jpeg decoder";
        gstDecode = QGstElement("jpegdec");
    } else {
//        qDebug() << "    camera delivers raw video";
        gstDecode = QGstElement("identity");
    }
    gstCameraBin.add(gstDecode);
    gstDecode.link(gstVideoConvert);

    auto caps = QGstMutableCaps::fromCameraFormat(f);
    if (!caps.isNull()) {
        if (!gstCamera.linkFiltered(gstDecode, caps))
            qWarning() << "linking filtered camera to decoder failed" << gstCamera.name() << gstDecode.name() << caps.toString();
    } else {
        if (!gstCamera.link(gstDecode))
            qWarning() << "linking camera to decoder failed" << gstCamera.name() << gstDecode.name();
    }
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

void QGstreamerCamera::setFocusMode(QCamera::FocusMode mode)
{
    if (mode == focusMode())
        return;

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
}

bool QGstreamerCamera::isFocusModeSupported(QCamera::FocusMode mode) const
{
    if (photography())
        return true;
    return mode == QCamera::FocusModeAuto;
}

void QGstreamerCamera::setFlashMode(QCamera::FlashMode mode)
{
    Q_UNUSED(mode);

    if (auto *p = photography()) {
        GstPhotographyFlashMode flashMode;
        gst_photography_get_flash_mode(p, &flashMode);

        switch (mode) {
        case QCamera::FlashAuto:
            flashMode = GST_PHOTOGRAPHY_FLASH_MODE_AUTO;
            break;
        case QCamera::FlashOff:
            flashMode = GST_PHOTOGRAPHY_FLASH_MODE_OFF;
            break;
        case QCamera::FlashOn:
            flashMode = GST_PHOTOGRAPHY_FLASH_MODE_ON;
            break;
        }

        if (gst_photography_set_flash_mode(p, flashMode))
            flashModeChanged(mode);
    }
}

bool QGstreamerCamera::isFlashModeSupported(QCamera::FlashMode mode) const
{
    if (photography())
        return true;

    return mode == QCamera::FlashAuto;
}

bool QGstreamerCamera::isFlashReady() const
{
    if (photography())
        return true;

    return false;
}

void QGstreamerCamera::setExposureMode(QCamera::ExposureMode mode)
{
    auto *p = photography();
    if (!p)
        return;

    GstPhotographySceneMode sceneMode = GST_PHOTOGRAPHY_SCENE_MODE_AUTO;

    switch (mode) {
    case QCamera::ExposureManual:
        sceneMode = GST_PHOTOGRAPHY_SCENE_MODE_MANUAL;
        break;
    case QCamera::ExposurePortrait:
        sceneMode = GST_PHOTOGRAPHY_SCENE_MODE_PORTRAIT;
        break;
    case QCamera::ExposureSports:
        sceneMode = GST_PHOTOGRAPHY_SCENE_MODE_SPORT;
        break;
    case QCamera::ExposureNight:
        sceneMode = GST_PHOTOGRAPHY_SCENE_MODE_NIGHT;
        break;
    case QCamera::ExposureAuto:
        sceneMode = GST_PHOTOGRAPHY_SCENE_MODE_AUTO;
        break;
    case QCamera::ExposureLandscape:
        sceneMode = GST_PHOTOGRAPHY_SCENE_MODE_LANDSCAPE;
        break;
    case QCamera::ExposureSnow:
        sceneMode = GST_PHOTOGRAPHY_SCENE_MODE_SNOW;
        break;
    case QCamera::ExposureBeach:
        sceneMode = GST_PHOTOGRAPHY_SCENE_MODE_BEACH;
        break;
    case QCamera::ExposureAction:
        sceneMode = GST_PHOTOGRAPHY_SCENE_MODE_ACTION;
        break;
    case QCamera::ExposureNightPortrait:
        sceneMode = GST_PHOTOGRAPHY_SCENE_MODE_NIGHT_PORTRAIT;
        break;
    case QCamera::ExposureTheatre:
        sceneMode = GST_PHOTOGRAPHY_SCENE_MODE_THEATRE;
        break;
    case QCamera::ExposureSunset:
        sceneMode = GST_PHOTOGRAPHY_SCENE_MODE_SUNSET;
        break;
    case QCamera::ExposureSteadyPhoto:
        sceneMode = GST_PHOTOGRAPHY_SCENE_MODE_STEADY_PHOTO;
        break;
    case QCamera::ExposureFireworks:
        sceneMode = GST_PHOTOGRAPHY_SCENE_MODE_FIREWORKS;
        break;
    case QCamera::ExposureParty:
        sceneMode = GST_PHOTOGRAPHY_SCENE_MODE_PARTY;
        break;
    case QCamera::ExposureCandlelight:
        sceneMode = GST_PHOTOGRAPHY_SCENE_MODE_CANDLELIGHT;
        break;
    case QCamera::ExposureBarcode:
        sceneMode = GST_PHOTOGRAPHY_SCENE_MODE_BARCODE;
        break;
    default:
        return;
    }

    if (gst_photography_set_scene_mode(p, sceneMode))
        exposureModeChanged(mode);
}

bool QGstreamerCamera::isExposureModeSupported(QCamera::ExposureMode mode) const
{
    if (photography())
        return true;

    return mode == QCamera::ExposureAuto;
}

void QGstreamerCamera::setExposureCompensation(float compensation)
{
    if (auto *p = photography()) {
        if (gst_photography_set_ev_compensation(p, compensation))
            exposureCompensationChanged(compensation);
    }
}

void QGstreamerCamera::setManualIsoSensitivity(int iso)
{
    if (auto *p = photography()) {
        if (gst_photography_set_iso_speed(p, iso))
            isoSensitivityChanged(iso);
    }
}

int QGstreamerCamera::isoSensitivity() const
{
    if (auto *p = photography()) {
        guint speed = 0;
        if (gst_photography_get_iso_speed(p, &speed))
            return speed;
    }
    return 100;
}

void QGstreamerCamera::setManualShutterSpeed(float secs)
{
    if (auto *p = photography()) {
        if (gst_photography_set_exposure(p, guint(secs*1000000)))
            shutterSpeedChanged(secs);
    }
}

float QGstreamerCamera::shutterSpeed() const
{
    if (auto *p = photography()) {
        guint32 exposure = 0;
        if (gst_photography_get_exposure(p, &exposure))
            return exposure/1000000.;
    }
    return -1;
}
#endif
