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
******************************************************************************/
#include "bbcameraservice_p.h"

#include "bbcameraaudioencodersettingscontrol_p.h"
#include "bbcameracontrol_p.h"
#include "bbcameraexposurecontrol_p.h"
#include "bbcamerafocuscontrol_p.h"
#include "bbcameraimagecapturecontrol_p.h"
#include "bbcameraimageprocessingcontrol_p.h"
#include "bbcameramediarecordercontrol_p.h"
#include "bbcamerasession_p.h"
#include "bbcameravideoencodersettingscontrol_p.h"
#include "bbvideorenderercontrol_p.h"

#include <QDebug>
#include <QVariant>

QT_BEGIN_NAMESPACE

BbCameraService::BbCameraService(QObject *parent)
    : QObject(parent)
    , m_cameraSession(new BbCameraSession(this))
    , m_cameraAudioEncoderSettingsControl(new BbCameraAudioEncoderSettingsControl(m_cameraSession, this))
    , m_cameraControl(new BbCameraControl(m_cameraSession, this))
    , m_cameraExposureControl(new BbCameraExposureControl(m_cameraSession, this))
    , m_cameraFocusControl(new BbCameraFocusControl(m_cameraSession, this))
    , m_cameraImageCaptureControl(new BbCameraImageCaptureControl(m_cameraSession, this))
    , m_cameraImageProcessingControl(new BbCameraImageProcessingControl(m_cameraSession, this))
    , m_cameraMediaRecorderControl(new BbCameraMediaRecorderControl(m_cameraSession, this))
    , m_cameraVideoEncoderSettingsControl(new BbCameraVideoEncoderSettingsControl(m_cameraSession, this))
    , m_videoRendererControl(new BbVideoRendererControl(m_cameraSession, this))
{
}

BbCameraService::~BbCameraService()
{
}

QPlatformCamera *BbCameraService::camera()
{
    return m_cameraControl;
}

QPlatformImageCapture *BbCameraService::imageCapture()
{
    return m_cameraImageCaptureControl;
}

QPlatformMediaRecorder *BbCameraService::mediaRecorder()
{
    return m_cameraMediaRecorderControl;
}

void BbCameraService::setVideoPreview(QVideoSink *surface)
{
    // ####
}

QT_END_NAMESPACE
