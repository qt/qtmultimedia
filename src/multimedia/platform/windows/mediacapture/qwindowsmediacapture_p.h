/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
#ifndef QWINDOWSMEDIACAPTURE_H
#define QWINDOWSMEDIACAPTURE_H

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

class QWindowsMediaEncoder;
class QWindowsCamera;
class QWindowsMediaDeviceSession;
class QWindowsImageCapture;
class QPlatformAudioInput;

class QWindowsMediaCaptureService : public QPlatformMediaCaptureSession
{
    Q_OBJECT

public:
    QWindowsMediaCaptureService();
    virtual ~QWindowsMediaCaptureService();

    QPlatformCamera *camera() override;
    void setCamera(QPlatformCamera *camera) override;

    QPlatformImageCapture *imageCapture() override;
    void setImageCapture(QPlatformImageCapture *imageCapture) override;

    QPlatformMediaRecorder *mediaRecorder() override;
    void setMediaRecorder(QPlatformMediaRecorder *recorder) override;

    void setAudioInput(QPlatformAudioInput *) override;

    void setAudioOutput(QPlatformAudioOutput *output) override;

    void setVideoPreview(QVideoSink *sink) override;

    QWindowsMediaDeviceSession *session() const;

private:
    QWindowsCamera              *m_camera = nullptr;
    QWindowsMediaDeviceSession  *m_mediaDeviceSession = nullptr;
    QWindowsImageCapture  *m_imageCapture = nullptr;
    QWindowsMediaEncoder        *m_encoder = nullptr;
};

QT_END_NAMESPACE

#endif // QWINDOWSMEDIAINTERFACE_H
