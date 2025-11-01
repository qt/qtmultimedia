// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "playbackengine/qffmpegrenderer_p.h"
#include <qloggingcategory.h>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

Q_STATIC_LOGGING_CATEGORY(qLcRenderer, "qt.multimedia.ffmpeg.renderer");

Renderer::Renderer(const PlaybackEngineObjectID &id, const TimeController &tc)
    : PlaybackEngineObject(id), m_sessionCtx{ tc }
{
}

TrackPosition Renderer::seekPosition() const
{
    return TrackPosition(m_sessionCtx.seekPos);
}

TrackPosition Renderer::lastPosition() const
{
    return TrackPosition(m_sessionCtx.lastPosition);
}

void Renderer::setPlaybackRate(float rate)
{
    invokePriorityMethod([this, rate]() {
        m_sessionCtx.timeController.setPlaybackRate(rate);
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
                m_sessionCtx.explicitNextFrameTime = SteadyClock::now();
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
        m_sessionCtx.timeController = tc;
        scheduleNextStep();
    });
}

void Renderer::seek(quint64 sessionId, const TimeController &tc, const LoopOffset &offset)
{
    updateSession(sessionId, [this, tc, offset]() {
        m_sessionCtx = { tc, offset.loopIndex };
        // don't clean m_isStepForced, otherwise a single frame might not be forced on pause

        seekInternal();
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

    m_sessionCtx.frames.enqueue(std::move(frame));

    if (m_sessionCtx.frames.size() == 1)
        scheduleNextStep();
}

void Renderer::onPauseChanged()
{
    m_sessionCtx.timeController.setPaused(isPaused());
    PlaybackEngineObject::onPauseChanged();
}

bool Renderer::canDoNextStep() const
{
    if (m_sessionCtx.frames.empty())
        return false;
    // do the step even if the TC is not started;
    // may be changed if the case is found.
    if (m_isStepForced)
        return true;
    if (!m_sessionCtx.timeController.isStarted())
        return false;
    return PlaybackEngineObject::canDoNextStep();
}

float Renderer::playbackRate() const
{
    return m_sessionCtx.timeController.playbackRate();
}

Renderer::TimePoint Renderer::nextTimePoint() const
{
    using namespace std::chrono_literals;

    if (m_sessionCtx.frames.empty())
        return PlaybackEngineObject::nextTimePoint();

    if (m_sessionCtx.explicitNextFrameTime)
        return *m_sessionCtx.explicitNextFrameTime;

    if (m_sessionCtx.frames.front().isValid())
        return m_sessionCtx.timeController.timeFromPosition(
                m_sessionCtx.frames.front().absolutePts());

    if (m_sessionCtx.lastFrameEnd > TrackPosition(0))
        return m_sessionCtx.timeController.timeFromPosition(m_sessionCtx.lastFrameEnd);

    return PlaybackEngineObject::nextTimePoint();
}

bool Renderer::setForceStepDone()
{
    if (!m_isStepForced.testAndSetOrdered(true, false))
        return false;

    m_sessionCtx.explicitNextFrameTime.reset();
    emit forceStepDone();
    return true;
}

void Renderer::doNextStep()
{
    Frame frame = m_sessionCtx.frames.front();

    if (setForceStepDone()) {
        // if (frame.isValid() && frame.pts() > m_forceStepMaxPos) {
        //    scheduleNextStep();
        //    return;
        // }
    }

    const auto result = renderInternal(frame);
    const bool frameIsValid = frame.isValid();

    if (result.done) {
        m_sessionCtx.explicitNextFrameTime.reset();
        m_sessionCtx.frames.dequeue();

        if (frameIsValid) {
            m_sessionCtx.lastPosition.storeRelease(
                    std::max(frame.absolutePts(), lastPosition()).get());

            // TODO: get rid of m_lastFrameEnd or m_seekPos
            m_sessionCtx.lastFrameEnd = frame.absoluteEnd();
            m_sessionCtx.seekPos.storeRelaxed(m_sessionCtx.lastFrameEnd.get());

            const auto loopIndex = frame.loopOffset().loopIndex;
            if (m_sessionCtx.loopIndex < loopIndex) {
                m_sessionCtx.loopIndex = loopIndex;
                emit loopChanged(id(), frame.loopOffset().loopStartTimeUs, m_sessionCtx.loopIndex);
            }

            emit frameProcessed(std::move(frame));
        } else {
            m_sessionCtx.lastPosition.storeRelease(
                    std::max(m_sessionCtx.lastFrameEnd, lastPosition()).get());
        }
    } else {
        m_sessionCtx.explicitNextFrameTime = SteadyClock::now() + result.recheckInterval;
    }

    setAtEnd(result.done && !frameIsValid);

    scheduleNextStep();
}

std::chrono::microseconds Renderer::frameDelay(const Frame &frame, TimePoint timePoint) const
{
    return std::chrono::duration_cast<std::chrono::microseconds>(
            timePoint - m_sessionCtx.timeController.timeFromPosition(frame.absolutePts()));
}

void Renderer::changeRendererTime(std::chrono::microseconds offset)
{
    const auto now = SteadyClock::now();
    const auto pos = m_sessionCtx.timeController.positionFromTime(now);
    m_sessionCtx.timeController.sync(now + offset, pos);
    emit synchronized(id(), now + offset, pos);
}

} // namespace QFFmpeg

QT_END_NAMESPACE

#include "moc_qffmpegrenderer_p.cpp"
