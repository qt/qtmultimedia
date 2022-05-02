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
#ifndef BBCAMERASERVICE_H
#define BBCAMERASERVICE_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QObject>

#include <private/qplatformmediacapture_p.h>

QT_BEGIN_NAMESPACE

class BbCameraAudioEncoderSettingsControl;
class BbCameraControl;
class BbCameraExposureControl;
class BbCameraFocusControl;
class BbCameraImageCaptureControl;
class BbCameraImageProcessingControl;
class BbCameraMediaRecorderControl;
class BbCameraSession;
class BbCameraVideoEncoderSettingsControl;
class BbVideoRendererControl;

class BbCameraService : public QPlatformMediaCaptureSession
{
    Q_OBJECT

public:
    explicit BbCameraService(QObject *parent = 0);
    ~BbCameraService();

    QPlatformCamera *camera() override;
    QPlatformImageCapture *imageCapture() override;
    QPlatformMediaRecorder *mediaRecorder() override;

    void setVideoPreview(QVideoSink *surface) override;

private:
    BbCameraSession* m_cameraSession;

    BbCameraAudioEncoderSettingsControl* m_cameraAudioEncoderSettingsControl;
    BbCameraControl* m_cameraControl;
    BbCameraExposureControl* m_cameraExposureControl;
    BbCameraFocusControl* m_cameraFocusControl;
    BbCameraImageCaptureControl* m_cameraImageCaptureControl;
    BbCameraImageProcessingControl* m_cameraImageProcessingControl;
    BbCameraMediaRecorderControl* m_cameraMediaRecorderControl;
    BbCameraVideoEncoderSettingsControl* m_cameraVideoEncoderSettingsControl;
    BbVideoRendererControl* m_videoRendererControl;
};

QT_END_NAMESPACE

#endif
