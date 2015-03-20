/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd and/or its subsidiary(-ies).
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/qvariant.h>
#include <QtCore/qdebug.h>
#include <QtCore/qsysinfo.h>

#include "avfcameraservice.h"
#include "avfcameracontrol.h"
#include "avfcamerainfocontrol.h"
#include "avfcamerasession.h"
#include "avfcameradevicecontrol.h"
#include "avfaudioinputselectorcontrol.h"
#include "avfcamerametadatacontrol.h"
#include "avfmediarecordercontrol.h"
#include "avfimagecapturecontrol.h"
#include "avfcamerarenderercontrol.h"
#include "avfmediarecordercontrol.h"
#include "avfimagecapturecontrol.h"
#include "avfmediavideoprobecontrol.h"
#include "avfcamerafocuscontrol.h"
#include "avfcameraexposurecontrol.h"
#include "avfcameraviewfindersettingscontrol.h"
#include "avfimageencodercontrol.h"
#include "avfcameraflashcontrol.h"

#ifdef Q_OS_IOS
#include "avfcamerazoomcontrol.h"
#include "avfmediarecordercontrol_ios.h"
#endif

#include <private/qmediaplaylistnavigator_p.h>
#include <qmediaplaylist.h>

QT_USE_NAMESPACE

AVFCameraService::AVFCameraService(QObject *parent):
    QMediaService(parent),
    m_videoOutput(0)
{
    m_session = new AVFCameraSession(this);
    m_cameraControl = new AVFCameraControl(this);
    m_cameraInfoControl = new AVFCameraInfoControl(this);
    m_videoDeviceControl = new AVFCameraDeviceControl(this);
    m_audioInputSelectorControl = new AVFAudioInputSelectorControl(this);

    m_metaDataControl = new AVFCameraMetaDataControl(this);
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
    m_cameraExposureControl = 0;
#if defined(Q_OS_IOS) && QT_IOS_PLATFORM_SDK_EQUAL_OR_ABOVE(__IPHONE_8_0)
    if (QSysInfo::MacintoshVersion >= QSysInfo::MV_IOS_8_0)
        m_cameraExposureControl = new AVFCameraExposureControl(this);
#endif

    m_cameraZoomControl = 0;
#ifdef Q_OS_IOS
    m_cameraZoomControl = new AVFCameraZoomControl(this);
#endif
    m_viewfinderSettingsControl2 = new AVFCameraViewfinderSettingsControl2(this);
    m_viewfinderSettingsControl = new AVFCameraViewfinderSettingsControl(this);
    m_imageEncoderControl = new AVFImageEncoderControl(this);
    m_flashControl = new AVFCameraFlashControl(this);
}

AVFCameraService::~AVFCameraService()
{
    m_cameraControl->setState(QCamera::UnloadedState);

#ifdef Q_OS_IOS
    delete m_recorderControl;
#endif

    if (m_videoOutput) {
        m_session->setVideoOutput(0);
        delete m_videoOutput;
        m_videoOutput = 0;
    }

    //delete controls before session,
    //so they have a chance to do deinitialization
    delete m_imageCaptureControl;
    //delete m_recorderControl;
    delete m_metaDataControl;
    delete m_cameraControl;
    delete m_cameraFocusControl;
    delete m_cameraExposureControl;
#ifdef Q_OS_IOS
    delete m_cameraZoomControl;
#endif
    delete m_viewfinderSettingsControl2;
    delete m_viewfinderSettingsControl;
    delete m_imageEncoderControl;
    delete m_flashControl;

    delete m_session;
}

QMediaControl *AVFCameraService::requestControl(const char *name)
{
    if (qstrcmp(name, QCameraControl_iid) == 0)
        return m_cameraControl;

    if (qstrcmp(name, QCameraInfoControl_iid) == 0)
        return m_cameraInfoControl;

    if (qstrcmp(name, QVideoDeviceSelectorControl_iid) == 0)
        return m_videoDeviceControl;

    if (qstrcmp(name, QAudioInputSelectorControl_iid) == 0)
        return m_audioInputSelectorControl;

    //metadata support is not implemented yet
    //if (qstrcmp(name, QMetaDataWriterControl_iid) == 0)
    //    return m_metaDataControl;

    if (qstrcmp(name, QMediaRecorderControl_iid) == 0)
        return m_recorderControl;

    if (qstrcmp(name, QCameraImageCaptureControl_iid) == 0)
        return m_imageCaptureControl;

    if (qstrcmp(name, QCameraExposureControl_iid) == 0)
        return m_cameraExposureControl;

    if (qstrcmp(name, QCameraFocusControl_iid) == 0)
        return m_cameraFocusControl;

    if (qstrcmp(name, QCameraViewfinderSettingsControl2_iid) == 0)
        return m_viewfinderSettingsControl2;

    if (qstrcmp(name, QCameraViewfinderSettingsControl_iid) == 0)
        return m_viewfinderSettingsControl;

    if (qstrcmp(name, QImageEncoderControl_iid) == 0)
        return m_imageEncoderControl;

    if (qstrcmp(name, QCameraFlashControl_iid) == 0)
        return m_flashControl;

    if (qstrcmp(name,QMediaVideoProbeControl_iid) == 0) {
        AVFMediaVideoProbeControl *videoProbe = 0;
        videoProbe = new AVFMediaVideoProbeControl(this);
        m_session->addProbe(videoProbe);
        return videoProbe;
    }

#ifdef Q_OS_IOS
    if (qstrcmp(name, QCameraZoomControl_iid) == 0)
        return m_cameraZoomControl;
#endif

    if (!m_videoOutput) {
        if (qstrcmp(name, QVideoRendererControl_iid) == 0)
            m_videoOutput = new AVFCameraRendererControl(this);

        if (m_videoOutput) {
            m_session->setVideoOutput(m_videoOutput);
            return m_videoOutput;
        }
    }

    return 0;
}

void AVFCameraService::releaseControl(QMediaControl *control)
{
    if (m_videoOutput == control) {
        m_session->setVideoOutput(0);
        delete m_videoOutput;
        m_videoOutput = 0;
    }
    AVFMediaVideoProbeControl *videoProbe = qobject_cast<AVFMediaVideoProbeControl *>(control);
    if (videoProbe) {
        m_session->removeProbe(videoProbe);
        delete videoProbe;
        return;
    }

}

AVFMediaRecorderControl *AVFCameraService::recorderControl() const
{
#ifdef Q_OS_IOS
    return 0;
#else
    return static_cast<AVFMediaRecorderControl *>(m_recorderControl);
#endif
}

AVFMediaRecorderControlIOS *AVFCameraService::recorderControlIOS() const
{
#ifdef Q_OS_OSX
    return 0;
#else
    return static_cast<AVFMediaRecorderControlIOS *>(m_recorderControl);
#endif
}


#include "moc_avfcameraservice.cpp"
