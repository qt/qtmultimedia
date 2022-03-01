/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QAVFCAMERA_H
#define QAVFCAMERA_H

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

#include "qavfcamerabase_p.h"
#include <private/qplatformmediaintegration_p.h>
#include <private/qvideooutputorientationhandler_p.h>
#define AVMediaType XAVMediaType
#include "qffmpeghwaccel_p.h"
#undef AVMediaType

#include <qfilesystemwatcher.h>
#include <qsocketnotifier.h>
#include <qmutex.h>

#include <dispatch/dispatch.h>

Q_FORWARD_DECLARE_OBJC_CLASS(AVCaptureSession);
Q_FORWARD_DECLARE_OBJC_CLASS(AVCaptureDeviceInput);
Q_FORWARD_DECLARE_OBJC_CLASS(AVCaptureVideoDataOutput);
Q_FORWARD_DECLARE_OBJC_CLASS(AVCaptureDevice);

QT_BEGIN_NAMESPACE

Q_FORWARD_DECLARE_OBJC_CLASS(QAVFSampleBufferDelegate);

class QFFmpegVideoSink;

class QAVFCamera : public QAVFCameraBase
{
    Q_OBJECT

public:
    explicit QAVFCamera(QCamera *parent);
    ~QAVFCamera();

    bool isActive() const override;
    void setActive(bool active) override;

    void setCaptureSession(QPlatformMediaCaptureSession *) override;

    void setCamera(const QCameraDevice &camera) override;
    bool setCameraFormat(const QCameraFormat &format) override;

    void syncHandleFrame(const QVideoFrame &frame);

    void deviceOrientationChanged(int angle = -1);

    const void *ffmpegHWAccel() const override { return &hwAccel; }

private:
    void requestCameraPermissionIfNeeded();
    void cameraAuthorizationChanged(bool authorized);
    void updateCameraFormat(const QCameraFormat &format);
    void updateVideoInput();
    void attachVideoInputDevice();
    uint setPixelFormat(const QVideoFrameFormat::PixelFormat pixelFormat);

    AVCaptureDevice *device() const;

//    QVideoFrameFormat::YCbCrColorSpace colorSpace = QVideoFrameFormat::YCbCr_Undefined;

    QMediaCaptureSession *m_session = nullptr;
    AVCaptureSession *m_captureSession = nullptr;
    AVCaptureDeviceInput *m_videoInput = nullptr;
    AVCaptureVideoDataOutput *m_videoDataOutput = nullptr;
    QAVFSampleBufferDelegate *m_sampleBufferDelegate = nullptr;
    dispatch_queue_t m_delegateQueue;
    QVideoOutputOrientationHandler m_orientationHandler;
    QFFmpeg::HWAccel hwAccel;
};

QT_END_NAMESPACE


#endif  // QFFMPEGCAMERA_H

