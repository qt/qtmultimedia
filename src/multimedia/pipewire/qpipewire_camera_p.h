// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPIPEWIRE_CAMERA_P_H
#define QPIPEWIRE_CAMERA_P_H

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

#include <QtMultimedia/private/qplatformcamera_p.h>
#include <QtMultimedia/private/qtmultimediaglobal_p.h>
#include <QtMultimedia/qcameradevice.h>

#include <memory>

QT_BEGIN_NAMESPACE

namespace QtPipeWire {

class QPipeWireVideoStream;

class Q_MULTIMEDIA_EXPORT QPipeWireCamera : public QPlatformCamera
{
    Q_OBJECT

public:
    explicit QPipeWireCamera(QCamera *camera);
    ~QPipeWireCamera() override;

    bool isActive() const override;
    void setActive(bool active) override;

    void setCamera(const QCameraDevice &camera) override;
    bool setCameraFormat(const QCameraFormat &format) override;

    QVideoFrameFormat frameFormat() const override;

private:
    void startStream();
    void stopStream();

    // Returns true if the resolved format changed.
    bool resolveCameraFormat(const QCameraFormat &format);

    bool m_active = false;
    QCameraDevice m_cameraDevice;
    std::unique_ptr<QPipeWireVideoStream> m_stream;
};

} // namespace QtPipeWire

QT_END_NAMESPACE

#endif // QPIPEWIRE_CAMERA_P_H
