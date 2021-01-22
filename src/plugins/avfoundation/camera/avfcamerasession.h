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

#ifndef AVFCAMERASESSION_H
#define AVFCAMERASESSION_H

#include <QtCore/qmutex.h>
#include <QtMultimedia/qcamera.h>
#include <QVideoFrame>

#import <AVFoundation/AVFoundation.h>

@class AVFCameraSessionObserver;

QT_BEGIN_NAMESPACE

class AVFCameraControl;
class AVFCameraService;
class AVFCameraRendererControl;
class AVFMediaVideoProbeControl;
class AVFCameraWindowControl;

struct AVFCameraInfo
{
    AVFCameraInfo() : position(QCamera::UnspecifiedPosition), orientation(0)
    { }

    QByteArray deviceId;
    QString description;
    QCamera::Position position;
    int orientation;
};

class AVFCameraSession : public QObject
{
    Q_OBJECT
public:
    AVFCameraSession(AVFCameraService *service, QObject *parent = nullptr);
    ~AVFCameraSession();

    static int defaultCameraIndex();
    static const QList<AVFCameraInfo> &availableCameraDevices();
    static AVFCameraInfo cameraDeviceInfo(const QByteArray &device);
    AVFCameraInfo activeCameraInfo() const { return m_activeCameraInfo; }

    void setVideoOutput(AVFCameraRendererControl *output);
    void setCapturePreviewOutput(AVFCameraWindowControl *output);
    AVCaptureSession *captureSession() const { return m_captureSession; }
    AVCaptureDevice *videoCaptureDevice() const;

    QCamera::State state() const;
    QCamera::State requestedState() const { return m_state; }
    bool isActive() const { return m_active; }

    void addProbe(AVFMediaVideoProbeControl *probe);
    void removeProbe(AVFMediaVideoProbeControl *probe);
    FourCharCode defaultCodec();

    AVCaptureDeviceInput *videoInput() const {return m_videoInput;}

public Q_SLOTS:
    void setState(QCamera::State state);

    void processRuntimeError();
    void processSessionStarted();
    void processSessionStopped();

    void onCaptureModeChanged(QCamera::CaptureModes mode);

    void onCameraFrameFetched(const QVideoFrame &frame);

Q_SIGNALS:
    void readyToConfigureConnections();
    void stateChanged(QCamera::State newState);
    void activeChanged(bool);
    void newViewfinderFrame(const QVideoFrame &frame);
    void error(int error, const QString &errorString);

private:
    static void updateCameraDevices();
    void attachVideoInputDevice();
    bool applyImageEncoderSettings();
    bool applyViewfinderSettings();

    static int m_defaultCameraIndex;
    static QList<AVFCameraInfo> m_cameraDevices;
    AVFCameraInfo m_activeCameraInfo;

    AVFCameraService *m_service;
    AVFCameraRendererControl *m_videoOutput;
    AVFCameraWindowControl *m_capturePreviewWindowOutput;

    QCamera::State m_state;
    bool m_active;

    AVCaptureSession *m_captureSession;
    AVCaptureDeviceInput *m_videoInput;
    AVFCameraSessionObserver *m_observer;

    QSet<AVFMediaVideoProbeControl *> m_videoProbes;
    QMutex m_videoProbesMutex;

    FourCharCode m_defaultCodec;
};

QT_END_NAMESPACE

#endif
