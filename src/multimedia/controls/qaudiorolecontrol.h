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

#ifndef QAUDIOROLECONTROL_H
#define QAUDIOROLECONTROL_H

#include <QtMultimedia/qmediacontrol.h>
#include <QtMultimedia/qaudio.h>

QT_BEGIN_NAMESPACE

// Class forward declaration required for QDoc bug
class QString;

class Q_MULTIMEDIA_EXPORT QAudioRoleControl : public QMediaControl
{
    Q_OBJECT

public:
    virtual ~QAudioRoleControl();

    virtual QAudio::Role audioRole() const = 0;
    virtual void setAudioRole(QAudio::Role role) = 0;

    virtual QList<QAudio::Role> supportedAudioRoles() const = 0;

Q_SIGNALS:
    void audioRoleChanged(QAudio::Role role);

protected:
    explicit QAudioRoleControl(QObject *parent = nullptr);
};

#define QAudioRoleControl_iid "org.qt-project.qt.audiorolecontrol/5.6"
Q_MEDIA_DECLARE_CONTROL(QAudioRoleControl, QAudioRoleControl_iid)

QT_END_NAMESPACE

#endif // QAUDIOROLECONTROL_H
