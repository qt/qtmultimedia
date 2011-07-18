/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Toolkit.
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

#include <qcameralockscontrol.h>
#include  "qmediacontrol_p.h"

QT_BEGIN_NAMESPACE

/*!
    \class QCameraLocksControl



    \brief The QCameraLocksControl class is an abstract base class for
    classes that control still cameras or video cameras.

    \inmodule QtMultimediaKit
    \ingroup camera
    \since 1.1

    This service is provided by a QMediaService object via
    QMediaService::control().  It is used by QCamera.

    The interface name of QCameraLocksControl is \c com.nokia.Qt.QCameraLocksControl/1.0 as
    defined in QCameraLocksControl_iid.


    \sa QMediaService::requestControl(), QCamera
*/

/*!
    \macro QCameraLocksControl_iid

    \c com.nokia.Qt.QCameraLocksControl/1.0

    Defines the interface name of the QCameraLocksControl class.

    \relates QCameraLocksControl
*/

/*!
    Constructs a camera locks control object with \a parent.
*/

QCameraLocksControl::QCameraLocksControl(QObject *parent):
    QMediaControl(*new QMediaControlPrivate, parent)
{
}

/*!
    Destruct the camera locks control object.
*/

QCameraLocksControl::~QCameraLocksControl()
{
}

/*!
    \fn QCameraLocksControl::supportedLocks() const

    Returns the lock types, the camera supports.
    \since 1.1
*/

/*!
    \fn QCameraLocksControl::lockStatus(QCamera::LockType lock) const

    Returns the camera \a lock status.
    \since 1.1
*/

/*!
    \fn QCameraLocksControl::searchAndLock(QCamera::LockTypes locks)

    Request camera \a locks.
    \since 1.1
*/

/*!
    \fn QCameraLocksControl::unlock(QCamera::LockTypes locks)

    Unlock camera \a locks.
    \since 1.1
*/

/*!
    \fn QCameraLocksControl::lockStatusChanged(QCamera::LockType lock, QCamera::LockStatus status, QCamera::LockChangeReason reason)

    Signals the \a lock \a status was changed with a specified \a reason.
    \since 1.1
*/



#include "moc_qcameralockscontrol.cpp"
QT_END_NAMESPACE
