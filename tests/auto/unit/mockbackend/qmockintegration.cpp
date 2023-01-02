// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qmockintegration.h"
#include "qmockmediaplayer.h"
#include "qmockaudiodecoder.h"
#include "qmockcamera.h"
#include "qmockmediacapturesession.h"
#include "qmockvideosink.h"
#include "qmockimagecapture.h"
#include "qmockaudiooutput.h"
#include "qmockscreencapture.h"
#include <private/qcameradevice_p.h>
#include <private/qplatformvideodevices_p.h>

QT_BEGIN_NAMESPACE

class QMockVideoDevices : public QPlatformVideoDevices
{
public:
    QMockVideoDevices(QPlatformMediaIntegration *pmi)
        : QPlatformVideoDevices(pmi)
    {
        QCameraDevicePrivate *info = new QCameraDevicePrivate;
        info->description = QString::fromUtf8("defaultCamera");
        info->id = "default";
        info->isDefault = true;
        auto *f = new QCameraFormatPrivate{
            QSharedData(),
            QVideoFrameFormat::Format_ARGB8888,
            QSize(640, 480),
            0,
            30
        };
        info->videoFormats << f->create();
        m_cameraDevices.append(info->create());
        info = new QCameraDevicePrivate;
        info->description = QString::fromUtf8("frontCamera");
        info->id = "front";
        info->isDefault = false;
        info->position = QCameraDevice::FrontFace;
        f = new QCameraFormatPrivate{
            QSharedData(),
            QVideoFrameFormat::Format_XRGB8888,
            QSize(1280, 720),
            0,
            30
        };
        info->videoFormats << f->create();
        m_cameraDevices.append(info->create());
        info = new QCameraDevicePrivate;
        info->description = QString::fromUtf8("backCamera");
        info->id = "back";
        info->isDefault = false;
        info->position = QCameraDevice::BackFace;
        m_cameraDevices.append(info->create());
    }

    QList<QCameraDevice> videoDevices() const override
    {
        return m_cameraDevices;
    }

private:
    QList<QCameraDevice> m_cameraDevices;
};

QMockIntegration::QMockIntegration()
{
    setIntegration(this);
    m_videoDevices = std::make_unique<QMockVideoDevices>(this);
}

QMockIntegration::~QMockIntegration()
{
    setIntegration(nullptr);
}

QMaybe<QPlatformAudioDecoder *> QMockIntegration::createAudioDecoder(QAudioDecoder *decoder)
{
    if (m_flags & NoAudioDecoderInterface)
        m_lastAudioDecoderControl = nullptr;
    else
        m_lastAudioDecoderControl = new QMockAudioDecoder(decoder);
    return m_lastAudioDecoderControl;
}

QMaybe<QPlatformMediaPlayer *> QMockIntegration::createPlayer(QMediaPlayer *parent)
{
    if (m_flags & NoPlayerInterface)
        m_lastPlayer = nullptr;
    else
        m_lastPlayer = new QMockMediaPlayer(parent);
    return m_lastPlayer;
}

QMaybe<QPlatformCamera *> QMockIntegration::createCamera(QCamera *parent)
{
    if (m_flags & NoCaptureInterface)
        m_lastCamera = nullptr;
    else
        m_lastCamera = new QMockCamera(parent);
    return m_lastCamera;
}

QMaybe<QPlatformImageCapture *> QMockIntegration::createImageCapture(QImageCapture *capture)
{
    return new QMockImageCapture(capture);
}

QMaybe<QPlatformMediaRecorder *> QMockIntegration::createRecorder(QMediaRecorder *recorder)
{
    return new QMockMediaEncoder(recorder);
}

QPlatformScreenCapture *QMockIntegration::createScreenCapture(QScreenCapture *capture)
{
    if (m_flags & NoCaptureInterface)
        m_lastScreenCapture = nullptr;
    else
        m_lastScreenCapture = new QMockScreenCapture(capture);

    return m_lastScreenCapture;
}

QMaybe<QPlatformMediaCaptureSession *> QMockIntegration::createCaptureSession()
{
    if (m_flags & NoCaptureInterface)
        m_lastCaptureService = nullptr;
    else
        m_lastCaptureService = new QMockMediaCaptureSession();
    return m_lastCaptureService;
}

QMaybe<QPlatformVideoSink *> QMockIntegration::createVideoSink(QVideoSink *sink)
{
    m_lastVideoSink = new QMockVideoSink(sink);
    return m_lastVideoSink;
}

QMaybe<QPlatformAudioOutput *> QMockIntegration::createAudioOutput(QAudioOutput *q)
{
    return new QMockAudioOutput(q);
}

bool QMockCamera::simpleCamera = false;

QT_END_NAMESPACE
