// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qlibcamera_camera_p.h"
#include "qlibcamera_instance_p.h"
#include "qlibcamera_videodevices_p.h"

#include <QtMultimedia/private/qplatformlibcameraintegration_p.h>
#include <QtMultimedia/private/qplatformlibcameraplugin_p.h>

QT_BEGIN_NAMESPACE

namespace QLibCamera {

class QLibCameraIntegration : public QPlatformLibCameraIntegration
{
public:
    QLibCameraIntegration() : m_instance(QLibCameraInstance::instance()) { }

    QPlatformVideoDevices *createVideoDevices(QPlatformMediaIntegration *parent) override
    {
        std::unique_ptr<QLibCameraVideoDevices> devices = QLibCameraVideoDevices::create(parent);
        return devices.release();
    }

    q23::expected<QPlatformCamera *, QString> createCamera(QCamera *camera) override
    {
        return new QLibCameraCamera(camera, m_instance);
    }

private:
    std::shared_ptr<QLibCameraInstance> m_instance;
};

class QLibCameraPlugin : public QPlatformLibCameraPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QPlatformLibCameraPlugin_iid FILE "libcamera.json")

public:
    std::unique_ptr<QPlatformLibCameraIntegration> create() override
    {
        return std::make_unique<QLibCameraIntegration>();
    }
};

} // namespace QLibCamera

QT_END_NAMESPACE

#include "qlibcamera_plugin.moc"
