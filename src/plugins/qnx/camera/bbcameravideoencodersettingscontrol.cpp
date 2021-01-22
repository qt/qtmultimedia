/****************************************************************************
**
** Copyright (C) 2016 Research In Motion
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
#include "bbcameravideoencodersettingscontrol.h"

#include "bbcamerasession.h"

QT_BEGIN_NAMESPACE

BbCameraVideoEncoderSettingsControl::BbCameraVideoEncoderSettingsControl(BbCameraSession *session, QObject *parent)
    : QVideoEncoderSettingsControl(parent)
    , m_session(session)
{
}

QList<QSize> BbCameraVideoEncoderSettingsControl::supportedResolutions(const QVideoEncoderSettings &settings, bool *continuous) const
{
    return m_session->supportedResolutions(settings, continuous);
}

QList<qreal> BbCameraVideoEncoderSettingsControl::supportedFrameRates(const QVideoEncoderSettings &settings, bool *continuous) const
{
    return m_session->supportedFrameRates(settings, continuous);
}

QStringList BbCameraVideoEncoderSettingsControl::supportedVideoCodecs() const
{
    return QStringList() << QLatin1String("none") << QLatin1String("avc1") << QLatin1String("h264");
}

QString BbCameraVideoEncoderSettingsControl::videoCodecDescription(const QString &codecName) const
{
    if (codecName == QLatin1String("none"))
        return tr("No compression");
    else if (codecName == QLatin1String("avc1"))
        return tr("AVC1 compression");
    else if (codecName == QLatin1String("h264"))
        return tr("H264 compression");

    return QString();
}

QVideoEncoderSettings BbCameraVideoEncoderSettingsControl::videoSettings() const
{
    return m_session->videoSettings();
}

void BbCameraVideoEncoderSettingsControl::setVideoSettings(const QVideoEncoderSettings &settings)
{
    m_session->setVideoSettings(settings);
}

QT_END_NAMESPACE
