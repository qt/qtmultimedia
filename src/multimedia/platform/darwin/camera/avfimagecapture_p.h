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

#ifndef AVFCAMERAIMAGECAPTURE_H
#define AVFCAMERAIMAGECAPTURE_H

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

#import <AVFoundation/AVFoundation.h>

#include <QtCore/qqueue.h>
#include <QtCore/qsemaphore.h>
#include <QtCore/qsharedpointer.h>
#include <private/qplatformimagecapture_p.h>
#include "avfcamerasession_p.h"

QT_BEGIN_NAMESPACE

class AVFImageCapture : public QPlatformImageCapture
{
Q_OBJECT
public:
    struct CaptureRequest {
        int captureId;
        QSharedPointer<QSemaphore> previewReady;
    };

    AVFImageCapture(QImageCapture *parent = nullptr);
    ~AVFImageCapture();

    bool isReadyForCapture() const override;

    AVCaptureStillImageOutput *stillImageOutput() const {return m_stillImageOutput;}

    int doCapture(const QString &fileName);
    int capture(const QString &fileName) override;
    int captureToBuffer() override;

    QImageEncoderSettings imageSettings() const override;
    void setImageSettings(const QImageEncoderSettings &settings) override;
    bool applySettings();

    void setCaptureSession(QPlatformMediaCaptureSession *session);

private Q_SLOTS:
    void updateCaptureConnection();
    void updateReadyStatus();
    void onNewViewfinderFrame(const QVideoFrame &frame);
    void onCameraChanged();

private:
    void makeCapturePreview(CaptureRequest request, const QVideoFrame &frame, int rotation);
    bool videoCaptureDeviceIsValid() const;

    AVFCameraService *m_service = nullptr;
    AVFCameraSession *m_session = nullptr;
    AVFCamera *m_cameraControl = nullptr;
    bool m_ready = false;
    int m_lastCaptureId = 0;
    AVCaptureStillImageOutput *m_stillImageOutput;
    AVCaptureConnection *m_videoConnection = nullptr;

    QMutex m_requestsMutex;
    QQueue<CaptureRequest> m_captureRequests;
    QImageEncoderSettings m_settings;
};

Q_DECLARE_TYPEINFO(AVFImageCapture::CaptureRequest, Q_PRIMITIVE_TYPE);

QT_END_NAMESPACE

#endif
