/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include <QDebug>

#include <qcamera.h>
#include <qcamerafocus.h>

#include <qmediaobject_p.h>
#include <qcameracontrol.h>
#include <qcameraexposurecontrol.h>
#include <qcamerafocuscontrol.h>
#include <qmediarecordercontrol.h>
#include <qcameraimagecapturecontrol.h>
#include <qvideodevicecontrol.h>

QT_BEGIN_NAMESPACE

namespace
{
class CameraFocusRegisterMetaTypes
{
public:
    CameraFocusRegisterMetaTypes()
    {
        qRegisterMetaType<QCameraFocus::FocusModes>("QCameraFocus::FocusModes");
        qRegisterMetaType<QCameraFocus::FocusPointMode>("QCameraFocus::FocusPointMode");
    }
} _registerCameraFocusMetaTypes;
}


class QCameraFocusZoneData : public QSharedData
{
public:
    QCameraFocusZoneData():
        status(QCameraFocusZone::Invalid)
    {

    }

    QCameraFocusZoneData(const QRectF &_area, QCameraFocusZone::FocusZoneStatus _status):
        area(_area),
        status(_status)
    {

    }


    QCameraFocusZoneData(const QCameraFocusZoneData &other):
        QSharedData(other),
        area(other.area),
        status(other.status)
    {
    }

    QCameraFocusZoneData& operator=(const QCameraFocusZoneData &other)
    {
        area = other.area;
        status = other.status;
        return *this;
    }

    QRectF area;
    QCameraFocusZone::FocusZoneStatus status;
};

QCameraFocusZone::QCameraFocusZone()
    :d(new QCameraFocusZoneData)
{

}

QCameraFocusZone::QCameraFocusZone(const QRectF &area, QCameraFocusZone::FocusZoneStatus status)
    :d(new QCameraFocusZoneData(area, status))
{
}

QCameraFocusZone::QCameraFocusZone(const QCameraFocusZone &other)
    :d(other.d)
{

}

QCameraFocusZone::~QCameraFocusZone()
{

}

QCameraFocusZone& QCameraFocusZone::operator=(const QCameraFocusZone &other)
{
    d = other.d;
    return *this;
}

bool QCameraFocusZone::operator==(const QCameraFocusZone &other) const
{
    return d == other.d ||
           (d->area == other.d->area && d->status == other.d->status);
}

bool QCameraFocusZone::operator!=(const QCameraFocusZone &other) const
{
    return !(*this == other);
}

bool QCameraFocusZone::isValid() const
{
    return d->status != Invalid && !d->area.isValid();
}

QRectF QCameraFocusZone::area() const
{
    return d->area;
}

QCameraFocusZone::FocusZoneStatus QCameraFocusZone::status() const
{
    return d->status;
}

void QCameraFocusZone::setStatus(QCameraFocusZone::FocusZoneStatus status)
{
    d->status = status;
}


/*!
    \class QCameraFocus


    \brief The QCameraFocus class provides interface for
    focus and zoom related camera settings.

    \inmodule QtMultimedia
    \ingroup camera
    \since 1.1

*/


class QCameraFocusPrivate : public QMediaObjectPrivate
{
    Q_DECLARE_NON_CONST_PUBLIC(QCameraFocus)
public:
    void initControls();

    QCameraFocus *q_ptr;

    QCamera *camera;
    QCameraFocusControl *focusControl;
};


