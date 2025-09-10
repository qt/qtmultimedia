// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef AVFCAMERASESSION_H
#define AVFCAMERASESSION_H

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

#include <QtCore/qmutex.h>
#include <QtMultimedia/qcamera.h>
#include <QVideoFrame>
#include <qcameradevice.h>
#include "avfaudiopreviewdelegate_p.h"

#import <AVFoundation/AVFoundation.h>

@class AVFCameraSessionObserver;

QT_BEGIN_NAMESPACE

class AVFCamera;
class AVFCameraService;
class AVFCameraRenderer;
class AVFVideoSink;
class QVideoSink;

class AVFCameraSession : public QObject
{
    Q_OBJECT
public:
    AVFCameraSession(AVFCameraService *service, QObject *parent = nullptr);
    ~AVFCameraSession() override;

    QCameraDevice activecameraDevice() const { return m_activeCameraDevice; }
    void setActiveCamera(const QCameraDevice &info);

    void setCameraFormat(const QCameraFormat &format);
    QCameraFormat cameraFormat() const;

    AVFCameraRenderer *videoOutput() const { return m_videoOutput; }
    AVCaptureAudioDataOutput *audioOutput() const { return m_audioOutput; }
    AVFAudioPreviewDelegate *audioPreviewDelegate() const { return m_audioPreviewDelegate; }

    AVCaptureSession *captureSession() const { return m_captureSession; }
    AVCaptureDevice *videoCaptureDevice() const;
    AVCaptureDevice *audioCaptureDevice() const;

    bool isActive() const;

    FourCharCode defaultCodec();

    AVCaptureDeviceInput *videoInput() const { return m_videoInput; }
    AVCaptureDeviceInput *audioInput() const { return m_audioInput; }

    void setVideoSink(QVideoSink *sink);

    void updateVideoInput();

    void updateAudioInput();
    void updateAudioOutput();

public Q_SLOTS:
    void setActive(bool active);

    void setAudioInputVolume(float volume);
    void setAudioInputMuted(bool muted);
    void setAudioOutputMuted(bool muted);
    void setAudioOutputVolume(float volume);

    void processRuntimeError();
    void processSessionStarted();
    void processSessionStopped();

Q_SIGNALS:
    void readyToConfigureConnections();
    void activeChanged(bool);
    void error(int error, const QString &errorString);
    void newViewfinderFrame(const QVideoFrame &frame);

private:
    void updateCameraFormat(const QCameraFormat &format);

    void setVideoOutput(AVFCameraRenderer *output);
    void updateVideoOutput();

    void addAudioCapture();

    AVCaptureDevice *createVideoCaptureDevice();
    AVCaptureDevice *createAudioCaptureDevice();
    void attachVideoInputDevice();
    void attachAudioInputDevice();
    bool checkCameraPermission();
    bool checkMicrophonePermission();

    bool applyImageEncoderSettings();

    QCameraDevice m_activeCameraDevice;
    QCameraFormat m_cameraFormat;

    AVFCameraService *m_service;
    AVCaptureSession *m_captureSession;
    AVFCameraSessionObserver *m_observer;

    AVFCameraRenderer *m_videoOutput = nullptr;
    AVFVideoSink *m_videoSink = nullptr;

    AVCaptureDeviceInput *m_videoInput = nullptr;
    AVCaptureDeviceInput *m_audioInput = nullptr;

    AVCaptureAudioDataOutput *m_audioOutput = nullptr;
    AVFAudioPreviewDelegate *m_audioPreviewDelegate = nullptr;

    bool m_active = false;

    float m_inputVolume = 1.0;
    bool m_inputMuted = false;

    FourCharCode m_defaultCodec;
};

QT_END_NAMESPACE

#endif
