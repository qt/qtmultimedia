/****************************************************************************
**
** Copyright (C) 2016 Research In Motion
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/
#include "bbcameraservice.h"

#include "bbcameraaudioencodersettingscontrol.h"
#include "bbcameracapturebufferformatcontrol.h"
#include "bbcameracapturedestinationcontrol.h"
#include "bbcameracontrol.h"
#include "bbcameraexposurecontrol.h"
#include "bbcameraflashcontrol.h"
#include "bbcamerafocuscontrol.h"
#include "bbcameraimagecapturecontrol.h"
#include "bbcameraimageprocessingcontrol.h"
#include "bbcamerainfocontrol.h"
#include "bbcameralockscontrol.h"
#include "bbcameramediarecordercontrol.h"
#include "bbcamerasession.h"
#include "bbcameravideoencodersettingscontrol.h"
#include "bbcameraviewfindersettingscontrol.h"
#include "bbcamerazoomcontrol.h"
#include "bbimageencodercontrol.h"
#include "bbvideodeviceselectorcontrol.h"
#include "bbvideorenderercontrol.h"

#include <QDebug>
#include <QVariant>

QT_BEGIN_NAMESPACE

BbCameraService::BbCameraService(QObject *parent)
    : QMediaService(parent)
    , m_cameraSession(new BbCameraSession(this))
    , m_cameraAudioEncoderSettingsControl(new BbCameraAudioEncoderSettingsControl(m_cameraSession, this))
    , m_cameraCaptureBufferFormatControl(new BbCameraCaptureBufferFormatControl(this))
    , m_cameraCaptureDestinationControl(new BbCameraCaptureDestinationControl(m_cameraSession, this))
    , m_cameraControl(new BbCameraControl(m_cameraSession, this))
    , m_cameraExposureControl(new BbCameraExposureControl(m_cameraSession, this))
    , m_cameraFlashControl(new BbCameraFlashControl(m_cameraSession, this))
    , m_cameraFocusControl(new BbCameraFocusControl(m_cameraSession, this))
    , m_cameraImageCaptureControl(new BbCameraImageCaptureControl(m_cameraSession, this))
    , m_cameraImageProcessingControl(new BbCameraImageProcessingControl(m_cameraSession, this))
    , m_cameraInfoControl(new BbCameraInfoControl(this))
    , m_cameraLocksControl(new BbCameraLocksControl(m_cameraSession, this))
    , m_cameraMediaRecorderControl(new BbCameraMediaRecorderControl(m_cameraSession, this))
    , m_cameraVideoEncoderSettingsControl(new BbCameraVideoEncoderSettingsControl(m_cameraSession, this))
    , m_cameraViewfinderSettingsControl(new BbCameraViewfinderSettingsControl(m_cameraSession, this))
    , m_cameraZoomControl(new BbCameraZoomControl(m_cameraSession, this))
    , m_imageEncoderControl(new BbImageEncoderControl(m_cameraSession, this))
    , m_videoDeviceSelectorControl(new BbVideoDeviceSelectorControl(m_cameraSession, this))
    , m_videoRendererControl(new BbVideoRendererControl(m_cameraSession, this))
{
}

BbCameraService::~BbCameraService()
{
}

QMediaControl* BbCameraService::requestControl(const char *name)
{
    if (qstrcmp(name, QAudioEncoderSettingsControl_iid) == 0)
        return m_cameraAudioEncoderSettingsControl;
    else if (qstrcmp(name, QCameraCaptureBufferFormatControl_iid) == 0)
        return m_cameraCaptureBufferFormatControl;
    else if (qstrcmp(name, QCameraCaptureDestinationControl_iid) == 0)
        return m_cameraCaptureDestinationControl;
    else if (qstrcmp(name, QCameraControl_iid) == 0)
        return m_cameraControl;
    else if (qstrcmp(name, QCameraInfoControl_iid) == 0)
        return m_cameraInfoControl;
    else if (qstrcmp(name, QCameraExposureControl_iid) == 0)
        return m_cameraExposureControl;
    else if (qstrcmp(name, QCameraFlashControl_iid) == 0)
        return m_cameraFlashControl;
    else if (qstrcmp(name, QCameraFocusControl_iid) == 0)
        return m_cameraFocusControl;
    else if (qstrcmp(name, QCameraImageCaptureControl_iid) == 0)
        return m_cameraImageCaptureControl;
    else if (qstrcmp(name, QCameraImageProcessingControl_iid) == 0)
        return m_cameraImageProcessingControl;
    else if (qstrcmp(name, QCameraLocksControl_iid) == 0)
        return m_cameraLocksControl;
    else if (qstrcmp(name, QMediaRecorderControl_iid) == 0)
        return m_cameraMediaRecorderControl;
    else if (qstrcmp(name, QVideoEncoderSettingsControl_iid) == 0)
        return m_cameraVideoEncoderSettingsControl;
    else if (qstrcmp(name, QCameraViewfinderSettingsControl_iid) == 0)
        return m_cameraViewfinderSettingsControl;
    else if (qstrcmp(name, QCameraZoomControl_iid) == 0)
        return m_cameraZoomControl;
    else if (qstrcmp(name, QImageEncoderControl_iid) == 0)
        return m_imageEncoderControl;
    else if (qstrcmp(name, QVideoDeviceSelectorControl_iid) == 0)
        return m_videoDeviceSelectorControl;
    else if (qstrcmp(name, QVideoRendererControl_iid) == 0)
        return m_videoRendererControl;

    return 0;
}

void BbCameraService::releaseControl(QMediaControl *control)
{
    Q_UNUSED(control)

    // Implemented as a singleton, so we do nothing.
}

QT_END_NAMESPACE
