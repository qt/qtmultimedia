// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpipewire_cameradevices_p.h"

#include <QtMultimedia/private/qpipewire_cameracontextmanager_p.h>

QT_BEGIN_NAMESPACE

namespace QtPipeWire {

using namespace std::chrono_literals;

QCameraDevices::QCameraDevices(QPlatformMediaIntegration *integration)
    : QPlatformVideoDevices(integration)
{
    Q_ASSERT(isSupported());

    m_changeCompressionTimer.setSingleShot(true);
    m_changeCompressionTimer.callOnTimeout(this, [this] {
        updateVideoInputsCache();
    });

    connect(&QCameraContextManager::deviceMonitor(), &QCameraDeviceMonitor::camerasChanged,
            &m_changeCompressionTimer, &QChronoTimer::start);
}

bool QCameraDevices::isSupported()
{
    return QCameraContextManager::isSupported();
}

QList<QCameraDevice> QCameraDevices::findVideoInputs() const
{
    return QCameraContextManager::deviceMonitor().cameraDevices();
}

} // namespace QtPipeWire

QT_END_NAMESPACE
