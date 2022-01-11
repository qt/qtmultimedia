/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef QFFMPEGCLOCK_P_H
#define QFFMPEGCLOCK_P_H

#include "qffmpeg_p.h"

#include <qelapsedtimer.h>
#include <qlist.h>
#include <qmutex.h>
#include <qmetaobject.h>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

class ClockController;

// Clock runs in displayTime, ie. if playbackRate is not 1, it runs faster or slower
// than a regular clock. All methods take displayTime
// Exception: usecsTo() will return the real time that should pass until we will
// hit the requested display time
class Clock
{
    ClockController *controller = nullptr;
    QElapsedTimer m_timer;
protected:
    mutable QMutex m_clockMutex;
    qint64 m_baseTime = 0;
    float m_playbackRate = 1.;
    bool m_paused = true;
    bool m_isMaster = false;
public:
    enum Type {
        SystemClock,
        AudioClock
    };
    virtual ~Clock();
    virtual Type type() const;

    qint64 baseTime() const
    {
        QMutexLocker l(&m_clockMutex);
        return m_baseTime;
    }
    float playbackRate() const
    {
        QMutexLocker l(&m_clockMutex);
        return m_playbackRate;
    }
    bool isMaster() const
    {
        QMutexLocker l(&m_clockMutex);
        return m_isMaster;
    }

    // all times in usecs
    virtual qint64 currentTime() const;
    virtual qint64 usecsTo(qint64 displayTime);

protected:
    virtual void syncTo(qint64 usecs);
    virtual void adjustBy(qint64 usecs);
    virtual void setPlaybackRate(float rate);
    virtual void setPaused(bool paused);

    void timeUpdated(qint64 currentTime);

private:
    friend class ClockController;
    void setController(ClockController *c)
    {
        Q_ASSERT(!controller);
        controller = c;
    }
    void setIsMaster(bool b)
    {
        QMutexLocker l(&m_clockMutex);
        m_isMaster = b;
    }

};

class ClockController
{
    QList<Clock *> m_clocks;
    Clock *m_master = nullptr;
    float m_playbackRate = 1.;
    bool m_isPaused = true;

    friend class Clock;
    void timeUpdated(Clock *clock, qint64 time);
    QObject *notifyObject = nullptr;
    QMetaMethod notify;
public:
    // max 25 msecs tolerance for the clock
    enum { ClockTolerance = 25000 };
    ClockController() = default;
    ~ClockController();

    void addClock(Clock *provider);
    void removeClock(Clock *provider);

    qint64 skew() const;
    qint64 currentTime() const;

    void syncTo(qint64 usecs);

    void setPlaybackRate(float s);
    float playbackRate() const { return m_playbackRate; }
    void setPaused(bool paused);

    void setNotify(QObject *object, QMetaMethod method)
    {
        notifyObject = object;
        notify = method;
    }
};

}

QT_END_NAMESPACE

#endif
