/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
    ~AVFCameraSession();

    QCameraDevice activecameraDevice() const { return m_activeCameraDevice; }
    void setActiveCamera(const QCameraDevice &info);

    void setCameraFormat(const QCameraFormat &format);

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

    void cameraAuthorizationChanged(bool authorized);
    void microphoneAuthorizationChanged(bool authorized);

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
    void requestCameraPermissionIfNeeded();
    void requestMicrophonePermissionIfNeeded();

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

    AVAuthorizationStatus m_cameraAuthorizationStatus = AVAuthorizationStatusDenied;
    AVAuthorizationStatus m_microphoneAuthorizationStatus = AVAuthorizationStatusDenied;
};

QT_END_NAMESPACE

#endif
