// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QANDROIDSCREENCAPTURE_P_H
#define QANDROIDSCREENCAPTURE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtMultimedia/private/qplatformsurfacecapture_p.h>
#include <QtFFmpegMediaPluginImpl/private/qandroidvideoframebuffer_p.h>
#include <QJniObject>
#include <memory>

QT_BEGIN_NAMESPACE

class QAndroidVideoFrameFactory;

class QAndroidScreenCapture : public QPlatformSurfaceCapture
{
    class Grabber;

public:
    explicit QAndroidScreenCapture();
    ~QAndroidScreenCapture() override;

    QVideoFrameFormat frameFormat() const override;

    static bool registerNativeMethods();
    void onNewFrameReceived(QtJniTypes::Image image);
protected:
    bool setActiveInternal(bool active) override;

private:
    std::unique_ptr<Grabber> m_grabber;
    std::shared_ptr<QAndroidVideoFrameFactory> m_frameFactory;
};

QT_END_NAMESPACE

#endif // QANDROIDSCREENCAPTURE_P_H
