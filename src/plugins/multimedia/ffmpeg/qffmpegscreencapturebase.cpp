// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpegscreencapturebase_p.h"

#include <QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

void QFFmpegScreenCaptureBase::setActive(bool active)
{
    if (m_active == active)
        return;

    if (!setActiveInternal(active)) {
        qWarning() << "Failed to change active status to value" << active;
        return;
    }

    m_active = active;
    emit screenCapture()->activeChanged(active);
}

bool QFFmpegScreenCaptureBase::isActive() const
{
    return m_active;
}

void QFFmpegScreenCaptureBase::setScreen(QScreen *screen)
{
    setSource(m_screen, screen, &QScreenCapture::screenChanged);
}

QScreen *QFFmpegScreenCaptureBase::screen() const
{
    return m_screen;
}

void QFFmpegScreenCaptureBase::setWindow(QWindow *w)
{
    setSource(m_window, w, nullptr);
}

QWindow *QFFmpegScreenCaptureBase::window() const
{
    return m_window;
}

void QFFmpegScreenCaptureBase::setWindowId(WId id)
{
    setSource(m_wid, id, nullptr);
}

WId QFFmpegScreenCaptureBase::windowId() const
{
    return m_wid;
}

template<typename Source, typename NewSource, typename Signal>
void QFFmpegScreenCaptureBase::setSource(Source &source, NewSource newSource, Signal sig)
{
    if (source == newSource)
        return;

    if (m_active)
        setActiveInternal(false);

    // check other sources has been reset before setting not null source
    if (newSource) {
        source = {};

        Q_ASSERT(!m_screen);
        Q_ASSERT(!m_wid);
        Q_ASSERT(!m_window);
    }

    source = newSource;

    if (m_active && newSource)
        setActiveInternal(true);

    if constexpr (!std::is_same_v<Signal, std::nullptr_t>)
        emit (screenCapture()->*sig)(newSource);
    else
        Q_UNUSED(sig);
}

QT_END_NAMESPACE
