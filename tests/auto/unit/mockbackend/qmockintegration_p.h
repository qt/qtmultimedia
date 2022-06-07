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

class QMockIntegration : public QPlatformMediaIntegration
{
public:
    QMockIntegration();
    ~QMockIntegration();

    QPlatformMediaFormatInfo *formatInfo() override { return nullptr; }

    QPlatformAudioDecoder *createAudioDecoder(QAudioDecoder *decoder) override;
    QPlatformMediaPlayer *createPlayer(QMediaPlayer *) override;
    QPlatformCamera *createCamera(QCamera *) override;
    QPlatformMediaRecorder *createRecorder(QMediaRecorder *) override;
    QPlatformImageCapture *createImageCapture(QImageCapture *) override;
    QPlatformMediaCaptureSession *createCaptureSession() override;
    QPlatformVideoSink *createVideoSink(QVideoSink *) override;

    QPlatformAudioOutput *createAudioOutput(QAudioOutput *) override;

    enum Flag {
        NoPlayerInterface = 0x1,
        NoAudioDecoderInterface = 0x2,
        NoCaptureInterface = 0x4
    };
    Q_DECLARE_FLAGS(Flags, Flag);

    void setFlags(Flags f) { m_flags = f; }
    Flags flags() const { return m_flags; }

    QMockMediaPlayer *lastPlayer() const { return m_lastPlayer; }
    QMockAudioDecoder *lastAudioDecoder() const { return m_lastAudioDecoderControl; }
    QMockCamera *lastCamera() const { return m_lastCamera; }
    // QMockMediaEncoder *lastEncoder const { return m_lastEncoder; }
    QMockMediaCaptureSession *lastCaptureService() const { return m_lastCaptureService; }
    QMockVideoSink *lastVideoSink() const { return m_lastVideoSink; }

private:
    Flags m_flags = {};
    QMockMediaPlayer *m_lastPlayer = nullptr;
    QMockAudioDecoder *m_lastAudioDecoderControl = nullptr;
    QMockCamera *m_lastCamera = nullptr;
    // QMockMediaEncoder *m_lastEncoder = nullptr;
    QMockMediaCaptureSession *m_lastCaptureService = nullptr;
    QMockVideoSink *m_lastVideoSink;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QMockIntegration::Flags);

QT_END_NAMESPACE

#endif
