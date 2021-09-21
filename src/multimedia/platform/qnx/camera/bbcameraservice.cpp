/****************************************************************************
**
** Copyright (C) 2016 Research In Motion
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