void QCameraFocusPrivate::initControls()
{
    Q_Q(QCameraFocus);

    focusControl = 0;

    QMediaService *service = camera->service();
    if (service)
        focusControl = qobject_cast<QCameraFocusControl *>(service->requestControl(QCameraFocusControl_iid));

    if (focusControl) {
        q->connect(focusControl, SIGNAL(opticalZoomChanged(qreal)), q, SIGNAL(opticalZoomChanged(qreal)));
        q->connect(focusControl, SIGNAL(digitalZoomChanged(qreal)), q, SIGNAL(digitalZoomChanged(qreal)));
        q->connect(focusControl, SIGNAL(maximumOpticalZoomChanged(qreal)),
                   q, SIGNAL(maximumOpticalZoomChanged(qreal)));
        q->connect(focusControl, SIGNAL(maximumDigitalZoomChanged(qreal)),
                   q, SIGNAL(maximumDigitalZoomChanged(qreal)));
        q->connect(focusControl, SIGNAL(focusZonesChanged()), q, SIGNAL(focusZonesChanged()));
    }
}

/*!
    Construct a QCameraFocus for \a camera.
*/

QCameraFocus::QCameraFocus(QCamera *camera):
    QObject(camera), d_ptr(new QCameraFocusPrivate)
{
    Q_D(QCameraFocus);
    d->camera = camera;
    d->q_ptr = this;
    d->initControls();
}


/*!
    Destroys the camera focus object.
*/

QCameraFocus::~QCameraFocus()
{
}

/*!
    Returns true if focus related settings are supported by this camera.
    \since 1.1
*/
bool QCameraFocus::isAvailable() const
{
    return d_func()->focusControl != 0;
}

/*!
  \property QCameraFocus::focusMode
  \brief The current camera focus mode.

  \since 1.1
  \sa QCameraFocus::isFocusModeSupported()
*/

QCameraFocus::FocusMode QCameraFocus::focusMode() const
{
    return d_func()->focusControl ? d_func()->focusControl->focusMode() : QCameraFocus::AutoFocus;
}

void QCameraFocus::setFocusMode(QCameraFocus::FocusMode mode)
{
    if (d_func()->focusControl)
        d_func()->focusControl->setFocusMode(mode);
}

/*!
    Returns true if the focus \a mode is supported by camera.
    \since 1.1
*/

bool QCameraFocus::isFocusModeSupported(QCameraFocus::FocusMode mode) const
{
    return d_func()->focusControl ? d_func()->focusControl->isFocusModeSupported(mode) : false;
}

/*!
  \property QCameraFocus::focusPointMode
  \brief The current camera focus point selection mode.

  \sa QCameraFocus::isFocusPointModeSupported()
  \since 1.1
*/

QCameraFocus::FocusPointMode QCameraFocus::focusPointMode() const
{
    return d_func()->focusControl ?
            d_func()->focusControl->focusPointMode() :
            QCameraFocus::FocusPointAuto;
}

void QCameraFocus::setFocusPointMode(QCameraFocus::FocusPointMode mode)
{
    if (d_func()->focusControl)
        d_func()->focusControl->setFocusPointMode(mode);
    else
        qWarning("Focus points mode selection is not supported");
}

/*!
  Returns true if focus point \a mode is supported.
  \since 1.1
 */
bool QCameraFocus::isFocusPointModeSupported(QCameraFocus::FocusPointMode mode) const
{
    return d_func()->focusControl ?
            d_func()->focusControl->isFocusPointModeSupported(mode) :
            false;

}

/*!
  \property QCameraFocus::customFocusPoint

  Position of custom focus point, in relative frame coordinates:
  QPointF(0,0) points to the left top frame point, QPointF(0.5,0.5) points to the frame center.

  Custom focus point is used only in FocusPointCustom focus mode.
  \since 1.1
 */

QPointF QCameraFocus::customFocusPoint() const
{
    return d_func()->focusControl ?
            d_func()->focusControl->customFocusPoint() :
            QPointF(0.5,0.5);
}

void QCameraFocus::setCustomFocusPoint(const QPointF &point)
{
    if (d_func()->focusControl)
        d_func()->focusControl->setCustomFocusPoint(point);
    else
        qWarning("Focus points selection is not supported");

}

