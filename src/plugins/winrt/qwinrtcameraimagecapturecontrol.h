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
**
**
**
****************************************************************************/

#ifndef QWINRTCAMERAIMAGECAPTURECONTROL_H
#define QWINRTCAMERAIMAGECAPTURECONTROL_H

#include <QtMultimedia/QCamera>
#include <QtMultimedia/QCameraImageCaptureControl>
#include <QtCore/qt_windows.h>

namespace ABI {
    namespace Windows {
        namespace Foundation {
            struct IAsyncAction;
            enum class AsyncStatus;
        }
    }
}

QT_BEGIN_NAMESPACE

class QWinRTCameraControl;

class QWinRTCameraImageCaptureControlPrivate;
class QWinRTCameraImageCaptureControl : public QCameraImageCaptureControl
{
    Q_OBJECT
public:
    explicit QWinRTCameraImageCaptureControl(QWinRTCameraControl *parent);

    bool isReadyForCapture() const override;

    QCameraImageCapture::DriveMode driveMode() const override;
    void setDriveMode(QCameraImageCapture::DriveMode mode) override;

    int capture(const QString &fileName) override;
    void cancelCapture() override;

private slots:
    void onCameraStateChanged(QCamera::State state);

signals:
    void captureQueueChanged(bool isEmpty);

private:
    HRESULT onCaptureCompleted(ABI::Windows::Foundation::IAsyncAction *,
                               ABI::Windows::Foundation::AsyncStatus);

    QScopedPointer<QWinRTCameraImageCaptureControlPrivate> d_ptr;
    Q_DECLARE_PRIVATE(QWinRTCameraImageCaptureControl)
};

QT_END_NAMESPACE

#endif // QWINRTCAMERAIMAGECAPTURECONTROL_H
