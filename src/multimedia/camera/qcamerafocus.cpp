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

#include "qcamerafocus.h"
#include "private/qobject_p.h"

#include <qcamera.h>
#include <private/qplatformcamera_p.h>
#include <private/qplatformcameraexposure_p.h>
#include <private/qplatformcamerafocus_p.h>
#include <private/qplatformcameraimagecapture_p.h>

#include <QtCore/QDebug>

QT_BEGIN_NAMESPACE

/*!
    \class QCameraFocus

    \brief The QCameraFocus class provides an interface for focus and zoom related camera settings.

    \inmodule QtMultimedia
    \ingroup multimedia
    \ingroup multimedia_camera

    On hardware that supports it, this class lets you adjust the focus
    or zoom (both optical and digital).  This also includes things
    like "Macro" mode for close up work (e.g. reading barcodes, or
    recognising letters), or "touch to focus" - indicating an
    interesting area of the viewfinder for the hardware to attempt
    to focus on.

    \snippet multimedia-snippets/camera.cpp Camera custom zoom

    Zooming can be accomplished in a number of ways - usually the more
    expensive but higher quality approach is an optical zoom, which allows
    using the full extent of the camera sensor to gather image pixels. In
    addition it is possible to digitally zoom, which will generally just
    enlarge part of the sensor frame and throw away other parts.  If the
    camera hardware supports optical zoom this should generally always
    be used first.  The \l maximumOpticalZoom() method allows this to be
    checked.  The \l zoomTo() method allows changing both optical and
    digital zoom at once.

    \snippet multimedia-snippets/camera.cpp Camera combined zoom

    \section2 Some notes on autofocus
    Some hardware supports a movable focus lens assembly, and typically
    this hardware also supports automatically focusing via some heuristic.
    You can influence this via the \l focusPoint property (usually provided
    by a user in a "touch to focus" scenario).
*/

#define Q_DECLARE_NON_CONST_PUBLIC(Class) \
    inline Class* q_func() { return static_cast<Class *>(q_ptr); } \
    friend class Class;

class QCameraFocusPrivate : public QObjectPrivate
{
    Q_DECLARE_NON_CONST_PUBLIC(QCameraFocus)
public:
    void init(QPlatformCamera *cameraControl);

    QCamera *camera;

    QPlatformCameraFocus *focusControl;
    float zoomFactor = 1.;
    QPointF customFocusPoint{-1, -1};
};

#undef Q_DECLARE_NON_CONST_PUBLIC

void QCameraFocusPrivate::init(QPlatformCamera *cameraControl)
{
    Q_Q(QCameraFocus);

    focusControl = cameraControl->focusControl();

    if (!focusControl)
        return;

    q->connect(focusControl, SIGNAL(zoomFactorChanged(qreal)),
               q, SIGNAL(zoomFactorChanged(qreal)));
    q->connect(focusControl, SIGNAL(minimumZoomFactorChanged(float)),
               q, SIGNAL(minimumZoomFactorChanged(float)));
    q->connect(focusControl, SIGNAL(maximumZoomFactorChanged(float)),
               q, SIGNAL(maximumZoomFactorChanged(float)));
}

/*!
    \internal
    Construct a QCameraFocus for \a camera.
*/

QCameraFocus::QCameraFocus(QCamera *camera, QPlatformCamera *cameraControl)
    : QObject(*new QCameraFocusPrivate, camera)
{
    Q_D(QCameraFocus);
    d->camera = camera;
    d->init(cameraControl);
}


/*!
    Destroys the camera focus object.
*/

QCameraFocus::~QCameraFocus() = default;

/*!
    Returns true if focus related settings are supported by this camera.

    You may need to also check if any specific features are supported.
*/
bool QCameraFocus::isAvailable() const
{
    return d_func()->focusControl != nullptr;
}

/*!
    \property QCameraFocus::focusMode
    \brief the current camera focus mode.

    Sets up different focus modes for the camera. All auto focus modes will focus continuously.
    Locking the focus is possible by setting the focus mode to \l FocusModeManual. This will keep
    the current focus and stop any automatic focusing.

    \sa QCameraFocus::isFocusModeSupported()
*/

QCameraFocus::FocusMode QCameraFocus::focusMode() const
{
    Q_D(const QCameraFocus);
    return d->focusControl ? d->focusControl->focusMode() : QCameraFocus::FocusModeAuto;
}

void QCameraFocus::setFocusMode(QCameraFocus::FocusMode mode)
{
    Q_D(QCameraFocus);
    if (!d->focusControl || d->focusControl->focusMode() == mode)
        return;
    d->focusControl->setFocusMode(mode);
    emit focusModeChanged();
}

/*!
    Returns true if the focus \a mode is supported by camera.
*/

bool QCameraFocus::isFocusModeSupported(FocusMode mode) const
{
    Q_D(const QCameraFocus);
    return d->focusControl ? d->focusControl->isFocusModeSupported(mode) : false;
}

