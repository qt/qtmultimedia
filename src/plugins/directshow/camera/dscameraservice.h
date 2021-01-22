/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#ifndef DSCAMERASERVICE_H
#define DSCAMERASERVICE_H

#include <QtCore/qobject.h>

#include <qmediaservice.h>

QT_BEGIN_NAMESPACE

class DSCameraControl;
class DSCameraSession;
class DSVideoDeviceControl;
class DSImageCaptureControl;
class DSCameraViewfinderSettingsControl;
class DSCameraImageProcessingControl;
class DirectShowCameraExposureControl;
class DirectShowCameraCaptureDestinationControl;
class DirectShowCameraCaptureBufferFormatControl;
class DirectShowVideoProbeControl;
class DirectShowCameraZoomControl;
class DirectShowCameraImageEncoderControl;

class DSCameraService : public QMediaService
{
    Q_OBJECT

public:
    DSCameraService(QObject *parent = nullptr);
    ~DSCameraService() override;

    QMediaControl* requestControl(const char *name) override;
    void releaseControl(QMediaControl *control) override;

private:
    DSCameraSession        *m_session;
    DSCameraControl        *m_control;
    DSVideoDeviceControl   *m_videoDevice;
    QMediaControl          *m_videoRenderer = nullptr;
    DSImageCaptureControl  *m_imageCapture;
    DSCameraViewfinderSettingsControl *m_viewfinderSettings;
    DSCameraImageProcessingControl *m_imageProcessingControl;
    DirectShowCameraExposureControl *m_exposureControl;
    DirectShowCameraCaptureDestinationControl *m_captureDestinationControl;
    DirectShowCameraCaptureBufferFormatControl *m_captureBufferFormatControl;
    DirectShowVideoProbeControl *m_videoProbeControl = nullptr;
    DirectShowCameraZoomControl *m_zoomControl;
    DirectShowCameraImageEncoderControl *m_imageEncoderControl;
};

QT_END_NAMESPACE

#endif
