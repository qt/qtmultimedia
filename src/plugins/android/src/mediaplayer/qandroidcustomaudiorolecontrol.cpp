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

#include "qandroidcustomaudiorolecontrol.h"

QT_BEGIN_NAMESPACE

QAndroidCustomAudioRoleControl::QAndroidCustomAudioRoleControl(QObject *parent)
    : QCustomAudioRoleControl(parent)
{
}

QString QAndroidCustomAudioRoleControl::customAudioRole() const
{
    return m_role;
}

void QAndroidCustomAudioRoleControl::setCustomAudioRole(const QString &role)
{
    if (m_role == role)
        return;

    m_role = role;
    emit customAudioRoleChanged(m_role);
}

QStringList QAndroidCustomAudioRoleControl::supportedCustomAudioRoles() const
{
    return QStringList()
        << "CONTENT_TYPE_MOVIE"
        << "CONTENT_TYPE_MUSIC"
        << "CONTENT_TYPE_SONIFICATION"
        << "CONTENT_TYPE_SPEECH"
        << "USAGE_ALARM"
        << "USAGE_ASSISTANCE_ACCESSIBILITY"
        << "USAGE_ASSISTANCE_NAVIGATION_GUIDANCE"
        << "USAGE_ASSISTANCE_SONIFICATION"
        << "USAGE_ASSISTANT"
        << "USAGE_GAME"
        << "USAGE_MEDIA"
        << "USAGE_NOTIFICATION"
        << "USAGE_NOTIFICATION_COMMUNICATION_DELAYED"
        << "USAGE_NOTIFICATION_COMMUNICATION_INSTANT"
        << "USAGE_NOTIFICATION_COMMUNICATION_REQUEST"
        << "USAGE_NOTIFICATION_EVENT"
        << "USAGE_NOTIFICATION_RINGTONE"
        << "USAGE_VOICE_COMMUNICATION"
        << "USAGE_VOICE_COMMUNICATION_SIGNALLING";
}

QT_END_NAMESPACE