/*!
    Returns the point currently used by the auto focus system to focus onto.
 */
QPointF QCameraFocus::focusPoint() const
{
    Q_D(const QCameraFocus);
    return d->focusControl ? d->focusControl->focusPoint() : QPointF(-1., -1.);

}

/*!
  \property QCameraFocus::customFocusPoint

  This property represents the position of the custom focus point, in relative frame coordinates:
  QPointF(0,0) points to the left top frame point, QPointF(0.5,0.5) points to the frame center.

  The custom focus point property is used only in \c FocusPointCustom focus mode.
 */

QPointF QCameraFocus::customFocusPoint() const
{
    Q_D(const QCameraFocus);
    return d->customFocusPoint;
}

void QCameraFocus::setCustomFocusPoint(const QPointF &point)
{
    Q_D(QCameraFocus);
    if (d->customFocusPoint == point)
        return;
    d->customFocusPoint = point;
    if (d->focusControl)
        d->focusControl->setCustomFocusPoint(point);
    Q_EMIT customFocusPointChanged();
}

bool QCameraFocus::isCustomFocusPointSupported() const
{
    Q_D(const QCameraFocus);
    return d->focusControl ? d->focusControl->isCustomFocusPointSupported() : false;
}

/*!
    \property QCameraFocus::focusDistance

    This property return an approximate focus distance of the camera. The value reported is between 0 and 1, 0 being the closest
    possible focus distance, 1 being as far away as possible. Note that 1 is often, but not always infinity.

    Setting the focus distance will be ignored unless the focus mode is set to \l FocusModeManual.
 */
void QCameraFocus::setFocusDistance(float d)
{
    if (focusMode() != FocusModeManual)
        return;
    if (d_func()->focusControl)
        d_func()->focusControl->setFocusDistance(d);
}

float QCameraFocus::focusDistance() const
{
    if (d_func()->focusControl)
        return d_func()->focusControl->focusDistance();
    return 0.;
}

/*!
    Returns the maximum zoom factor.

    This will be \c 1.0 on cameras that do not support zooming.
*/

float QCameraFocus::maximumZoomFactor() const
{
    Q_D(const QCameraFocus);
    return d->focusControl ? d->focusControl->zoomFactorRange().max : 1.;
}

/*!
    Returns the minimum zoom factor.

    This will be \c 1.0 on cameras that do not support zooming.
*/

float QCameraFocus::minimumZoomFactor() const
{
    Q_D(const QCameraFocus);
    return d->focusControl ? d->focusControl->zoomFactorRange().min : 1.;
}

/*!
  \property QCameraFocus::zoomFactor
  \brief The current zoom factor.
*/
float QCameraFocus::zoomFactor() const
{
    return d_func()->zoomFactor;
}

void QCameraFocus::setZoomFactor(float factor)
{
    Q_D(QCameraFocus);
    factor = qBound(minimumZoomFactor(), factor, maximumZoomFactor());
    d->zoomFactor = factor;
    d->focusControl->zoomTo(factor, -1);
}

/*!
    Zooms to a zoom factor \a factor using \a rate.

    The rate is specified in powers of two per second. A rate of 1
    would take two seconds to zoom from a zoom factor of 1 to a zoom factor of 4.
 */
void QCameraFocus::zoomTo(float factor, float rate)
{
    Q_ASSERT(rate > 0);

    Q_D(QCameraFocus);
    factor = qBound(minimumZoomFactor(), factor, maximumZoomFactor());
    d->zoomFactor = factor;
    d->focusControl->zoomTo(factor, rate);
}

/*!
    \enum QCameraFocus::FocusMode

    \value FocusModeAuto        Continuous auto focus mode.
    \value FocusModeAutoNear    Continuous auto focus mode on near objects.
    \value FocusModeAutoFar     Continuous auto focus mode on objects far away.
    \value FocusModeHyperfocal  Focus to hyperfocal distance, with the maximum depth of field achieved.
                                All objects at distances from half of this
                                distance out to infinity will be acceptably sharp.
    \value FocusModeInfinity    Focus strictly to infinity.
    \value FocusModeManual      Manual or fixed focus mode.
*/

/*!
    \fn void QCameraFocus::opticalZoomChanged(qreal value)

    Signal emitted when optical zoom value changes to new \a value.
*/

/*!
    \fn void QCameraFocus::digitalZoomChanged(qreal value)

    Signal emitted when digital zoom value changes to new \a value.
*/

/*!
    \fn void QCameraFocus::maximumOpticalZoomChanged(qreal zoom)

    Signal emitted when the maximum supported optical \a zoom value changed.
*/

/*!
    \fn void QCameraFocus::maximumDigitalZoomChanged(qreal zoom)

    Signal emitted when the maximum supported digital \a zoom value changed.

    The maximum supported zoom value can depend on other camera settings,
    like capture mode or resolution.
*/

QT_END_NAMESPACE

#include "moc_qcamerafocus.cpp"
