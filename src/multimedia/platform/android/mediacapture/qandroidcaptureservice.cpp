/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2016 Ruslan Baratov
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

#include "qandroidcaptureservice_p.h"

#include "qandroidmediarecordercontrol_p.h"
#include "qandroidcapturesession_p.h"
#include "qandroidcameracontrol_p.h"
#include "qandroidcamerasession_p.h"
#include "qandroidcameravideorenderercontrol_p.h"
#include "qandroidcameraexposurecontrol_p.h"
#include "qandroidcamerafocuscontrol_p.h"
#include "qandroidcameraimageprocessingcontrol_p.h"
#include "qandroidimageencodercontrol_p.h"
#include "qandroidcameraimagecapturecontrol_p.h"

QT_BEGIN_NAMESPACE

QAndroidCaptureService::QAndroidCaptureService(QMediaRecorder::CaptureMode mode)
    : m_videoEnabled(mode == QMediaRecorder::AudioAndVideo)
    , m_videoRendererControl(0)
{
    if (m_videoEnabled) {
        m_cameraSession = new QAndroidCameraSession;
        m_cameraControl = new QAndroidCameraControl(m_cameraSession);
        m_cameraExposureControl = new QAndroidCameraExposureControl(m_cameraSession);
        m_cameraFocusControl = new QAndroidCameraFocusControl(m_cameraSession);
        m_cameraImageProcessingControl = new QAndroidCameraImageProcessingControl(m_cameraSession);
        m_imageEncoderControl = new QAndroidImageEncoderControl(m_cameraSession);
        m_imageCaptureControl = new QAndroidCameraImageCaptureControl(m_cameraSession);
    } else {
        m_cameraSession = 0;
        m_cameraControl = 0;
        m_cameraExposureControl = 0;
        m_cameraFocusControl = 0;
        m_cameraImageProcessingControl = 0;
        m_imageEncoderControl = 0;
        m_imageCaptureControl = 0;
    }

    m_captureSession = new QAndroidCaptureSession(m_cameraSession);
    m_recorderControl = new QAndroidMediaRecorderControl(m_captureSession);
}

QAndroidCaptureService::~QAndroidCaptureService()
{
    delete m_recorderControl;
    delete m_captureSession;
    delete m_cameraControl;
    delete m_videoRendererControl;
    delete m_cameraExposureControl;
    delete m_cameraFocusControl;
    delete m_cameraImageProcessingControl;
    delete m_imageEncoderControl;
    delete m_imageCaptureControl;
    delete m_cameraSession;
}

QObject *QAndroidCaptureService::requestControl(const char *name)
{
    if (qstrcmp(name, QMediaRecorderControl_iid) == 0)
        return m_recorderControl;

    if (qstrcmp(name, QCameraControl_iid) == 0)
        return m_cameraControl;

    if (qstrcmp(name, QCameraExposureControl_iid) == 0)
        return m_cameraExposureControl;

    if (qstrcmp(name, QCameraFocusControl_iid) == 0)
        return m_cameraFocusControl;

    if (qstrcmp(name, QCameraImageProcessingControl_iid) == 0)
        return m_cameraImageProcessingControl;

    if (qstrcmp(name, QImageEncoderControl_iid) == 0)
        return m_imageEncoderControl;

    if (qstrcmp(name, QCameraImageCaptureControl_iid) == 0)
        return m_imageCaptureControl;

    if (qstrcmp(name, QVideoRendererControl_iid) == 0
            && m_videoEnabled
            && !m_videoRendererControl) {
        m_videoRendererControl = new QAndroidCameraVideoRendererControl(m_cameraSession);
        return m_videoRendererControl;
    }

    return 0;
}

void QAndroidCaptureService::releaseControl(QObject *control)
{
    if (control) {
        if (control == m_videoRendererControl) {
            delete m_videoRendererControl;
            m_videoRendererControl = 0;
            return;
        }
    }

}

QT_END_NAMESPACE
