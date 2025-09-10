// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSMEDIADEVICESESSION_H
#define QWINDOWSMEDIADEVICESESSION_H

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
#include <qcamera.h>
#include <qaudiodevice.h>
#include <private/qwindowsmultimediautils_p.h>
#include <private/qplatformmediarecorder_p.h>

QT_BEGIN_NAMESPACE

class QAudioInput;
class QAudioOutput;
class QVideoSink;
class QWindowsMediaDeviceReader;

class QWindowsMediaDeviceSession : public QObject
{
    Q_OBJECT
public:
    explicit QWindowsMediaDeviceSession(QObject *parent = nullptr);
    ~QWindowsMediaDeviceSession();

    bool isActive() const;
    void setActive(bool active);

    bool isActivating() const;

    void setActiveCamera(const QCameraDevice &camera);
    QCameraDevice activeCamera() const;

    void setCameraFormat(const QCameraFormat &cameraFormat);

    void setVideoSink(QVideoSink *surface);

public Q_SLOTS:
    void setAudioInputMuted(bool muted);
    void setAudioInputVolume(float volume);
    void audioInputDeviceChanged();
    void setAudioOutputMuted(bool muted);
    void setAudioOutputVolume(float volume);
    void audioOutputDeviceChanged();

public:
    void setAudioInput(QAudioInput *input);
    void setAudioOutput(QAudioOutput *output);

    QMediaRecorder::Error startRecording(QMediaEncoderSettings &settings, const QString &fileName, bool audioOnly);
    void stopRecording();
    bool pauseRecording();
    bool resumeRecording();

Q_SIGNALS:
    void activeChanged(bool);
    void readyForCaptureChanged(bool);
    void durationChanged(qint64 duration);
    void recordingStarted();
    void recordingStopped();
    void streamingError(int errorCode);
    void recordingError(int errorCode);
    void videoFrameChanged(const QVideoFrame &frame);

private Q_SLOTS:
    void handleStreamingStarted();
    void handleStreamingStopped();
    void handleStreamingError(int errorCode);
    void handleVideoFrameChanged(const QVideoFrame &frame);

private:
    void reactivate();
    quint32 estimateVideoBitRate(const GUID &videoFormat, quint32 width, quint32 height,
                                qreal frameRate, QMediaRecorder::Quality quality);
    quint32 estimateAudioBitRate(const GUID &audioFormat, QMediaRecorder::Quality quality);
    bool m_active = false;
    bool m_activating = false;
    QCameraDevice m_activeCameraDevice;
    QCameraFormat m_cameraFormat;
    QWindowsMediaDeviceReader *m_mediaDeviceReader = nullptr;
    QAudioInput *m_audioInput = nullptr;
    QAudioOutput *m_audioOutput = nullptr;
    QVideoSink  *m_surface = nullptr;
};

QT_END_NAMESPACE

#endif // QWINDOWSMEDIADEVICESESSION_H
