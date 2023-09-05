// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

QT_BEGIN_NAMESPACE

class QMockMediaPlayer;
class QMockAudioDecoder;
class QMockCamera;
class QMockMediaCaptureSession;
class QMockVideoSink;
class QMockSurfaceCapture;

class QMockIntegration : public QPlatformMediaIntegration
{
public:
    ~QMockIntegration();

    static QMockIntegration *instance()
    {
        return static_cast<QMockIntegration *>(QPlatformMediaIntegration::instance());
    }

    QMaybe<QPlatformAudioDecoder *> createAudioDecoder(QAudioDecoder *decoder) override;
    QMaybe<QPlatformMediaPlayer *> createPlayer(QMediaPlayer *) override;
    QMaybe<QPlatformCamera *> createCamera(QCamera *) override;
    QMaybe<QPlatformMediaRecorder *> createRecorder(QMediaRecorder *) override;
    QMaybe<QPlatformImageCapture *> createImageCapture(QImageCapture *) override;
    QMaybe<QPlatformMediaCaptureSession *> createCaptureSession() override;
    QMaybe<QPlatformVideoSink *> createVideoSink(QVideoSink *) override;

    QMaybe<QPlatformAudioOutput *> createAudioOutput(QAudioOutput *) override;

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

private:
    friend class QMockIntegrationFactory;
    QMockIntegration();

    Flags m_flags = {};
    QMockMediaPlayer *m_lastPlayer = nullptr;
    QMockAudioDecoder *m_lastAudioDecoderControl = nullptr;
    QMockCamera *m_lastCamera = nullptr;
    // QMockMediaEncoder *m_lastEncoder = nullptr;
    QMockMediaCaptureSession *m_lastCaptureService = nullptr;
    QMockVideoSink *m_lastVideoSink = nullptr;
    QMockSurfaceCapture *m_lastScreenCapture = nullptr;
    QMockSurfaceCapture *m_lastWindowCapture = nullptr;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QMockIntegration::Flags);

class QMockIntegrationFactory
{
public:
    QMockIntegrationFactory() { QMockIntegration::setPlatformFactory(std::ref(*this)); }

    ~QMockIntegrationFactory() { QMockIntegration::setPlatformFactory(nullptr); }

    std::unique_ptr<QPlatformMediaIntegration> operator()()
    {
        Q_ASSERT(!m_wasRun);
        m_wasRun = true;
        return std::unique_ptr<QPlatformMediaIntegration>(new QMockIntegration);
    }

    bool wasRun() const { return m_wasRun; }

    Q_DISABLE_COPY(QMockIntegrationFactory);

private:
    bool m_wasRun = false;
};

QT_END_NAMESPACE

#endif
