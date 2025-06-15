// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtMultimedia/private/qplatformmediaplugin_p.h>
#include "qmockintegration.h"
#include "qmockmediaplayer.h"
#include "qmockaudiodecoder.h"
#include "qmockaudiodevices.h"
#include "qmockvideodevices.h"
#include "qmockcamera.h"
#include "qmockmediacapturesession.h"
#include "qmockvideosink.h"
#include "qmockimagecapture.h"
#include "qmockaudiooutput.h"
#include "qmocksurfacecapture.h"
#include <private/qplatformmediaformatinfo_p.h>

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

QMockIntegration::QMockIntegration() : QPlatformMediaIntegration(QLatin1String("mock")) { }
QMockIntegration::~QMockIntegration() = default;

QPlatformMediaFormatInfo *QMockIntegration::getWritableFormatInfo()
{
    // In tests, we want to populate the format info before running tests.
    // We can therefore allow casting away const here, to make unit testing easier.
    return const_cast<QPlatformMediaFormatInfo *>(formatInfo());
}

QMockAudioDevices* QMockIntegration::audioDevices() {
    return static_cast<QMockAudioDevices*>(QPlatformMediaIntegration::audioDevices());
}

QMockVideoDevices* QMockIntegration::videoDevices() {
    return static_cast<QMockVideoDevices*>(QPlatformMediaIntegration::videoDevices());
}

QPlatformVideoDevices *QMockIntegration::createVideoDevices()
{
    ++m_createVideoDevicesInvokeCount;
    return new QMockVideoDevices(this);
}

std::unique_ptr<QPlatformAudioDevices> QMockIntegration::createAudioDevices()
{
    ++m_createAudioDevicesInvokeCount;
    return std::make_unique<QMockAudioDevices>();
}

q23::expected<QPlatformAudioDecoder *, QString> QMockIntegration::createAudioDecoder(QAudioDecoder *decoder)
{
    if (m_flags & NoAudioDecoderInterface)
        m_lastAudioDecoderControl = nullptr;
    else
        m_lastAudioDecoderControl = new QMockAudioDecoder(decoder);
    if (m_lastAudioDecoderControl)
        return m_lastAudioDecoderControl;
    else
        return q23::unexpected{ QStringLiteral("No audio decoder") };
}

q23::expected<QPlatformMediaPlayer *, QString> QMockIntegration::createPlayer(QMediaPlayer *parent)
{
    if (m_flags & NoPlayerInterface)
        m_lastPlayer = nullptr;
    else
        m_lastPlayer = new QMockMediaPlayer(parent);
    if (m_lastPlayer)
        return m_lastPlayer;
    else
        return q23::unexpected{ QStringLiteral("No media player") };
}

q23::expected<QPlatformCamera *, QString> QMockIntegration::createCamera(QCamera *parent)
{
    if (m_flags & NoCaptureInterface)
        m_lastCamera = nullptr;
    else
        m_lastCamera = new QMockCamera(parent);
    if (m_lastCamera)
        return m_lastCamera;
    else
        return q23::unexpected{ QStringLiteral("No camera") };
}

q23::expected<QPlatformImageCapture *, QString> QMockIntegration::createImageCapture(QImageCapture *capture)
{
    return new QMockImageCapture(capture);
}

q23::expected<QPlatformMediaRecorder *, QString> QMockIntegration::createRecorder(QMediaRecorder *recorder)
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

q23::expected<QPlatformMediaCaptureSession *, QString> QMockIntegration::createCaptureSession()
{
    if (m_flags & NoCaptureInterface)
        m_lastCaptureService = nullptr;
    else
        m_lastCaptureService = new QMockMediaCaptureSession();
    if (m_lastCaptureService)
        return m_lastCaptureService;
    else
        return q23::unexpected{ QStringLiteral("No media capture session") };
}

q23::expected<QPlatformVideoSink *, QString> QMockIntegration::createVideoSink(QVideoSink *sink)
{
    m_lastVideoSink = new QMockVideoSink(sink);
    return m_lastVideoSink;
}

q23::expected<QPlatformAudioOutput *, QString> QMockIntegration::createAudioOutput(QAudioOutput *q)
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
