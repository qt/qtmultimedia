/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#include <qcamerafocuscontrol.h>
#include  "qmediacontrol_p.h"

QT_BEGIN_NAMESPACE

/*!
    \class QCameraFocusControl


    \brief The QCameraFocusControl class supplies control for
    focusing related camera parameters.

    \inmodule QtMultimedia
    \ingroup multimedia-serv
    \since 1.1

    The interface name of QCameraFocusControl is \c com.nokia.Qt.QCameraFocusControl/1.0 as
    defined in QCameraFocusControl_iid.


    \sa QMediaService::requestControl(), QCamera
*/

/*!
    \macro QCameraFocusControl_iid

    \c com.nokia.Qt.QCameraFocusControl/1.0

    Defines the interface name of the QCameraFocusControl class.

    \relates QCameraFocusControl
*/

/*!
    Constructs a camera control object with \a parent.
*/

QCameraFocusControl::QCameraFocusControl(QObject *parent):
    QMediaControl(*new QMediaControlPrivate, parent)
{
}

/*!
    Destruct the camera control object.
*/

QCameraFocusControl::~QCameraFocusControl()
{
}


/*!
  \fn QCameraFocus::FocusMode QCameraFocusControl::focusMode() const

  Returns the focus mode being used.
  \since 1.1
*/


/*!
  \fn void QCameraFocusControl::setFocusMode(QCameraFocus::FocusMode mode)

  Set the focus mode to \a mode.
  \since 1.1
*/


/*!
  \fn bool QCameraFocusControl::isFocusModeSupported(QCameraFocus::FocusMode mode) const

  Returns true if focus \a mode is supported.
  \since 1.1
*/


/*!
  \fn qreal QCameraFocusControl::maximumOpticalZoom() const

  Returns the maximum optical zoom value, or 1.0 if optical zoom is not supported.
  \since 1.1
*/


/*!
  \fn qreal QCameraFocusControl::maximumDigitalZoom() const

  Returns the maximum digital zoom value, or 1.0 if digital zoom is not supported.
  \since 1.1
*/


/*!
  \fn qreal QCameraFocusControl::opticalZoom() const

  Return the current optical zoom value.
  \since 1.1
*/

/*!
  \fn qreal QCameraFocusControl::digitalZoom() const

  Return the current digital zoom value.
  \since 1.1
*/


/*!
  \fn void QCameraFocusControl::zoomTo(qreal optical, qreal digital)

  Sets \a optical and \a digital zoom values.
  \since 1.1
*/

/*!
  \fn QCameraFocusControl::focusPointMode() const

  Returns the camera focus point selection mode.
  \since 1.1
*/

/*!
  \fn QCameraFocusControl::setFocusPointMode(QCameraFocus::FocusPointMode mode)

  Sets the camera focus point selection \a mode.
  \since 1.1
*/

/*!
  \fn QCameraFocusControl::isFocusPointModeSupported(QCameraFocus::FocusPointMode mode) const

  Returns true if the camera focus point \a mode is supported.
  \since 1.1
*/

/*!
  \fn QCameraFocusControl::customFocusPoint() const

  Return the position of custom focus point, in relative frame coordinates:
  QPointF(0,0) points to the left top frame point, QPointF(0.5,0.5) points to the frame center.

  Custom focus point is used only in FocusPointCustom focus mode.
  \since 1.1
*/

/*!
  \fn QCameraFocusControl::setCustomFocusPoint(const QPointF &point)

  Sets the custom focus \a point.

  If camera supports fixed set of focus points,
  it should use the nearest supported focus point,
  and return the actual focus point with QCameraFocusControl::focusZones().

  \since 1.1
  \sa QCameraFocusControl::customFocusPoint(), QCameraFocusControl::focusZones()
*/

/*!
  \fn QCameraFocusControl::focusZones() const

  Returns the list of zones, the camera is using for focusing or focused on.
  \since 1.1
*/

/*!
    \fn void QCameraFocusControl::opticalZoomChanged(qreal zoom)

    Signal emitted when the optical \a zoom value changed.
    \since 1.1
*/

/*!
    \fn void QCameraFocusControl::digitalZoomChanged(qreal zoom)

    Signal emitted when the digital \a zoom value changed.
    \since 1.1
*/

/*!
    \fn void QCameraFocusControl::maximumOpticalZoomChanged(qreal zoom)

    Signal emitted when the maximum supported optical \a zoom value changed.
    \since 1.1
*/

/*!
    \fn void QCameraFocusControl::maximumDigitalZoomChanged(qreal zoom)

    Signal emitted when the maximum supported digital \a zoom value changed.

    The maximum supported zoom value can depend on other camera settings,
    like capture mode or resolution.
    \since 1.1
*/


/*!
  \fn QCameraFocusControl::focusZonesChanged()

  Signal is emitted when the set of zones, camera focused on is changed.

  Usually the zones list is changed when the camera is focused.

  \since 1.1
  \sa QCameraFocusControl::focusZones()
*/



#include "moc_qcamerafocuscontrol.cpp"
QT_END_NAMESPACE

