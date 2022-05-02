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
#ifndef QPLATFORMMEDIACAPTURE_H
#define QPLATFORMMEDIACAPTURE_H

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

#include <private/qtmultimediaglobal_p.h>
#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE
class QPlatformCamera;
class QPlatformImageCapture;
class QPlatformMediaRecorder;
class QAudioDevice;
class QCameraDevice;
class QVideoSink;
class QPlatformAudioInput;
class QPlatformAudioOutput;

class Q_MULTIMEDIA_EXPORT QPlatformMediaCaptureSession : public QObject
{
    Q_OBJECT
public:
    QPlatformMediaCaptureSession() = default;
    virtual ~QPlatformMediaCaptureSession();

    virtual QPlatformCamera *camera() = 0;
    virtual void setCamera(QPlatformCamera *) {}

    virtual QPlatformImageCapture *imageCapture() = 0;
    virtual void setImageCapture(QPlatformImageCapture *) {}

    virtual QPlatformMediaRecorder *mediaRecorder() = 0;
    virtual void setMediaRecorder(QPlatformMediaRecorder *) {}

    virtual void setAudioInput(QPlatformAudioInput *input) = 0;

    virtual void setVideoPreview(QVideoSink * /*sink*/) {}

    virtual void setAudioOutput(QPlatformAudioOutput *) {}

Q_SIGNALS:
    void cameraChanged();
    void imageCaptureChanged();
    void encoderChanged();
};

QT_END_NAMESPACE


#endif // QPLATFORMMEDIAINTERFACE_H
