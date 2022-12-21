// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "playbackengine/qffmpegplaybackengineobject_p.h"

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

bool PlaybackEngineObject::isPaused() const
{
    return m_paused;
}

void PlaybackEngineObject::setAtEnd(bool isAtEnd)
{
    if (m_atEnd.exchange(isAtEnd) != isAtEnd)
        emit atEnd();
}

bool PlaybackEngineObject::isAtEnd() const
{
    return m_atEnd;
}

void PlaybackEngineObject::setPaused(bool isPaused)
{
    if (m_paused.exchange(isPaused) != isPaused)
        QMetaObject::invokeMethod(this, &PlaybackEngineObject::onPauseChanged);
}

void PlaybackEngineObject::kill()
{
    m_deleting = true;
    setPaused(true);

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
        m_timer = new QTimer(this);
        m_timer->setTimerType(Qt::PreciseTimer);
        m_timer->setSingleShot(true);
        connect(m_timer, &QTimer::timeout, this, [this]() {
            if (!m_deleting && canDoNextStep())
                doNextStep();
        });
    }

    return *m_timer;
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
