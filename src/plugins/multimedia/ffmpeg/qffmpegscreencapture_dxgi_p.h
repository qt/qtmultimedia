// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFFMPEGSCREENCAPTURE_WINDOWS_H
#define QFFMPEGSCREENCAPTURE_WINDOWS_H

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

#include "qvideoframeformat.h"
#include <QtCore/private/qcomptr_p.h>
#include <QtMultimedia/private/qplatformsurfacecapture_p.h>
#include <memory>

QT_BEGIN_NAMESPACE

class QFFmpegScreenCaptureDxgi : public QPlatformSurfaceCapture
{
public:
    explicit QFFmpegScreenCaptureDxgi();

    ~QFFmpegScreenCaptureDxgi() override;

    QVideoFrameFormat frameFormat() const override;

private:
    bool setActiveInternal(bool active) override;

private:
    class Grabber;
    std::unique_ptr<Grabber> m_grabber;
};

QT_END_NAMESPACE

#endif // QFFMPEGSCREENCAPTURE_WINDOWS_H
