// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef AVFCAMERA_H
#define AVFCAMERA_H

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

#include <QtMultimedia/private/qavfcamerabase_p.h>

QT_BEGIN_NAMESPACE

class AVFCameraSession;
class AVFCameraService;
class AVFCameraSession;

class AVFCamera : public QAVFCameraBase
{
Q_OBJECT
public:
    AVFCamera(QCamera *camera);
    ~AVFCamera() override;

    void setCaptureSession(QPlatformMediaCaptureSession *) override;

protected:
    void onActiveChanged(bool active) override;
    void onCameraDeviceChanged(const QCameraDevice &device) override;
    bool tryApplyCameraFormat(const QCameraFormat&) override;

private:
    friend class AVFCameraSession;
    AVFCameraService *m_service = nullptr;
    AVFCameraSession *m_session = nullptr;
};

QT_END_NAMESPACE

#endif
