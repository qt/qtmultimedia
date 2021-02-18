/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd and/or its subsidiary(-ies).
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

#include <QtCore/qvariant.h>
#include <QtCore/qdebug.h>

#include "avfcameraservice_p.h"
#include "avfcameracontrol_p.h"
#include "avfcamerasession_p.h"
#include "avfmediarecordercontrol_p.h"
#include "avfimagecapturecontrol_p.h"
#include "avfcamerarenderercontrol_p.h"
#include "avfmediarecordercontrol_p.h"
#include "avfimagecapturecontrol_p.h"
#include "avfcamerafocuscontrol_p.h"
#include "avfcameraexposurecontrol_p.h"
#include "avfcameraimageprocessingcontrol_p.h"
#include "avfcamerawindowcontrol_p.h"

#ifdef Q_OS_IOS
#include "avfmediarecordercontrol_ios_p.h"
#endif

QT_USE_NAMESPACE

AVFCameraService::AVFCameraService()
{
    m_session = new AVFCameraSession(this);
    m_cameraControl = new AVFCameraControl(this);

#ifndef Q_OS_IOS
    // This will connect a slot to 'captureModeChanged'
    // and will break viewfinder by attaching AVCaptureMovieFileOutput
    // in this slot.
    m_recorderControl = new AVFMediaRecorderControl(this);
#else
    m_recorderControl = new AVFMediaRecorderControlIOS(this);
#endif
    m_imageCaptureControl = new AVFImageCaptureControl(this);
    m_cameraFocusControl = new AVFCameraFocusControl(this);
    m_cameraImageProcessingControl = new AVFCameraImageProcessingControl(this);
    m_cameraExposureControl = nullptr;
#ifdef Q_OS_IOS
    m_cameraExposureControl = new AVFCameraExposureControl(this);
#endif

}

AVFCameraService::~AVFCameraService()
{
    m_cameraControl->setState(QCamera::UnloadedState);

#ifdef Q_OS_IOS
    delete m_recorderControl;
#endif

    //delete controls before session,
    //so they have a chance to do deinitialization
    delete m_imageCaptureControl;
    //delete m_recorderControl;
    delete m_cameraControl;
    delete m_cameraFocusControl;
    delete m_cameraExposureControl;
    delete m_cameraImageProcessingControl;

    delete m_session;
}

QCameraControl *AVFCameraService::cameraControl()
{
    return m_cameraControl;
}

QCameraImageCaptureControl *AVFCameraService::imageCaptureControl()
{
    return m_imageCaptureControl;
}

QMediaRecorderControl *AVFCameraService::mediaRecorderControl()
{
    return m_recorderControl;
}

QCameraImageProcessingControl *AVFCameraService::cameraImageProcessingControl() const
{
    return m_cameraImageProcessingControl;
}


#include "moc_avfcameraservice_p.cpp"
