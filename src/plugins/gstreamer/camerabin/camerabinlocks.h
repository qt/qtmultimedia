/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Mobility Components.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef CAMERABINLOCKSCONTROL_H
#define CAMERABINLOCKSCONTROL_H

#include <qcamera.h>
#include <qcameralockscontrol.h>

#include <gst/gst.h>
#include <glib.h>

class CameraBinSession;
class CameraBinFocus;

QT_USE_NAMESPACE

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

private slots:
    void updateFocusStatus(QCamera::LockStatus status, QCamera::LockChangeReason reason);

private:
    CameraBinSession *m_session;
    CameraBinFocus *m_focus;
};

#endif
