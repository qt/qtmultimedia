/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qdeclarativecamera_p.h"
#include "qdeclarativecameraflash_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmlclass CameraFlash QDeclarativeCameraFlash
    \inqmlmodule QtMultimedia 5
    \brief The CameraFlash element provides interface for flash related camera settings.
    \ingroup multimedia_qml
    \ingroup camera_qml

    This element is part of the \b{QtMultimedia 5.0} module.

    The CameraFlash element allows you to operate the camera flash
    hardware and control the flash mode used.  Not all cameras have
    flash hardware (and in some cases it is shared with the
    \l {Torch}{torch} hardware).

    It should not be constructed separately but provided by the
    \l Camera element's \c flash property.

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
    \qmlproperty bool QtMultimedia5::QDeclarativeCameraFlash::ready
    \property bool QDeclarativeCameraFlash::ready

    Indicates flash is charged.
*/
bool QDeclarativeCameraFlash::isFlashReady() const
{
    return m_exposure->isFlashReady();
}

/*!
    \qmlproperty enumeration QtMultimedia5::CameraExposure::flashMode
    \property QDeclarativeCameraFlash::flashMode

    \table
    \header \li Value \li Description
    \row \li Camera.FlashOff             \li Flash is Off.
    \row \li Camera.FlashOn              \li Flash is On.
    \row \li Camera.FlashAuto            \li Automatic flash.
    \row \li Camera.FlashRedEyeReduction \li Red eye reduction flash.
    \row \li Camera.FlashFill            \li Use flash to fillin shadows.
    \row \li Camera.FlashTorch           \li Constant light source, useful for focusing and video capture.
    \row \li Camera.FlashSlowSyncFrontCurtain
                                \li Use the flash in conjunction with a slow shutter speed.
                                This mode allows better exposure of distant objects and/or motion blur effect.
    \row \li Camera.FlashSlowSyncRearCurtain
                                \li The similar mode to FlashSlowSyncFrontCurtain but flash is fired at the end of exposure.
    \row \li Camera.FlashManual          \li Flash power is manually set.
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
    \qmlsignal QtMultimedia5::CameraExposure::flashModeChanged(int)
    \fn void QDeclarativeCameraFlash::flashModeChanged(int)
*/

/*!
    \qmlsignal QtMultimedia5::CameraExposure::flashReady(bool)
    \fn void QDeclarativeCameraFlash::flashReady(bool)
*/

QT_END_NAMESPACE

#include "moc_qdeclarativecameraflash_p.cpp"
