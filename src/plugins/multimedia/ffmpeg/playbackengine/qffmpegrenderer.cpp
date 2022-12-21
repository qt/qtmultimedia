// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "playbackengine/qffmpegrenderer_p.h"

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

Renderer::Renderer(const TimeController &tc, const std::chrono::microseconds &seekPosTimeOffset)
    : m_timeController(tc),
      m_lastPosition(tc.currentPosition()),
      m_seekPos(tc.currentPosition(-seekPosTimeOffset))
{
}

void Renderer::syncSoft(TimePoint tp, qint64 trackTime)
{
    QMetaObject::invokeMethod(this, [=]() {
        m_timeController.syncSoft(tp, trackTime);
        scheduleNextStep(true);
    });
}

qint64 Renderer::seekPosition() const
{
    return m_seekPos;
}

qint64 Renderer::lastPosition() const
{
    return m_lastPosition;
}

void Renderer::setPlaybackRate(float rate)
{
    QMetaObject::invokeMethod(this, [=]() {
        m_timeController.setPlaybackRate(rate);
        onPlaybackRateChanged();
        scheduleNextStep();
    });
}

void Renderer::doForceStep()
{
    if (!m_isStepForced.exchange(true))
        QMetaObject::invokeMethod(this, [=]() {
            // maybe set m_forceStepMaxPos

            if (isAtEnd())
                setForceStepDone();
            else
                scheduleNextStep();
        });
}

bool Renderer::isStepForced() const
{
    return m_isStepForced;
}

void Renderer::onFinalFrameReceived()
{
    render({});
}

void Renderer::render(Frame frame)
{
    using namespace std::chrono;

    const auto isFrameOutdated = frame.isValid() && frame.end() < m_seekPos;

    if (isFrameOutdated) {
        emit frameProcessed(frame);
        return;
    }

    m_frames.enqueue(frame);

    if (m_frames.size() == 1)
        scheduleNextStep();
}

void Renderer::onPauseChanged()
{
    m_timeController.setPaused(isPaused());
    PlaybackEngineObject::onPauseChanged();
}

bool Renderer::canDoNextStep() const
{
    return !m_frames.empty() && (m_isStepForced || PlaybackEngineObject::canDoNextStep());
}

float Renderer::playbackRate() const
{
    return m_timeController.playbackRate();
}

int Renderer::timerInterval() const
{
    if (auto frame = m_frames.front(); frame.isValid() && !m_isStepForced) {
        using namespace std::chrono;
        const auto delay = m_timeController.timeFromPosition(frame.pts()) - steady_clock::now();
        return std::max(0, static_cast<int>(duration_cast<milliseconds>(delay).count()));
    }

    return 0;
}

bool Renderer::setForceStepDone()
{
    if (!m_isStepForced.exchange(false))
        return false;

    emit forceStepDone();
    return true;
}

void Renderer::doNextStep()
{
    auto frame = m_frames.front();

    if (setForceStepDone()) {
        // if (frame.isValid() && frame.pts() > m_forceStepMaxPos) {
        //    scheduleNextStep(false);
        //    return;
        // }
    }

    const auto result = renderInternal(frame);
    const bool done = result.timeLeft.count() <= 0;

    if (result.timeLeft.count() && frame.isValid()) {
        const auto now = std::chrono::steady_clock::now();
        m_timeController.sync(now + result.timeLeft, frame.pts());
        emit synchronized(now + result.timeLeft, frame.pts());
    }

    if (done) {
        m_frames.dequeue();

        if (frame.isValid()) {
            m_lastPosition = std::max(frame.pts(), m_lastPosition.load());
            m_seekPos = frame.end();
            emit frameProcessed(frame);
        }
    }

    setAtEnd(done && !frame.isValid());

    scheduleNextStep(false);
}
} // namespace QFFmpeg

QT_END_NAMESPACE

#include "moc_qffmpegrenderer_p.cpp"
