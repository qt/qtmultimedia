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

#ifndef QCAMERAFOCUS_H
#define QCAMERAFOCUS_H

#include <QtCore/qstringlist.h>
#include <QtCore/qpair.h>
#include <QtCore/qsize.h>
#include <QtCore/qpoint.h>
#include <QtCore/qrect.h>
#include <QtCore/qshareddata.h>
#include <QtCore/qobject.h>

#include <QtMultimedia/qtmultimediaglobal.h>
#include <QtMultimedia/qmediaenumdebug.h>

QT_BEGIN_NAMESPACE


class QCamera;
class QPlatformCamera;

class QCameraFocusPrivate;
class Q_MULTIMEDIA_EXPORT QCameraFocus : public QObject
{
    Q_OBJECT

    Q_PROPERTY(FocusMode focusMode READ focusMode WRITE setFocusMode)
    Q_PROPERTY(QPointF customFocusPoint READ customFocusPoint WRITE setCustomFocusPoint NOTIFY customFocusPointChanged)
    Q_PROPERTY(float focusDistance READ focusDistance WRITE setFocusDistance NOTIFY focusDistanceChanged)

    Q_PROPERTY(float minimumZoomFactor READ minimumZoomFactor)
    Q_PROPERTY(float maximumZoomFactor READ maximumZoomFactor)
    Q_PROPERTY(float zoomFactor READ zoomFactor WRITE setZoomFactor NOTIFY zoomFactorChanged)

    Q_ENUMS(FocusMode)
public:
    enum FocusMode {
        FocusModeAuto,
        FocusModeAutoNear,
        FocusModeAutoFar,
        FocusModeHyperfocal,
        FocusModeInfinity,
        FocusModeManual
#if 1 // QT_DEPRECATED
        , ContinuousFocus = FocusModeAuto,
        AutoFocus = FocusModeAuto, // Not quite
        MacroFocus = FocusModeAutoNear,
        HyperfocalFocus = FocusModeHyperfocal,
        InfinityFocus = FocusModeInfinity,
        ManualFocus = FocusModeManual
#endif
    };

    bool isAvailable() const;

    FocusMode focusMode() const;
    void setFocusMode(FocusMode mode);
    bool isFocusModeSupported(FocusMode mode) const;

    QPointF focusPoint() const;

    QPointF customFocusPoint() const;
    void setCustomFocusPoint(const QPointF &point);
    bool isCustomFocusPointSupported() const;

    void setFocusDistance(float d);
    float focusDistance() const;

    float minimumZoomFactor() const;
    float maximumZoomFactor() const;
    float zoomFactor() const;
    void setZoomFactor(float factor);

    void zoomTo(float zoom, float rate);

Q_SIGNALS:
    void focusModeChanged();
    void zoomFactorChanged(float);
    void focusDistanceChanged(float);
    void customFocusPointChanged();

protected:
    ~QCameraFocus();

private:
    friend class QCamera;
    friend class QCameraPrivate;
    QCameraFocus(QCamera *camera, QPlatformCamera *cameraControl);

    Q_DISABLE_COPY(QCameraFocus)
    Q_DECLARE_PRIVATE(QCameraFocus)
};

QT_END_NAMESPACE

Q_MEDIA_ENUM_DEBUG(QCameraFocus, FocusMode)

#endif  // QCAMERAFOCUS_H
