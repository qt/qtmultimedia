// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qandroidimagecapture_p.h"
#include <qandroidcamera_p.h>

QT_BEGIN_NAMESPACE

QAndroidImageCapture::QAndroidImageCapture(QImageCapture *parent)
    : QFFmpegImageCapture(parent)
{
}

QAndroidImageCapture::~QAndroidImageCapture()
{
}

int QAndroidImageCapture::doCapture(const QString &fileName)
{
    auto ret = QFFmpegImageCapture::doCapture(fileName);
    if (ret >= 0) {
        auto androidCamera = static_cast<QAndroidCamera *>(m_camera);
        if (androidCamera)
            androidCamera->capture();
    }

    return ret;
}


void QAndroidImageCapture::setupCameraConnections()
{
    connect(m_camera, &QPlatformCamera::activeChanged, this, &QFFmpegImageCapture::cameraActiveChanged);
    auto androidCamera = static_cast<QAndroidCamera *>(m_camera);
    if (androidCamera)
        connect(androidCamera, &QAndroidCamera::onCaptured, this, &QAndroidImageCapture::newVideoFrame);
}
