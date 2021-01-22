/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd and/or its subsidiary(-ies).
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

#ifndef QWINRTCAMERALOCKSCONTROL_H
#define QWINRTCAMERALOCKSCONTROL_H

#include <QCameraLocksControl>

QT_BEGIN_NAMESPACE

class QWinRTCameraControl;
class QWinRTCameraLocksControl : public QCameraLocksControl
{
    Q_OBJECT
public:
    explicit QWinRTCameraLocksControl(QObject *parent);

    QCamera::LockTypes supportedLocks() const override;
    QCamera::LockStatus lockStatus(QCamera::LockType lock) const override;
    void searchAndLock(QCamera::LockTypes locks) override;
    void unlock(QCamera::LockTypes locks) override;
    void initialize();

private:
    Q_INVOKABLE void searchAndLockFocus();
    Q_INVOKABLE void unlockFocus();
    QCamera::LockTypes m_supportedLocks;
    QCamera::LockStatus m_focusLockStatus;
};

QT_END_NAMESPACE

#endif // QWINRTCAMERALOCKSCONTROL_H
