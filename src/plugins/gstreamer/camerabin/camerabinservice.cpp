/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "camerabinservice.h"
#include "camerabinsession.h"
#include "camerabinrecorder.h"
#include "camerabincontainer.h"
#include "camerabinaudioencoder.h"
#include "camerabinvideoencoder.h"
#include "camerabinimageencoder.h"
#include "qgstreamerbushelper.h"
#include "camerabincontrol.h"
#include "camerabinlocks.h"
#include "camerabinmetadata.h"
#include "camerabinexposure.h"
#include "camerabinflash.h"
#include "camerabinfocus.h"
#include "camerabinimagecapture.h"
#include "camerabinimageprocessing.h"
#include "camerabincapturebufferformat.h"
#include "camerabincapturedestination.h"

#include "qgstreameraudioinputendpointselector.h"
#include "qgstreamervideoinputdevicecontrol.h"

#include "qgstreamervideooverlay.h"
#include "qgstreamervideowindow.h"
#include "qgstreamervideorenderer.h"

#if defined(Q_WS_MAEMO_6) && defined(__arm__)
#include "qgstreamergltexturerenderer.h"
#endif

#include "qgstreamervideowidget.h"

#include <qmediaserviceprovider.h>

#include <QtCore/qdebug.h>
#include <QtCore/qprocess.h>

#if defined(Q_WS_MAEMO_6)
#include "camerabuttonlistener_meego.h"
#endif

CameraBinService::CameraBinService(const QString &service, QObject *parent):
    QMediaService(parent)
{
    m_captureSession = 0;
    m_cameraControl = 0;
    m_metaDataControl = 0;

    m_audioInputEndpointSelector = 0;
    m_videoInputDevice = 0;

    m_videoOutput = 0;
    m_videoRenderer = 0;
    m_videoWindow = 0;
    m_videoWidgetControl = 0;
    m_imageCaptureControl = 0;   

    if (service == Q_MEDIASERVICE_CAMERA) {
        m_captureSession = new CameraBinSession(this);
        m_cameraControl = new CameraBinControl(m_captureSession);
        m_videoInputDevice = new QGstreamerVideoInputDeviceControl(m_captureSession);
        m_imageCaptureControl = new CameraBinImageCapture(m_captureSession);

        connect(m_videoInputDevice, SIGNAL(selectedDeviceChanged(QString)),
                m_captureSession, SLOT(setDevice(QString)));

        if (m_videoInputDevice->deviceCount())
            m_captureSession->setDevice(m_videoInputDevice->deviceName(m_videoInputDevice->selectedDevice()));        

#if defined(Q_WS_MAEMO_6) && defined(__arm__)
        m_videoRenderer = new QGstreamerGLTextureRenderer(this);
#else
        m_videoRenderer = new QGstreamerVideoRenderer(this);
#endif


#ifdef Q_WS_MAEMO_6
        m_videoWindow = new QGstreamerVideoWindow(this, "omapxvsink");
        //m_videoWindow = new QGstreamerVideoWindow(this);
#else
        m_videoWindow = new QGstreamerVideoOverlay(this);
#endif

        m_videoWidgetControl = new QGstreamerVideoWidgetControl(this);

    }
    
    if (!m_captureSession) {
        qWarning() << Q_FUNC_INFO << "Service type is not supported:" << service;
        return;
    }

    m_audioInputEndpointSelector = new QGstreamerAudioInputEndpointSelector(this);
    connect(m_audioInputEndpointSelector, SIGNAL(activeEndpointChanged(QString)), m_captureSession, SLOT(setCaptureDevice(QString)));

    if (m_captureSession && m_audioInputEndpointSelector->availableEndpoints().size() > 0)
        m_captureSession->setCaptureDevice(m_audioInputEndpointSelector->defaultEndpoint());

    m_metaDataControl = new CameraBinMetaData(this);
    connect(m_metaDataControl, SIGNAL(metaDataChanged(QMap<QByteArray,QVariant>)),
            m_captureSession, SLOT(setMetaData(QMap<QByteArray,QVariant>)));

#if defined(Q_WS_MAEMO_6)
    new CameraButtonListener(this);
#endif
}

CameraBinService::~CameraBinService()
{
}

QMediaControl *CameraBinService::requestControl(const char *name)
{
    if (!m_captureSession)
        return 0;

    //qDebug() << "Request control" << name;

    if (!m_videoOutput) {
        if (qstrcmp(name, QVideoRendererControl_iid) == 0) {
            m_videoOutput = m_videoRenderer;
        } else if (qstrcmp(name, QVideoWindowControl_iid) == 0) {
            m_videoOutput = m_videoWindow;
        } else if (qstrcmp(name, QVideoWidgetControl_iid) == 0) {
            m_videoOutput = m_videoWidgetControl;
        }

        if (m_videoOutput) {
            m_captureSession->setViewfinder(m_videoOutput);
            return m_videoOutput;
        }
    }

    if (qstrcmp(name,QAudioEndpointSelector_iid) == 0)
        return m_audioInputEndpointSelector;

    if (qstrcmp(name,QVideoDeviceControl_iid) == 0)
        return m_videoInputDevice;

    if (qstrcmp(name,QMediaRecorderControl_iid) == 0)
        return m_captureSession->recorderControl();

    if (qstrcmp(name,QAudioEncoderControl_iid) == 0)
        return m_captureSession->audioEncodeControl();

    if (qstrcmp(name,QVideoEncoderControl_iid) == 0)
        return m_captureSession->videoEncodeControl();

    if (qstrcmp(name,QImageEncoderControl_iid) == 0)
        return m_captureSession->imageEncodeControl();


    if (qstrcmp(name,QMediaContainerControl_iid) == 0)
        return m_captureSession->mediaContainerControl();

    if (qstrcmp(name,QCameraControl_iid) == 0)
        return m_cameraControl;

    if (qstrcmp(name,QMetaDataWriterControl_iid) == 0)
        return m_metaDataControl;

    if (qstrcmp(name, QCameraImageCaptureControl_iid) == 0)
        return m_imageCaptureControl;

    if (qstrcmp(name, QCameraExposureControl_iid) == 0)
        return m_captureSession->cameraExposureControl();

    if (qstrcmp(name, QCameraFlashControl_iid) == 0)
        return m_captureSession->cameraFlashControl();

    if (qstrcmp(name, QCameraFocusControl_iid) == 0)
        return m_captureSession->cameraFocusControl();

    if (qstrcmp(name, QCameraImageProcessingControl_iid) == 0)
        return m_captureSession->imageProcessingControl();

    if (qstrcmp(name, QCameraLocksControl_iid) == 0)
        return m_captureSession->cameraLocksControl();

    if (qstrcmp(name, QCameraCaptureDestinationControl_iid) == 0)
        return m_captureSession->captureDestinationControl();

    if (qstrcmp(name, QCameraCaptureBufferFormatControl_iid) == 0)
        return m_captureSession->captureBufferFormatControl();

    return 0;
}

void CameraBinService::releaseControl(QMediaControl *control)
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
