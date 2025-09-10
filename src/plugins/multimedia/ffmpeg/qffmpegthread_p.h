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

#include <QtMultimedia/private/qtmultimediaglobal_p.h>

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
    struct Deleter
    {
        void operator()(ConsumerThread *thread) const { thread->stopAndDelete(); }
    };

protected:
    /*!
        Stops the thread and deletes this object
     */
    void stopAndDelete();

    /*!
        Called on this thread when thread starts
     */
    virtual bool init() = 0;

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
        hasData() returns false. The method is supposed to be invoked
        right after the scope of QMutexLocker that lockLoopData returns.
    */
    void dataReady();

    /*!
        Must return true when data is available for processing
     */
    virtual bool hasData() const = 0;

    /*!
        Locks the loop data mutex. It must be used to protect loop data
        like a queue of video frames.
     */
    QMutexLocker<QMutex> lockLoopData() const;

private:
    void run() final;

    mutable QMutex m_loopDataMutex;
    QWaitCondition m_condition;
    bool m_exit = false;
};

template <typename T>
using ConsumerThreadUPtr = std::unique_ptr<T, ConsumerThread::Deleter>;
} // namespace QFFmpeg

QT_END_NAMESPACE

#endif
