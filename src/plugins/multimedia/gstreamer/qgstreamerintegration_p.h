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
    ~QGstreamerIntegration() override;

    static QGstreamerIntegration *instance()
    {
        return static_cast<QGstreamerIntegration *>(QPlatformMediaIntegration::instance());
    }

    q23::expected<QPlatformAudioDecoder *, QString> createAudioDecoder(QAudioDecoder *decoder) override;
    q23::expected<QPlatformMediaCaptureSession *, QString> createCaptureSession() override;
    q23::expected<QPlatformMediaPlayer *, QString> createPlayer(QMediaPlayer *player) override;
    q23::expected<QPlatformCamera *, QString> createCamera(QCamera *) override;
    q23::expected<QPlatformMediaRecorder *, QString> createRecorder(QMediaRecorder *) override;
    q23::expected<QPlatformImageCapture *, QString> createImageCapture(QImageCapture *) override;

    q23::expected<QPlatformVideoSink *, QString> createVideoSink(QVideoSink *sink) override;

    q23::expected<QPlatformAudioInput *, QString> createAudioInput(QAudioInput *) override;
    q23::expected<QPlatformAudioOutput *, QString> createAudioOutput(QAudioOutput *) override;

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
