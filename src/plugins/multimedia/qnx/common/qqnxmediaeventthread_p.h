// Copyright (C) 2017 QNX Software Systems. All rights reserved.
// Copyright (C) 2021 The Qt Company
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQNXMEDIAEVENTTHREAD_P_H
#define QQNXMEDIAEVENTTHREAD_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/QThread>

#include <sys/neutrino.h>
#include <sys/siginfo.h>

QT_BEGIN_NAMESPACE

typedef struct mmr_context mmr_context_t;

class QQnxMediaEventThread : public QThread
{
    Q_OBJECT

public:
    QQnxMediaEventThread(mmr_context_t *context);
    ~QQnxMediaEventThread() override;

    void signalRead();

protected:
    void run() override;

Q_SIGNALS:
    void eventPending();

private:
    void shutdown();

    int m_channelId;
    int m_connectionId;
    struct sigevent m_mmrEvent;
    mmr_context_t *m_mmrContext;
};

QT_END_NAMESPACE

#endif // QQnxMediaEventThread_H
