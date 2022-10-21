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

#include "qgstreamerintegration_p.h"
#include "qgstreamermediadevices_p.h"
#include "private/qgstreamermediaplayer_p.h"
#include "private/qgstreamermediacapture_p.h"
#include "private/qgstreameraudiodecoder_p.h"
#include "private/qgstreamercamera_p.h"
#include "private/qgstreamermediaencoder_p.h"
#include "private/qgstreamerimagecapture_p.h"
#include "private/qgstreamerformatinfo_p.h"
#include "private/qgstreamervideosink_p.h"
#include "private/qgstreameraudioinput_p.h"
#include "private/qgstreameraudiooutput_p.h"

QT_BEGIN_NAMESPACE

QGstreamerIntegration::QGstreamerIntegration()
{
    gst_init(nullptr, nullptr);
    m_devices = new QGstreamerMediaDevices();
    m_formatsInfo = new QGstreamerFormatInfo();
}

QGstreamerIntegration::~QGstreamerIntegration()
{
    delete m_devices;
    delete m_formatsInfo;
}

QPlatformMediaDevices *QGstreamerIntegration::devices()
{
    return m_devices;
}

QPlatformMediaFormatInfo *QGstreamerIntegration::formatInfo()
{
    return m_formatsInfo;
}

QPlatformAudioDecoder *QGstreamerIntegration::createAudioDecoder(QAudioDecoder *decoder)
{
    return new QGstreamerAudioDecoder(decoder);
}

QPlatformMediaCaptureSession *QGstreamerIntegration::createCaptureSession()
{
    return new QGstreamerMediaCapture();
}

QPlatformMediaPlayer *QGstreamerIntegration::createPlayer(QMediaPlayer *player)
{
    return new QGstreamerMediaPlayer(player);
}

QPlatformCamera *QGstreamerIntegration::createCamera(QCamera *camera)
{
    return new QGstreamerCamera(camera);
}

QPlatformMediaRecorder *QGstreamerIntegration::createRecorder(QMediaRecorder *recorder)
{
    return new QGstreamerMediaEncoder(recorder);
}

QPlatformImageCapture *QGstreamerIntegration::createImageCapture(QImageCapture *imageCapture)
{
    return new QGstreamerImageCapture(imageCapture);
}

QPlatformVideoSink *QGstreamerIntegration::createVideoSink(QVideoSink *sink)
{
    return new QGstreamerVideoSink(sink);
}

QPlatformAudioInput *QGstreamerIntegration::createAudioInput(QAudioInput *q)
{
    return new QGstreamerAudioInput(q);
}

QPlatformAudioOutput *QGstreamerIntegration::createAudioOutput(QAudioOutput *q)
{
    return new QGstreamerAudioOutput(q);
}

QT_END_NAMESPACE
