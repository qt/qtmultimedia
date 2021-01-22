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

#ifndef CAMERABINLOCKSCONTROL_H
#define CAMERABINLOCKSCONTROL_H

#include <qcamera.h>
#include <qcameralockscontrol.h>

#include <QtCore/qbasictimer.h>

#include <gst/gst.h>
#include <glib.h>

QT_BEGIN_NAMESPACE

class CameraBinSession;
class CameraBinFocus;

class CameraBinLocks  : public QCameraLocksControl
{
    Q_OBJECT

public:
    CameraBinLocks(CameraBinSession *session);
    virtual ~CameraBinLocks();

    QCamera::LockTypes supportedLocks() const override;

    QCamera::LockStatus lockStatus(QCamera::LockType lock) const override;

    void searchAndLock(QCamera::LockTypes locks) override;
    void unlock(QCamera::LockTypes locks) override;

protected:
#if GST_CHECK_VERSION(1, 2, 0)
    void timerEvent(QTimerEvent *event) override;
#endif

private slots:
    void updateFocusStatus(QCamera::LockStatus status, QCamera::LockChangeReason reason);

private:
#if GST_CHECK_VERSION(1, 2, 0)
    bool isExposureLocked() const;
    void lockExposure(QCamera::LockChangeReason reason);
    void unlockExposure(QCamera::LockStatus status, QCamera::LockChangeReason reason);

    bool isWhiteBalanceLocked() const;
    void lockWhiteBalance(QCamera::LockChangeReason reason);
    void unlockWhiteBalance(QCamera::LockStatus status, QCamera::LockChangeReason reason);
#endif

    CameraBinSession *m_session;
    CameraBinFocus *m_focus;
    QBasicTimer m_lockTimer;
    QCamera::LockTypes m_pendingLocks;
};

QT_END_NAMESPACE

#endif
