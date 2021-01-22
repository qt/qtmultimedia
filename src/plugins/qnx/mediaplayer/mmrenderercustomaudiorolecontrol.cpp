/****************************************************************************
**
** Copyright (C) 2017 QNX Software Systems. All rights reserved.
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
#include "mmrenderercustomaudiorolecontrol.h"
#include "mmrendererutil.h"

QT_BEGIN_NAMESPACE

MmRendererCustomAudioRoleControl::MmRendererCustomAudioRoleControl(QObject *parent)
    : QCustomAudioRoleControl(parent)
{
}

QString MmRendererCustomAudioRoleControl::customAudioRole() const
{
    return m_role;
}

void MmRendererCustomAudioRoleControl::setCustomAudioRole(const QString &role)
{
    if (m_role != role) {
        m_role = role;
        emit customAudioRoleChanged(m_role);
    }
}

QStringList MmRendererCustomAudioRoleControl::supportedCustomAudioRoles() const
{
    return QStringList();
}

QT_END_NAMESPACE
