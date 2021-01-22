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
#ifndef BBCAMERASERVICE_H
#define BBCAMERASERVICE_H

#include <QObject>

#include <qmediaservice.h>

QT_BEGIN_NAMESPACE

class BbCameraAudioEncoderSettingsControl;
class BbCameraCaptureBufferFormatControl;
class BbCameraCaptureDestinationControl;
class BbCameraControl;
class BbCameraExposureControl;
class BbCameraFlashControl;
class BbCameraFocusControl;
class BbCameraImageCaptureControl;
class BbCameraImageProcessingControl;
class BbCameraInfoControl;
class BbCameraLocksControl;
class BbCameraMediaRecorderControl;
class BbCameraSession;
class BbCameraVideoEncoderSettingsControl;
class BbCameraViewfinderSettingsControl;
class BbCameraZoomControl;
class BbImageEncoderControl;
class BbVideoDeviceSelectorControl;
class BbVideoRendererControl;

class BbCameraService : public QMediaService
{
    Q_OBJECT

public:
    explicit BbCameraService(QObject *parent = 0);
    ~BbCameraService();

    virtual QMediaControl* requestControl(const char *name);
    virtual void releaseControl(QMediaControl *control);

private:
    BbCameraSession* m_cameraSession;

    BbCameraAudioEncoderSettingsControl* m_cameraAudioEncoderSettingsControl;
    BbCameraCaptureBufferFormatControl* m_cameraCaptureBufferFormatControl;
    BbCameraCaptureDestinationControl* m_cameraCaptureDestinationControl;
    BbCameraControl* m_cameraControl;
    BbCameraExposureControl* m_cameraExposureControl;
    BbCameraFlashControl* m_cameraFlashControl;
    BbCameraFocusControl* m_cameraFocusControl;
    BbCameraImageCaptureControl* m_cameraImageCaptureControl;
    BbCameraImageProcessingControl* m_cameraImageProcessingControl;
    BbCameraInfoControl* m_cameraInfoControl;
    BbCameraLocksControl* m_cameraLocksControl;
    BbCameraMediaRecorderControl* m_cameraMediaRecorderControl;
    BbCameraVideoEncoderSettingsControl* m_cameraVideoEncoderSettingsControl;
    BbCameraViewfinderSettingsControl* m_cameraViewfinderSettingsControl;
    BbCameraZoomControl* m_cameraZoomControl;
    BbImageEncoderControl* m_imageEncoderControl;
    BbVideoDeviceSelectorControl* m_videoDeviceSelectorControl;
    BbVideoRendererControl* m_videoRendererControl;
};

QT_END_NAMESPACE

#endif
