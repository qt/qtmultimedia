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
#ifndef MMRENDERERAUDIOROLECONTROL_H
#define MMRENDERERAUDIOROLECONTROL_H

#include <qaudiorolecontrol.h>

QT_BEGIN_NAMESPACE

class MmRendererAudioRoleControl : public QAudioRoleControl
{
    Q_OBJECT
public:
    explicit MmRendererAudioRoleControl(QObject *parent = 0);

    QAudio::Role audioRole() const override;
    void setAudioRole(QAudio::Role role) override;

    QList<QAudio::Role> supportedAudioRoles() const override;

private:
    QAudio::Role m_role;
};

QT_END_NAMESPACE

#endif
