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

#ifndef QCAMERACONTROL_H
#define QCAMERACONTROL_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qobject.h>
#include <QtMultimedia/qtmultimediaglobal.h>

#include <QtMultimedia/qcamera.h>

QT_BEGIN_NAMESPACE

class QPlatformCameraExposure;
class QPlatformCameraImageProcessing;

class Q_MULTIMEDIA_EXPORT QPlatformCamera : public QObject
{
    Q_OBJECT

public:
    virtual bool isActive() const = 0;
    virtual void setActive(bool active) = 0;

    virtual QCamera::Status status() const { return m_status; }

    virtual void setCamera(const QCameraInfo &camera) = 0;
    virtual bool setCameraFormat(const QCameraFormat &/*format*/) { return false; }

    virtual void setCaptureSession(QPlatformMediaCaptureSession *) {}

    virtual QPlatformCameraExposure *exposureControl() { return nullptr; }
    virtual QPlatformCameraImageProcessing *imageProcessingControl() { return nullptr; }

    virtual bool isFocusModeSupported(QCamera::FocusMode mode) const { return mode == QCamera::FocusModeAuto; }
    virtual void setFocusMode(QCamera::FocusMode /*mode*/) {}

    virtual bool isCustomFocusPointSupported() const { return false; }
    virtual void setCustomFocusPoint(const QPointF &/*point*/) {}

    virtual void setFocusDistance(float) {}

    // smaller 0: zoom instantly, rate in power-of-two/sec
    virtual void zoomTo(float /*newZoomFactor*/, float /*rate*/ = -1.) {}

    QCamera::FocusMode focusMode() const { return m_focusMode; }
    QPointF focusPoint() const { return m_customFocusPoint; }

    float minZoomFactor() const { return m_minZoom; }
    float maxZoomFactor() const { return m_maxZoom; }
    float zoomFactor() const { return m_zoomFactor; }
    QPointF customFocusPoint() const { return m_customFocusPoint; }
    float focusDistance() const { return m_focusDistance; }

    void statusChanged(QCamera::Status);
    void minimumZoomFactorChanged(float factor);
    void maximumZoomFactorChanged(float);
    void focusModeChanged(QCamera::FocusMode mode);
    void customFocusPointChanged(const QPointF &point);
    void focusDistanceChanged(float d);
    void zoomFactorChanged(float zoom);

Q_SIGNALS:
    void activeChanged(bool);
    void error(int error, const QString &errorString);

protected:
    explicit QPlatformCamera(QCamera *parent);

    static QCameraFormat findBestCameraFormat(const QCameraInfo &camera);
private:
    QCamera *m_camera = nullptr;
    QCamera::Status m_status = QCamera::InactiveStatus;
    QCamera::FocusMode m_focusMode = QCamera::FocusModeAuto;
    float m_minZoom = 1.;
    float m_maxZoom = 1.;
    float m_zoomFactor = 1.;
    float m_focusDistance = 1.;
    QPointF m_customFocusPoint{.5, .5};
};

QT_END_NAMESPACE


#endif  // QCAMERACONTROL_H

