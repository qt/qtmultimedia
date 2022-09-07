// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGSTREAMERCAPTURESERVICE_H
#define QGSTREAMERCAPTURESERVICE_H

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

#include <qgst_p.h>
#include <qgstpipeline_p.h>

#include <qtimer.h>

QT_BEGIN_NAMESPACE

class QGstreamerCamera;
class QGstreamerImageCapture;
class QGstreamerMediaEncoder;
class QGstreamerAudioInput;
class QGstreamerAudioOutput;
class QGstreamerVideoOutput;
class QGstreamerVideoSink;

class QGstreamerMediaCapture : public QPlatformMediaCaptureSession
{
    Q_OBJECT

public:
    static QMaybe<QPlatformMediaCaptureSession *> create();
    virtual ~QGstreamerMediaCapture();

    QPlatformCamera *camera() override;
    void setCamera(QPlatformCamera *camera) override;

    QPlatformImageCapture *imageCapture() override;
    void setImageCapture(QPlatformImageCapture *imageCapture) override;

    QPlatformMediaRecorder *mediaRecorder() override;
    void setMediaRecorder(QPlatformMediaRecorder *recorder) override;

    void setAudioInput(QPlatformAudioInput *input) override;
    QGstreamerAudioInput *audioInput() { return gstAudioInput; }

    void setVideoPreview(QVideoSink *sink) override;
    void setAudioOutput(QPlatformAudioOutput *output) override;

    void linkEncoder(QGstPad audioSink, QGstPad videoSink);
    void unlinkEncoder();

    QGstPipeline pipeline() const { return gstPipeline; }

    QGstreamerVideoSink *gstreamerVideoSink() const;

private:
    QGstreamerMediaCapture(QGstreamerVideoOutput *videoOutput);

    friend QGstreamerMediaEncoder;
    // Gst elements
    QGstPipeline gstPipeline;

    QGstreamerAudioInput *gstAudioInput = nullptr;
    QGstreamerCamera *gstCamera = nullptr;

    QGstElement gstAudioTee;
    QGstElement gstVideoTee;
    QGstElement encoderVideoCapsFilter;
    QGstElement encoderAudioCapsFilter;

    QGstPad encoderAudioSink;
    QGstPad encoderVideoSink;
    QGstPad imageCaptureSink;

    QGstreamerAudioOutput *gstAudioOutput = nullptr;
    QGstreamerVideoOutput *gstVideoOutput = nullptr;

    QGstreamerMediaEncoder *m_mediaEncoder = nullptr;
    QGstreamerImageCapture *m_imageCapture = nullptr;
};

QT_END_NAMESPACE

#endif // QGSTREAMERCAPTURESERVICE_H
