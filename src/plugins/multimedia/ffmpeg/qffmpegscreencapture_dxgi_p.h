// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR
// GPL-3.0-only

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
#include <private/qwindowsiupointer_p.h>
#include <private/qplatformscreencapture_p.h>
#include <memory>

QT_BEGIN_NAMESPACE

class DxgiScreenGrabberActive;
class QFFmpegScreenCaptureDxgi : public QPlatformScreenCapture
{
public:
    explicit QFFmpegScreenCaptureDxgi(QScreenCapture *screenCapture);
    ~QFFmpegScreenCaptureDxgi();

    void setActive(bool active) override;
    bool isActive() const override { return bool(m_active); }
    QVideoFrameFormat format() const override;

    void setScreen(QScreen *screen) override;
    QScreen *screen() const override { return m_screen; }

private:
    void setActiveInternal(bool active);

    void resetGrabber();

private:
    std::unique_ptr<DxgiScreenGrabberActive> m_active;
    QScreen *m_screen = nullptr;
    QVideoFrameFormat m_format;
};

QT_END_NAMESPACE

#endif // QFFMPEGSCREENCAPTURE_WINDOWS_H
