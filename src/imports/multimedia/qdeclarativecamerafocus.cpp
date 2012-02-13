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
#include "qdeclarativecamerafocus_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmlclass CameraFocus QDeclarativeCameraFocus
    \brief The CameraFocus element provides interface for focus related camera settings.
    \ingroup multimedia_qml
    \ingroup camera_qml

    This element is part of the \bold{QtMultimedia 5.0} module.

    The CameraFocus element allows control over manual and automatic
    focus settings, including information about any parts of the
    camera frame that are selected for autofocusing.

    It is not constructed separately but is provided by the
    Camera element's \l {Camera::focus}{focus} property.

    \qml
    import QtQuick 2.0
    import QtMultimedia 5.0

    Camera {
        id: camera

        focus {
            focusMode: Camera.FocusMacro
            focusPointMode: Camera.FocusPointCustom
            customFocusPoint: Qt.point(0.2, 0.2) //focus to top-left corner
        }
    }

    \endqml
*/

/*!
    \class QDeclarativeCameraFocus
    \internal
    \brief The CameraFocus element provides interface for focus related camera settings.
*/

/*!
    Construct a declarative camera focus object using \a parent object.
 */

QDeclarativeCameraFocus::QDeclarativeCameraFocus(QCamera *camera, QObject *parent) :
    QObject(parent)
{
    m_focus = camera->focus();
    m_focusZones = new FocusZonesModel(this);

    updateFocusZones();

    connect(m_focus, SIGNAL(focusZonesChanged()), SLOT(updateFocusZones()));
}

QDeclarativeCameraFocus::~QDeclarativeCameraFocus()
{
}


/*!
    \qmlproperty Camera::FocusMode CameraFocus::focusMode
    \property QDeclarativeCameraFocus::focusMode

    The current camera focus mode.

    It's possible to combine multiple Camera::FocusMode values,
    for example Camera.FocusMacro + Camera.FocusContinuous.

    In automatic focusing modes, the \l focusPointMode property
    and \l focusZones property provide information and control
    over how automatic focusing is performed.
*/
QDeclarativeCamera::FocusMode QDeclarativeCameraFocus::focusMode() const
{
    return QDeclarativeCamera::FocusMode(int(m_focus->focusMode()));
}

/*!
    \qmlmethod bool CameraFocus::isFocusModeSupported(mode)
    \fn QDeclarativeCameraFocus::isFocusPointModeSupported(QDeclarativeCamera::FocusMode mode)

    Returns true if the supplied \a mode is a supported focus mode, and
    false otherwise.
*/
bool QDeclarativeCameraFocus::isFocusModeSupported(QDeclarativeCamera::FocusMode mode) const
{
    return m_focus->isFocusModeSupported(QCameraFocus::FocusModes(int(mode)));
}

void QDeclarativeCameraFocus::setFocusMode(QDeclarativeCamera::FocusMode mode)
{
    m_focus->setFocusMode(QCameraFocus::FocusModes(int(mode)));
}

/*!
    \qmlproperty CameraFocus::FocusPointMode CameraFocus::focusPointMode
    \property QDeclarativeCameraFocus::focusPointMode

    The current camera focus point mode.  This is used in automatic
    focusing modes to determine what to focus on.

    If the current focus point mode is \c Camera.FocusPointCustom, the
    \l customFocusPoint property allows you to specify which part of
    the frame to focus on.
*/
QDeclarativeCamera::FocusPointMode QDeclarativeCameraFocus::focusPointMode() const
{
    return QDeclarativeCamera::FocusPointMode(m_focus->focusPointMode());
}

void QDeclarativeCameraFocus::setFocusPointMode(QDeclarativeCamera::FocusPointMode mode)
{
    if (mode != focusPointMode()) {
        m_focus->setFocusPointMode(QCameraFocus::FocusPointMode(mode));
        emit focusPointModeChanged(focusPointMode());
    }
}

/*!
    \qmlmethod bool CameraFocus::isFocusPointModeSupported(mode)
    \fn QDeclarativeCameraFocus::isFocusPointModeSupported(QDeclarativeCamera::FocusPointMode mode)

    Returns true if the supplied \a mode is a supported focus point mode, and
    false otherwise.
*/
bool QDeclarativeCameraFocus::isFocusPointModeSupported(QDeclarativeCamera::FocusPointMode mode) const
{
    return m_focus->isFocusPointModeSupported(QCameraFocus::FocusPointMode(mode));
}

/*!
  \qmlproperty QPointF CameraFocus::customFocusPoint
  \property QDeclarativeCameraFocus::customFocusPoint

  Position of custom focus point, in relative frame coordinates:
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

/*!
  \qmlproperty QPointF CameraFocus::focusZones
  \property QDeclarativeCameraFocus::focusZones

  List of current camera focus zones,
  each including \c area specified in the same coordinates as \l customFocusPoint
  and zone \c status as one of the following values:

    \table
    \header \o Value \o Description
    \row \o Camera.FocusAreaUnused  \o This focus point area is currently unused in autofocusing.
    \row \o Camera.FocusAreaSelected    \o This focus point area is used in autofocusing, but is not in focus.
    \row \o Camera.FocusAreaFocused  \o This focus point is used in autofocusing, and is in focus.
    \endtable


  \qml

  VideoOutput {
      id: viewfinder
      source: camera

      //display focus areas on camera viewfinder:
      Repeater {
            model: camera.focus.focusZones

            Rectangle {
                border {
                    width: 2
                    color: status == Camera.FocusAreaFocused ? "green" : "white"
                }
                color: "transparent"

                // Map from the relative, normalized frame coordinates
                property mappedRect: viewfinder.mapNormalizedRectToItem(area);

                x: mappedRect.x
                y: mappedRect.y
                width: mappedRect.width
                height: mappedRect.height
            }
      }
  }
  \endqml
*/

QAbstractListModel *QDeclarativeCameraFocus::focusZones() const
{
    return m_focusZones;
}

/*! \internal */
void QDeclarativeCameraFocus::updateFocusZones()
{
    m_focusZones->setFocusZones(m_focus->focusZones());
}


FocusZonesModel::FocusZonesModel(QObject *parent)
    :QAbstractListModel(parent)
{
    QHash<int, QByteArray> roles;
    roles[StatusRole] = "status";
    roles[AreaRole] = "area";
    setRoleNames(roles);
}

int FocusZonesModel::rowCount(const QModelIndex &parent) const
{
    if (parent == QModelIndex())
        return m_focusZones.count();

    return 0;
}

QVariant FocusZonesModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() > m_focusZones.count())
        return QVariant();

    QCameraFocusZone zone = m_focusZones.value(index.row());

    if (role == StatusRole)
        return zone.status();

    if (role == AreaRole)
        return zone.area();

    return QVariant();
}

void FocusZonesModel::setFocusZones(const QCameraFocusZoneList &zones)
{
    beginResetModel();
    m_focusZones = zones;
    endResetModel();
}

QT_END_NAMESPACE

#include "moc_qdeclarativecamerafocus_p.cpp"
