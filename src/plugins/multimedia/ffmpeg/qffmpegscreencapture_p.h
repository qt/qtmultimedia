// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFFMPEGSCREENCAPTURE_P_H
#define QFFMPEGSCREENCAPTURE_P_H

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
#include <private/qplatformscreencapture_p.h>
#include <memory>

QT_BEGIN_NAMESPACE

class ScreenGrabberActive;
class QFFmpegScreenCapture : public QPlatformScreenCapture
{
public:
    explicit QFFmpegScreenCapture(QScreenCapture *screenCapture);
    ~QFFmpegScreenCapture();

    void setActive(bool active) override;
    bool isActive() const override { return bool(m_active); }
    QVideoFrameFormat format() const override;

    void setScreen(QScreen *screen) override;
    QScreen *screen() const override { return m_screen; }

private:
    void setActiveInternal(bool active);

    std::unique_ptr<ScreenGrabberActive> m_active;
    QScreen *m_screen = nullptr;
    QVideoFrameFormat m_format;
};

QT_END_NAMESPACE

#endif // QFFMPEGSCREENCAPTURE_P_H
