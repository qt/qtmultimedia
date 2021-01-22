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
#ifndef MMRENDERERCUSTOMAUDIOROLECONTROL_H
#define MMRENDERERCUSTOMAUDIOROLECONTROL_H

#include <qcustomaudiorolecontrol.h>

QT_BEGIN_NAMESPACE

class MmRendererCustomAudioRoleControl : public QCustomAudioRoleControl
{
    Q_OBJECT
public:
    explicit MmRendererCustomAudioRoleControl(QObject *parent = 0);

    QString customAudioRole() const Q_DECL_OVERRIDE;
    void setCustomAudioRole(const QString &role) Q_DECL_OVERRIDE;

    QStringList supportedCustomAudioRoles() const Q_DECL_OVERRIDE;

private:
    QString m_role;
};

QT_END_NAMESPACE

#endif
