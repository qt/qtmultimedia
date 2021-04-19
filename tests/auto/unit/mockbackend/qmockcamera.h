/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef MOCKCAMERACONTROL_H
#define MOCKCAMERACONTROL_H

#include "private/qplatformcamera_p.h"
#include "qcamerainfo.h"
#include "qmockcamerafocus.h"
#include "qmockcameraimageprocessing.h"
#include "qmockcameraexposure.h"
#include <qtimer.h>

class QMockCamera : public QPlatformCamera
{
    friend class MockCaptureControl;
    Q_OBJECT

    static bool simpleCamera;
public:

    struct Simple {
        Simple() { simpleCamera = true; }
        ~Simple() { simpleCamera = false; }
    };

    QMockCamera(QCamera *parent)
        : QPlatformCamera(parent),
          m_status(QCamera::InactiveStatus),
          m_propertyChangesSupported(false)
    {
        if (!simpleCamera) {
            mockExposure = new QMockCameraExposure(this);
            mockFocus = new QMockCameraFocus(this);
            mockImageProcessing = new QMockCameraImageProcessing(this);
        }
    }

    ~QMockCamera() {}

    bool isActive() const override { return m_active; }
    void setActive(bool active) override {
        if (m_active == active)
            return;
        m_active = active;
        setStatus(active ? QCamera::ActiveStatus : QCamera::InactiveStatus);
        emit activeChanged(active);
    }

    QCamera::Status status() const override { return m_status; }

    /* helper method to emit the signal error */
    void setError(QCamera::Error err, QString errorString)
    {
        emit error(err, errorString);
    }

    /* helper method to emit the signal statusChaged */
    void setStatus(QCamera::Status newStatus)
    {
        m_status = newStatus;
        emit statusChanged(newStatus);
    }

    void setCamera(const QCameraInfo &camera) override
    {
        m_camera = camera;
    }

    QPlatformCameraFocus *focusControl() override{ return mockFocus; }
    QPlatformCameraExposure *exposureControl() override { return mockExposure; }
    QPlatformCameraImageProcessing *imageProcessingControl() override { return mockImageProcessing; }

    bool m_active = false;
    QCamera::Status m_status;
    QCameraInfo m_camera;
    bool m_propertyChangesSupported;

    QMockCameraExposure *mockExposure = nullptr;
    QMockCameraFocus *mockFocus = nullptr;
    QMockCameraImageProcessing *mockImageProcessing = nullptr;
};



#endif // MOCKCAMERACONTROL_H
