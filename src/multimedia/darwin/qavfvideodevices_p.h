// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QAVFVIDEODEVICES_H
#define QAVFVIDEODEVICES_H

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

#include <QtMultimedia/qtmultimediaexports.h>
#include <QtMultimedia/private/qplatformvideodevices_p.h>
#include <QtCore/private/qcore_mac_p.h>

#include <functional>

QT_BEGIN_NAMESPACE

class QPlatformMediaIntegration;
class Q_MULTIMEDIA_EXPORT QAVFVideoDevices : public QPlatformVideoDevices
{
public:
    // Takes a delegate to check whether a given CvPixelFormat is supported for a capture session.
    // If given a nullptr, it assumes all formats are supported.
    QAVFVideoDevices(
        QPlatformMediaIntegration *integration,
        std::function<bool(uint32_t)> &&isCvPixelFormatSupportedDelegate = nullptr);
    ~QAVFVideoDevices();

    // Returns true if the given CvPixelFormat is supported for camera capture session.
    [[nodiscard]] bool isCvPixelFormatSupported(uint32_t cvPixelFormat) const;

protected:
    QList<QCameraDevice> findVideoInputs() const override;

private:
    void updateCameraDevices();

    QMacNotificationObserver m_deviceConnectedObserver;
    QMacNotificationObserver m_deviceDisconnectedObserver;
    std::function<bool(uint32_t)> m_isCvPixelFormatSupportedDelegate;

    QList<QCameraDevice> m_cameraDevices;
};

QT_END_NAMESPACE

#endif
