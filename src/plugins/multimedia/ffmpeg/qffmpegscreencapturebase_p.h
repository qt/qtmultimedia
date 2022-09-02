// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFFMPEGSCREENCAPTUREBASE_P_H
#define QFFMPEGSCREENCAPTUREBASE_P_H

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

#include <private/qplatformscreencapture_p.h>

#include "qpointer.h"

QT_BEGIN_NAMESPACE

class QFFmpegScreenCaptureBase : public QPlatformScreenCapture
{
public:
    using QPlatformScreenCapture::QPlatformScreenCapture;

    void setActive(bool active) final;

    bool isActive() const final;

    void setScreen(QScreen *screen) final;

    QScreen *screen() const final;

    void setWindow(QWindow *w) final;

    QWindow *window() const final;

    void setWindowId(WId id) final;

    WId windowId() const final;

protected:
    virtual bool setActiveInternal(bool active) = 0;

private:
    template<typename Source, typename NewSource, typename Signal>
    void setSource(Source &source, NewSource newSource, Signal sig);

private:
    bool m_active = false;
    QPointer<QScreen> m_screen;
    QPointer<QWindow> m_window;
    WId m_wid = 0;
};

QT_END_NAMESPACE

#endif // QFFMPEGSCREENCAPTUREBASE_P_H
