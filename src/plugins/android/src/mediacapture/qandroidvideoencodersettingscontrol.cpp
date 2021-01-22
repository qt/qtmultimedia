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

#include "qandroidvideoencodersettingscontrol.h"

#include "qandroidcapturesession.h"

QT_BEGIN_NAMESPACE

QAndroidVideoEncoderSettingsControl::QAndroidVideoEncoderSettingsControl(QAndroidCaptureSession *session)
    : QVideoEncoderSettingsControl()
    , m_session(session)
{
}

QList<QSize> QAndroidVideoEncoderSettingsControl::supportedResolutions(const QVideoEncoderSettings &, bool *continuous) const
{
    if (continuous)
        *continuous = false;

    return m_session->supportedResolutions();
}

QList<qreal> QAndroidVideoEncoderSettingsControl::supportedFrameRates(const QVideoEncoderSettings &, bool *continuous) const
{
    if (continuous)
        *continuous = false;

    return m_session->supportedFrameRates();
}

QStringList QAndroidVideoEncoderSettingsControl::supportedVideoCodecs() const
{
    return QStringList() << QLatin1String("h263")
                         << QLatin1String("h264")
                         << QLatin1String("mpeg4_sp");
}

QString QAndroidVideoEncoderSettingsControl::videoCodecDescription(const QString &codecName) const
{
    if (codecName == QLatin1String("h263"))
        return tr("H.263 compression");
    else if (codecName == QLatin1String("h264"))
        return tr("H.264 compression");
    else if (codecName == QLatin1String("mpeg4_sp"))
        return tr("MPEG-4 SP compression");

    return QString();
}

QVideoEncoderSettings QAndroidVideoEncoderSettingsControl::videoSettings() const
{
    return m_session->videoSettings();
}

void QAndroidVideoEncoderSettingsControl::setVideoSettings(const QVideoEncoderSettings &settings)
{
    m_session->setVideoSettings(settings);
}

QT_END_NAMESPACE
