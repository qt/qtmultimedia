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

#ifndef QANDROIDCAMERALOCKSCONTROL_H
#define QANDROIDCAMERALOCKSCONTROL_H

#include <qcameralockscontrol.h>

QT_BEGIN_NAMESPACE

class QAndroidCameraSession;
class QTimer;

class QAndroidCameraLocksControl : public QCameraLocksControl
{
    Q_OBJECT
public:
    explicit QAndroidCameraLocksControl(QAndroidCameraSession *session);

    QCamera::LockTypes supportedLocks() const override;
    QCamera::LockStatus lockStatus(QCamera::LockType lock) const override;
    void searchAndLock(QCamera::LockTypes locks) override;
    void unlock(QCamera::LockTypes locks) override;

private Q_SLOTS:
    void onCameraOpened();
    void onCameraAutoFocusComplete(bool success);
    void onRecalculateTimeOut();
    void onWhiteBalanceChanged();

private:
    void setFocusLockStatus(QCamera::LockStatus status, QCamera::LockChangeReason reason);
    void setWhiteBalanceLockStatus(QCamera::LockStatus status, QCamera::LockChangeReason reason);
    void setExposureLockStatus(QCamera::LockStatus status, QCamera::LockChangeReason reason);

    QAndroidCameraSession *m_session;

    QTimer *m_recalculateTimer;

    QCamera::LockTypes m_supportedLocks;

    QCamera::LockStatus m_focusLockStatus;
    QCamera::LockStatus m_exposureLockStatus;
    QCamera::LockStatus m_whiteBalanceLockStatus;
};

QT_END_NAMESPACE

#endif // QANDROIDCAMERALOCKSCONTROL_H
