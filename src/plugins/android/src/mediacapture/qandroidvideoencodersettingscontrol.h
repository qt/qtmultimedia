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

#ifndef QANDROIDVIDEOENCODERSETTINGSCONTROL_H
#define QANDROIDVIDEOENCODERSETTINGSCONTROL_H

#include <qvideoencodersettingscontrol.h>

QT_BEGIN_NAMESPACE

class QAndroidCaptureSession;

class QAndroidVideoEncoderSettingsControl : public QVideoEncoderSettingsControl
{
    Q_OBJECT
public:
    explicit QAndroidVideoEncoderSettingsControl(QAndroidCaptureSession *session);

    QList<QSize> supportedResolutions(const QVideoEncoderSettings &settings, bool *continuous = 0) const override;
    QList<qreal> supportedFrameRates(const QVideoEncoderSettings &settings, bool *continuous = 0) const override;
    QStringList supportedVideoCodecs() const override;
    QString videoCodecDescription(const QString &codecName) const override;
    QVideoEncoderSettings videoSettings() const override;
    void setVideoSettings(const QVideoEncoderSettings &settings) override;

private:
    QAndroidCaptureSession *m_session;
};

QT_END_NAMESPACE

#endif // QANDROIDVIDEOENCODERSETTINGSCONTROL_H
