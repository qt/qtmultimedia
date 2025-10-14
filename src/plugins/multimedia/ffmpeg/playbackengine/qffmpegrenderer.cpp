// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "playbackengine/qffmpegrenderer_p.h"
#include <qloggingcategory.h>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

Q_STATIC_LOGGING_CATEGORY(qLcRenderer, "qt.multimedia.ffmpeg.renderer");

Renderer::Renderer(const PlaybackEngineObjectID &id, const TimeController &tc)
    : PlaybackEngineObject(id),
      m_timeController(tc),
      m_lastFrameEnd(tc.currentPosition()),
      m_lastPosition(m_lastFrameEnd.get()),
      m_seekPos(tc.currentPosition().get())
{
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
    invokePriorityMethod([this, rate]() {
        m_timeController.setPlaybackRate(rate);
        onPlaybackRateChanged();
        scheduleNextStep();
    });
}

void Renderer::doForceStep()
{
    if (m_isStepForced.testAndSetOrdered(false, true))
        invokePriorityMethod([this]() {
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

void Renderer::setTimeController(const TimeController &tc)
{
    Q_ASSERT(tc.isStarted());
    invokePriorityMethod([this, tc]() {
        m_timeController = tc;
        scheduleNextStep();
    });
}

void Renderer::onFinalFrameReceived(PlaybackEngineObjectID sourceID)
{
    if (checkSessionID(sourceID.sessionID))
        render({});
}

void Renderer::render(Frame frame)
{
    if (frame.isValid() && !checkSessionID(frame.sourceID().sessionID)) {
        qCDebug(qLcRenderer) << "Frame session outdated. Source id:" << frame.sourceID() << "current id:" << id();
        // else don't need to report
        return;
    }

    const bool frameOutdated = frame.isValid() && frame.absoluteEnd() < seekPosition();

    if (frameOutdated) {
        qCDebug(qLcRenderer) << "frame outdated! absEnd:" << frame.absoluteEnd().get() << "absPts"
                             << frame.absolutePts().get() << "seekPos:" << seekPosition().get();

        emit frameProcessed(std::move(frame));
        return;
    }

    m_frames.enqueue(std::move(frame));

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
    // do the step even if the TC is not started;
    // may be changed if the case is found.
    if (m_isStepForced)
        return true;
    if (!m_timeController.isStarted())
        return false;
    return PlaybackEngineObject::canDoNextStep();
}

float Renderer::playbackRate() const
{
    return m_timeController.playbackRate();
}

Renderer::TimePoint Renderer::nextTimePoint() const
{
    using namespace std::chrono_literals;

    if (m_frames.empty())
        return PlaybackEngineObject::nextTimePoint();

    if (m_explicitNextFrameTime)
        return *m_explicitNextFrameTime;

    if (m_frames.front().isValid())
        return m_timeController.timeFromPosition(m_frames.front().absolutePts());

    if (m_lastFrameEnd > TrackPosition(0))
        return m_timeController.timeFromPosition(m_lastFrameEnd);

    return PlaybackEngineObject::nextTimePoint();
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
    Frame frame = m_frames.front();

    if (setForceStepDone()) {
        // if (frame.isValid() && frame.pts() > m_forceStepMaxPos) {
        //    scheduleNextStep();
        //    return;
        // }
    }

    const auto result = renderInternal(frame);
    const bool frameIsValid = frame.isValid();

    if (result.done) {
        m_explicitNextFrameTime.reset();
        m_frames.dequeue();

        if (frameIsValid) {
            m_lastPosition.storeRelease(std::max(frame.absolutePts(), lastPosition()).get());

            // TODO: get rid of m_lastFrameEnd or m_seekPos
            m_lastFrameEnd = frame.absoluteEnd();
            m_seekPos.storeRelaxed(m_lastFrameEnd.get());

            const auto loopIndex = frame.loopOffset().loopIndex;
            if (m_loopIndex < loopIndex) {
                m_loopIndex = loopIndex;
                emit loopChanged(id(), frame.loopOffset().loopStartTimeUs, m_loopIndex);
            }

            emit frameProcessed(std::move(frame));
        } else {
            m_lastPosition.storeRelease(std::max(m_lastFrameEnd, lastPosition()).get());
        }
    } else {
        m_explicitNextFrameTime = SteadyClock::now() + result.recheckInterval;
    }

    setAtEnd(result.done && !frameIsValid);

    scheduleNextStep();
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