/*!
  \property QCameraFocus::focusZones

  Returns the list of active focus zones.

  If QCamera::FocusPointAuto or QCamera::FocusPointFaceDetection focus mode is selected
  this method returns the list of zones the camera is actually focused on.

  The coordinates system is the same as for custom focus points:
  QPointF(0,0) points to the left top frame point, QPointF(0.5,0.5) points to the frame center.
  \since 1.1
 */
QCameraFocusZoneList QCameraFocus::focusZones() const
{
    return d_func()->focusControl ?
            d_func()->focusControl->focusZones() :
            QCameraFocusZoneList();
}

/*!
    Returns the maximum optical zoom
    \since 1.1
*/

qreal QCameraFocus::maximumOpticalZoom() const
{
    return d_func()->focusControl ? d_func()->focusControl->maximumOpticalZoom() : 1.0;
}

/*!
    Returns the maximum digital zoom
    \since 1.1
*/

qreal QCameraFocus::maximumDigitalZoom() const
{
    return d_func()->focusControl ? d_func()->focusControl->maximumDigitalZoom() : 1.0;
}

/*!
  \property QCameraFocus::opticalZoom
  \brief The current optical zoom value.

  \since 1.1
  \sa QCameraFocus::digitalZoom
*/

qreal QCameraFocus::opticalZoom() const
{
    return d_func()->focusControl ? d_func()->focusControl->opticalZoom() : 1.0;
}

/*!
  \property QCameraFocus::digitalZoom
  \brief The current digital zoom value.

  \since 1.1
  \sa QCameraFocus::opticalZoom
*/
qreal QCameraFocus::digitalZoom() const
{
    return d_func()->focusControl ? d_func()->focusControl->digitalZoom() : 1.0;
}


/*!
    Set the camera \a optical and \a digital zoom values.
    \since 1.1
*/
void QCameraFocus::zoomTo(qreal optical, qreal digital)
{
    if (d_func()->focusControl)
        d_func()->focusControl->zoomTo(optical, digital);
    else
        qWarning("The camera doesn't support zooming.");
}

/*!
    \enum QCameraFocus::FocusMode

    \value ManualFocus          Manual or fixed focus mode.
    \value AutoFocus            One-shot auto focus mode.
    \value ContinuousFocus      Continuous auto focus mode.
    \value InfinityFocus        Focus strictly to infinity.
    \value HyperfocalFocus      Focus to hyperfocal distance, with with the maximum depth of field achieved.
                                All objects at distances from half of this
                                distance out to infinity will be acceptably sharp.
    \value MacroFocus           One shot auto focus to objects close to camera.
*/

/*!
    \enum QCameraFocus::FocusPointMode

    \value FocusPointAuto       Automatically select one or multiple focus points.
    \value FocusPointCenter     Focus to the frame center.
    \value FocusPointFaceDetection Focus on faces in the frame.
    \value FocusPointCustom     Focus to the custom point, defined by QCameraFocus::customFocusPoint property.
*/

/*!
    \fn void QCameraFocus::opticalZoomChanged(qreal value)

    Signal emitted when optical zoom value changes to new \a value.
    \since 1.1
*/

/*!
    \fn void QCameraFocus::digitalZoomChanged(qreal value)

    Signal emitted when digital zoom value changes to new \a value.
    \since 1.1
*/

/*!
    \fn void QCameraFocus::maximumOpticalZoomChanged(qreal zoom)

    Signal emitted when the maximum supported optical \a zoom value changed.
    \since 1.1
*/

/*!
    \fn void QCameraFocus::maximumDigitalZoomChanged(qreal zoom)

    Signal emitted when the maximum supported digital \a zoom value changed.

    The maximum supported zoom value can depend on other camera settings,
    like capture mode or resolution.
    \since 1.1
*/



/*!
  \fn QCameraFocus::focusZonesChanged()

  Signal is emitted when the set of zones, camera focused on is changed.

  Usually the zones list is changed when the camera is focused.
  \since 1.1
*/


#include "moc_qcamerafocus.cpp"
QT_END_NAMESPACE
