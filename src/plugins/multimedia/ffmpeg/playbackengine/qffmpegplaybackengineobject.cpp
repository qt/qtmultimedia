// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "playbackengine/qffmpegplaybackengineobject_p.h"

#include "QtCore/qchronotimer.h"
#include "QtCore/qdebug.h"
#include "QtCore/qscopedvaluerollback.h"

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

PlaybackEngineObject::PlaybackEngineObject(const PlaybackEngineObjectID &id) : m_id{ id } { }

PlaybackEngineObject::~PlaybackEngineObject()
{
    if (!thread()->isCurrentThread())
        qWarning() << "The playback engine object is being removed in an unexpected thread";
}

bool PlaybackEngineObject::isPaused() const
{
    return m_paused;
}

void PlaybackEngineObject::setAtEnd(bool isAtEnd)
{
    if (m_atEnd.testAndSetRelease(!isAtEnd, isAtEnd) && isAtEnd)
        emit atEnd(id());
}

bool PlaybackEngineObject::isAtEnd() const
{
    return m_atEnd;
}

void PlaybackEngineObject::setPaused(bool isPaused)
{
    if (m_paused.testAndSetRelease(!isPaused, isPaused))
        invokePriorityMethod([this]() { onPauseChanged(); });
}

void PlaybackEngineObject::kill()
{
    m_invalidateCounter.fetch_add(1, std::memory_order_relaxed);

    disconnect();
    deleteLater();
}

bool PlaybackEngineObject::canDoNextStep() const
{
    return !m_paused;
}

QChronoTimer &PlaybackEngineObject::timer()
{
    if (!m_timer) {
        m_timer = std::make_unique<QChronoTimer>();
        m_timer->setTimerType(Qt::PreciseTimer);
        m_timer->setSingleShot(true);
        connect(m_timer.get(), &QChronoTimer::timeout, this, &PlaybackEngineObject::onTimeout);
    }

    return *m_timer;
}

void PlaybackEngineObject::onTimeout()
{
    Q_ASSERT(m_timePoint && !m_nextTimePoint && m_stepType == StepType::None);

    m_timePoint.reset();
    if (isValid() && canDoNextStep())
        doNextStep(StepType::Timeout);
}

PlaybackEngineObject::TimePoint PlaybackEngineObject::nextTimePoint() const
{
    return TimePoint::min();
}

void PlaybackEngineObject::onPauseChanged()
{
    scheduleNextStep();
}

void PlaybackEngineObject::scheduleNextStep()
{
    using std::chrono::milliseconds;
    using namespace std::chrono_literals;

    if (isValid() && canDoNextStep())
        m_nextTimePoint = nextTimePoint();
    else
        m_nextTimePoint.reset();

    if (m_stepType == StepType::Immediate)
        return;

    std::optional<TimePoint> nowCached;
    auto now = [&nowCached, this]() {
        Q_ASSERT(m_nextTimePoint);

        if (!nowCached) {
            Q_ASSERT(m_nextTimePoint);
            if (*m_nextTimePoint == TimePoint::min())
                // 2 optimizations: reduce invoking SteadyClock::now() and timer's restarts
                nowCached = TimePoint::min();
            else
                nowCached = SteadyClock::now();
        }

        return *nowCached;
    };

    if (m_stepType == StepType::None && m_nextTimePoint) {
        if (*m_nextTimePoint <= now()) {
            m_nextTimePoint.reset();
            doNextStep(StepType::Immediate);
            nowCached.reset(); // doNextStep() may take some time, 'now' is not valid anymore
        }
    }

    if (m_nextTimePoint) {
        *m_nextTimePoint = std::max(*m_nextTimePoint, now());
        if (!m_timePoint || *m_nextTimePoint != std::max(*m_timePoint, now())) {
            timer().setInterval(*m_nextTimePoint - now());
            timer().start();
        }
    } else if (m_timePoint) {
        timer().stop();
    }

    m_timePoint = std::exchange(m_nextTimePoint, std::nullopt);
}

void PlaybackEngineObject::doNextStep(StepType type)
{
    Q_ASSERT(m_stepType == StepType::None && type != StepType::None);
    QScopedValueRollback rollback(m_stepType, type);
    doNextStep();
}

bool PlaybackEngineObject::event(QEvent *e)
{
    if (e->type() == FuncEventType) {
        e->accept();
        static_cast<FuncEvent *>(e)->invoke();
        return true;
    }

    return QObject::event(e);
}

} // namespace QFFmpeg

QT_END_NAMESPACE

#include "moc_qffmpegplaybackengineobject_p.cpp"
