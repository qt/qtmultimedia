/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd and/or its subsidiary(-ies).
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QWINRTCAMERAFLASHCONTROL_H
#define QWINRTCAMERAFLASHCONTROL_H
#include <qcameraflashcontrol.h>

#include <wrl.h>

namespace ABI {
    namespace Windows {
        namespace Media {
            namespace Devices {
                struct IAdvancedVideoCaptureDeviceController2;
            }
        }
    }
}

QT_BEGIN_NAMESPACE

class QWinRTCameraControl;
class QWinRTCameraFlashControlPrivate;
class QWinRTCameraFlashControl : public QCameraFlashControl
{
    Q_OBJECT
public:
    explicit QWinRTCameraFlashControl(QWinRTCameraControl *parent);

    void initialize(Microsoft::WRL::ComPtr<ABI::Windows::Media::Devices::IAdvancedVideoCaptureDeviceController2> &controller);

    QCameraExposure::FlashModes flashMode() const override;
    void setFlashMode(QCameraExposure::FlashModes mode) override;
    bool isFlashModeSupported(QCameraExposure::FlashModes mode) const override;

    bool isFlashReady() const override;

private:
    QScopedPointer<QWinRTCameraFlashControlPrivate> d_ptr;
    Q_DECLARE_PRIVATE(QWinRTCameraFlashControl)
};

#endif // QWINRTCAMERAFLASHCONTROL_H
