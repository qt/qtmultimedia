/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qcameraviewfindersettingscontrol.h"

QT_BEGIN_NAMESPACE

/*!
    \class QCameraviewfinderSettingsControl
    \inmodule QtMultimedia
    \ingroup multimedia_control
    \since 5.5

    \brief The QCameraviewfinderSettingsControl class provides access to the viewfinder settings
    of a camera media service.

    The functionality provided by this control is exposed to application code through the QCamera class.

    The interface name of QCameraviewfinderSettingsControl is \c org.qt-project.qt.cameraviewfinderSettingsControl/6.0 as
    defined in QCameraViewfinderSettingsControl_iid.

    \sa QMediaService::requestControl(), QCameraViewfinderSettings, QCamera
*/

/*!
    \macro QCameraViewfinderSettingsControl_iid

    \c org.qt-project.qt.cameraviewfinderSettingsControl/5.5

    Defines the interface name of the QCameraviewfinderSettingsControl class.

    \relates QCameraviewfinderSettingsControl
*/

/*!
    Constructs a camera viewfinder settings control object with \a parent.
*/
QCameraViewfinderSettingsControl::QCameraViewfinderSettingsControl(QObject *parent)
    : QObject(parent)
{
}

/*!
    \fn QCameraviewfinderSettingsControl::supportedViewfinderSettings() const

    Returns a list of supported camera viewfinder settings.

    The list is ordered by preference; preferred settings come first.
*/

/*!
    \fn QCameraviewfinderSettingsControl::viewfinderSettings() const

    Returns the viewfinder settings.

    If undefined or unsupported values are passed to QCameraviewfinderSettingsControl::setViewfinderSettings(),
    this function returns the actual settings used by the camera viewfinder. These may be available
    only once the camera is active.
*/

/*!
    \fn QCameraviewfinderSettingsControl::setViewfinderSettings(const QCameraViewfinderSettings &settings)

    Sets the camera viewfinder \a settings.
*/

QT_END_NAMESPACE

#include "moc_qcameraviewfindersettingscontrol.cpp"
