// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSCAMERA_P_H
#define QOHOSCAMERA_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qplatformcamera_p.h>

#include <memory>

QT_BEGIN_NAMESPACE

class QOhosCameraSession;
class QOhosMediaCaptureSession;

class QOhosCamera : public QPlatformCamera
{
    Q_OBJECT
public:
    explicit QOhosCamera(QCamera *camera);
    ~QOhosCamera() override;

    bool isActive() const override;
    void setActive(bool active) override;
    void setCamera(const QCameraDevice &camera) override;
    bool setCameraFormat(const QCameraFormat &format) override;
    void setCaptureSession(QPlatformMediaCaptureSession *session) override;

    QOhosCameraSession *session() const { return m_session.get(); }

private:
    QOhosMediaCaptureSession *m_captureSession{ nullptr };
    std::unique_ptr<QOhosCameraSession> m_session;
};

QT_END_NAMESPACE

#endif // QOHOSCAMERA_P_H
