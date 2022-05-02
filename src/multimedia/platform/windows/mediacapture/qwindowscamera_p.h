/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

#ifndef QWINDOWSCAMERA_H
#define QWINDOWSCAMERA_H

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

#include <private/qplatformcamera_p.h>

QT_BEGIN_NAMESPACE

class QWindowsMediaCaptureService;
class QWindowsMediaDeviceSession;

class QWindowsCamera : public QPlatformCamera
{
    Q_OBJECT
public:
    explicit QWindowsCamera(QCamera *camera);
    virtual ~QWindowsCamera();

    bool isActive() const override;

    void setCamera(const QCameraDevice &camera) override;

    void setCaptureSession(QPlatformMediaCaptureSession *) override;

    bool setCameraFormat(const QCameraFormat &format) override;

    void setActive(bool active) override;

private Q_SLOTS:
    void onActiveChanged(bool active);

private:
    QWindowsMediaCaptureService *m_captureService = nullptr;
    QWindowsMediaDeviceSession  *m_mediaDeviceSession = nullptr;
    QCameraDevice m_cameraDevice;
    QCameraFormat m_cameraFormat;
    bool m_active = false;
};

QT_END_NAMESPACE

#endif  // QWINDOWSCAMERA_H
