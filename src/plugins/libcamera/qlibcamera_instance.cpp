// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qlibcamera_instance_p.h"
#include "qlibcameralogging_p.h"

#include <QtCore/qglobalstatic.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qmutex.h>
#include <QtCore/private/qsystemerror_p.h>

#include <libcamera/base/signal.h>
#include <libcamera/camera.h>
#include <libcamera/camera_manager.h>

#include <mutex>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcLibCamera, "qt.multimedia.libcamera")

namespace QLibCamera {

using namespace libcamera;

namespace {

struct InstanceHolder
{
    QMutex mutex;
    std::weak_ptr<QLibCameraInstance> instance;
};

Q_GLOBAL_STATIC(InstanceHolder, s_libCameraInstance)

} // namespace

QLibCameraInstance::QLibCameraInstance() : m_manager(std::make_unique<CameraManager>())
{
    const int startResult = m_manager->start();
    m_started = startResult == 0;
    if (!m_started) {
        qCWarning(lcLibCamera) << "libcamera::CameraManager::start() failed:"
                               << QSystemError::stdString(-startResult);
        return;
    }

    m_manager->cameraAdded.connect(this, [this](std::shared_ptr<Camera> camera) {
        // called on libcamera thread
        const QString id = QString::fromStdString(camera->id());
        QMetaObject::invokeMethod(this, [this, id] {
            Q_EMIT cameraAdded(id);
        }, Qt::QueuedConnection);
    });
    m_manager->cameraRemoved.connect(this, [this](std::shared_ptr<Camera> camera) {
        // called on libcamera thread
        const QString id = QString::fromStdString(camera->id());
        QMetaObject::invokeMethod(this, [this, id] {
            Q_EMIT cameraRemoved(id);
        }, Qt::QueuedConnection);
    });

    qCDebug(lcLibCamera) << "libcamera" << CameraManager::version().c_str() << "started,"
                         << int(m_manager->cameras().size()) << "camera(s) found";
}

QLibCameraInstance::~QLibCameraInstance()
{
    if (m_started)
        m_manager->stop();
}

std::shared_ptr<QLibCameraInstance> QLibCameraInstance::instance()
{
    std::lock_guard guard{ s_libCameraInstance->mutex };
    std::shared_ptr<QLibCameraInstance> ret = s_libCameraInstance->instance.lock();
    if (!ret) {
        ret = std::shared_ptr<QLibCameraInstance>(new QLibCameraInstance);
        if (!ret->m_started)
            return nullptr; // CameraManager::start() failed

        s_libCameraInstance->instance = ret;
    }
    return ret;
}

std::shared_ptr<Camera> QLibCameraInstance::get(const QString &id) const
{
    return m_manager->get(id.toStdString());
}

} // namespace QLibCamera

QT_END_NAMESPACE

#include "moc_qlibcamera_instance_p.cpp"
