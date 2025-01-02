// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "playbackengine/qffmpegplaybackengineobject_p.h"

#include "qtimer.h"
#include "qdebug.h"

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

static QAtomicInteger<PlaybackEngineObject::Id> PersistentId = 0;

PlaybackEngineObject::PlaybackEngineObject() : m_id(PersistentId.fetchAndAddRelaxed(1)) { }

PlaybackEngineObject::~PlaybackEngineObject()
{
    if (thread() != QThread::currentThread())
        qWarning() << "The playback engine object is being removed in an unexpected thread";
}

bool PlaybackEngineObject::isPaused() const
{
    return m_paused;
}

void PlaybackEngineObject::setAtEnd(bool isAtEnd)
{
    if (m_atEnd.testAndSetRelease(!isAtEnd, isAtEnd) && isAtEnd)
        emit atEnd();
}

bool PlaybackEngineObject::isAtEnd() const
{
    return m_atEnd;
}

PlaybackEngineObject::Id PlaybackEngineObject::id() const
{
    return m_id;
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

QTimer &PlaybackEngineObject::timer()
{
    if (!m_timer) {
        m_timer = std::make_unique<QTimer>();
        m_timer->setTimerType(Qt::PreciseTimer);
        m_timer->setSingleShot(true);
        connect(m_timer.get(), &QTimer::timeout, this, &PlaybackEngineObject::onTimeout);
    }

    return *m_timer;
}

void PlaybackEngineObject::onTimeout()
{
    if (!m_deleting && canDoNextStep())
        doNextStep();
}

int PlaybackEngineObject::timerInterval() const
{
    return 0;
}

void PlaybackEngineObject::onPauseChanged()
{
    scheduleNextStep();
}

void PlaybackEngineObject::scheduleNextStep(bool allowDoImmediatelly)
{
    if (!m_deleting && canDoNextStep()) {
        const auto interval = timerInterval();
        if (interval == 0 && allowDoImmediatelly) {
            timer().stop();
            doNextStep();
        } else {
            timer().start(interval);
        }
    } else {
        timer().stop();
    }
}
} // namespace QFFmpeg

QT_END_NAMESPACE

#include "moc_qffmpegplaybackengineobject_p.cpp"
