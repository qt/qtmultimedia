// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#include "qffmpegthread_p.h"


QT_BEGIN_NAMESPACE

using namespace QFFmpeg;

void ConsumerThread::stopAndDelete()
{
    {
        QMutexLocker locker(&exitMutex);
        exit = true;
    }
    dataReady();
    wait();
    delete this;
}

void ConsumerThread::dataReady()
{
    condition.wakeAll();
}

void ConsumerThread::run()
{
    init();

    while (true) {

        {
            QMutexLocker locker(&exitMutex);
            while (!hasData() && !exit)
                condition.wait(&exitMutex);

            if (exit)
                break;
        }

        processOne();
    }

    cleanup();
}

QT_END_NAMESPACE
