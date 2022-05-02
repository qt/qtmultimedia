/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

#ifndef QANDROIDCAPTURESERVICE_H
#define QANDROIDCAPTURESERVICE_H

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

#include <private/qplatformmediacapture_p.h>
#include <private/qplatformmediaintegration_p.h>

QT_BEGIN_NAMESPACE

class QAndroidMediaEncoder;
class QAndroidCaptureSession;
class QAndroidCamera;
class QAndroidCameraSession;
class QAndroidImageCapture;

class QAndroidMediaCaptureSession : public QPlatformMediaCaptureSession
{
    Q_OBJECT

public:
    explicit QAndroidMediaCaptureSession();
    virtual ~QAndroidMediaCaptureSession();

    QPlatformCamera *camera() override;
    void setCamera(QPlatformCamera *camera) override;

    QPlatformImageCapture *imageCapture() override;
    void setImageCapture(QPlatformImageCapture *imageCapture) override;

    QPlatformMediaRecorder *mediaRecorder() override;
    void setMediaRecorder(QPlatformMediaRecorder *recorder) override;

    void setAudioInput(QPlatformAudioInput *input) override;

    void setVideoPreview(QVideoSink *sink) override;

    void setAudioOutput(QPlatformAudioOutput *output) override;

    QAndroidCaptureSession *captureSession() const { return m_captureSession; }
    QAndroidCameraSession *cameraSession() const { return m_cameraSession; }

private:
    QAndroidMediaEncoder *m_encoder = nullptr;
    QAndroidCaptureSession *m_captureSession = nullptr;
    QAndroidCamera *m_cameraControl = nullptr;
    QAndroidCameraSession *m_cameraSession = nullptr;
    QAndroidImageCapture *m_imageCaptureControl = nullptr;
};

QT_END_NAMESPACE

#endif // QANDROIDCAPTURESERVICE_H
