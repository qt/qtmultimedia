// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGSTREAMERMEDIACAPTURESESSION_H
#define QGSTREAMERMEDIACAPTURESESSION_H

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

#include <QtCore/private/qexpected_p.h>

#include <private/qplatformmediacapture_p.h>
#include <private/qplatformmediaintegration_p.h>

#include <common/qgst_bus_observer_p.h>
#include <common/qgst_p.h>
#include <common/qgstpipeline_p.h>

QT_BEGIN_NAMESPACE

class QGstreamerCameraBase;
class QGstreamerImageCapture;
class QGstreamerMediaRecorder;
class QGstreamerAudioInput;
class QGstreamerAudioOutput;
class QGstreamerVideoOutput;
class QGstreamerVideoSink;

class QGstreamerMediaCaptureSession final : public QPlatformMediaCaptureSession,
                                            QGstreamerBusMessageFilter
{
public:
    static q23::expected<QPlatformMediaCaptureSession *, QString> create();
    ~QGstreamerMediaCaptureSession() override;

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

    const QGstPipeline &pipeline() const;

    QGstreamerVideoSink *gstreamerVideoSink() const;

    struct RecorderElements
    {
        QGstBin encodeBin;
        QGstElement fileSink;

        QGstPad audioSink;
        QGstPad videoSink;
    };
    void linkAndStartEncoder(RecorderElements, const QMediaMetaData &);
    void unlinkRecorder();
    void finalizeRecorder();

private:
    bool processBusMessage(const QGstreamerMessage &) override;
    bool processBusMessageError(const QGstreamerMessage &);
    bool processBusMessageLatency(const QGstreamerMessage &);

    void setCameraActive(bool activate);

    explicit QGstreamerMediaCaptureSession(QGstreamerVideoOutput *videoOutput);

    friend QGstreamerMediaRecorder;
    // Gst elements
    QGstPipeline capturePipeline;

    QGstreamerAudioInput *gstAudioInput = nullptr;
    QGstreamerCameraBase *gstCamera = nullptr;
    QMetaObject::Connection gstCameraActiveConnection;

    QGstElement gstAudioTee;
    QGstPad audioSrcPadForEncoder;
    QGstPad audioSrcPadForOutput;

    QGstElement gstVideoTee;
    QGstPad videoSrcPadForEncoder;
    QGstPad videoSrcPadForOutput;
    QGstPad videoSrcPadForImageCapture;

    QGstElement encoderVideoCapsFilter;
    QGstElement encoderAudioCapsFilter;

    QGstPad imageCaptureSink();
    QGstPad videoOutputSink();
    QGstPad audioOutputSink();

    std::optional<RecorderElements> m_currentRecorderState;

    QGstreamerAudioOutput *gstAudioOutput = nullptr;
    QGstreamerVideoOutput *gstVideoOutput = nullptr;

    QGstreamerMediaRecorder *m_mediaRecorder = nullptr;
    QGstreamerImageCapture *m_imageCapture = nullptr;
};

QT_END_NAMESPACE

#endif // QGSTREAMERMEDIACAPTURESESSION_H
