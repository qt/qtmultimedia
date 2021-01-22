/****************************************************************************
**
** Copyright (C) 2017 QNX Software Systems. All rights reserved.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef MMREVENTTHREAD_H
#define MMREVENTTHREAD_H

#include <QtCore/QThread>

#include <sys/neutrino.h>
#include <sys/siginfo.h>

QT_BEGIN_NAMESPACE

typedef struct mmr_context mmr_context_t;

class MmrEventThread : public QThread
{
    Q_OBJECT

public:
    MmrEventThread(mmr_context_t *context);
    ~MmrEventThread() override;

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

#endif // MMREVENTTHREAD_H
