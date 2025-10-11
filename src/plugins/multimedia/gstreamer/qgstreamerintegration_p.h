// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGSTREAMERINTEGRATION_H
#define QGSTREAMERINTEGRATION_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtMultimedia/private/qplatformmediaintegration_p.h>
#include <QtMultimedia/private/qgstreamer_platformspecificinterface_p.h>

#include <gst/gst.h>

QT_BEGIN_NAMESPACE

class QGstreamerFormatInfo;

class QGStreamerPlatformSpecificInterfaceImplementation : public QGStreamerPlatformSpecificInterface
{
public:
    ~QGStreamerPlatformSpecificInterfaceImplementation() override;

    QAudioDevice makeCustomGStreamerAudioInput(const QByteArray &gstreamerPipeline) override;
    QAudioDevice makeCustomGStreamerAudioOutput(const QByteArray &gstreamerPipeline) override;
    QCamera *makeCustomGStreamerCamera(const QByteArray &gstreamerPipeline,
                                       QObject *parent) override;

    QCamera *makeCustomGStreamerCamera(GstElement *, QObject *parent) override;

    GstPipeline *gstPipeline(QMediaPlayer *) override;
    GstPipeline *gstPipeline(QMediaCaptureSession *) override;
};

class QGstreamerIntegration : public QPlatformMediaIntegration
{
public:
    QGstreamerIntegration();

    static QGstreamerIntegration *instance()
    {
        return static_cast<QGstreamerIntegration *>(QPlatformMediaIntegration::instance());
    }

    QMaybe<QPlatformAudioDecoder *> createAudioDecoder(QAudioDecoder *decoder) override;
    QMaybe<QPlatformMediaCaptureSession *> createCaptureSession() override;
    QMaybe<QPlatformMediaPlayer *> createPlayer(QMediaPlayer *player) override;
    QMaybe<QPlatformCamera *> createCamera(QCamera *) override;
    QMaybe<QPlatformMediaRecorder *> createRecorder(QMediaRecorder *) override;
    QMaybe<QPlatformImageCapture *> createImageCapture(QImageCapture *) override;

    QMaybe<QPlatformVideoSink *> createVideoSink(QVideoSink *sink) override;

    QMaybe<QPlatformAudioInput *> createAudioInput(QAudioInput *) override;
    QMaybe<QPlatformAudioOutput *> createAudioOutput(QAudioOutput *) override;

    const QGstreamerFormatInfo *gstFormatsInfo();
    GstDevice *videoDevice(const QByteArray &id);

    QAbstractPlatformSpecificInterface *platformSpecificInterface() override;

protected:
    QPlatformMediaFormatInfo *createFormatInfo() override;
    QPlatformVideoDevices *createVideoDevices() override;

    QGStreamerPlatformSpecificInterfaceImplementation m_platformSpecificImplementation;
};

QT_END_NAMESPACE

#endif
