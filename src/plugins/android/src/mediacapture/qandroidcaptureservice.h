/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Copyright (C) 2016 Ruslan Baratov
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

#ifndef QANDROIDCAPTURESERVICE_H
#define QANDROIDCAPTURESERVICE_H

#include <qmediaservice.h>
#include <qmediacontrol.h>

QT_BEGIN_NAMESPACE

class QAndroidMediaRecorderControl;
class QAndroidCaptureSession;
class QAndroidCameraControl;
class QAndroidCameraInfoControl;
class QAndroidVideoDeviceSelectorControl;
class QAndroidAudioInputSelectorControl;
class QAndroidCameraSession;
class QAndroidCameraVideoRendererControl;
class QAndroidCameraZoomControl;
class QAndroidCameraExposureControl;
class QAndroidCameraFlashControl;
class QAndroidCameraFocusControl;
class QAndroidViewfinderSettingsControl2;
class QAndroidCameraLocksControl;
class QAndroidCameraImageProcessingControl;
class QAndroidImageEncoderControl;
class QAndroidCameraImageCaptureControl;
class QAndroidCameraCaptureDestinationControl;
class QAndroidCameraCaptureBufferFormatControl;
class QAndroidAudioEncoderSettingsControl;
class QAndroidVideoEncoderSettingsControl;
class QAndroidMediaContainerControl;

class QAndroidCaptureService : public QMediaService
{
    Q_OBJECT

public:
    explicit QAndroidCaptureService(const QString &service, QObject *parent = 0);
    virtual ~QAndroidCaptureService();

    QMediaControl *requestControl(const char *name);
    void releaseControl(QMediaControl *);

private:
    QString m_service;

    QAndroidMediaRecorderControl *m_recorderControl;
    QAndroidCaptureSession *m_captureSession;
    QAndroidCameraControl *m_cameraControl;
    QAndroidCameraInfoControl *m_cameraInfoControl;
    QAndroidVideoDeviceSelectorControl *m_videoInputControl;
    QAndroidAudioInputSelectorControl *m_audioInputControl;
    QAndroidCameraSession *m_cameraSession;
    QAndroidCameraVideoRendererControl *m_videoRendererControl;
    QAndroidCameraZoomControl *m_cameraZoomControl;
    QAndroidCameraExposureControl *m_cameraExposureControl;
    QAndroidCameraFlashControl *m_cameraFlashControl;
    QAndroidCameraFocusControl *m_cameraFocusControl;
    QAndroidViewfinderSettingsControl2 *m_viewfinderSettingsControl2;
    QAndroidCameraLocksControl *m_cameraLocksControl;
    QAndroidCameraImageProcessingControl *m_cameraImageProcessingControl;
    QAndroidImageEncoderControl *m_imageEncoderControl;
    QAndroidCameraImageCaptureControl *m_imageCaptureControl;
    QAndroidCameraCaptureDestinationControl *m_captureDestinationControl;
    QAndroidCameraCaptureBufferFormatControl *m_captureBufferFormatControl;
    QAndroidAudioEncoderSettingsControl *m_audioEncoderSettingsControl;
    QAndroidVideoEncoderSettingsControl *m_videoEncoderSettingsControl;
    QAndroidMediaContainerControl *m_mediaContainerControl;
};

QT_END_NAMESPACE

#endif // QANDROIDCAPTURESERVICE_H
