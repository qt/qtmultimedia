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

#include <private/qgst_p.h>
#include <private/qgstpipeline_p.h>

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
    QGstreamerMediaCapture();
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
