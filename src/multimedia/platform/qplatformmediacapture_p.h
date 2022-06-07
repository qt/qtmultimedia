// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
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
class QMediaCaptureSession;

class Q_MULTIMEDIA_EXPORT QPlatformMediaCaptureSession : public QObject
{
    Q_OBJECT
public:
    QPlatformMediaCaptureSession() = default;
    virtual ~QPlatformMediaCaptureSession();

    void setCaptureSession(QMediaCaptureSession *session) { m_session = session; }
    QMediaCaptureSession *captureSession() const { return m_session; }

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

private:
    QMediaCaptureSession *m_session = nullptr;
};

QT_END_NAMESPACE


#endif // QPLATFORMMEDIAINTERFACE_H
