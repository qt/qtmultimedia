/****************************************************************************
**
** Copyright (C) 2016 Research In Motion
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
#ifndef BBCAMERALOCKSCONTROL_H
#define BBCAMERALOCKSCONTROL_H

#include <qcameralockscontrol.h>

QT_BEGIN_NAMESPACE

class BbCameraSession;

class BbCameraLocksControl : public QCameraLocksControl
{
    Q_OBJECT
public:
    enum LocksApplyMode
    {
        IndependentMode,
        FocusExposureBoundMode,
        AllBoundMode,
        FocusOnlyMode
    };

    explicit BbCameraLocksControl(BbCameraSession *session, QObject *parent = 0);

    QCamera::LockTypes supportedLocks() const override;
    QCamera::LockStatus lockStatus(QCamera::LockType lock) const override;
    void searchAndLock(QCamera::LockTypes locks) override;
    void unlock(QCamera::LockTypes locks) override;

private Q_SLOTS:
    void cameraOpened();
    void focusStatusChanged(int value);

private:
    BbCameraSession *m_session;

    LocksApplyMode m_locksApplyMode;
    QCamera::LockStatus m_focusLockStatus;
    QCamera::LockStatus m_exposureLockStatus;
    QCamera::LockStatus m_whiteBalanceLockStatus;
    QCamera::LockTypes m_currentLockTypes;
    QCamera::LockTypes m_supportedLockTypes;
};

QT_END_NAMESPACE

#endif
