/****************************************************************************
**
** Copyright (C) 2013 Research In Motion
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "bbplayervideorenderercontrol.h"

#include "windowgrabber.h"

#include <QCoreApplication>
#include <QDebug>
#include <QVideoSurfaceFormat>

#include <mm/renderer.h>

QT_BEGIN_NAMESPACE

static int winIdCounter = 0;

BbPlayerVideoRendererControl::BbPlayerVideoRendererControl(QObject *parent)
    : QVideoRendererControl(parent)
    , m_windowGrabber(new WindowGrabber(this))
    , m_context(0)
    , m_videoId(-1)
{
    connect(m_windowGrabber, SIGNAL(frameGrabbed(QImage)), SLOT(frameGrabbed(QImage)));
}

BbPlayerVideoRendererControl::~BbPlayerVideoRendererControl()
{
    detachDisplay();
}

QAbstractVideoSurface *BbPlayerVideoRendererControl::surface() const
{
    return m_surface;
}

void BbPlayerVideoRendererControl::setSurface(QAbstractVideoSurface *surface)
{
    m_surface = QPointer<QAbstractVideoSurface>(surface);
}

void BbPlayerVideoRendererControl::attachDisplay(mmr_context_t *context)
{
    if (m_videoId != -1) {
        qWarning() << "BbPlayerVideoRendererControl: Video output already attached!";
        return;
    }

    if (!context) {
        qWarning() << "BbPlayerVideoRendererControl: No media player context!";
        return;
    }

    const QByteArray windowGroupId = m_windowGrabber->windowGroupId();
    if (windowGroupId.isEmpty()) {
        qWarning() << "BbPlayerVideoRendererControl: Unable to find window group";
        return;
    }

    const QString windowName = QStringLiteral("BbPlayerVideoRendererControl_%1_%2")
                                             .arg(winIdCounter++)
                                             .arg(QCoreApplication::applicationPid());

    m_windowGrabber->setWindowId(windowName.toLatin1());

    // Start with an invisible window, because we just want to grab the frames from it.
    const QString videoDeviceUrl = QStringLiteral("screen:?winid=%1&wingrp=%2&initflags=invisible&nodstviewport=1")
                                                 .arg(windowName)
                                                 .arg(QString::fromLatin1(windowGroupId));

    m_videoId = mmr_output_attach(context, videoDeviceUrl.toLatin1(), "video");
    if (m_videoId == -1) {
        qWarning() << "mmr_output_attach() for video failed";
        return;
    }

    m_context = context;
}

void BbPlayerVideoRendererControl::detachDisplay()
{
    m_windowGrabber->stop();

    if (m_surface)
        m_surface->stop();

    if (m_context && m_videoId != -1)
        mmr_output_detach(m_context, m_videoId);

    m_context = 0;
    m_videoId = -1;
}

void BbPlayerVideoRendererControl::pause()
{
    m_windowGrabber->pause();
}

void BbPlayerVideoRendererControl::resume()
{
    m_windowGrabber->resume();
}

void BbPlayerVideoRendererControl::frameGrabbed(const QImage &frame)
{
    if (m_surface) {
        if (!m_surface->isActive()) {
            m_surface->start(QVideoSurfaceFormat(frame.size(), QVideoFrame::Format_ARGB32));
        } else {
            if (m_surface->surfaceFormat().frameSize() != frame.size()) {
                m_surface->stop();
                m_surface->start(QVideoSurfaceFormat(frame.size(), QVideoFrame::Format_ARGB32));
            }
        }

        m_surface->present(frame.copy());
    }
}

QT_END_NAMESPACE
