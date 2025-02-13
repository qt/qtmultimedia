// Copyright (C) 2022 The Qt Company Ltd.
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

#include <QtMultimedia/private/qplatformcamera_p.h>
#include <QtCore/private/qcomptr_p.h>

QT_BEGIN_NAMESPACE

class ActiveCamera;

class QWindowsCamera : public QPlatformCamera
{
    Q_OBJECT

public:
    explicit QWindowsCamera(QCamera *parent);
    ~QWindowsCamera() override;

    bool isActive() const override { return bool(m_active); }
    void setActive(bool active) override;
    void setCamera(const QCameraDevice &camera) override;
    bool setCameraFormat(const QCameraFormat &/*format*/) override;

private:
    QCameraDevice m_cameraDevice;
    std::unique_ptr<ActiveCamera> m_active;
};

QT_END_NAMESPACE

#endif //QWINDOWSCAMERA_H
