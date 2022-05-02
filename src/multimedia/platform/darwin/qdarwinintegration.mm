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

#include "qdarwinintegration_p.h"
#include "qdarwinmediadevices_p.h"
#include <private/avfmediaplayer_p.h>
#include <private/avfcameraservice_p.h>
#include <private/avfcamera_p.h>
#include <private/avfimagecapture_p.h>
#include <private/avfmediaencoder_p.h>
#include <private/qdarwinformatsinfo_p.h>
#include <private/avfvideosink_p.h>
#include <private/avfaudiodecoder_p.h>
#include <VideoToolbox/VideoToolbox.h>
#include <qdebug.h>

QT_BEGIN_NAMESPACE

QDarwinIntegration::QDarwinIntegration()
{
#if defined(Q_OS_MACOS) && QT_MACOS_PLATFORM_SDK_EQUAL_OR_ABOVE(__MAC_11_0)
    if (__builtin_available(macOS 11.0, *))
        VTRegisterSupplementalVideoDecoderIfAvailable(kCMVideoCodecType_VP9);
#endif
}

QDarwinIntegration::~QDarwinIntegration()
{
    delete m_devices;
    delete m_formatInfo;
}

QPlatformMediaDevices *QDarwinIntegration::devices()
{
    if (!m_devices)
        m_devices = new QDarwinMediaDevices();
    return m_devices;
}

QPlatformMediaFormatInfo *QDarwinIntegration::formatInfo()
{
    if (!m_formatInfo)
        m_formatInfo = new QDarwinFormatInfo();
    return m_formatInfo;
}

QPlatformAudioDecoder *QDarwinIntegration::createAudioDecoder(QAudioDecoder *decoder)
{
    return new AVFAudioDecoder(decoder);
}

QPlatformMediaCaptureSession *QDarwinIntegration::createCaptureSession()
{
    return new AVFCameraService;
}

QPlatformMediaPlayer *QDarwinIntegration::createPlayer(QMediaPlayer *player)
{
    return new AVFMediaPlayer(player);
}

QPlatformCamera *QDarwinIntegration::createCamera(QCamera *camera)
{
    return new AVFCamera(camera);
}

QPlatformMediaRecorder *QDarwinIntegration::createRecorder(QMediaRecorder *recorder)
{
    return new AVFMediaEncoder(recorder);
}

QPlatformImageCapture *QDarwinIntegration::createImageCapture(QImageCapture *imageCapture)
{
    return new AVFImageCapture(imageCapture);
}

QPlatformVideoSink *QDarwinIntegration::createVideoSink(QVideoSink *sink)
{
    return new AVFVideoSink(sink);
}

QT_END_NAMESPACE
