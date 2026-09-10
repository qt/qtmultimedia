// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "tst_multimediaquick_qml_capturesessionhelper.h"

QT_USE_NAMESPACE

bool CaptureSessionHelper::implicitlyConvertibleToQMediaCaptureSession(
    QMediaCaptureSession *session)
{
    // If we resolve correctly to this function, it means our QML CaptureSession
    // was implicitly convertible to QMediaCaptureSession.
    return session != nullptr;
}

void CaptureSessionHelper::clearCamera(QMediaCaptureSession *session)
{
    if (session)
        session->setCamera(nullptr);
}

void CaptureSessionHelper::clearScreenCapture(QMediaCaptureSession *session)
{
    if (session)
        session->setScreenCapture(nullptr);
}

void CaptureSessionHelper::clearWindowCapture(QMediaCaptureSession *session)
{
    if (session)
        session->setWindowCapture(nullptr);
}

void CaptureSessionHelper::clearImageCapture(QMediaCaptureSession *session)
{
    if (session)
        session->setImageCapture(nullptr);
}

void CaptureSessionHelper::clearRecorder(QMediaCaptureSession *session)
{
    if (session)
        session->setRecorder(nullptr);
}

void CaptureSessionHelper::clearAudioInput(QMediaCaptureSession *session)
{
    if (session)
        session->setAudioInput(nullptr);
}

void CaptureSessionHelper::clearAudioOutput(QMediaCaptureSession *session)
{
    if (session)
        session->setAudioOutput(nullptr);
}

void CaptureSessionHelper::clearVideoOutput(QMediaCaptureSession *session)
{
    if (session)
        session->setVideoOutput(nullptr);
}
