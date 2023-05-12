// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QMEDIACAPTURESESSION_H
#define QMEDIACAPTURESESSION_H

#include <QtCore/qobject.h>
#include <QtMultimedia/qtmultimediaglobal.h>

QT_BEGIN_NAMESPACE

class QCamera;
class QAudioInput;
class QAudioOutput;
class QCameraDevice;
class QImageCapture;
class QMediaRecorder;
class QPlatformMediaCaptureSession;
class QVideoSink;
class QScreenCapture;
class QWindowCapture;

class QMediaCaptureSessionPrivate;
class Q_MULTIMEDIA_EXPORT QMediaCaptureSession : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QAudioInput *audioInput READ audioInput WRITE setAudioInput NOTIFY audioInputChanged)
    Q_PROPERTY(QAudioOutput *audioOutput READ audioOutput WRITE setAudioOutput NOTIFY audioOutputChanged)
    Q_PROPERTY(QCamera *camera READ camera WRITE setCamera NOTIFY cameraChanged)
    Q_PROPERTY(
            QScreenCapture *screenCapture READ screenCapture WRITE setScreenCapture NOTIFY screenCaptureChanged)
    Q_PROPERTY(
            QWindowCapture *windowCapture READ windowCapture WRITE setWindowCapture NOTIFY windowCaptureChanged)
    Q_PROPERTY(QImageCapture *imageCapture READ imageCapture WRITE setImageCapture NOTIFY imageCaptureChanged)
    Q_PROPERTY(QMediaRecorder *recorder READ recorder WRITE setRecorder NOTIFY recorderChanged)
    Q_PROPERTY(QObject *videoOutput READ videoOutput WRITE setVideoOutput NOTIFY videoOutputChanged)
public:
    explicit QMediaCaptureSession(QObject *parent = nullptr);
    ~QMediaCaptureSession();

    QAudioInput *audioInput() const;
    void setAudioInput(QAudioInput *input);

    QCamera *camera() const;
    void setCamera(QCamera *camera);

    QImageCapture *imageCapture();
    void setImageCapture(QImageCapture *imageCapture);

    QScreenCapture *screenCapture();
    void setScreenCapture(QScreenCapture *screenCapture);

    QWindowCapture *windowCapture();
    void setWindowCapture(QWindowCapture *windowCapture);

    QMediaRecorder *recorder();
    void setRecorder(QMediaRecorder *recorder);

    void setVideoOutput(QObject *output);
    QObject *videoOutput() const;

    void setVideoSink(QVideoSink *sink);
    QVideoSink *videoSink() const;

    void setAudioOutput(QAudioOutput *output);
    QAudioOutput *audioOutput() const;

    QPlatformMediaCaptureSession *platformSession() const;

Q_SIGNALS:
    void audioInputChanged();
    void cameraChanged();
    void screenCaptureChanged();
    void windowCaptureChanged();
    void imageCaptureChanged();
    void recorderChanged();
    void videoOutputChanged();
    void audioOutputChanged();

private:
    QMediaCaptureSessionPrivate *d_ptr;
    Q_DISABLE_COPY(QMediaCaptureSession)
    Q_DECLARE_PRIVATE(QMediaCaptureSession)
};

QT_END_NAMESPACE

#endif  // QMEDIACAPTURESESSION_H
