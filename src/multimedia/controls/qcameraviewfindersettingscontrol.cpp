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

#include "qcameraviewfindersettingscontrol.h"
#include "qmediacontrol_p.h"

QT_BEGIN_NAMESPACE

/*!
    \class QCameraViewfinderSettingsControl
    \inmodule QtMultimedia

    \ingroup multimedia
    \ingroup multimedia_control


    \brief The QCameraViewfinderSettingsControl class provides an abstract class
    for controlling camera viewfinder parameters.

    The interface name of QCameraViewfinderSettingsControl is \c org.qt-project.qt.cameraviewfindersettingscontrol/5.0 as
    defined in QCameraViewfinderSettingsControl_iid.

    \sa QMediaService::requestControl(), QCamera
*/

/*!
    \macro QCameraViewfinderSettingsControl_iid

    \c org.qt-project.qt.cameraviewfinderresettingscontrol/5.0

    Defines the interface name of the QCameraViewfinderSettingsControl class.

    \relates QCameraViewfinderSettingsControl
*/

/*!
    Constructs a camera viewfinder control object with \a parent.
*/
QCameraViewfinderSettingsControl::QCameraViewfinderSettingsControl(QObject *parent)
    : QMediaControl(*new QMediaControlPrivate, parent)
{
}

/*!
    Destroys the camera viewfinder control object.
*/
QCameraViewfinderSettingsControl::~QCameraViewfinderSettingsControl()
{
}

/*!
  \enum QCameraViewfinderSettingsControl::ViewfinderParameter
  \value Resolution
         Viewfinder resolution, QSize.
  \value PixelAspectRatio
         Pixel aspect ratio, QSize as in QVideoSurfaceFormat::pixelAspectRatio
  \value MinimumFrameRate
         Minimum viewfinder frame rate, qreal
  \value MaximumFrameRate
         Maximum viewfinder frame rate, qreal
  \value PixelFormat
         Viewfinder pixel format, QVideoFrame::PixelFormat
  \value UserParameter
         The base value for platform specific extended parameters.
         For such parameters the sequential values starting from UserParameter shuld be used.
*/

/*!
    \fn QCameraViewfinderSettingsControl::isViewfinderParameterSupported(ViewfinderParameter parameter)

    Returns true if configuration of viewfinder \a parameter is supported by camera backend.
*/

/*!
    \fn QCameraViewfinderSettingsControl::viewfinderParameter(ViewfinderParameter parameter) const

    Returns the value of viewfinder \a parameter.
*/

/*!
    \fn QCameraViewfinderSettingsControl::setViewfinderParameter(ViewfinderParameter parameter, const QVariant &value)

    Set the prefferred \a value of viewfinder \a parameter.

    Calling this while the camera is active may result in the camera being
    stopped and reloaded. If video recording is in progress, this call may be ignored.

    If an unsupported parameter is specified the camera may fail to load,
    or the setting may be ignored.

    Viewfinder parameters may also depend on other camera settings,
    especially in video capture mode. If camera configuration conflicts
    with viewfinder settings, the camara configuration is usually preferred.
*/

#include "moc_qcameraviewfindersettingscontrol.cpp"
QT_END_NAMESPACE

