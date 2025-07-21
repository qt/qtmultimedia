// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtFFmpegMediaPluginImpl/private/qavfimagecapture_p.h>
#include <QtFFmpegMediaPluginImpl/private/qavfcamera_p.h>

#include <QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(qAvfImageCaptureLc, "qt.multimedia.avfimagecapture");

namespace QFFmpeg {

QAVFImageCapture::QAVFImageCapture(QImageCapture *parent)
    : QFFmpegImageCapture(parent)
{
}

int QAVFImageCapture::doCapture(const QString &fileName)
{
    int ret = QFFmpegImageCapture::doCapture(fileName);
    if (ret >= 0) {
        if (QAVFCamera *avfCamera = qobject_cast<QAVFCamera*>(videoSource())) {
            q23::expected<void, QString> result = avfCamera->requestStillPhotoCapture();
            if (!result) {
                QFFmpegImageCapture::cancelPendingImage(
                    QImageCapture::Error::ResourceError,
                    result.error());
            }
        }
    }

    return ret;
}

void QAVFImageCapture::setupVideoSourceConnections()
{
    if (QAVFCamera *avfCamera = qobject_cast<QAVFCamera*>(videoSource())) {
        connect(
            avfCamera,
            &QAVFCamera::stillPhotoSucceeded,
            this,
            &QAVFImageCapture::newVideoFrame);

        connect(
            avfCamera,
            &QAVFCamera::stillPhotoFailed,
            this,
            &QFFmpegImageCapture::cancelPendingImage);
    }
    else {
        QFFmpegImageCapture::setupVideoSourceConnections();
    }
}

} // namespace QFFmpeg

QT_END_NAMESPACE
