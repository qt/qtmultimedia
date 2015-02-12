/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
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

    QCamera::LockTypes supportedLocks() const;

    QCamera::LockStatus lockStatus(QCamera::LockType lock) const;

    void searchAndLock(QCamera::LockTypes locks);
    void unlock(QCamera::LockTypes locks);

protected:
#if GST_CHECK_VERSION(1, 2, 0)
    void timerEvent(QTimerEvent *event);
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
