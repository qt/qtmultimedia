// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QFFMPEGTHREAD_P_H
#define QFFMPEGTHREAD_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qtmultimediaglobal_p.h>

#include <qmutex.h>
#include <qwaitcondition.h>
#include <qthread.h>

QT_BEGIN_NAMESPACE

class QAudioSink;

namespace QFFmpeg
{

class Thread : public QThread
{
public:
    mutable QMutex mutex;
    qint64 timeOut = -1;
private:
    QWaitCondition condition;

protected:
    QAtomicInteger<bool> exit = false;

public:
    // public API is thread-safe

    void kill();
    virtual void killHelper() {}

    void wake() {
        condition.wakeAll();
    }

protected:
    virtual void init() {}
    virtual void cleanup() {}
    // loop() should never block, all blocking has to happen in shouldWait()
    virtual void loop() = 0;
    virtual bool shouldWait() const { return false; }

private:
    void maybePause();

    void run() override;
};

}

QT_END_NAMESPACE

#endif
