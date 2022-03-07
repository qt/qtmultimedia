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

QFFmpeg::Clock::Clock(ClockController *controller)
    : controller(controller)
{
    Q_ASSERT(controller);
    controller->addClock(this);
}

QFFmpeg::Clock::~Clock()
{
    if (controller)
        controller->removeClock(this);
}

qint64 QFFmpeg::Clock::currentTime() const
{
    return controller ? controller->currentTime() : 0.;
}

void QFFmpeg::Clock::syncTo(qint64 time)
{
    qCDebug(qLcClock) << "syncTo" << time << isMaster();
}

void QFFmpeg::Clock::setPlaybackRate(float rate, qint64 currentTime)
{
    qCDebug(qLcClock) << "Clock::setPlaybackRate" << rate;
    Q_UNUSED(rate)
    Q_UNUSED(currentTime)
}

void QFFmpeg::Clock::setPaused(bool paused)
{
    qCDebug(qLcClock) << "Clock::setPaused" << paused;
    Q_UNUSED(paused)
}

qint64 QFFmpeg::Clock::timeUpdated(qint64 currentTime)
{
    if (controller)
        return controller->timeUpdated(this, currentTime);
    return currentTime;
}

qint64 QFFmpeg::Clock::usecsTo(qint64 currentTime, qint64 displayTime)
{
    if (!controller || controller->m_isPaused)
        return -1;
    int t = qRound64((displayTime - currentTime)/playbackRate());
    return t < 0 ? 0 : t;
}

QFFmpeg::Clock::Type QFFmpeg::Clock::type() const
{
    return SystemClock;
}

QFFmpeg::ClockController::~ClockController()
{
    QMutexLocker l(&m_mutex);
    for (auto *p : qAsConst(m_clocks))
        p->setController(nullptr);
}

qint64 QFFmpeg::ClockController::timeUpdated(Clock *clock, qint64 time)
{
    QMutexLocker l(&m_mutex);
    if (clock != m_master) {
        // If the clock isn't the master clock, simply return the current time
        // so we can make adjustments as needed
        return currentTimeNoLock();
    }

    // if the clock is the master, adjust our base timing
    m_baseTime = time;
    m_timer.restart();

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

void QFFmpeg::ClockController::addClock(Clock *clock)
{
    QMutexLocker l(&m_mutex);
    qCDebug(qLcClock) << "addClock" << clock;
    Q_ASSERT(clock != nullptr);

    if (m_clocks.contains(clock))
        return;

    if (!m_master)
        m_master = clock;

    m_clocks.append(clock);
    clock->syncTo(currentTimeNoLock());
    clock->setPaused(m_isPaused);

    // update master clock
    if (m_master != clock && clock->type() > m_master->type())
        m_master = clock;
}

void QFFmpeg::ClockController::removeClock(Clock *clock)
{
    QMutexLocker l(&m_mutex);
    qCDebug(qLcClock) << "removeClock" << clock;
    m_clocks.removeAll(clock);
    if (m_master == clock) {
        // find a new master clock
        m_master = nullptr;
        for (auto *c : qAsConst(m_clocks)) {
            if (!m_master || m_master->type() < c->type())
                m_master = c;
        }
    }
}

qint64 QFFmpeg::ClockController::currentTime() const
{
    QMutexLocker l(&m_mutex);
    return currentTimeNoLock();
}

void QFFmpeg::ClockController::syncTo(qint64 usecs)
{
    QMutexLocker l(&m_mutex);
    qCDebug(qLcClock) << "syncTo" << usecs;
    m_baseTime = usecs;
    m_seekTime = usecs;
    m_timer.restart();
    for (auto *p : qAsConst(m_clocks))
        p->syncTo(usecs);
}

void QFFmpeg::ClockController::setPlaybackRate(float s)
{
    QMutexLocker l(&m_mutex);
    qCDebug(qLcClock) << "setPlaybackRate" << s;
    m_baseTime = currentTimeNoLock();
    m_timer.restart();
    m_playbackRate = s;
    for (auto *p : qAsConst(m_clocks))
        p->setPlaybackRate(s, m_baseTime);
}

void QFFmpeg::ClockController::setPaused(bool paused)
{
    QMutexLocker l(&m_mutex);
    if (m_isPaused == paused)
        return;
    qCDebug(qLcClock) << "setPaused" << paused;
    m_isPaused = paused;
    if (m_isPaused) {
        m_baseTime = currentTimeNoLock();
        m_seekTime = m_baseTime;
    } else {
        m_timer.restart();
    }
    for (auto *p : qAsConst(m_clocks))
        p->setPaused(paused);
}

QT_END_NAMESPACE
