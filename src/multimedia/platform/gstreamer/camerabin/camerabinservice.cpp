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

#include <QtMultimedia/private/qtmultimediaglobal_p.h>
#include "camerabinservice.h"
#include "camerabinsession.h"
#include "camerabinrecorder.h"
#include "camerabincontainer.h"
#include "camerabinaudioencoder.h"
#include "camerabinvideoencoder.h"
#include "camerabinimageencoder.h"
#include "camerabincontrol.h"
#include "camerabinmetadata.h"

#if QT_CONFIG(gstreamer_photography)
#include "camerabinexposure.h"
#include "camerabinfocus.h"
#endif

#include "camerabinimagecapture.h"
#include "camerabinimageprocessing.h"
#include <private/qgstreamerbushelper_p.h>
#include <private/qgstutils_p.h>

    #include <private/qgstreamervideoinputdevicecontrol_p.h>

#include <private/qgstreamervideowindow_p.h>
#include <private/qgstreamervideorenderer_p.h>

#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

CameraBinService::CameraBinService(GstElementFactory *sourceFactory, QObject *parent)
    : QMediaService(parent)
{
    m_captureSession = 0;
    m_metaDataControl = 0;

    m_videoInputDevice = 0;

    m_videoOutput = 0;
    m_videoRenderer = 0;
    m_videoWindow = 0;
    m_imageCaptureControl = 0;

    m_captureSession = new CameraBinSession(sourceFactory, this);
    m_videoInputDevice = new QGstreamerVideoInputDeviceControl(sourceFactory, m_captureSession);
    m_imageCaptureControl = new CameraBinImageCapture(m_captureSession);

    connect(m_videoInputDevice, SIGNAL(selectedDeviceChanged(QString)),
            m_captureSession, SLOT(setDevice(QString)));

    if (m_videoInputDevice->deviceCount())
        m_captureSession->setDevice(m_videoInputDevice->deviceName(m_videoInputDevice->selectedDevice()));

    m_videoRenderer = new QGstreamerVideoRenderer(this);

    m_videoWindow = new QGstreamerVideoWindow(this);
    // If the GStreamer video sink is not available, don't provide the video window control since
    // it won't work anyway.
    if (!m_videoWindow->videoSink()) {
        delete m_videoWindow;
        m_videoWindow = 0;
    }

    m_metaDataControl = new CameraBinMetaData(this);
    connect(m_metaDataControl, SIGNAL(metaDataChanged(QMap<QByteArray,QVariant>)),
            m_captureSession, SLOT(setMetaData(QMap<QByteArray,QVariant>)));
}

CameraBinService::~CameraBinService()
{
}

QObject *CameraBinService::requestControl(const char *name)
{
    if (!m_captureSession)
        return 0;

    if (!m_videoOutput) {
        if (qstrcmp(name, QVideoRendererControl_iid) == 0) {
            m_videoOutput = m_videoRenderer;
        } else if (qstrcmp(name, QVideoWindowControl_iid) == 0) {
            m_videoOutput = m_videoWindow;
        }

        if (m_videoOutput) {
            m_captureSession->setViewfinder(m_videoOutput);
            return m_videoOutput;
        }
    }

    if (qstrcmp(name, QMediaVideoProbeControl_iid) == 0)
        return m_captureSession->videoProbe();

    if (qstrcmp(name,QVideoDeviceSelectorControl_iid) == 0)
        return m_videoInputDevice;

    if (qstrcmp(name,QMediaRecorderControl_iid) == 0)
        return m_captureSession->recorderControl();

    if (qstrcmp(name,QAudioEncoderSettingsControl_iid) == 0)
        return m_captureSession->audioEncodeControl();

    if (qstrcmp(name,QVideoEncoderSettingsControl_iid) == 0)
        return m_captureSession->videoEncodeControl();

    if (qstrcmp(name,QImageEncoderControl_iid) == 0)
        return m_captureSession->imageEncodeControl();


    if (qstrcmp(name,QMediaContainerControl_iid) == 0)
        return m_captureSession->mediaContainerControl();

    if (qstrcmp(name,QCameraControl_iid) == 0)
        return m_captureSession->cameraControl();

    if (qstrcmp(name,QMetaDataWriterControl_iid) == 0)
        return m_metaDataControl;

    if (qstrcmp(name, QCameraImageCaptureControl_iid) == 0)
        return m_imageCaptureControl;

#if QT_CONFIG(gstreamer_photography)
    if (qstrcmp(name, QCameraExposureControl_iid) == 0)
        return m_captureSession->cameraExposureControl();

    if (qstrcmp(name, QCameraFocusControl_iid) == 0)
        return m_captureSession->cameraFocusControl();
#endif

    if (qstrcmp(name, QCameraImageProcessingControl_iid) == 0)
        return m_captureSession->imageProcessingControl();

    return nullptr;
}

void CameraBinService::releaseControl(QObject *control)
{
    if (control && control == m_videoOutput) {
        m_videoOutput = 0;
        m_captureSession->setViewfinder(0);
    }
}

bool CameraBinService::isCameraBinAvailable()
{
    GstElementFactory *factory = gst_element_factory_find("camerabin");
    if (factory) {
        gst_object_unref(GST_OBJECT(factory));
        return true;
    }

    return false;
}

QT_END_NAMESPACE
