// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qplatformaudiobufferinput_p.h"
#include "qmediainputencoderinterface_p.h"
#include "qmediaframeinput_p.h"

QT_BEGIN_NAMESPACE

class QAudioBufferInputPrivate : public QMediaFrameInputPrivate
{
public:
    // QAudioBufferInputPrivate(QAudioBufferInput *q) : q(q) { }

    bool sendAudioBuffer(const QAudioBuffer &audioBuffer)
    {
        return sendMediaFrame(
                [&]() { emit m_platfromAudioBufferInput->newAudioBuffer(audioBuffer); });
    }

    void initialize()
    {
        m_platfromAudioBufferInput = std::make_unique<QPlatformAudioBufferInput>();
        addUpdateSignal(m_platfromAudioBufferInput.get(),
                        &QPlatformAudioBufferInput::encoderUpdated);
    }

    void uninitialize()
    {
        m_platfromAudioBufferInput.reset();

        // if (m_captureSession)
        //     m_captureSession->setVideoBufferInput(nullptr);
    }

    QMediaCaptureSession *session() const { return m_captureSession; }

    QPlatformAudioBufferInput *platfromAudioBufferInput() const
    {
        return m_platfromAudioBufferInput.get();
    }

private:
    void updateCaptureSessionConnections(QMediaCaptureSession *prevSession,
                                         QMediaCaptureSession *newSession) override
    {
        if (prevSession)
            removeUpdateSignal(prevSession, &QMediaCaptureSession::audioOutputChanged);

        if (newSession)
            addUpdateSignal(newSession, &QMediaCaptureSession::audioOutputChanged);
    }

    bool checkIfCanSendMediaFrame() const override
    {
        if (auto encoderInterface = m_platfromAudioBufferInput->encoderInterface())
            return encoderInterface->canPushFrame();

        // Not implemented yet
        // return captureSession()->audioOutput() != nullptr;
        return false;
    }

    void emitReadyToSendMediaFrame() override
    {
        // TODO
    }

private:
    // QAudioBufferInput *q = nullptr;
    QMediaCaptureSession *m_captureSession = nullptr;
    std::unique_ptr<QPlatformAudioBufferInput> m_platfromAudioBufferInput;
};

QT_END_NAMESPACE
