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
#ifdef Q_OS_MACOS
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
