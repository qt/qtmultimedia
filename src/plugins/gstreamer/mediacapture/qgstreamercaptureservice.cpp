/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qgstreamercaptureservice.h"
#include "qgstreamercapturesession.h"
#include "qgstreamerrecordercontrol.h"
#include "qgstreamermediacontainercontrol.h"
#include "qgstreameraudioencode.h"
#include "qgstreamervideoencode.h"
#include "qgstreamerimageencode.h"
#include "qgstreamercameracontrol.h"
#include <private/qgstreamerbushelper_p.h>
#include "qgstreamerv4l2input.h"
#include "qgstreamercapturemetadatacontrol.h"

#include "qgstreameraudioinputendpointselector.h"
#include "qgstreamervideoinputdevicecontrol.h"
#include "qgstreamerimagecapturecontrol.h"

#include "qgstreamervideorenderer.h"

#if defined(HAVE_WIDGETS)
#include "qgstreamervideooverlay.h"
#include "qgstreamervideowidget.h"
#endif

#include <qmediaserviceprovider.h>

QT_BEGIN_NAMESPACE

QGstreamerCaptureService::QGstreamerCaptureService(const QString &service, QObject *parent):
    QMediaService(parent)
{
    m_captureSession = 0;
    m_cameraControl = 0;
    m_metaDataControl = 0;

    m_videoInput = 0;
    m_audioInputEndpointSelector = 0;
    m_videoInputDevice = 0;

    m_videoOutput = 0;
    m_videoRenderer = 0;
#if defined(HAVE_XVIDEO) && defined(HAVE_WIDGETS)
    m_videoWindow = 0;
    m_videoWidgetControl = 0;
#endif
    m_imageCaptureControl = 0;

    if (service == Q_MEDIASERVICE_AUDIOSOURCE) {
        m_captureSession = new QGstreamerCaptureSession(QGstreamerCaptureSession::Audio, this);
    }

   if (service == Q_MEDIASERVICE_CAMERA) {
        m_captureSession = new QGstreamerCaptureSession(QGstreamerCaptureSession::AudioAndVideo, this);
        m_cameraControl = new QGstreamerCameraControl(m_captureSession);
        m_videoInput = new QGstreamerV4L2Input(this);
        m_captureSession->setVideoInput(m_videoInput);
        m_videoInputDevice = new QGstreamerVideoInputDeviceControl(this);

        connect(m_videoInputDevice, SIGNAL(selectedDeviceChanged(QString)),
                m_videoInput, SLOT(setDevice(QString)));

        if (m_videoInputDevice->deviceCount())
            m_videoInput->setDevice(m_videoInputDevice->deviceName(m_videoInputDevice->selectedDevice()));

        m_videoRenderer = new QGstreamerVideoRenderer(this);

#if defined(HAVE_XVIDEO) && defined(HAVE_WIDGETS)
        m_videoWindow = new QGstreamerVideoOverlay(this);
        m_videoWidgetControl = new QGstreamerVideoWidgetControl(this);
#endif
        m_imageCaptureControl = new QGstreamerImageCaptureControl(m_captureSession);
    }

    m_audioInputEndpointSelector = new QGstreamerAudioInputEndpointSelector(this);
    connect(m_audioInputEndpointSelector, SIGNAL(activeEndpointChanged(QString)), m_captureSession, SLOT(setCaptureDevice(QString)));

    if (m_captureSession && m_audioInputEndpointSelector->availableEndpoints().size() > 0)
        m_captureSession->setCaptureDevice(m_audioInputEndpointSelector->defaultEndpoint());

    m_metaDataControl = new QGstreamerCaptureMetaDataControl(this);
    connect(m_metaDataControl, SIGNAL(metaDataChanged(QMap<QByteArray,QVariant>)),
            m_captureSession, SLOT(setMetaData(QMap<QByteArray,QVariant>)));
}

QGstreamerCaptureService::~QGstreamerCaptureService()
{
}

QMediaControl *QGstreamerCaptureService::requestControl(const char *name)
{
    if (!m_captureSession)
        return 0;

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

    if (!m_videoOutput) {
        if (qstrcmp(name, QVideoRendererControl_iid) == 0) {
            m_videoOutput = m_videoRenderer;
        }
#if defined(HAVE_WIDGETS) && defined(HAVE_XVIDEO)
        else if (qstrcmp(name, QVideoWindowControl_iid) == 0) {
            m_videoOutput = m_videoWindow;
        } else if (qstrcmp(name, QVideoWidgetControl_iid) == 0) {
            m_videoOutput = m_videoWidgetControl;
        }
#endif

        if (m_videoOutput) {
            m_captureSession->setVideoPreview(m_videoOutput);
            return m_videoOutput;
        }
    }

    return 0;
}

void QGstreamerCaptureService::releaseControl(QMediaControl *control)
{
    if (control && control == m_videoOutput) {
        m_videoOutput = 0;
        m_captureSession->setVideoPreview(0);
    }
}

QT_END_NAMESPACE
