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

#ifndef QANDROIDAUDIOENCODERSETTINGSCONTROL_H
#define QANDROIDAUDIOENCODERSETTINGSCONTROL_H

#include <qaudioencodersettingscontrol.h>

QT_BEGIN_NAMESPACE

class QAndroidCaptureSession;

class QAndroidAudioEncoderSettingsControl : public QAudioEncoderSettingsControl
{
    Q_OBJECT
public:
    explicit QAndroidAudioEncoderSettingsControl(QAndroidCaptureSession *session);

    QStringList supportedAudioCodecs() const override;
    QString codecDescription(const QString &codecName) const override;
    QList<int> supportedSampleRates(const QAudioEncoderSettings &settings, bool *continuous = 0) const override;
    QAudioEncoderSettings audioSettings() const override;
    void setAudioSettings(const QAudioEncoderSettings &settings) override;

private:
    QAndroidCaptureSession *m_session;
};

QT_END_NAMESPACE

#endif // QANDROIDAUDIOENCODERSETTINGSCONTROL_H
