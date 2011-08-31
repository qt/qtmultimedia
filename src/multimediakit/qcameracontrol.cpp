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

#include <qcameracontrol.h>
#include  "qmediacontrol_p.h"

QT_BEGIN_NAMESPACE

/*!
    \class QCameraControl



    \brief The QCameraControl class is an abstract base class for
    classes that control still cameras or video cameras.

    \inmodule QtMultimediaKit
    \ingroup multimedia-serv
    \since 1.1

    This service is provided by a QMediaService object via
    QMediaService::control().  It is used by QCamera.

    The interface name of QCameraControl is \c com.nokia.Qt.QCameraControl/1.0 as
    defined in QCameraControl_iid.



    \sa QMediaService::requestControl(), QCamera
*/

/*!
    \macro QCameraControl_iid

    \c com.nokia.Qt.QCameraControl/1.0

    Defines the interface name of the QCameraControl class.

    \relates QCameraControl
*/

/*!
    Constructs a camera control object with \a parent.
*/

QCameraControl::QCameraControl(QObject *parent):
    QMediaControl(*new QMediaControlPrivate, parent)
{
}

/*!
    Destruct the camera control object.
*/

QCameraControl::~QCameraControl()
{
}

/*!
    \fn QCameraControl::state() const

    Returns the state of the camera service.

    \since 1.1
    \sa QCamera::state
*/

/*!
    \fn QCameraControl::setState(QCamera::State state)

    Sets the camera \a state.

    State changes are synchronous and indicate user intention,
    while camera status is used as a feedback mechanism to inform application about backend status.
    Status changes are reported asynchronously with QCameraControl::statusChanged() signal.

    \since 1.1
    \sa QCamera::State
*/

/*!
    \fn void QCameraControl::stateChanged(QCamera::State state)

    Signal emitted when the camera \a state changes.

    In most cases the state chage is caused by QCameraControl::setState(),
    but if critical error has occurred the state changes to QCamera::UnloadedState.
    \since 1.1
*/

/*!
    \fn QCameraControl::status() const

    Returns the status of the camera service.

    \since 1.1
    \sa QCamera::state
*/

/*!
    \fn void QCameraControl::statusChanged(QCamera::Status status)

    Signal emitted when the camera \a status changes.
    \since 1.1
*/


/*!
    \fn void QCameraControl::error(int error, const QString &errorString)

    Signal emitted when an error occurs with error code \a error and
    a description of the error \a errorString.
    \since 1.1
*/

/*!
    \fn Camera::CaptureMode QCameraControl::captureMode() const = 0

    Returns the current capture mode.
    \since 1.1
*/

/*!
    \fn void QCameraControl::setCaptureMode(QCamera::CaptureMode mode) = 0;

    Sets the current capture \a mode.

    The capture mode changes are synchronous and allowed in any camera state.

    If the capture mode is changed while camera is active,
    it's recommended to change status to QCamera::LoadedStatus
    and start activating the camera in the next event loop
    with the status changed to QCamera::StartingStatus.
    This allows the capture settings to be applied before camera is started.
    Than change the status to QCamera::StartedStatus when the capture mode change is done.
    \since 1.1
*/

/*!
    \fn bool QCameraControl::isCaptureModeSupported(QCamera::CaptureMode mode) const = 0;

    Returns true if the capture \a mode is suported.
    \since 1.1
*/

/*!
    \fn QCameraControl::captureModeChanged(QCamera::CaptureMode mode)

    Signal emitted when the camera capture \a mode changes.
    \since 1.1
 */

/*!
    \fn bool QCameraControl::canChangeProperty(PropertyChangeType changeType, QCamera::Status status) const

    Returns true if backend can effectively apply changing camera properties of \a changeType type
    while the camera state is QCamera::Active and camera status matches \a status parameter.

    If backend doesn't support applying this change in the active state, it will be stopped
    before the settings are changed and restarted after.
    Otherwise the backend should apply the change in the current state,
    with the camera status indicating the progress, if necessary.
    \since 1.1
*/

/*!
  \enum QCameraControl::PropertyChangeType

  \value CaptureMode Indicates the capture mode is changed.
  \value ImageEncodingSettings Image encoder settings are changed, including resolution.
  \value VideoEncodingSettings
        Video encoder settings are changed, including audio, video and container settings.
  \value Viewfinder Viewfinder is changed.
*/

#include "moc_qcameracontrol.cpp"
QT_END_NAMESPACE
