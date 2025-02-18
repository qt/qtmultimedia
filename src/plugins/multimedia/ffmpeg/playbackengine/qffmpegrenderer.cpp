// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "playbackengine/qffmpegrenderer_p.h"
#include <qloggingcategory.h>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

Q_STATIC_LOGGING_CATEGORY(qLcRenderer, "qt.multimedia.ffmpeg.renderer");

Renderer::Renderer(const TimeController &tc)
    : m_timeController(tc),
      m_lastFrameEnd(tc.currentPosition()),
      m_lastPosition(m_lastFrameEnd.get()),
      m_seekPos(tc.currentPosition().get())
{
}

void Renderer::syncSoft(TimePoint tp, TrackPosition trackPos)
{
    QMetaObject::invokeMethod(this, [this, tp, trackPos]() {
        m_timeController.syncSoft(tp, trackPos);
        scheduleNextStep(true);
    });
}

TrackPosition Renderer::seekPosition() const
{
    return TrackPosition(m_seekPos);
}

TrackPosition Renderer::lastPosition() const
{
    return TrackPosition(m_lastPosition);
}

void Renderer::setPlaybackRate(float rate)
{
    QMetaObject::invokeMethod(this, [this, rate]() {
        m_timeController.setPlaybackRate(rate);
        onPlaybackRateChanged();
        scheduleNextStep();
    });
}

void Renderer::doForceStep()
{
    if (m_isStepForced.testAndSetOrdered(false, true))
        QMetaObject::invokeMethod(this, [this]() {
            // maybe set m_forceStepMaxPos

            if (isAtEnd()) {
                setForceStepDone();
            }
            else {
                m_explicitNextFrameTime = SteadyClock::now();
                scheduleNextStep();
            }
        });
}

bool Renderer::isStepForced() const
{
    return m_isStepForced;
}

void Renderer::start(const TimeController &tc)
{
    QMetaObject::invokeMethod(this, [this, tc]() {
        m_timeController = tc;
        m_started = true;
        scheduleNextStep();
    });
}

void Renderer::onFinalFrameReceived()
{
    render({});
}

void Renderer::render(Frame frame)
{
    const auto isFrameOutdated = frame.isValid() && frame.absoluteEnd() < seekPosition();

    if (isFrameOutdated) {
        qCDebug(qLcRenderer) << "frame outdated! absEnd:" << frame.absoluteEnd().get() << "absPts"
                             << frame.absolutePts().get() << "seekPos:" << seekPosition().get();
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
    if (m_frames.empty())
        return false;
    if (m_isStepForced)
        return true;
    if (!m_started)
        return false;
    return PlaybackEngineObject::canDoNextStep();
}

float Renderer::playbackRate() const
{
    return m_timeController.playbackRate();
}

std::chrono::milliseconds Renderer::timerInterval() const
{
    using namespace std::chrono_literals;

    if (m_frames.empty())
        return 0ms;

    auto calculateInterval = [](const TimePoint &nextTime) {
        using namespace std::chrono;

        const milliseconds delay = duration_cast<milliseconds>(nextTime - SteadyClock::now());
        return std::max(0ms, std::chrono::duration_cast<milliseconds>(delay));
    };

    if (m_explicitNextFrameTime)
        return calculateInterval(*m_explicitNextFrameTime);

    if (m_frames.front().isValid())
        return calculateInterval(m_timeController.timeFromPosition(m_frames.front().absolutePts()));

    if (m_lastFrameEnd > TrackPosition(0))
        return calculateInterval(m_timeController.timeFromPosition(m_lastFrameEnd));

    return 0ms;
}

bool Renderer::setForceStepDone()
{
    if (!m_isStepForced.testAndSetOrdered(true, false))
        return false;

    m_explicitNextFrameTime.reset();
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

    if (result.done) {
        m_explicitNextFrameTime.reset();
        m_frames.dequeue();

        if (frame.isValid()) {
            m_lastPosition.storeRelease(std::max(frame.absolutePts(), lastPosition()).get());

            // TODO: get rid of m_lastFrameEnd or m_seekPos
            m_lastFrameEnd = frame.absoluteEnd();
            m_seekPos.storeRelaxed(m_lastFrameEnd.get());

            const auto loopIndex = frame.loopOffset().loopIndex;
            if (m_loopIndex < loopIndex) {
                m_loopIndex = loopIndex;
                emit loopChanged(id(), frame.loopOffset().loopStartTimeUs, m_loopIndex);
            }

            emit frameProcessed(frame);
        } else {
            m_lastPosition.storeRelease(std::max(m_lastFrameEnd, lastPosition()).get());
        }
    } else {
        m_explicitNextFrameTime = SteadyClock::now() + result.recheckInterval;
    }

    setAtEnd(result.done && !frame.isValid());

    scheduleNextStep(false);
}

std::chrono::microseconds Renderer::frameDelay(const Frame &frame, TimePoint timePoint) const
{
    return std::chrono::duration_cast<std::chrono::microseconds>(
            timePoint - m_timeController.timeFromPosition(frame.absolutePts()));
}

void Renderer::changeRendererTime(std::chrono::microseconds offset)
{
    const auto now = SteadyClock::now();
    const auto pos = m_timeController.positionFromTime(now);
    m_timeController.sync(now + offset, pos);
    emit synchronized(id(), now + offset, pos);
}

} // namespace QFFmpeg

QT_END_NAMESPACE

#include "moc_qffmpegrenderer_p.cpp"
