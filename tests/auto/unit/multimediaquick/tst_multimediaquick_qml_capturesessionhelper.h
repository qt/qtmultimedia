// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef TST_MULTIMEDIAQUICK_QML_CAPTURESESSIONHELPER_H
#define TST_MULTIMEDIAQUICK_QML_CAPTURESESSIONHELPER_H

#include <QtCore/qobject.h>

#include <QtMultimedia/qmediacapturesession.h>

QT_BEGIN_NAMESPACE

class CaptureSessionHelper : public QObject
{
    Q_OBJECT

public:
    Q_INVOKABLE static bool implicitlyConvertibleToQMediaCaptureSession(QMediaCaptureSession *);

    Q_INVOKABLE static void clearCamera(QMediaCaptureSession *);
    Q_INVOKABLE static void clearScreenCapture(QMediaCaptureSession *);
    Q_INVOKABLE static void clearWindowCapture(QMediaCaptureSession *);
    Q_INVOKABLE static void clearImageCapture(QMediaCaptureSession *);
    Q_INVOKABLE static void clearRecorder(QMediaCaptureSession *);
    Q_INVOKABLE static void clearAudioInput(QMediaCaptureSession *);
    Q_INVOKABLE static void clearAudioOutput(QMediaCaptureSession *);
    Q_INVOKABLE static void clearVideoOutput(QMediaCaptureSession *);
};

QT_END_NAMESPACE

#endif // TST_MULTIMEDIAQUICK_QML_CAPTURESESSIONHELPER_H
