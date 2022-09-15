// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QFFMPEGCLOCK_P_H
#define QFFMPEGCLOCK_P_H

#include "qffmpeg_p.h"

#include <qatomic.h>
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
public:
    enum Type {
        SystemClock,
        AudioClock
    };
    Clock(ClockController *controller);
    virtual ~Clock();
    virtual Type type() const;

    float playbackRate() const;
    bool isMaster() const;

    // all times in usecs
    qint64 currentTime() const;
    qint64 seekTime() const;
    qint64 usecsTo(qint64 currentTime, qint64 displayTime);

protected:
    virtual void syncTo(qint64 usecs);
    virtual void setPlaybackRate(float rate, qint64 currentTime);
    virtual void setPaused(bool paused);

    qint64 timeUpdated(qint64 currentTime);

private:
    friend class ClockController;
    void setController(ClockController *c)
    {
        controller = c;
    }
};

class ClockController
{
    mutable QMutex m_mutex;
    QList<Clock *> m_clocks;
    QAtomicPointer<Clock> m_master = nullptr;

    QElapsedTimer m_elapsedTimer;
    qint64 m_baseTime = 0;
    qint64 m_seekTime = 0;
    float m_playbackRate = 1.;
    bool m_isPaused = true;

    qint64 currentTimeNoLock() const;

    friend class Clock;
    qint64 timeUpdated(Clock *clock, qint64 time);
    void addClock(Clock *provider);
    void removeClock(Clock *provider);
    bool isMaster(const Clock *clock) const;

public:
    ClockController() = default;
    ~ClockController();


    qint64 currentTime() const;

    void syncTo(qint64 usecs);

    void setPlaybackRate(float s);
    float playbackRate() const { return m_playbackRate; }
    void setPaused(bool paused);
};

inline float Clock::playbackRate() const
{
    return controller ? controller->m_playbackRate : 1.;
}

inline bool Clock::isMaster() const
{
    return controller && controller->isMaster(this);
}

inline qint64 Clock::seekTime() const
{
    return controller ? controller->m_seekTime : 0;
}


}

QT_END_NAMESPACE

#endif
