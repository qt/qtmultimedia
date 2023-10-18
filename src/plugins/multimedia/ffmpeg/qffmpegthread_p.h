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

/*!
    FFmpeg thread that is used to implement a consumer pattern.

    This thread processes work items until no more data is available.
    When no more data is available, it sleeps until it is notified about
    more available data.
 */
class ConsumerThread : public QThread
{
public:
    /*!
        Stops the thread and deletes this object
     */
    void stopAndDelete();

protected:

    /*!
        Called on this thread when thread starts
     */
    virtual void init() = 0;

    /*!
        Called on this thread before thread exits
     */
    virtual void cleanup() = 0;

    /*!
        Process one work item. Called repeatedly until hasData() returns
        false, in which case the thread sleeps until the next dataReady()
        notification.

        Note: processOne() should never block.
     */
    virtual void processOne() = 0;

    /*!
        Wake thread from sleep and process data until
        hasData() returns false.
    */
    void dataReady();

    /*!
        Must return true when data is available for processing
     */
    virtual bool hasData() const = 0;

private:
    void run() final;

    QMutex exitMutex; // Protects exit flag.
    QWaitCondition condition;
    bool exit = false;
};

}

QT_END_NAMESPACE

#endif
