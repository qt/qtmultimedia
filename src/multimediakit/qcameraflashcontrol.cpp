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

#include <qcameraflashcontrol.h>
#include  "qmediacontrol_p.h"

QT_BEGIN_NAMESPACE

/*!
    \class QCameraFlashControl

    \brief The QCameraFlashControl class allows controlling a camera's flash.

    \ingroup multimedia-serv
    \inmodule QtMultimediaKit
    \since 1.1

    \inmodule QtMultimediaKit

    You can set the type of flash effect used when an image is captured, and test to see
    if the flash hardware is ready to fire.

    You can retrieve this control from the camera object in the usual way:

    Some camera devices may not have flash hardware, or may not be configurable.  In that
    case, there will be no QCameraFlashControl available.

    The interface name of QCameraFlashControl is \c com.nokia.Qt.QCameraFlashControl/1.0 as
    defined in QCameraFlashControl_iid.

    \sa QCamera
*/

/*!
    \macro QCameraFlashControl_iid

    \c com.nokia.Qt.QCameraFlashControl/1.0

    Defines the interface name of the QCameraFlashControl class.

    \relates QCameraFlashControl
*/

/*!
    Constructs a camera flash control object with \a parent.
*/
QCameraFlashControl::QCameraFlashControl(QObject *parent):
    QMediaControl(*new QMediaControlPrivate, parent)
{
}

/*!
    Destroys the camera control object.
*/
QCameraFlashControl::~QCameraFlashControl()
{
}

/*!
  \fn QCamera::FlashModes QCameraFlashControl::flashMode() const

  Returns the current flash mode.
  \since 1.1
*/

/*!
  \fn void QCameraFlashControl::setFlashMode(QCameraExposure::FlashModes mode)

  Set the current flash \a mode.

  Usually a single QCameraExposure::FlashMode flag is used,
  but some non conflicting flags combination are also allowed,
  like QCameraExposure::FlashManual | QCameraExposure::FlashSlowSyncRearCurtain.
  \since 1.1
*/


/*!
  \fn QCameraFlashControl::isFlashModeSupported(QCameraExposure::FlashModes mode) const

  Return true if the reqested flash \a mode is supported.
  Some QCameraExposure::FlashMode values can be combined,
  for example QCameraExposure::FlashManual | QCameraExposure::FlashSlowSyncRearCurtain
  \since 1.1
*/

/*!
  \fn bool QCameraFlashControl::isFlashReady() const

  Returns true if flash is charged.
  \since 1.1
*/

/*!
    \fn void QCameraFlashControl::flashReady(bool ready)

    Signal emitted when flash state changes to \a ready.
    \since 1.1
*/

#include "moc_qcameraflashcontrol.cpp"
QT_END_NAMESPACE

