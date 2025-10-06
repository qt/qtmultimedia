// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QANDROIDVIDEOFRAMEFACTORY_H
#define QANDROIDVIDEOFRAMEFACTORY_H

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

#include <QtFFmpegMediaPluginImpl/private/qandroidvideoframebuffer_p.h>
#include <memory>

class QAndroidVideoFrameFactory : public QAndroidVideoFrameBuffer::FrameReleaseDelegate
                                , public std::enable_shared_from_this<QAndroidVideoFrameFactory>
{
public:
    using QAndroidVideoFrameFactoryPtr = std::shared_ptr<QAndroidVideoFrameFactory>;
    static QAndroidVideoFrameFactoryPtr create() {
        return std::shared_ptr<QAndroidVideoFrameFactory>(
            new QAndroidVideoFrameFactory());
    };
    QVideoFrame createVideoFrame(QtJniTypes::Image frame,
                                 QtVideo::Rotation rotation = QtVideo::Rotation::None);
    void onFrameReleased() override;

    Q_DISABLE_COPY_MOVE(QAndroidVideoFrameFactory)
private:

    QAndroidVideoFrameFactory() {}
    std::atomic_int m_framesCounter = 0;
    qint64 m_lastTimestamp = 0;
};

#endif // QANDROIDVIDEOFRAMEFACTORY_H
