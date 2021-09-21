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

#include "qmockintegration_p.h"
#include "qmockmediadevices_p.h"
#include "qmockmediaplayer.h"
#include "qmockaudiodecoder.h"
#include "qmockcamera.h"
#include "qmockmediacapturesession.h"
#include "qmockvideosink.h"
#include "qmockimagecapture.h"
#include "qmockaudiooutput.h"

QT_BEGIN_NAMESPACE

QMockIntegration::QMockIntegration()
{
    setIntegration(this);
}

QMockIntegration::~QMockIntegration()
{
    setIntegration(nullptr);
    delete m_devices;
}

QPlatformMediaDevices *QMockIntegration::devices()
{
    if (!m_devices)
        m_devices = new QMockMediaDevices();
    return m_devices;
}

QPlatformAudioDecoder *QMockIntegration::createAudioDecoder(QAudioDecoder *decoder)
{
    if (m_flags & NoAudioDecoderInterface)
        m_lastAudioDecoderControl = nullptr;
    else
        m_lastAudioDecoderControl = new QMockAudioDecoder(decoder);
    return m_lastAudioDecoderControl;
}

QPlatformMediaPlayer *QMockIntegration::createPlayer(QMediaPlayer *parent)
{
    if (m_flags & NoPlayerInterface)
        m_lastPlayer = nullptr;
    else
        m_lastPlayer = new QMockMediaPlayer(parent);
    return m_lastPlayer;
}

QPlatformCamera *QMockIntegration::createCamera(QCamera *parent)
{
    if (m_flags & NoCaptureInterface)
        m_lastCamera = nullptr;
    else
        m_lastCamera = new QMockCamera(parent);
    return m_lastCamera;
}

QPlatformImageCapture *QMockIntegration::createImageCapture(QImageCapture *capture)
{
    return new QMockImageCapture(capture);
}

QPlatformMediaRecorder *QMockIntegration::createRecorder(QMediaRecorder *recorder)
{
    return new QMockMediaEncoder(recorder);
}

QPlatformMediaCaptureSession *QMockIntegration::createCaptureSession()
{
    if (m_flags & NoCaptureInterface)
        m_lastCaptureService = nullptr;
    else
        m_lastCaptureService = new QMockMediaCaptureSession();
    return m_lastCaptureService;
}

QPlatformVideoSink *QMockIntegration::createVideoSink(QVideoSink *sink)
{
    m_lastVideoSink = new QMockVideoSink(sink);
    return m_lastVideoSink;
}

QPlatformAudioOutput *QMockIntegration::createAudioOutput(QAudioOutput *q)
{
    return new QMockAudioOutput(q);
}

bool QMockCamera::simpleCamera = false;

QT_END_NAMESPACE
