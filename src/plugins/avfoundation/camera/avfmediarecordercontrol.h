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

#ifndef AVFMEDIARECORDERCONTROL_H
#define AVFMEDIARECORDERCONTROL_H

#include <QtCore/qurl.h>
#include <QtMultimedia/qmediarecordercontrol.h>

#import <AVFoundation/AVFoundation.h>
#include "avfstoragelocation.h"
#include "avfcamerautility.h"

@class AVFMediaRecorderDelegate;

QT_BEGIN_NAMESPACE

class AVFCameraSession;
class AVFCameraControl;
class AVFAudioInputSelectorControl;
class AVFCameraService;

class AVFMediaRecorderControl : public QMediaRecorderControl
{
Q_OBJECT
public:
    AVFMediaRecorderControl(AVFCameraService *service, QObject *parent = nullptr);
    ~AVFMediaRecorderControl();

    QUrl outputLocation() const override;
    bool setOutputLocation(const QUrl &location) override;

    QMediaRecorder::State state() const override;
    QMediaRecorder::Status status() const override;

    qint64 duration() const override;

    bool isMuted() const override;
    qreal volume() const override;

    void applySettings() override;
    void unapplySettings();

public Q_SLOTS:
    void setState(QMediaRecorder::State state) override;
    void setMuted(bool muted) override;
    void setVolume(qreal volume) override;

    void handleRecordingStarted();
    void handleRecordingFinished();
    void handleRecordingFailed(const QString &message);

private Q_SLOTS:
    void setupSessionForCapture();
    void updateStatus();

private:
    AVFCameraService *m_service;
    AVFCameraControl *m_cameraControl;
    AVFAudioInputSelectorControl *m_audioInputControl;
    AVFCameraSession *m_session;

    bool m_connected;
    QUrl m_outputLocation;
    QMediaRecorder::State m_state;
    QMediaRecorder::Status m_lastStatus;

    bool m_recordingStarted;
    bool m_recordingFinished;

    bool m_muted;
    qreal m_volume;

    AVCaptureDeviceInput *m_audioInput;
    AVCaptureMovieFileOutput *m_movieOutput;
    AVFMediaRecorderDelegate *m_recorderDelagate;
    AVFStorageLocation m_storageLocation;

    AVFPSRange m_restoreFPS;
};

QT_END_NAMESPACE

#endif
