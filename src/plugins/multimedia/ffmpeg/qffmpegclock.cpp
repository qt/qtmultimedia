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
#include <qffmpegclock_p.h>
#include <qloggingcategory.h>

Q_LOGGING_CATEGORY(qLcClock, "qt.multimedia.ffmpeg.clock")

QT_BEGIN_NAMESPACE

QFFmpeg::Clock::~Clock()
{
    if (controller)
        controller->removeClock(this);
}

qint64 QFFmpeg::Clock::currentTime() const
{
    QMutexLocker l(&m_clockMutex);
    if (m_paused)
        return m_baseTime;
    return qRound64(m_timer.nsecsElapsed()*(m_playbackRate/1000.)) + m_baseTime;
}

void QFFmpeg::Clock::syncTo(qint64 time)
{
    qCDebug(qLcClock) << "syncTo" << time << isMaster();
    QMutexLocker l(&m_clockMutex);
    m_baseTime = time;
    m_timer.restart();
}

void QFFmpeg::Clock::adjustBy(qint64 usecs)
{
    qCDebug(qLcClock) << "adjustBy" << usecs << isMaster();
    QMutexLocker l(&m_clockMutex);
    m_baseTime += usecs;
}

void QFFmpeg::Clock::setPlaybackRate(float rate)
{
    QMutexLocker l(&m_clockMutex);
    if (!m_paused) {
        m_baseTime += qRound64(m_timer.nsecsElapsed()*(m_playbackRate/1000.));
        m_timer.restart();
    }
    m_playbackRate = rate;
}

void QFFmpeg::Clock::setPaused(bool paused)
{
    QMutexLocker l(&m_clockMutex);
    m_paused = paused;
    if (m_paused)
        m_baseTime += qRound64(m_timer.nsecsElapsed()*(m_playbackRate/1000.));
    else
        m_timer.restart();
}

qint64 QFFmpeg::Clock::timeUpdated(qint64 currentTime)
{
    {
        QMutexLocker l(&m_clockMutex);
        if (m_isMaster) {
            m_baseTime = currentTime;
            m_timer.restart();
        }
    }

    if (controller)
        return controller->timeUpdated(this, currentTime);
    return currentTime;
}

qint64 QFFmpeg::Clock::usecsTo(qint64 currentTime, qint64 displayTime)
{
    QMutexLocker l(&m_clockMutex);
    if (m_paused)
        return -1;
    int t = qRound64((displayTime - currentTime)/m_playbackRate);
    return t < 0 ? -1 : t;
}

QFFmpeg::Clock::Type QFFmpeg::Clock::type() const
{
    return SystemClock;
}


qint64 QFFmpeg::ClockController::timeUpdated(Clock *clock, qint64 time)
{
    if (clock == m_master) {
        // Avoid posting too many updates to the notifyObject, or we can overload
        // the event queue with too many notifications
        if (qAbs(time - m_lastMasterTime) < 5000)
            return time;
        m_lastMasterTime = time;
//        qCDebug(qLcClock) << "ClockController::timeUpdated(master)" << time << "skew" << skew();
        if (notifyObject)
            notify.invoke(notifyObject, Qt::QueuedConnection, Q_ARG(qint64, time));
        return time;
    }

    // check if we need to adjust clocks
    qint64 mtime = m_master->currentTime();
    qint64 skew =  mtime - time;
    qCDebug(qLcClock) << "ClockController::timeUpdated(slave)" << time << "master" << m_master->currentTime() << "skew" << skew;
    if (qAbs(skew) > ClockTolerance) {
        // we adjust if clock skew is larger than 25ms
        clock->adjustBy(skew);
    }
    return mtime;
}

QFFmpeg::ClockController::~ClockController()
{
    for (auto *p : qAsConst(m_clocks))
        p->setController(nullptr);
}

void QFFmpeg::ClockController::addClock(Clock *clock)
{
    Q_ASSERT(clock != nullptr);

    if (m_clocks.contains(clock))
        return;

    if (!m_master) {
        m_master = clock;
        m_master->setIsMaster(true);
    }
    m_clocks.append(clock);
    clock->setController(this);

    if (m_master != clock) {
        auto t = m_master->currentTime();
        clock->syncTo(t);
        if (!m_isPaused)
            clock->setPaused(false);
        // update master clock
        if (clock->type() > m_master->type()) {
            m_master->setIsMaster(false);
            m_master = clock;
            m_master->setIsMaster(true);
        }
    }
}

void QFFmpeg::ClockController::removeClock(Clock *clock)
{
    m_clocks.removeAll(clock);
    clock->setController(nullptr);
    if (m_master == clock) {
        // find a new master clock and resync
        qint64 t = clock->currentTime();
        m_master->setIsMaster(false);
        m_master = nullptr;
        for (auto *c : qAsConst(m_clocks)) {
            if (!m_master || m_master->type() < c->type())
                m_master = c;
            c->syncTo(t);
        }
        if (m_master)
            m_master->setIsMaster(true);
    }
}

qint64 QFFmpeg::ClockController::skew() const
{
//    qCDebug(qLcClock) << "skew:";
    qint64 min = std::numeric_limits<qint64>::max();
    qint64 max = std::numeric_limits<qint64>::min();
    for (auto *p : qAsConst(m_clocks)) {
        qint64 t = p->currentTime();
//        qCDebug(qLcClock) << "    " << p->currentTime() << p->baseTime() << p->isMaster();
        min = qMin(min, t);
        max = qMax(max, t);
    }
    return max - min;
}

qint64 QFFmpeg::ClockController::currentTime() const
{
    if (!m_master)
        return 0;
    return m_master->currentTime();
}

void QFFmpeg::ClockController::syncTo(qint64 usecs)
{
    for (auto *p : qAsConst(m_clocks))
        p->syncTo(usecs);
}

void QFFmpeg::ClockController::setPlaybackRate(float s)
{
    m_playbackRate = s;
    for (auto *p : qAsConst(m_clocks))
        p->setPlaybackRate(s);
}

void QFFmpeg::ClockController::setPaused(bool paused)
{
    if (m_isPaused == paused)
        return;
    m_isPaused = paused;
    if (!paused && m_master) {
        qint64 time = m_master->currentTime();
        qCDebug(qLcClock) << "syncing clocks to" << time;
        for (auto *p : qAsConst(m_clocks))
            if (p != m_master)
                p->syncTo(time);
    }
    for (auto *p : qAsConst(m_clocks))
        p->setPaused(paused);
}

QT_END_NAMESPACE
