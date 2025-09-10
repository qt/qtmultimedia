// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qplatformvideodevices_p.h"

QT_BEGIN_NAMESPACE

QPlatformVideoDevices::QPlatformVideoDevices(QPlatformMediaIntegration *integration)
    : m_integration(integration)
{
    qRegisterMetaType<PrivateTag>(); // for queued connections
}

QPlatformVideoDevices::~QPlatformVideoDevices() = default;

void QPlatformVideoDevices::onVideoInputsChanged() {
    m_videoInputs.reset();
    emit videoInputsChanged(PrivateTag{});
}

void QPlatformVideoDevices::updateVideoInputsCache()
{
    if (m_videoInputs.update(findVideoInputs()))
        emit videoInputsChanged(PrivateTag{});
}

QList<QCameraDevice> QPlatformVideoDevices::videoInputs() const {
    return m_videoInputs.ensure([this]() {
        return findVideoInputs();
    });
}

QT_END_NAMESPACE

#include "moc_qplatformvideodevices_p.cpp"
