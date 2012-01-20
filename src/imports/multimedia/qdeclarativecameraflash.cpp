/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qdeclarativecamera_p.h"
#include "qdeclarativecameraflash_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmlclass CameraFlash QDeclarativeCameraFlash
    \brief The CameraFlash element provides interface for flash related camera settings.
    \ingroup multimedia_qml

    This element is part of the \bold{QtMultimedia 5.0} module.

    It should not be constructed separately but provided by Camera.flash.

    \qml
    import QtQuick 2.0
    import QtMultimedia 5.0

    Camera {
        id: camera

        exposure.exposureCompensation: -1.0
        flash.mode: Camera.FlashRedEyeReduction
    }

    \endqml
*/

/*!
    \class QDeclarativeCameraFlash
    \internal
    \brief The CameraFlash element provides interface for flash related camera settings.
*/

/*!
    Construct a declarative camera flash object using \a parent object.
 */
QDeclarativeCameraFlash::QDeclarativeCameraFlash(QCamera *camera, QObject *parent) :
    QObject(parent)
{
    m_exposure = camera->exposure();
    connect(m_exposure, SIGNAL(flashReady(bool)), this, SIGNAL(flashReady(bool)));
}

QDeclarativeCameraFlash::~QDeclarativeCameraFlash()
{
}

/*!
    \qmlproperty bool QDeclarativeCameraFlash::ready
    \property bool QDeclarativeCameraFlash::ready

    Indicates flash is charged.
*/
bool QDeclarativeCameraFlash::isFlashReady() const
{
    return m_exposure->isFlashReady();
}

/*!
    \qmlproperty enumeration CameraExposure::flashMode
    \property QDeclarativeCameraFlash::flashMode

    \table
    \header \o Value \o Description
    \row \o Camera.FlashOff             \o Flash is Off.
    \row \o Camera.FlashOn              \o Flash is On.
    \row \o Camera.FlashAuto            \o Automatic flash.
    \row \o Camera.FlashRedEyeReduction \o Red eye reduction flash.
    \row \o Camera.FlashFill            \o Use flash to fillin shadows.
    \row \o Camera.FlashTorch           \o Constant light source, useful for focusing and video capture.
    \row \o Camera.FlashSlowSyncFrontCurtain
                                \o Use the flash in conjunction with a slow shutter speed.
                                This mode allows better exposure of distant objects and/or motion blur effect.
    \row \o Camera.FlashSlowSyncRearCurtain
                                \o The similar mode to FlashSlowSyncFrontCurtain but flash is fired at the end of exposure.
    \row \o Camera.FlashManual          \o Flash power is manually set.
    \endtable

*/
int QDeclarativeCameraFlash::flashMode() const
{
    return m_exposure->flashMode();
}

void QDeclarativeCameraFlash::setFlashMode(int mode)
{
    if (m_exposure->flashMode() != mode) {
        m_exposure->setFlashMode(QCameraExposure::FlashModes(mode));
        emit flashModeChanged(mode);
    }
}

/*!
    \qmlsignal CameraExposure::flashModeChanged(int)
    \fn void QDeclarativeCameraFlash::flashModeChanged(int)
*/

/*!
    \qmlsignal CameraExposure::flashReady(bool)
    \fn void QDeclarativeCameraFlash::flashReady(bool)
*/

QT_END_NAMESPACE

#include "moc_qdeclarativecameraflash_p.cpp"
