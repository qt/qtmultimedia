// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpegthread_p.h"


QT_BEGIN_NAMESPACE

using namespace QFFmpeg;

void ConsumerThread::stopAndDelete()
{
    {
        QMutexLocker locker(&m_loopDataMutex);
        m_exit = true;
    }
    dataReady();
    wait();
    delete this;
}

void ConsumerThread::dataReady()
{
    m_condition.wakeAll();
}

void ConsumerThread::run()
{
    if (!init())
        return;

    while (true) {

        {
            QMutexLocker locker(&m_loopDataMutex);
            while (!hasData() && !m_exit)
                m_condition.wait(&m_loopDataMutex);

            if (m_exit)
                break;
        }

        processOne();
    }

    cleanup();
}

QMutexLocker<QMutex> ConsumerThread::lockLoopData() const
{
    return QMutexLocker(&m_loopDataMutex);
}

QT_END_NAMESPACE
