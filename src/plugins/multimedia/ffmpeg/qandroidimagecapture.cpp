// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qandroidimagecapture_p.h"
#include <qandroidcamera_p.h>

QT_BEGIN_NAMESPACE

QAndroidImageCapture::QAndroidImageCapture(QImageCapture *parent)
    : QFFmpegImageCapture(parent)
{
    connect(this, &QPlatformImageCapture::imageSaved, this, &QAndroidImageCapture::updateExif);
}

QAndroidImageCapture::~QAndroidImageCapture()
{
}

int QAndroidImageCapture::doCapture(const QString &fileName)
{
    auto ret = QFFmpegImageCapture::doCapture(fileName);
    if (ret >= 0) {
        auto androidCamera = qobject_cast<QAndroidCamera *>(videoSource());
        if (androidCamera)
            androidCamera->capture();
    }

    return ret;
}

void QAndroidImageCapture::setupVideoSourceConnections()
{
    auto androidCamera = qobject_cast<QAndroidCamera *>(videoSource());
    if (androidCamera)
        connect(androidCamera, &QAndroidCamera::onCaptured, this, &QAndroidImageCapture::newVideoFrame);
    else
        QFFmpegImageCapture::setupVideoSourceConnections();
}

void QAndroidImageCapture::updateExif(int id, const QString &filename)
{
    Q_UNUSED(id);
    auto androidCamera = qobject_cast<QAndroidCamera *>(videoSource());
    if (androidCamera)
        androidCamera->updateExif(filename);
}
