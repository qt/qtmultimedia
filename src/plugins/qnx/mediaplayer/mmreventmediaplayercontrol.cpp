/****************************************************************************
**
** Copyright (C) 2017 QNX Software Systems. All rights reserved.
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

#include "mmreventmediaplayercontrol.h"
#include "mmreventthread.h"
#include "mmrenderervideowindowcontrol.h"

#include <mm/renderer.h>

QT_BEGIN_NAMESPACE

MmrEventMediaPlayerControl::MmrEventMediaPlayerControl(QObject *parent)
    : MmRendererMediaPlayerControl(parent)
    , m_eventThread(nullptr)
    , m_state(MMR_STATE_IDLE)
{
    openConnection();
}

MmrEventMediaPlayerControl::~MmrEventMediaPlayerControl()
{
    destroy();
}

void MmrEventMediaPlayerControl::startMonitoring()
{
    m_eventThread = new MmrEventThread(m_context);

    connect(m_eventThread, &MmrEventThread::eventPending,
            this, &MmrEventMediaPlayerControl::readEvents);

    m_eventThread->setObjectName(QStringLiteral("MmrEventThread-") + QString::number(m_id));
    m_eventThread->start();
}

void MmrEventMediaPlayerControl::stopMonitoring()
{
    delete m_eventThread;
    m_eventThread = nullptr;
}

bool MmrEventMediaPlayerControl::nativeEventFilter(const QByteArray &eventType,
                                                   void *message,
                                                   long *result)
{
    Q_UNUSED(result)
    if (eventType == "screen_event_t") {
        screen_event_t event = static_cast<screen_event_t>(message);
        if (MmRendererVideoWindowControl *control = videoWindowControl())
            control->screenEventHandler(event);
    }

    return false;
}

void MmrEventMediaPlayerControl::readEvents()
{
    const mmr_event_t *event;

    while ((event = mmr_event_get(m_context))) {
        if (event->type == MMR_EVENT_NONE)
            break;

        switch (event->type) {
        case MMR_EVENT_STATUS: {
            if (event->data) {
                const strm_string_t *value;
                value = strm_dict_find_rstr(event->data, "bufferstatus");
                if (value)
                    setMmBufferStatus(QString::fromLatin1(strm_string_get(value)));

                value = strm_dict_find_rstr(event->data, "bufferlevel");
                if (value)
                    setMmBufferLevel(QString::fromLatin1(strm_string_get(value)));
            }

            if (event->pos_str) {
                const QByteArray valueBa = QByteArray(event->pos_str);
                bool ok;
                const qint64 position = valueBa.toLongLong(&ok);
                if (!ok) {
                    qCritical("Could not parse position from '%s'", valueBa.constData());
                } else {
                    setMmPosition(position);
                }
            }
            break;
        }
        case MMR_EVENT_METADATA: {
            updateMetaData(event->data);
            break;
        }
        case MMR_EVENT_ERROR:
        case MMR_EVENT_STATE:
        case MMR_EVENT_NONE:
        case MMR_EVENT_OVERFLOW:
        case MMR_EVENT_WARNING:
        case MMR_EVENT_PLAYLIST:
        case MMR_EVENT_INPUT:
        case MMR_EVENT_OUTPUT:
        case MMR_EVENT_CTXTPAR:
        case MMR_EVENT_TRKPAR:
        case MMR_EVENT_OTHER: {
            break;
        }
        }

        // Currently, any exit from the playing state is considered a stop (end-of-media).
        // If you ever need to separate end-of-media from things like "stopped unexpectedly"
        // or "stopped because of an error", you'll find that end-of-media is signaled by an
        // MMR_EVENT_ERROR of MMR_ERROR_NONE with state changed to MMR_STATE_STOPPED.
        if (event->state != m_state && m_state == MMR_STATE_PLAYING)
            handleMmStopped();
        m_state = event->state;
    }

    if (m_eventThread)
        m_eventThread->signalRead();
}

QT_END_NAMESPACE
