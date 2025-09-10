// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qmediaframeinput_p.h"

QT_BEGIN_NAMESPACE

void QMediaFrameInputPrivate::setCaptureSession(QMediaCaptureSession *session)
{
    if (session == m_captureSession)
        return;

    auto prevSession = std::exchange(m_captureSession, session);
    updateCaptureSessionConnections(prevSession, session);
    updateCanSendMediaFrame();
}

void QMediaFrameInputPrivate::updateCanSendMediaFrame()
{
    const bool canSendMediaFrame = m_captureSession && checkIfCanSendMediaFrame();
    if (m_canSendMediaFrame != canSendMediaFrame) {
        m_canSendMediaFrame = canSendMediaFrame;
        if (m_canSendMediaFrame)
            emitReadyToSendMediaFrame();
    }
}

void QMediaFrameInputPrivate::postponeCheckReadyToSend()
{
    if (m_canSendMediaFrame && !m_postponeReadyToSendCheckRun) {
        m_postponeReadyToSendCheckRun = true;
        QMetaObject::invokeMethod(
                q_ptr,
                [this]() {
                    m_postponeReadyToSendCheckRun = false;
                    if (m_canSendMediaFrame)
                        emitReadyToSendMediaFrame();
                },
                Qt::QueuedConnection);
    }
}

QT_END_NAMESPACE
