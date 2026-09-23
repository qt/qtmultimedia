// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QLIBCAMERA_VIDEODEVICES_P_H
#define QLIBCAMERA_VIDEODEVICES_P_H

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

#include <QtMultimedia/private/qplatformvideodevices_p.h>

#include <memory>

QT_BEGIN_NAMESPACE

class QPlatformMediaIntegration;

namespace QLibCamera {

class QLibCameraInstance;

class QLibCameraVideoDevices : public QPlatformVideoDevices
{
    Q_OBJECT
public:
    // Returns null only when libcamera::CameraManager::start() failed -- not when the camera
    // list is currently empty. QPlatformMediaIntegration::videoDevices() memoizes its result
    // under a std::once_flag, so bailing out on an empty list would make libcamera permanently
    // unreachable for a camera plugged in after this call.
    static std::unique_ptr<QLibCameraVideoDevices> create(QPlatformMediaIntegration *);

    ~QLibCameraVideoDevices() override;

protected:
    QList<QCameraDevice> findVideoInputs() const override;

private:
    explicit QLibCameraVideoDevices(QPlatformMediaIntegration *,
                                    std::shared_ptr<QLibCameraInstance>);

    // Set once in the constructor and never mutated -- safe to read from any thread, satisfying
    // findVideoInputs()'s "callable from any thread" contract without needing a mutex of our
    // own. libcamera::CameraManager::cameras()/get() are themselves documented thread-safe.
    const std::shared_ptr<QLibCameraInstance> m_instance;
};

} // namespace QLibCamera

QT_END_NAMESPACE

#endif // QLIBCAMERA_VIDEODEVICES_P_H
