// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#include "qffmpegthread_p.h"


QT_BEGIN_NAMESPACE

using namespace QFFmpeg;

void ConsumerThread::stopAndDelete()
{
    {
        QMutexLocker locker(&m_exitMutex);
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
    init();

    while (true) {

        {
            QMutexLocker locker(&m_exitMutex);
            while (!hasData() && !m_exit)
                m_condition.wait(&m_exitMutex);

            if (m_exit)
                break;
        }

        processOne();
    }

    cleanup();
}

QT_END_NAMESPACE
