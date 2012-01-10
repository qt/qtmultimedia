/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef MOCKCAMERALOCKCONTROL_H
#define MOCKCAMERALOCKCONTROL_H

#include <QTimer>
#include "qcameralockscontrol.h"

class MockCameraLocksControl : public QCameraLocksControl
{
    Q_OBJECT
public:
    MockCameraLocksControl(QObject *parent = 0):
            QCameraLocksControl(parent),
            m_focusLock(QCamera::Unlocked),
            m_exposureLock(QCamera::Unlocked)
    {
    }

    ~MockCameraLocksControl() {}

    QCamera::LockTypes supportedLocks() const
    {
        return QCamera::LockExposure | QCamera::LockFocus;
    }

    QCamera::LockStatus lockStatus(QCamera::LockType lock) const
    {
        switch (lock) {
        case QCamera::LockExposure:
            return m_exposureLock;
        case QCamera::LockFocus:
            return m_focusLock;
        default:
            return QCamera::Unlocked;
        }
    }

    void searchAndLock(QCamera::LockTypes locks)
    {
        if (locks & QCamera::LockExposure) {
            QCamera::LockStatus newStatus = locks & QCamera::LockFocus ? QCamera::Searching : QCamera::Locked;

            if (newStatus != m_exposureLock)
                emit lockStatusChanged(QCamera::LockExposure,
                                       m_exposureLock = newStatus,
                                       QCamera::UserRequest);
        }

        if (locks & QCamera::LockFocus) {
            emit lockStatusChanged(QCamera::LockFocus,
                                   m_focusLock = QCamera::Searching,
                                   QCamera::UserRequest);

            QTimer::singleShot(5, this, SLOT(focused()));
        }
    }

    void unlock(QCamera::LockTypes locks) {
        if (locks & QCamera::LockFocus && m_focusLock != QCamera::Unlocked) {
            emit lockStatusChanged(QCamera::LockFocus,
                                   m_focusLock = QCamera::Unlocked,
                                   QCamera::UserRequest);
        }

        if (locks & QCamera::LockExposure && m_exposureLock != QCamera::Unlocked) {
            emit lockStatusChanged(QCamera::LockExposure,
                                   m_exposureLock = QCamera::Unlocked,
                                   QCamera::UserRequest);
        }
    }

    /* helper method to emit the signal with LockChangeReason */
    void setLockChangeReason (QCamera::LockChangeReason lockChangeReason)
    {
        emit lockStatusChanged(QCamera::NoLock,
                               QCamera::Unlocked,
                               lockChangeReason);

    }

private slots:
    void focused()
    {
        if (m_focusLock == QCamera::Searching) {
            emit lockStatusChanged(QCamera::LockFocus,
                                   m_focusLock = QCamera::Locked,
                                   QCamera::UserRequest);
        }

        if (m_exposureLock == QCamera::Searching) {
            emit lockStatusChanged(QCamera::LockExposure,
                                   m_exposureLock = QCamera::Locked,
                                   QCamera::UserRequest);
        }
    }


private:
    QCamera::LockStatus m_focusLock;
    QCamera::LockStatus m_exposureLock;
};


#endif // MOCKCAMERALOCKCONTROL_H
