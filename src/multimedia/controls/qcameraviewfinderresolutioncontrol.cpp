/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qcameraviewfinderresolutioncontrol.h"
#include "qmediacontrol_p.h"

QT_BEGIN_NAMESPACE

/*!
    \class QCameraViewfinderResolutionControl
    \inmodule QtMultimedia

    \ingroup multimedia
    \ingroup multimedia_control


    \brief The QCameraViewfinderResolutionControl class provides an abstract class
    for controlling camera viewfinder parameters.

    The interface name of QCameraViewfinderResolutionControl is \c org.qt-project.qt.cameraviewfinderresolutioncontrol/5.0 as
    defined in QCameraViewfinderResolutionControl_iid.

    \sa QMediaService::requestControl(), QCamera
*/

/*!
    \macro QCameraViewfinderResolutionControl_iid

    \c org.qt-project.qt.cameraviewfinderresolutioncontrol/5.0

    Defines the interface name of the QCameraViewfinderResolutionControl class.

    \relates QCameraViewfinderResolutionControl
*/

/*!
    Constructs a camera viewfinder control object with \a parent.
*/
QCameraViewfinderResolutionControl::QCameraViewfinderResolutionControl(QObject *parent)
    : QMediaControl(*new QMediaControlPrivate, parent)
{
}

/*!
    Destroys the camera viewfinder control object.
*/
QCameraViewfinderResolutionControl::~QCameraViewfinderResolutionControl()
{
}


/*!
    \fn QCameraViewfinderResolutionControl::viewfinderResolution() const
    Returns the current resolution of the camera viewfinder stream.
*/

/*!
    \fn QCameraViewfinderResolutionControl::setViewfinderResolution(const QSize &resolution)
    Set the resolution of the camera viewfinder stream to \a resolution.

    Calling this while the camera is active may result in the camera being unloaded and
    reloaded.  If video recording is in progress, this call may be ignored.  If an unsupported
    resolution is specified the camera may fail to load, or the setting may be ignored.

    Returns false if this setting cannot be applied at this time or if the resolution is
    invalid.  Returns true if the setting will be applied (or attempted to be applied).
*/

#include "moc_qcameraviewfinderresolutioncontrol.cpp"
QT_END_NAMESPACE

