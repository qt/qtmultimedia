/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Mobility Components.
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

#include <QtCore/qvariant.h>
#include <QtGui/qwidget.h>
#include <QtCore/qlist.h>

#include "s60cameraservice.h"
#include "s60cameracontrol.h"
#include "s60videodevicecontrol.h"
#include "s60camerafocuscontrol.h"
#include "s60cameraexposurecontrol.h"
#include "s60cameraflashcontrol.h"
#include "s60cameraimageprocessingcontrol.h"
#include "s60cameraimagecapturecontrol.h"
#include "s60mediarecordercontrol.h"
#include "s60videocapturesession.h"
#include "s60imagecapturesession.h"
#include "s60videowidgetcontrol.h"
#include "s60mediacontainercontrol.h"
#include "s60videoencodercontrol.h"
#include "s60audioencodercontrol.h"
#include "s60imageencodercontrol.h"
#include "s60cameralockscontrol.h"
#include "s60videorenderercontrol.h"
#include "s60videowindowcontrol.h"

#include "s60cameraviewfinderengine.h" // ViewfinderOutputType

S60CameraService::S60CameraService(QObject *parent) :
    QMediaService(parent)
{
    // Session classes for video and image capturing
    m_imagesession = new S60ImageCaptureSession(this);
    m_videosession = new S60VideoCaptureSession(this);

    if (m_imagesession && m_videosession) {
        // Different control classes implementing the Camera API
        m_control = new S60CameraControl(m_videosession, m_imagesession, this);
        m_videoDeviceControl = new S60VideoDeviceControl(m_control, this);
        m_focusControl = new S60CameraFocusControl(m_imagesession, this);
        m_exposureControl = new S60CameraExposureControl(m_imagesession, this);
        m_flashControl = new S60CameraFlashControl(m_imagesession, this);
        m_imageProcessingControl = new S60CameraImageProcessingControl(m_imagesession, this);
        m_imageCaptureControl = new S60CameraImageCaptureControl(this, m_imagesession, this);
        m_media = new S60MediaRecorderControl(this, m_videosession, this);
        m_mediaFormat = new S60MediaContainerControl(m_videosession, this);
        m_videoEncoder = new S60VideoEncoderControl(m_videosession, this);
        m_audioEncoder = new S60AudioEncoderControl(m_videosession, this);
        m_viewFinderWidget = new S60VideoWidgetControl(this);
        m_imageEncoderControl = new S60ImageEncoderControl(m_imagesession, this);
        m_locksControl = new S60CameraLocksControl(this, m_imagesession, this);
        m_rendererControl = new S60VideoRendererControl(this);
        m_windowControl = new S60VideoWindowControl(this);
    }
}

S60CameraService::~S60CameraService()
{
    // Delete controls
    if (m_videoDeviceControl)
        delete m_videoDeviceControl;
    if (m_focusControl)
        delete m_focusControl;
    if (m_exposureControl)
        delete m_exposureControl;
    if (m_flashControl)
        delete m_flashControl;
    if (m_imageProcessingControl)
        delete m_imageProcessingControl;
    if (m_imageCaptureControl)
        delete m_imageCaptureControl;
    if (m_media)
        delete m_media;
    if (m_mediaFormat)
        delete m_mediaFormat;
    if (m_videoEncoder)
        delete m_videoEncoder;
    if (m_audioEncoder)
        delete m_audioEncoder;
    if (m_imageEncoderControl)
        delete m_imageEncoderControl;
    if (m_locksControl)
        delete m_locksControl;

    // CameraControl destroys:
    // * ViewfinderEngine
    // * CameraEngine
    if (m_control)
        delete m_control;

    // Delete viewfinder controls after CameraControl to be sure that
    // ViewFinder gets stopped before widget (and window) is destroyed
    if (m_viewFinderWidget)
        delete m_viewFinderWidget;
    if (m_rendererControl)
        delete m_rendererControl;
    if (m_windowControl)
        delete m_windowControl;

    // Delete sessions
    if (m_videosession)
        delete m_videosession;
    if (m_imagesession)
        delete m_imagesession;
}

QMediaControl *S60CameraService::requestControl(const char *name)
{
    if (qstrcmp(name, QMediaRecorderControl_iid) == 0)
        return m_media;

    if (qstrcmp(name, QCameraControl_iid) == 0)
        return m_control;

    if (qstrcmp(name, QVideoEncoderControl_iid) == 0)
        return m_videoEncoder;

    if (qstrcmp(name, QAudioEncoderControl_iid) == 0)
        return m_audioEncoder;

    if (qstrcmp(name, QMediaContainerControl_iid) == 0)
        return m_mediaFormat;

    if (qstrcmp(name, QCameraExposureControl_iid) == 0)
        return m_exposureControl;

    if (qstrcmp(name, QCameraFlashControl_iid) == 0)
        return m_flashControl;

    if (qstrcmp(name, QVideoWidgetControl_iid) == 0) {
        if (m_viewFinderWidget) {
            m_control->setVideoOutput(m_viewFinderWidget,
                                      S60CameraViewfinderEngine::OutputTypeVideoWidget);
            return m_viewFinderWidget;
        }
        else
            return 0;
    }

    if (qstrcmp(name, QVideoRendererControl_iid) == 0) {
        if (m_rendererControl) {
            m_control->setVideoOutput(m_rendererControl,
                                      S60CameraViewfinderEngine::OutputTypeRenderer);
            return m_rendererControl;
        }
        else
            return 0;
    }

    if (qstrcmp(name, QVideoWindowControl_iid) == 0) {
        if (m_windowControl) {
            m_control->setVideoOutput(m_windowControl,
                                      S60CameraViewfinderEngine::OutputTypeVideoWindow);
            return m_windowControl;
        }
        else
            return 0;
    }


    if (qstrcmp(name, QCameraFocusControl_iid) == 0)
        return m_focusControl;

    if (qstrcmp(name, QCameraImageProcessingControl_iid) == 0)
        return m_imageProcessingControl;

    if (qstrcmp(name, QCameraImageCaptureControl_iid) == 0)
        return m_imageCaptureControl;

    if (qstrcmp(name, QVideoDeviceControl_iid) == 0)
        return m_videoDeviceControl;

    if (qstrcmp(name, QImageEncoderControl_iid) == 0)
        return m_imageEncoderControl;

    if (qstrcmp(name, QCameraLocksControl_iid) == 0)
        return m_locksControl;

    return 0;
}

void S60CameraService::releaseControl(QMediaControl *control)
{
    if (control == 0)
        return;

    // Release viewfinder output
    if (control == m_viewFinderWidget) {
        if (m_viewFinderWidget)
            m_control->releaseVideoOutput(S60CameraViewfinderEngine::OutputTypeVideoWidget);
    }

    if (control == m_rendererControl) {
        if (m_rendererControl)
            m_control->releaseVideoOutput(S60CameraViewfinderEngine::OutputTypeRenderer);
    }

    if (control == m_windowControl) {
        if (m_windowControl)
            m_control->releaseVideoOutput(S60CameraViewfinderEngine::OutputTypeVideoWindow);
    }
}

int S60CameraService::deviceCount()
{
    return S60CameraControl::deviceCount();
}

QString S60CameraService::deviceDescription(const int index)
{
    return S60CameraControl::description(index);
}

QString S60CameraService::deviceName(const int index)
{
    return S60CameraControl::name(index);
}

// End of file

