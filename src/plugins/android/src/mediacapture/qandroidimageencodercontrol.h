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

#ifndef QANDROIDIMAGEENCODERCONTROL_H
#define QANDROIDIMAGEENCODERCONTROL_H

#include <qimageencodercontrol.h>

QT_BEGIN_NAMESPACE

class QAndroidCameraSession;

class QAndroidImageEncoderControl : public QImageEncoderControl
{
    Q_OBJECT
public:
    explicit QAndroidImageEncoderControl(QAndroidCameraSession *session);

    QStringList supportedImageCodecs() const override;
    QString imageCodecDescription(const QString &codecName) const override;
    QList<QSize> supportedResolutions(const QImageEncoderSettings &settings, bool *continuous = 0) const override;
    QImageEncoderSettings imageSettings() const override;
    void setImageSettings(const QImageEncoderSettings &settings) override;

private Q_SLOTS:
    void onCameraOpened();

private:
    QAndroidCameraSession *m_session;

    QList<QSize> m_supportedResolutions;
};

QT_END_NAMESPACE

#endif // QANDROIDIMAGEENCODERCONTROL_H
