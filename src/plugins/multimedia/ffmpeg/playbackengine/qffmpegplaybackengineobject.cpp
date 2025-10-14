// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "playbackengine/qffmpegplaybackengineobject_p.h"

#include "QtCore/qchronotimer.h"
#include "QtCore/qdebug.h"

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
        QMetaObject::invokeMethod(this, &PlaybackEngineObject::onPauseChanged);
}

void PlaybackEngineObject::kill()
{
    m_deleting.storeRelease(true);

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
    if (!m_deleting && canDoNextStep())
        doNextStep();
}

std::chrono::milliseconds PlaybackEngineObject::timerInterval() const
{
    using namespace std::chrono_literals;
    return 0ms;
}

void PlaybackEngineObject::onPauseChanged()
{
    scheduleNextStep();
}

void PlaybackEngineObject::scheduleNextStep(bool allowDoImmediatelly)
{
    using std::chrono::milliseconds;
    using namespace std::chrono_literals;

    if (!m_deleting && canDoNextStep()) {
        const milliseconds interval = timerInterval();
        if (interval == 0ms && allowDoImmediatelly) {
            timer().stop();
            doNextStep();
        } else {
            timer().setInterval(interval);
            timer().start();
        }
    } else {
        timer().stop();
    }
}

} // namespace QFFmpeg

QT_END_NAMESPACE

#include "moc_qffmpegplaybackengineobject_p.cpp"
