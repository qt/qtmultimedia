// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QMOCKINTEGRATION_H
#define QMOCKINTEGRATION_H

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

#include <private/qplatformmediaintegration_p.h>
#include <QtCore/qplugin.h>

QT_BEGIN_NAMESPACE

class QMockMediaPlayer;
class QMockAudioDecoder;
class QMockCamera;
class QMockMediaCaptureSession;
class QMockVideoSink;
class QMockSurfaceCapture;
class QPlatformMediaFormatInfo;
class QMockAudioDevices;
class QMockVideoDevices;

class QMockIntegration : public QPlatformMediaIntegration
{
public:
    QMockIntegration();
    ~QMockIntegration();

    static QMockIntegration *instance()
    {
        return static_cast<QMockIntegration *>(QPlatformMediaIntegration::instance());
    }

    using QPlatformMediaIntegration::resetInstance;

    q23::expected<QPlatformAudioDecoder *, QString> createAudioDecoder(QAudioDecoder *decoder) override;
    q23::expected<QPlatformMediaPlayer *, QString> createPlayer(QMediaPlayer *) override;
    q23::expected<QPlatformCamera *, QString> createCamera(QCamera *) override;
    q23::expected<QPlatformMediaRecorder *, QString> createRecorder(QMediaRecorder *) override;
    q23::expected<QPlatformImageCapture *, QString> createImageCapture(QImageCapture *) override;
    q23::expected<QPlatformMediaCaptureSession *, QString> createCaptureSession() override;
    q23::expected<QPlatformVideoSink *, QString> createVideoSink(QVideoSink *) override;

    q23::expected<QPlatformAudioOutput *, QString> createAudioOutput(QAudioOutput *) override;

    QPlatformSurfaceCapture *createScreenCapture(QScreenCapture *) override;
    QPlatformSurfaceCapture *createWindowCapture(QWindowCapture *) override;

    void addNewCamera();

    enum Flag { NoPlayerInterface = 0x1, NoAudioDecoderInterface = 0x2, NoCaptureInterface = 0x4 };
    Q_DECLARE_FLAGS(Flags, Flag);

    void setFlags(Flags f) { m_flags = f; }
    Flags flags() const { return m_flags; }

    QMockMediaPlayer *lastPlayer() const { return m_lastPlayer; }
    QMockAudioDecoder *lastAudioDecoder() const { return m_lastAudioDecoderControl; }
    QMockCamera *lastCamera() const { return m_lastCamera; }
    // QMockMediaEncoder *lastEncoder const { return m_lastEncoder; }
    QMockMediaCaptureSession *lastCaptureService() const { return m_lastCaptureService; }
    QMockVideoSink *lastVideoSink() const { return m_lastVideoSink; }
    QMockSurfaceCapture *lastScreenCapture() { return m_lastScreenCapture; }
    QMockSurfaceCapture *lastWindowCapture() { return m_lastWindowCapture; }
    QPlatformMediaFormatInfo *getWritableFormatInfo();

    QMockAudioDevices* audioDevices();
    QMockVideoDevices* videoDevices();

    int createVideoDevicesInvokeCount() const { return m_createVideoDevicesInvokeCount; }
    int createAudioDevicesInvokeCount() const { return m_createAudioDevicesInvokeCount; }

protected:
    QPlatformVideoDevices *createVideoDevices() override;
    std::unique_ptr<QPlatformAudioDevices> createAudioDevices() override;

private:
    Flags m_flags = {};
    QMockMediaPlayer *m_lastPlayer = nullptr;
    QMockAudioDecoder *m_lastAudioDecoderControl = nullptr;
    QMockCamera *m_lastCamera = nullptr;
    // QMockMediaEncoder *m_lastEncoder = nullptr;
    QMockMediaCaptureSession *m_lastCaptureService = nullptr;
    QMockVideoSink *m_lastVideoSink = nullptr;
    QMockSurfaceCapture *m_lastScreenCapture = nullptr;
    QMockSurfaceCapture *m_lastWindowCapture = nullptr;
    int m_createVideoDevicesInvokeCount = 0;
    int m_createAudioDevicesInvokeCount = 0;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QMockIntegration::Flags);

#define Q_ENABLE_MOCK_MULTIMEDIA_PLUGIN          \
    Q_IMPORT_PLUGIN(MockMultimediaPlugin)        \
    struct EnableMockPlugin                      \
    {                                            \
        EnableMockPlugin()                       \
        {                                        \
            qputenv("QT_MEDIA_BACKEND", "mock"); \
        }                                        \
    };                                           \
    static EnableMockPlugin s_mockMultimediaPluginEnabler;


QT_END_NAMESPACE

#endif
