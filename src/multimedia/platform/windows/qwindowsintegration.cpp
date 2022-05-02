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

#include "qwindowsintegration_p.h"
#include <private/qwindowsmediadevices_p.h>
#include <private/qwindowsformatinfo_p.h>
#include <private/qwindowsmediacapture_p.h>
#include <private/qwindowsimagecapture_p.h>
#include <private/qwindowscamera_p.h>
#include <private/qwindowsmediaencoder_p.h>
#include <private/mfplayercontrol_p.h>
#include <private/mfaudiodecodercontrol_p.h>
#include <private/mfevrvideowindowcontrol_p.h>

QT_BEGIN_NAMESPACE

static int g_refCount = 0;

QWindowsMediaIntegration::QWindowsMediaIntegration()
{
    g_refCount++;
    if (g_refCount == 1) {
        CoInitialize(NULL);
        MFStartup(MF_VERSION);
    }
}

QWindowsMediaIntegration::~QWindowsMediaIntegration()
{
    delete m_devices;
    delete m_formatInfo;

    g_refCount--;
    if (g_refCount == 0) {
        // ### This currently crashes on exit
//        MFShutdown();
//        CoUninitialize();
    }
}

QPlatformMediaDevices *QWindowsMediaIntegration::devices()
{
    if (!m_devices)
        m_devices = new QWindowsMediaDevices();
    return m_devices;
}

QPlatformMediaFormatInfo *QWindowsMediaIntegration::formatInfo()
{
    if (!m_formatInfo)
        m_formatInfo = new QWindowsFormatInfo();
    return m_formatInfo;
}

QPlatformMediaCaptureSession *QWindowsMediaIntegration::createCaptureSession()
{
    return new QWindowsMediaCaptureService();
}

QPlatformAudioDecoder *QWindowsMediaIntegration::createAudioDecoder(QAudioDecoder *decoder)
{
    return new MFAudioDecoderControl(decoder);
}

QPlatformMediaPlayer *QWindowsMediaIntegration::createPlayer(QMediaPlayer *parent)
{
    return new MFPlayerControl(parent);
}

QPlatformCamera *QWindowsMediaIntegration::createCamera(QCamera *camera)
{
    return new QWindowsCamera(camera);
}

QPlatformMediaRecorder *QWindowsMediaIntegration::createRecorder(QMediaRecorder *recorder)
{
    return new QWindowsMediaEncoder(recorder);
}

QPlatformImageCapture *QWindowsMediaIntegration::createImageCapture(QImageCapture *imageCapture)
{
    return new QWindowsImageCapture(imageCapture);
}

QPlatformVideoSink *QWindowsMediaIntegration::createVideoSink(QVideoSink *sink)
{
    return new MFEvrVideoWindowControl(sink);
}

QT_END_NAMESPACE
