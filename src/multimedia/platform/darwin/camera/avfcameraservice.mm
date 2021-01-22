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
#include "avfcamerametadatacontrol_p.h"
#include "avfmediarecordercontrol_p.h"
#include "avfimagecapturecontrol_p.h"
#include "avfcamerarenderercontrol_p.h"
#include "avfmediarecordercontrol_p.h"
#include "avfimagecapturecontrol_p.h"
#include "avfmediavideoprobecontrol_p.h"
#include "avfcamerafocuscontrol_p.h"
#include "avfcameraexposurecontrol_p.h"
#include "avfimageencodercontrol_p.h"
#include "avfaudioencodersettingscontrol_p.h"
#include "avfvideoencodersettingscontrol_p.h"
#include "avfmediacontainercontrol_p.h"
#include "avfcamerawindowcontrol_p.h"

#ifdef Q_OS_IOS
#include "avfmediarecordercontrol_ios_p.h"
#endif

QT_USE_NAMESPACE

AVFCameraService::AVFCameraService()
    : m_videoOutput(nullptr),
      m_captureWindowControl(nullptr)
{
    m_session = new AVFCameraSession(this);
    m_cameraControl = new AVFCameraControl(this);

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
    m_cameraExposureControl = nullptr;
#ifdef Q_OS_IOS
    m_cameraExposureControl = new AVFCameraExposureControl(this);
#endif

    m_imageEncoderControl = new AVFImageEncoderControl(this);
    m_audioEncoderSettingsControl = new AVFAudioEncoderSettingsControl(this);
    m_videoEncoderSettingsControl = new AVFVideoEncoderSettingsControl(this);
    m_mediaContainerControl = new AVFMediaContainerControl(this);
}

AVFCameraService::~AVFCameraService()
{
    m_cameraControl->setState(QCamera::UnloadedState);

#ifdef Q_OS_IOS
    delete m_recorderControl;
#endif

    if (m_captureWindowControl) {
        m_session->setCapturePreviewOutput(nullptr);
        delete m_captureWindowControl;
        m_captureWindowControl = nullptr;
    }

    if (m_videoOutput) {
        m_session->setVideoOutput(nullptr);
        delete m_videoOutput;
        m_videoOutput = nullptr;
    }

    //delete controls before session,
    //so they have a chance to do deinitialization
    delete m_imageCaptureControl;
    //delete m_recorderControl;
    delete m_metaDataControl;
    delete m_cameraControl;
    delete m_cameraFocusControl;
    delete m_cameraExposureControl;
    delete m_imageEncoderControl;
    delete m_audioEncoderSettingsControl;
    delete m_videoEncoderSettingsControl;
    delete m_mediaContainerControl;

    delete m_session;
}

QObject *AVFCameraService::requestControl(const char *name)
{
    if (qstrcmp(name, QCameraControl_iid) == 0)
        return m_cameraControl;

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

    if (qstrcmp(name, QImageEncoderControl_iid) == 0)
        return m_imageEncoderControl;

    if (qstrcmp(name, QAudioEncoderSettingsControl_iid) == 0)
        return m_audioEncoderSettingsControl;

    if (qstrcmp(name, QVideoEncoderSettingsControl_iid) == 0)
        return m_videoEncoderSettingsControl;

    if (qstrcmp(name, QMediaContainerControl_iid) == 0)
        return m_mediaContainerControl;

    if (qstrcmp(name,QMediaVideoProbeControl_iid) == 0) {
        AVFMediaVideoProbeControl *videoProbe = nullptr;
        videoProbe = new AVFMediaVideoProbeControl(this);
        m_session->addProbe(videoProbe);
        return videoProbe;
    }

    if (!m_captureWindowControl) {
        if (qstrcmp(name, QVideoWindowControl_iid) == 0) {
            m_captureWindowControl = new AVFCameraWindowControl(this);
            m_session->setCapturePreviewOutput(m_captureWindowControl);
            return m_captureWindowControl;
        }
    }

    if (!m_videoOutput) {
        if (qstrcmp(name, QVideoRendererControl_iid) == 0)
            m_videoOutput = new AVFCameraRendererControl(this);

        if (m_videoOutput) {
            m_session->setVideoOutput(m_videoOutput);
            return m_videoOutput;
        }
    }

    return nullptr;
}

void AVFCameraService::releaseControl(QObject *control)
{
    AVFMediaVideoProbeControl *videoProbe = qobject_cast<AVFMediaVideoProbeControl *>(control);
    if (videoProbe) {
        m_session->removeProbe(videoProbe);
        delete videoProbe;
    } else if (m_videoOutput == control) {
        m_session->setVideoOutput(nullptr);
        delete m_videoOutput;
        m_videoOutput = nullptr;
    }
    else if (m_captureWindowControl == control) {
        m_session->setCapturePreviewOutput(nullptr);
        delete m_captureWindowControl;
        m_captureWindowControl = nullptr;
    }
}


#include "moc_avfcameraservice_p.cpp"
