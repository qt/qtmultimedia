/****************************************************************************
**
** Copyright (C) 2016 Research In Motion
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "bpsmediaplayercontrol.h"
#include "mmrenderervideowindowcontrol.h"

#include <bps/mmrenderer.h>
#include <bps/screen.h>

QT_BEGIN_NAMESPACE

BpsMediaPlayerControl::BpsMediaPlayerControl(QObject *parent)
    : MmRendererMediaPlayerControl(parent),
      m_eventMonitor(0)
{
    openConnection();
}

BpsMediaPlayerControl::~BpsMediaPlayerControl()
{
    destroy();
}

void BpsMediaPlayerControl::startMonitoring(int contextId, const QString &contextName)
{
    m_eventMonitor = mmrenderer_request_events(contextName.toLatin1().constData(), 0, contextId);
    if (!m_eventMonitor) {
        qDebug() << "Unable to request multimedia events";
        emit error(0, "Unable to request multimedia events");
    }
}

void BpsMediaPlayerControl::stopMonitoring()
{
    if (m_eventMonitor) {
        mmrenderer_stop_events(m_eventMonitor);
        m_eventMonitor = 0;
    }
}

bool BpsMediaPlayerControl::nativeEventFilter(const QByteArray &eventType, void *message, long *result)
{
    Q_UNUSED(result)
    Q_UNUSED(eventType)

    bps_event_t * const event = static_cast<bps_event_t *>(message);
    if (!event ||
        (bps_event_get_domain(event) != mmrenderer_get_domain() &&
         bps_event_get_domain(event) != screen_get_domain()))
        return false;

    if (event && bps_event_get_domain(event) == screen_get_domain()) {
        const screen_event_t screen_event = screen_event_get_event(event);
        if (MmRendererVideoWindowControl *control = videoWindowControl())
            control->screenEventHandler(screen_event);
    }

    if (bps_event_get_domain(event) == mmrenderer_get_domain()) {
        if (bps_event_get_code(event) == MMRENDERER_STATE_CHANGE) {
            const mmrenderer_state_t newState = mmrenderer_event_get_state(event);
            if (newState == MMR_STOPPED) {
                handleMmStopped();
                return false;
            }
        }

        if (bps_event_get_code(event) == MMRENDERER_STATUS_UPDATE) {
            const qint64 newPosition = QString::fromLatin1(mmrenderer_event_get_position(event)).
                                       toLongLong();
            handleMmStatusUpdate(newPosition);

            const QString status = QString::fromLatin1(mmrenderer_event_get_bufferstatus(event));
            setMmBufferStatus(status);

            const QString level = QString::fromLatin1(mmrenderer_event_get_bufferlevel(event));
            setMmBufferLevel(level);
        }
    }

    return false;
}

QT_END_NAMESPACE
