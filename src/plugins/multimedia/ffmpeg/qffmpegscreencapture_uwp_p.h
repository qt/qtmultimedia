// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFFMPEGSCREENCAPTURE_UWP_P_H
#define QFFMPEGSCREENCAPTURE_UWP_P_H

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

#include <QtCore/qnamespace.h>
#include <private/qplatformscreencapture_p.h>
#include "qvideoframeformat.h"
#include <memory>

QT_BEGIN_NAMESPACE

class ScreenGrabberActiveUwp;
class QFFmpegScreenCaptureUwp: public QPlatformScreenCapture
{
public:
    explicit QFFmpegScreenCaptureUwp(QScreenCapture *screenCapture);
    ~QFFmpegScreenCaptureUwp();

    void setActive(bool active) override;
    bool isActive() const override { return m_active; }
    QVideoFrameFormat format() const override;

    void setScreen(QScreen *screen) override;
    QScreen *screen() const override { return m_screen; }

    void setWindowId(WId id) override;
    WId windowId() const override { return m_wId; }

    void setWindow(QWindow *w) override;
    QWindow *window() const { return m_window; }

    static bool isSupported();

private:
    friend ScreenGrabberActiveUwp;

    void emitError(QScreenCapture::Error code, const QString &desc, HRESULT hr);
    void emitError(QScreenCapture::Error code, const QString &desc);

    bool setActiveInternal(bool active);

    std::unique_ptr<ScreenGrabberActiveUwp> m_screenGrabber;
    bool m_active = false;
    QScreen *m_screen = nullptr;
    QWindow *m_window = nullptr;
    WId m_wId = 0;
    QVideoFrameFormat m_format;
};

QT_END_NAMESPACE

#endif // QFFMPEGSCREENCAPTURE_UWP_P_H
