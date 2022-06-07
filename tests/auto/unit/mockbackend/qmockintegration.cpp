// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qmockintegration_p.h"
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
