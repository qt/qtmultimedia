/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd and/or its subsidiary(-ies).
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QWINRTCAMERACONTROL_H
#define QWINRTCAMERACONTROL_H

#include <QtMultimedia/QCameraControl>
#include <QtCore/qt_windows.h>

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

class QVideoRendererControl;
class QVideoDeviceSelectorControl;
class QCameraImageCaptureControl;
class QImageEncoderControl;

class QWinRTCameraControlPrivate;
class QWinRTCameraControl : public QCameraControl
{
    Q_OBJECT
public:
    explicit QWinRTCameraControl(QObject *parent = 0);
    ~QWinRTCameraControl();

    QCamera::State state() const Q_DECL_OVERRIDE;
    void setState(QCamera::State state) Q_DECL_OVERRIDE;

    QCamera::Status status() const Q_DECL_OVERRIDE;

    QCamera::CaptureModes captureMode() const Q_DECL_OVERRIDE;
    void setCaptureMode(QCamera::CaptureModes mode) Q_DECL_OVERRIDE;
    bool isCaptureModeSupported(QCamera::CaptureModes mode) const Q_DECL_OVERRIDE;

    bool canChangeProperty(PropertyChangeType changeType, QCamera::Status status) const Q_DECL_OVERRIDE;

    QVideoRendererControl *videoRenderer() const;
    QVideoDeviceSelectorControl *videoDeviceSelector() const;
    QCameraImageCaptureControl *imageCaptureControl() const;
    QImageEncoderControl *imageEncoderControl() const;

    ABI::Windows::Media::Capture::IMediaCapture *handle() const;

private slots:
    void onBufferRequested();

private:
    HRESULT enumerateDevices();
    HRESULT initialize();
    HRESULT onCaptureFailed(ABI::Windows::Media::Capture::IMediaCapture *,
                            ABI::Windows::Media::Capture::IMediaCaptureFailedEventArgs *);
    HRESULT onRecordLimitationExceeded(ABI::Windows::Media::Capture::IMediaCapture *);

    QScopedPointer<QWinRTCameraControlPrivate> d_ptr;
    Q_DECLARE_PRIVATE(QWinRTCameraControl)
};

QT_END_NAMESPACE

#endif // QWINRTCAMERACONTROL_H
