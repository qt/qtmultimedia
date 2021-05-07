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

#include "qplatformcamera_p.h"

QT_BEGIN_NAMESPACE

/*!
    \class QPlatformCamera
    \obsolete



    \brief The QPlatformCamera class is an abstract base class for
    classes that control still cameras or video cameras.

    \inmodule QtMultimedia

    \ingroup multimedia_control
*/

/*!
    Constructs a camera control object with \a parent.
*/

QPlatformCamera::QPlatformCamera(QCamera *parent)
  : QObject(parent),
    m_camera(parent)
{
}

QCameraFormat QPlatformCamera::findBestCameraFormat(const QCameraInfo &camera)
{
    QCameraFormat f;
    const auto formats = camera.videoFormats();
    for (const auto &fmt : formats) {
        // check if fmt is better. We try to find the highest resolution that offers
        // at least 30 FPS
        if (f.maxFrameRate() < 30 && fmt.maxFrameRate() > f.maxFrameRate())
            f = fmt;
        else if (f.maxFrameRate() == fmt.maxFrameRate() &&
                 f.resolution().width()*f.resolution().height() < fmt.resolution().width()*fmt.resolution().height())
            f = fmt;
    }
    return f;
}

/*!
    \fn QPlatformCamera::state() const

    Returns the state of the camera service.

    \sa QCamera::state
*/

/*!
    \fn QPlatformCamera::setState(QCamera::State state)

    Sets the camera \a state.

    State changes are synchronous and indicate user intention,
    while camera status is used as a feedback mechanism to inform application about backend status.
    Status changes are reported asynchronously with QPlatformCamera::statusChanged() signal.

    \sa QCamera::State
*/

/*!
    \fn void QPlatformCamera::stateChanged(QCamera::State state)

    Signal emitted when the camera \a state changes.

    In most cases the state chage is caused by QPlatformCamera::setState(),
    but if critical error has occurred the state changes to QCamera::UnloadedState.
*/

/*!
    \fn QPlatformCamera::status() const

    Returns the status of the camera service.

    \sa QCamera::state
*/

/*!
    \fn void QPlatformCamera::statusChanged(QCamera::Status status)

    Signal emitted when the camera \a status changes.
*/


/*!
    \fn void QPlatformCamera::error(int error, const QString &errorString)

    Signal emitted when an error occurs with error code \a error and
    a description of the error \a errorString.
*/

/*!
    \fn QPlatformCamera::supportedViewfinderSettings() const

    Returns a list of supported camera viewfinder settings.

    The list is ordered by preference; preferred settings come first.
*/

/*!
    \fn QPlatformCamera::viewfinderSettings() const

    Returns the viewfinder settings.

    If undefined or unsupported values are passed to QPlatformCamera::setViewfinderSettings(),
    this function returns the actual settings used by the camera viewfinder. These may be available
    only once the camera is active.
*/

/*!
    \fn QPlatformCamera::setViewfinderSettings(const QCameraViewfinderSettings &settings)

    Sets the camera viewfinder \a settings.
*/


void QPlatformCamera::statusChanged(QCamera::Status s)
{
    if (s == m_status)
        return;
    m_status = s;
    emit m_camera->statusChanged(s);
}


QT_END_NAMESPACE

#include "moc_qplatformcamera_p.cpp"
