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

#include "qandroidaudioencodersettingscontrol.h"

#include "qandroidcapturesession.h"

QT_BEGIN_NAMESPACE

QAndroidAudioEncoderSettingsControl::QAndroidAudioEncoderSettingsControl(QAndroidCaptureSession *session)
    : QAudioEncoderSettingsControl()
    , m_session(session)
{
}

QStringList QAndroidAudioEncoderSettingsControl::supportedAudioCodecs() const
{
    return QStringList() << QLatin1String("amr-nb") << QLatin1String("amr-wb") << QLatin1String("aac");
}

QString QAndroidAudioEncoderSettingsControl::codecDescription(const QString &codecName) const
{
    if (codecName == QLatin1String("amr-nb"))
        return tr("Adaptive Multi-Rate Narrowband (AMR-NB) audio codec");
    else if (codecName == QLatin1String("amr-wb"))
        return tr("Adaptive Multi-Rate Wideband (AMR-WB) audio codec");
    else if (codecName == QLatin1String("aac"))
        return tr("AAC Low Complexity (AAC-LC) audio codec");

    return QString();
}

QList<int> QAndroidAudioEncoderSettingsControl::supportedSampleRates(const QAudioEncoderSettings &settings, bool *continuous) const
{
    if (continuous)
        *continuous = false;

    if (settings.isNull() || settings.codec().isNull() || settings.codec() == QLatin1String("aac")) {
        return QList<int>() << 8000 << 11025 << 12000 << 16000 << 22050
                            << 24000 << 32000 << 44100 << 48000 << 96000;
    } else if (settings.codec() == QLatin1String("amr-nb")) {
        return QList<int>() << 8000;
    } else if (settings.codec() == QLatin1String("amr-wb")) {
        return QList<int>() << 16000;
    }

    return QList<int>();
}

QAudioEncoderSettings QAndroidAudioEncoderSettingsControl::audioSettings() const
{
    return m_session->audioSettings();
}

void QAndroidAudioEncoderSettingsControl::setAudioSettings(const QAudioEncoderSettings &settings)
{
    m_session->setAudioSettings(settings);
}

QT_END_NAMESPACE
