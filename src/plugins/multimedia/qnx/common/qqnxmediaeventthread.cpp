// Copyright (C) 2017 QNX Software Systems. All rights reserved.
// Copyright (C) 2021 The Qt Company
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqnxmediaeventthread_p.h"

#include <QtCore/QDebug>

#include <errno.h>
#include <mm/renderer/types.h>
#include <sys/neutrino.h>

extern "C" {
// ### Include mm/renderer/events.h once we have it
int mmr_event_arm(mmr_context_t *ctxt,
                  struct sigevent const *sev);
}

QT_BEGIN_NAMESPACE

static const int c_mmrCode = _PULSE_CODE_MINAVAIL + 0;
static const int c_readCode = _PULSE_CODE_MINAVAIL + 1;
static const int c_quitCode = _PULSE_CODE_MINAVAIL + 2;

QQnxMediaEventThread::QQnxMediaEventThread(mmr_context_t *context)
    : QThread(),
      m_mmrContext(context)
{
    if (Q_UNLIKELY((m_channelId = ChannelCreate(_NTO_CHF_DISCONNECT
                                                | _NTO_CHF_UNBLOCK
                                                | _NTO_CHF_PRIVATE)) == -1)) {
        qFatal("QQnxMediaEventThread: Can't continue without a channel");
    }

    if (Q_UNLIKELY((m_connectionId = ConnectAttach(0, 0, m_channelId,
                                                   _NTO_SIDE_CHANNEL, 0)) == -1)) {
        ChannelDestroy(m_channelId);
        qFatal("QQnxMediaEventThread: Can't continue without a channel connection");
    }

    SIGEV_PULSE_INIT(&m_mmrEvent, m_connectionId, SIGEV_PULSE_PRIO_INHERIT, c_mmrCode, 0);
}

QQnxMediaEventThread::~QQnxMediaEventThread()
{
    // block until thread terminates
    shutdown();

    ConnectDetach(m_connectionId);
    ChannelDestroy(m_channelId);
}

void QQnxMediaEventThread::run()
{
    int armResult = mmr_event_arm(m_mmrContext, &m_mmrEvent);
    if (armResult > 0)
        emit eventPending();

    while (1) {
        struct _pulse msg;
        memset(&msg, 0, sizeof(msg));
        int receiveId = MsgReceive(m_channelId, &msg, sizeof(msg), nullptr);
        if (receiveId == 0) {
            if (msg.code == c_mmrCode) {
                emit eventPending();
            } else if (msg.code == c_readCode) {
                armResult = mmr_event_arm(m_mmrContext, &m_mmrEvent);
                if (armResult > 0)
                    emit eventPending();
            } else if (msg.code == c_quitCode) {
                break;
            } else {
                qWarning() << Q_FUNC_INFO << "Unexpected pulse" << msg.code;
            }
        } else if (receiveId > 0) {
            qWarning() << Q_FUNC_INFO << "Unexpected message" << msg.code;
        } else {
            qWarning() << Q_FUNC_INFO << "MsgReceive error" << strerror(errno);
        }
    }
}

void QQnxMediaEventThread::signalRead()
{
    MsgSendPulse(m_connectionId, SIGEV_PULSE_PRIO_INHERIT, c_readCode, 0);
}

void QQnxMediaEventThread::shutdown()
{
    MsgSendPulse(m_connectionId, SIGEV_PULSE_PRIO_INHERIT, c_quitCode, 0);

    // block until thread terminates
    wait();
}

QT_END_NAMESPACE

#include "moc_qqnxmediaeventthread_p.cpp"
