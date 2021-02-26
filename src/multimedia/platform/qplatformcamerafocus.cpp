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

#include "qplatformcamerafocus_p.h"

QT_BEGIN_NAMESPACE

/*!
    \class QPlatformCameraFocus
    \obsolete


    \brief The QPlatformCameraFocus class supplies control for
    focusing related camera parameters.

    \inmodule QtMultimedia
    \ingroup multimedia_control

    \sa QCamera
*/

/*!
    Constructs a camera control object with \a parent.
*/

QPlatformCameraFocus::QPlatformCameraFocus(QObject *parent)
    : QObject(parent)
{
}

/*!
  \fn QCameraFocus::FocusModes QPlatformCameraFocus::focusMode() const

  Returns the focus mode being used.
*/


/*!
  \fn void QPlatformCameraFocus::setFocusMode(QCameraFocus::FocusModes mode)

  Set the focus mode to \a mode.
*/


/*!
  \fn bool QPlatformCameraFocus::isFocusModeSupported(QCameraFocus::FocusModes mode) const

  Returns true if focus \a mode is supported.
*/

/*!
  \fn QPlatformCameraFocus::focusPointMode() const

  Returns the camera focus point selection mode.
*/

/*!
  \fn QPlatformCameraFocus::setFocusPointMode(QCameraFocus::FocusPointMode mode)

  Sets the camera focus point selection \a mode.
*/

/*!
  \fn QPlatformCameraFocus::isFocusPointModeSupported(QCameraFocus::FocusPointMode mode) const

  Returns true if the camera focus point \a mode is supported.
*/

/*!
  \fn QPlatformCameraFocus::customFocusPoint() const

  Return the position of custom focus point, in relative frame coordinates:
  QPointF(0,0) points to the left top frame point, QPointF(0.5,0.5) points to the frame center.

  Custom focus point is used only in FocusPointCustom focus mode.
*/

/*!
  \fn QPlatformCameraFocus::setCustomFocusPoint(const QPointF &point)

  Sets the custom focus \a point.

  \sa QPlatformCameraFocus::customFocusPoint()
*/

/*!
  \fn void QPlatformCameraFocus::focusModeChanged(QCameraFocus::FocusModes mode)

  Signal is emitted when the focus \a mode is changed,
  usually in result of QPlatformCameraFocus::setFocusMode call or capture mode changes.

  \sa QPlatformCameraFocus::focusMode(), QPlatformCameraFocus::setFocusMode()
*/

/*!
  \fn void QPlatformCameraFocus::focusPointModeChanged(QCameraFocus::FocusPointMode mode)

  Signal is emitted when the focus point \a mode is changed,
  usually in result of QPlatformCameraFocus::setFocusPointMode call or capture mode changes.

  \sa QPlatformCameraFocus::focusPointMode(), QPlatformCameraFocus::setFocusPointMode()
*/

/*!
  \fn void QPlatformCameraFocus::customFocusPointChanged(const QPointF &point)

  Signal is emitted when the custom focus \a point is changed.

  \sa QPlatformCameraFocus::customFocusPoint(), QPlatformCameraFocus::setCustomFocusPoint()
*/

/*!
  \fn qreal QPlatformCameraFocus::maximumOpticalZoom() const

  Returns the maximum optical zoom value, or 1.0 if optical zoom is not supported.
*/


/*!
  \fn qreal QPlatformCameraFocus::maximumDigitalZoom() const

  Returns the maximum digital zoom value, or 1.0 if digital zoom is not supported.
*/


/*!
  \fn qreal QPlatformCameraFocus::requestedOpticalZoom() const

  Return the requested optical zoom value.
*/

/*!
  \fn qreal QPlatformCameraFocus::requestedDigitalZoom() const

  Return the requested digital zoom value.
*/

/*!
  \fn qreal QPlatformCameraFocus::currentOpticalZoom() const

  Return the current optical zoom value.
*/

/*!
  \fn qreal QPlatformCameraFocus::currentDigitalZoom() const

  Return the current digital zoom value.
*/

/*!
  \fn void QPlatformCameraFocus::zoomTo(qreal optical, qreal digital)

  Sets \a optical and \a digital zoom values.

  Zooming can be asynchronous with value changes reported with
  currentDigitalZoomChanged() and currentOpticalZoomChanged() signals.

  The backend should expect and correctly handle frequent zoomTo() calls
  during zoom animations or slider movements.
*/


/*!
    \fn void QPlatformCameraFocus::currentOpticalZoomChanged(qreal zoom)

    Signal emitted when the current optical \a zoom value changed.
*/

/*!
    \fn void QPlatformCameraFocus::currentDigitalZoomChanged(qreal zoom)

    Signal emitted when the current digital \a zoom value changed.
*/

/*!
    \fn void QPlatformCameraFocus::requestedOpticalZoomChanged(qreal zoom)

    Signal emitted when the requested optical \a zoom value changed.
*/

/*!
    \fn void QPlatformCameraFocus::requestedDigitalZoomChanged(qreal zoom)

    Signal emitted when the requested digital \a zoom value changed.
*/


/*!
    \fn void QPlatformCameraFocus::maximumOpticalZoomChanged(qreal zoom)

    Signal emitted when the maximum supported optical \a zoom value changed.

    The maximum supported zoom value can depend on other camera settings,
    like focusing mode.
*/

/*!
    \fn void QPlatformCameraFocus::maximumDigitalZoomChanged(qreal zoom)

    Signal emitted when the maximum supported digital \a zoom value changed.

    The maximum supported zoom value can depend on other camera settings,
    like capture mode or resolution.
*/

QT_END_NAMESPACE

#include "moc_qplatformcamerafocus_p.cpp"
