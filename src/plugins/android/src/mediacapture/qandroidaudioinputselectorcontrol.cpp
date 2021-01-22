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

#include "qandroidaudioinputselectorcontrol.h"

#include "qandroidcapturesession.h"

QT_BEGIN_NAMESPACE

QAndroidAudioInputSelectorControl::QAndroidAudioInputSelectorControl(QAndroidCaptureSession *session)
    : QAudioInputSelectorControl()
    , m_session(session)
{
    connect(m_session, SIGNAL(audioInputChanged(QString)), this, SIGNAL(activeInputChanged(QString)));
}

QList<QString> QAndroidAudioInputSelectorControl::availableInputs() const
{
    return QList<QString>() << QLatin1String("default")
                            << QLatin1String("mic")
                            << QLatin1String("voice_uplink")
                            << QLatin1String("voice_downlink")
                            << QLatin1String("voice_call")
                            << QLatin1String("voice_recognition");
}

QString QAndroidAudioInputSelectorControl::inputDescription(const QString& name) const
{
    return availableDeviceDescription(name.toLatin1());
}

QString QAndroidAudioInputSelectorControl::defaultInput() const
{
    return QLatin1String("default");
}

QString QAndroidAudioInputSelectorControl::activeInput() const
{
    return m_session->audioInput();
}

void QAndroidAudioInputSelectorControl::setActiveInput(const QString& name)
{
    m_session->setAudioInput(name);
}

QList<QByteArray> QAndroidAudioInputSelectorControl::availableDevices()
{
    return QList<QByteArray>() << "default"
                               << "mic"
                               << "voice_uplink"
                               << "voice_downlink"
                               << "voice_call"
                               << "voice_recognition";
}

QString QAndroidAudioInputSelectorControl::availableDeviceDescription(const QByteArray &device)
{
    if (device == "default")
        return QLatin1String("Default audio source");
    else if (device == "mic")
        return QLatin1String("Microphone audio source");
    else if (device == "voice_uplink")
        return QLatin1String("Voice call uplink (Tx) audio source");
    else if (device == "voice_downlink")
        return QLatin1String("Voice call downlink (Rx) audio source");
    else if (device == "voice_call")
        return QLatin1String("Voice call uplink + downlink audio source");
    else if (device == "voice_recognition")
        return QLatin1String("Microphone audio source tuned for voice recognition");
    else
        return QString();
}

QT_END_NAMESPACE
