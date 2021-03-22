/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#ifndef AVFCAMERASERVICE_H
#define AVFCAMERASERVICE_H

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

#include <QtCore/qobject.h>
#include <QtCore/qset.h>
#include <private/qplatformmediacapture_p.h>

QT_BEGIN_NAMESPACE
class QPlatformCamera;
class QPlatformMediaEncoder;
class QPlatformCameraImageProcessing;
class AVFCameraControl;
class AVFImageCaptureControl;
class AVFCameraSession;
class AVFCameraFocusControl;
class AVFCameraExposureControl;
class AVFCameraImageProcessingControl;
class AVFMediaEncoder;

Q_FORWARD_DECLARE_OBJC_CLASS(AVCaptureDevice);

class AVFCameraService : public QPlatformMediaCaptureSession
{
Q_OBJECT
public:
    AVFCameraService();
    ~AVFCameraService();

    QPlatformCamera *cameraControl() override;
    QPlatformCameraImageCapture *imageCaptureControl() override;
    QPlatformMediaEncoder *mediaEncoder() override;

    bool isMuted() const override;
    void setMuted(bool muted) override;
    qreal volume() const override;
    void setVolume(qreal volume) override;
    QAudioDeviceInfo audioInput() const override;
    bool setAudioInput(const QAudioDeviceInfo &) override;

    void setVideoPreview(QVideoSink *sink) override;

    AVFCameraSession *session() const { return m_session; }
    AVFCameraControl *avfCameraControl() const { return m_cameraControl; }
    AVFMediaEncoder *recorderControl() const { return m_recorderControl; }
    AVFImageCaptureControl *avfImageCaptureControl() const { return m_imageCaptureControl; }
    AVFCameraFocusControl *cameraFocusControl() const { return m_cameraFocusControl; }
    AVFCameraExposureControl *cameraExposureControl() const { return m_cameraExposureControl; }
    QPlatformCameraImageProcessing *cameraImageProcessingControl() const;

private:
    bool m_muted = false;
    qreal m_volume = 1.0;
    AVCaptureDevice *m_audioCaptureDevice = nullptr;

    AVFCameraSession *m_session;
    AVFCameraControl *m_cameraControl;
    AVFMediaEncoder *m_recorderControl;
    AVFImageCaptureControl *m_imageCaptureControl;
    AVFCameraFocusControl *m_cameraFocusControl;
    AVFCameraExposureControl *m_cameraExposureControl;
    AVFCameraImageProcessingControl *m_cameraImageProcessingControl;
};

QT_END_NAMESPACE

#endif
