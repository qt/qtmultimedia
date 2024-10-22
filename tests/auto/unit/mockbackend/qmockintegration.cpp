// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtMultimedia/private/qplatformmediaplugin_p.h>
#include "qmockintegration.h"
#include "qmockmediaplayer.h"
#include "qmockaudiodecoder.h"
#include "qmockcamera.h"
#include "qmockmediacapturesession.h"
#include "qmockvideosink.h"
#include "qmockimagecapture.h"
#include "qmockaudiooutput.h"
#include "qmocksurfacecapture.h"
#include <private/qcameradevice_p.h>
#include <private/qplatformvideodevices_p.h>
#include <private/qplatformmediaformatinfo_p.h>

#include "qmockmediadevices.h"

QT_BEGIN_NAMESPACE

class MockMultimediaPlugin : public QPlatformMediaPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QPlatformMediaPlugin_iid FILE "mock.json")

public:
    MockMultimediaPlugin() : QPlatformMediaPlugin() { }

    QPlatformMediaIntegration *create(const QString &name) override
    {
        if (name == QLatin1String("mock"))
            return new QMockIntegration;
        return nullptr;
    }
};

class QMockVideoDevices : public QPlatformVideoDevices
{
public:
    QMockVideoDevices(QPlatformMediaIntegration *pmi)
        : QPlatformVideoDevices(pmi)
    {
        QCameraDevicePrivate *info = new QCameraDevicePrivate;
        info->description = QStringLiteral("defaultCamera");
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
        info->description = QStringLiteral("frontCamera");
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
        info->description = QStringLiteral("backCamera");
        info->id = "back";
        info->isDefault = false;
        info->position = QCameraDevice::BackFace;
        m_cameraDevices.append(info->create());
    }

    void addNewCamera()
    {
        auto info = new QCameraDevicePrivate;
        info->description = QLatin1String("newCamera") + QString::number(m_cameraDevices.size());
        info->id =
                QString(QLatin1String("camera") + QString::number(m_cameraDevices.size())).toUtf8();
        info->isDefault = false;
        m_cameraDevices.append(info->create());

        emit videoInputsChanged();
    }

    QList<QCameraDevice> videoDevices() const override
    {
        return m_cameraDevices;
    }

private:
    QList<QCameraDevice> m_cameraDevices;
};

QMockIntegration::QMockIntegration() : QPlatformMediaIntegration(QLatin1String("mock")) { }
QMockIntegration::~QMockIntegration() = default;

QPlatformMediaFormatInfo *QMockIntegration::getWritableFormatInfo()
{
    // In tests, we want to populate the format info before running tests.
    // We can therefore allow casting away const here, to make unit testing easier.
    return const_cast<QPlatformMediaFormatInfo *>(formatInfo());
}

QPlatformVideoDevices *QMockIntegration::createVideoDevices()
{
    return new QMockVideoDevices(this);
}

std::unique_ptr<QPlatformMediaDevices> QMockIntegration::createMediaDevices()
{
    return std::make_unique<QMockMediaDevices>();
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

QPlatformSurfaceCapture *QMockIntegration::createScreenCapture(QScreenCapture * /*capture*/)
{
    if (m_flags & NoCaptureInterface)
        m_lastScreenCapture = nullptr;
    else
        m_lastScreenCapture = new QMockSurfaceCapture(QPlatformSurfaceCapture::ScreenSource{});

    return m_lastScreenCapture;
}

QPlatformSurfaceCapture *QMockIntegration::createWindowCapture(QWindowCapture *)
{
    if (m_flags & NoCaptureInterface)
        m_lastWindowCapture = nullptr;
    else
        m_lastWindowCapture = new QMockSurfaceCapture(QPlatformSurfaceCapture::WindowSource{});

    return m_lastWindowCapture;
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

void QMockIntegration::addNewCamera()
{
    static_cast<QMockVideoDevices *>(videoDevices())->addNewCamera();
}

bool QMockCamera::simpleCamera = false;

QT_END_NAMESPACE

#include "qmockintegration.moc"
