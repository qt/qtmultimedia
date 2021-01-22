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

#ifndef AVFAUDIOENCODERSETTINGSCONTROL_H
#define AVFAUDIOENCODERSETTINGSCONTROL_H

#include <qaudioencodersettingscontrol.h>

@class NSDictionary;
@class AVCaptureAudioDataOutput;

QT_BEGIN_NAMESPACE

class AVFCameraService;

class AVFAudioEncoderSettingsControl : public QAudioEncoderSettingsControl
{
public:
    explicit AVFAudioEncoderSettingsControl(AVFCameraService *service);

    QStringList supportedAudioCodecs() const override;
    QString codecDescription(const QString &codecName) const override;
    QList<int> supportedSampleRates(const QAudioEncoderSettings &settings, bool *continuous = nullptr) const override;
    QAudioEncoderSettings audioSettings() const override;
    void setAudioSettings(const QAudioEncoderSettings &settings) override;

    NSDictionary *applySettings();
    void unapplySettings();

private:
    AVFCameraService *m_service;

    QAudioEncoderSettings m_requestedSettings;
    QAudioEncoderSettings m_actualSettings;
};

QT_END_NAMESPACE

#endif // AVFAUDIOENCODERSETTINGSCONTROL_H
