/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
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
