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
#include "bbcameraaudioencodersettingscontrol.h"

#include "bbcamerasession.h"

QT_BEGIN_NAMESPACE

BbCameraAudioEncoderSettingsControl::BbCameraAudioEncoderSettingsControl(BbCameraSession *session, QObject *parent)
    : QAudioEncoderSettingsControl(parent)
    , m_session(session)
{
}

QStringList BbCameraAudioEncoderSettingsControl::supportedAudioCodecs() const
{
    return QStringList() << QLatin1String("none") << QLatin1String("aac") << QLatin1String("raw");
}

QString BbCameraAudioEncoderSettingsControl::codecDescription(const QString &codecName) const
{
    if (codecName == QLatin1String("none"))
        return tr("No compression");
    else if (codecName == QLatin1String("aac"))
        return tr("AAC compression");
    else if (codecName == QLatin1String("raw"))
        return tr("PCM uncompressed");

    return QString();
}

QList<int> BbCameraAudioEncoderSettingsControl::supportedSampleRates(const QAudioEncoderSettings &settings, bool *continuous) const
{
    Q_UNUSED(settings);
    Q_UNUSED(continuous);

    // no API provided by BB10 yet
    return QList<int>();
}

QAudioEncoderSettings BbCameraAudioEncoderSettingsControl::audioSettings() const
{
    return m_session->audioSettings();
}

void BbCameraAudioEncoderSettingsControl::setAudioSettings(const QAudioEncoderSettings &settings)
{
    m_session->setAudioSettings(settings);
}

QT_END_NAMESPACE
