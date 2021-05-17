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


#ifndef QGSTREAMERCAMERACONTROL_H
#define QGSTREAMERCAMERACONTROL_H

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

#include <QHash>
#include <private/qplatformcamera_p.h>
#include "qgstreamermediacapture_p.h"
#include <private/qgst_p.h>
#include <gst/video/colorbalance.h>

QT_BEGIN_NAMESPACE
class QGstreamerCameraExposure;
class QGstreamerImageProcessing;

class QGstreamerCamera : public QPlatformCamera
{
    Q_OBJECT
public:
    QGstreamerCamera(QCamera *camera);
    virtual ~QGstreamerCamera();

    bool isActive() const override;
    void setActive(bool active) override;

    void setCamera(const QCameraInfo &camera) override;
    bool setCameraFormat(const QCameraFormat &format) override;
    void setCameraFormatInternal(const QCameraFormat &format);

    void setCaptureSession(QPlatformMediaCaptureSession *session) override;

    QGstElement gstElement() const { return gstCameraBin.element(); }
    void setPipeline(const QGstPipeline &pipeline) { gstPipeline = pipeline; }

    QPlatformCameraImageProcessing *imageProcessingControl() override;
    QPlatformCameraExposure *exposureControl() override;

    void setFocusMode(QCamera::FocusMode mode) override;
    bool isFocusModeSupported(QCamera::FocusMode mode) const override;

#if QT_CONFIG(gstreamer_photography)
    GstPhotography *photography() const;
#endif
    QString v4l2Device() const { return m_v4l2Device; }
    bool isV4L2Camera() const { return !m_v4l2Device.isEmpty(); }

    GstColorBalance *colorBalance() const;

private:
    QGstreamerMediaCapture *m_session = nullptr;

    QGstreamerCameraExposure *exposure = nullptr;
    QGstreamerImageProcessing *imageProcessing = nullptr;

    QCameraInfo m_cameraInfo;

    QGstPipeline gstPipeline;

    QGstBin gstCameraBin;
    QGstElement gstCamera;
    QGstElement gstDecode;
    QGstElement gstVideoConvert;
    QGstElement gstVideoScale;

    bool m_active = false;
    QString m_v4l2Device;
};

QT_END_NAMESPACE

#endif // QGSTREAMERCAMERACONTROL_H
