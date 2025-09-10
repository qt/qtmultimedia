// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
