/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef AVFVIDEOENCODERSETTINGSCONTROL_H
#define AVFVIDEOENCODERSETTINGSCONTROL_H

#include <qvideoencodersettingscontrol.h>

#include "avfcamerautility.h"
#import <AVFoundation/AVFoundation.h>

@class NSDictionary;

QT_BEGIN_NAMESPACE

class AVFCameraService;

class AVFVideoEncoderSettingsControl : public QVideoEncoderSettingsControl
{
    Q_OBJECT

public:
    explicit AVFVideoEncoderSettingsControl(AVFCameraService *service);

    QList<QSize> supportedResolutions(const QVideoEncoderSettings &requestedVideoSettings,
                                      bool *continuous = nullptr) const override;

    QList<qreal> supportedFrameRates(const QVideoEncoderSettings &requestedVideoSettings,
                                     bool *continuous = nullptr) const override;

    QStringList supportedVideoCodecs() const override;
    QString videoCodecDescription(const QString &codecName) const override;

    QVideoEncoderSettings videoSettings() const override;
    void setVideoSettings(const QVideoEncoderSettings &requestedVideoSettings) override;

    NSDictionary *applySettings(AVCaptureConnection *connection);
    void unapplySettings(AVCaptureConnection *connection);

private:
    AVFCameraService *m_service;

    QVideoEncoderSettings m_requestedSettings;
    QVideoEncoderSettings m_actualSettings;

    AVCaptureDeviceFormat *m_restoreFormat;
    AVFPSRange m_restoreFps;
};

QT_END_NAMESPACE

#endif // AVFVIDEOENCODERSETTINGSCONTROL_H
