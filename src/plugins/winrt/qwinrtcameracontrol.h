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

#ifndef QWINRTCAMERACONTROL_H
#define QWINRTCAMERACONTROL_H

#include <QtMultimedia/QCameraControl>
#include <QtCore/QLoggingCategory>
#include <QtCore/qt_windows.h>

#include <wrl.h>

namespace ABI {
    namespace Windows {
        namespace Media {
            namespace Capture {
                struct IMediaCapture;
                struct IMediaCaptureFailedEventArgs;
            }
        }
        namespace Foundation {
            struct IAsyncAction;
            enum class AsyncStatus;
        }
    }
}

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcMMCamera)

class QVideoRendererControl;
class QVideoDeviceSelectorControl;
class QCameraImageCaptureControl;
class QImageEncoderControl;
class QCameraFlashControl;
class QCameraFocusControl;
class QCameraLocksControl;

class QWinRTCameraControlPrivate;
class QWinRTCameraControl : public QCameraControl
{
    Q_OBJECT
public:
    explicit QWinRTCameraControl(QObject *parent = nullptr);
    ~QWinRTCameraControl() override;

    QCamera::State state() const override;
    void setState(QCamera::State state) override;

    QCamera::Status status() const override;

    QCamera::CaptureModes captureMode() const override;
    void setCaptureMode(QCamera::CaptureModes mode) override;
    bool isCaptureModeSupported(QCamera::CaptureModes mode) const override;

    bool canChangeProperty(PropertyChangeType changeType, QCamera::Status status) const override;

    QVideoRendererControl *videoRenderer() const;
    QVideoDeviceSelectorControl *videoDeviceSelector() const;
    QCameraImageCaptureControl *imageCaptureControl() const;
    QImageEncoderControl *imageEncoderControl() const;
    QCameraFlashControl *cameraFlashControl() const;
    QCameraFocusControl *cameraFocusControl() const;
    QCameraLocksControl *cameraLocksControl() const;

    Microsoft::WRL::ComPtr<ABI::Windows::Media::Capture::IMediaCapture> handle() const;

    bool setFocus(QCameraFocus::FocusModes mode);
    bool setFocusPoint(const QPointF &point);
    bool focus();
    void clearFocusPoint();
    void emitError(int errorCode, const QString &errorString);
    bool lockFocus();
    bool unlockFocus();
    void frameMapped();
    void frameUnmapped();

private slots:
    void onBufferRequested();
    void onApplicationStateChanged(Qt::ApplicationState state);

private:
    HRESULT enumerateDevices();
    HRESULT initialize();
    HRESULT initializeFocus();
    HRESULT onCaptureFailed(ABI::Windows::Media::Capture::IMediaCapture *,
                            ABI::Windows::Media::Capture::IMediaCaptureFailedEventArgs *);
    HRESULT onRecordLimitationExceeded(ABI::Windows::Media::Capture::IMediaCapture *);
    HRESULT onInitializationCompleted(ABI::Windows::Foundation::IAsyncAction *,
                                      ABI::Windows::Foundation::AsyncStatus);

    QScopedPointer<QWinRTCameraControlPrivate> d_ptr;
    Q_DECLARE_PRIVATE(QWinRTCameraControl)
};

QT_END_NAMESPACE

#endif // QWINRTCAMERACONTROL_H
