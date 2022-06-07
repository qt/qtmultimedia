// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpegthread_p.h"

#include <qloggingcategory.h>

QT_BEGIN_NAMESPACE

using namespace QFFmpeg;

void Thread::kill()
{
    {
        QMutexLocker locker(&mutex);
        exit.storeRelease(true);
        killHelper();
    }
    wake();
    wait();
    delete this;
}

void Thread::maybePause()
{
    while (timeOut > 0 || shouldWait()) {
        if (exit.loadAcquire())
            break;

        QElapsedTimer timer;
        timer.start();
        if (condition.wait(&mutex, QDeadlineTimer(timeOut, Qt::PreciseTimer))) {
            if (timeOut >= 0) {
                timeOut -= timer.elapsed();
                if (timeOut < 0)
                    timeOut = -1;
            }
        } else {
            timeOut = -1;
        }
    }
}

void Thread::run()
{
    init();
    QMutexLocker locker(&mutex);
    while (1) {
        maybePause();
        if (exit.loadAcquire())
            break;
        loop();
    }
    cleanup();
}

QT_END_NAMESPACE
