/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
#include <qwindowsmultimediautils_p.h>
#include <qplatformmediarecorder_p.h>

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
