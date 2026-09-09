// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef TST_MULTIMEDIAQUICK_QML_CAMERAFORMATHELPER_H
#define TST_MULTIMEDIAQUICK_QML_CAMERAFORMATHELPER_H

#include <QtCore/qobject.h>

#include <QtMultimedia/qcameradevice.h>

QT_BEGIN_NAMESPACE

class CameraFormatHelper : public QObject
{
    Q_OBJECT

public:
    Q_INVOKABLE static QCameraFormat createCameraFormat(
        QSize resolution,
        QVideoFrameFormat::PixelFormat,
        float minFrameRate,
        float maxFrameRate);

    Q_INVOKABLE static bool implicitlyConvertibleToQCameraFormat(const QCameraFormat &);
};

QT_END_NAMESPACE

#endif // TST_MULTIMEDIAQUICK_QML_CAMERAFORMATHELPER_H
