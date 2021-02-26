/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qdeclarativecamera_p.h"
#include "qdeclarativecamerafocus_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype CameraFocus
    \instantiates QDeclarativeCameraFocus
    \inqmlmodule QtMultimedia
    \brief An interface for focus related camera settings.
    \ingroup multimedia_qml
    \ingroup camera_qml

    This type allows control over manual and automatic
    focus settings, including information about any parts of the
    camera frame that are selected for autofocusing.

    It should not be constructed separately, instead the
    \c focus property of a \l Camera should be used.

    \qml

    Item {
        width: 640
        height: 360

        Camera {
            id: camera

            focus {
                focusMode: Camera.FocusMacro
                customFocusPoint: Qt.point(0.2, 0.2) // Focus relative to top-left corner
            }
        }

        VideoOutput {
            source: camera
            anchors.fill: parent
        }
    }

    \endqml
*/

/*!
    \class QDeclarativeCameraFocus
    \internal
    \brief An interface for focus related camera settings.
*/

/*!
    Construct a declarative camera focus object using \a parent object.
 */

QDeclarativeCameraFocus::QDeclarativeCameraFocus(QCamera *camera, QObject *parent) :
    QObject(parent)
{
    m_focus = camera->focus();

    connect(camera, &QCamera::statusChanged, [this](QCamera::Status status) {
        if (status != QCamera::UnloadedStatus && status != QCamera::LoadedStatus
            && status != QCamera::ActiveStatus) {
            return;
        }

        emit supportedFocusModeChanged();
    });
}

QDeclarativeCameraFocus::~QDeclarativeCameraFocus() = default;
/*!
    \property QDeclarativeCameraFocus::focusMode

    This property holds the current camera focus mode.

    In automatic focusing modes, the \l focusPoint
    property provides a hint on where the camera should focus.
*/

/*!
    \qmlproperty enumeration CameraFocus::focusMode

    This property holds the current camera focus mode, which can be one of the following values:

    \table
     \header
      \li Value
      \li Description
     \row
      \li FocusModeAuto
      \li Continuous auto focus mode.
     \row
      \li FocusModeAutoNear
      \li Continuous auto focus, preferring objects near to the camera.
     \row
      \li FocusModeAutoFar
      \li Continuous auto focus, preferring objects far away from the camera.
     \row
      \li FocusModeHyperfocal
      \li Focus to hyperfocal distance, with the maximum depth of field achieved. All objects at distances
          from half of this distance out to infinity will be acceptably sharp.
     \row
      \li FocusModeInfinity
      \li Focus strictly to infinity.
     \row
      \li FocusModeManual
      \li Manual or fixed focus mode.
    \endtable

    In automatic focusing modes, the \l focusPoint property
    provides information and control
    over the area of the image that is being focused.
*/
QDeclarativeCameraFocus::FocusMode QDeclarativeCameraFocus::focusMode() const
{
    return QDeclarativeCameraFocus::FocusMode(int(m_focus->focusMode()));
}

/*!
    \qmlproperty list<FocusMode> CameraFocus::supportedFocusMode

    This property holds the supported focus modes of the camera.

    \since 5.11
    \sa focusMode
*/
QVariantList QDeclarativeCameraFocus::supportedFocusMode() const
{
    QVariantList supportedModes;

    for (int i = int(FocusManual); i <= int(FocusMacro); ++i) {
        if (m_focus->isFocusModeSupported((QCameraFocus::FocusMode) i))
            supportedModes.append(i);
    }

    return supportedModes;
}

void QDeclarativeCameraFocus::setFocusMode(QDeclarativeCameraFocus::FocusMode mode)
{
    if (mode != focusMode()) {
        m_focus->setFocusMode(QCameraFocus::FocusMode(int(mode)));
        emit focusModeChanged(focusMode());
    }
}

/*!
  \property QDeclarativeCameraFocus::customFocusPoint

  This property holds the position of the custom focus point in relative
  frame coordinates. For example, QPointF(0,0) pointing to the left top corner of the frame, and QPointF(0.5,0.5)
  pointing to the center of the frame.

  Custom focus point is used only in QCameraFocus::FocusPointCustom focus mode.
*/

/*!
  \qmlproperty point QtMultimedia::CameraFocus::customFocusPoint

  This property holds the position of custom focus point, in relative frame coordinates:
  QPointF(0,0) points to the left top frame point, QPointF(0.5,0.5)
  points to the frame center.

  Custom focus point is used only in FocusPointCustom focus mode.
*/

QPointF QDeclarativeCameraFocus::customFocusPoint() const
{
    return m_focus->customFocusPoint();
}

void QDeclarativeCameraFocus::setCustomFocusPoint(const QPointF &point)
{
    if (point != customFocusPoint()) {
        m_focus->setCustomFocusPoint(point);
        emit customFocusPointChanged(customFocusPoint());
    }
}

QT_END_NAMESPACE

#include "moc_qdeclarativecamerafocus_p.cpp"
