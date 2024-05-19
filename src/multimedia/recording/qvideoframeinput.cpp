// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qmediaframeinput_p.h"
#include "qmediainputencoderinterface_p.h"
#include "qplatformvideoframeinput_p.h"

QT_BEGIN_NAMESPACE

class QVideoFrameInputPrivate : public QMediaFrameInputPrivate
{
public:
    // QVideoFrameInputPrivate(QVideoFrameInput *q) : q(q) { }

    bool sendVideoFrame(const QVideoFrame &frame)
    {
        return sendMediaFrame([&]() { emit m_platfromVideoFrameInput->newVideoFrame(frame); });
    }

    void initialize()
    {
        m_platfromVideoFrameInput = std::make_unique<QPlatformVideoFrameInput>();
        addUpdateSignal(m_platfromVideoFrameInput.get(), &QPlatformVideoFrameInput::encoderUpdated);
    }

    void uninitialize()
    {
        m_platfromVideoFrameInput.reset();

        // if (m_captureSession)
        //     m_captureSession->setVideoFrameInput(nullptr);
    }

    QPlatformVideoFrameInput *platfromVideoFrameInput() const
    {
        return m_platfromVideoFrameInput.get();
    }

protected:
    void updateCaptureSessionConnections(QMediaCaptureSession *prevSession,
                                         QMediaCaptureSession *newSession) override
    {
        if (prevSession)
            removeUpdateSignal(prevSession, &QMediaCaptureSession::videoOutputChanged);

        if (newSession)
            addUpdateSignal(newSession, &QMediaCaptureSession::videoOutputChanged);
    }

    bool checkIfCanSendMediaFrame() const override
    {
        if (auto encoderInterface = m_platfromVideoFrameInput->encoderInterface())
            return encoderInterface->canPushFrame();

        return captureSession()->videoOutput() || captureSession()->videoSink();
    }

    void emitReadyToSendMediaFrame() override
    {
        // TODO
    }

private:
    // QVideoFrameInput *q = nullptr;
    std::unique_ptr<QPlatformVideoFrameInput> m_platfromVideoFrameInput;
};

QT_END_NAMESPACE
