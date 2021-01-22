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

#include "qandroidaudiorolecontrol.h"

QT_BEGIN_NAMESPACE

QAndroidAudioRoleControl::QAndroidAudioRoleControl(QObject *parent)
    : QAudioRoleControl(parent)
{
}

QAudio::Role QAndroidAudioRoleControl::audioRole() const
{
    return m_role;
}

void QAndroidAudioRoleControl::setAudioRole(QAudio::Role role)
{
    if (m_role == role)
        return;

    m_role = role;
    emit audioRoleChanged(m_role);
}

QList<QAudio::Role> QAndroidAudioRoleControl::supportedAudioRoles() const
{
    return QList<QAudio::Role>()
        << QAudio::VoiceCommunicationRole
        << QAudio::MusicRole
        << QAudio::VideoRole
        << QAudio::SonificationRole
        << QAudio::AlarmRole
        << QAudio::NotificationRole
        << QAudio::RingtoneRole
        << QAudio::AccessibilityRole
        << QAudio::GameRole;
}

QT_END_NAMESPACE
