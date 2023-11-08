// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QFFMPEGRENDERER_P_H
#define QFFMPEGRENDERER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "playbackengine/qffmpegplaybackengineobject_p.h"
#include "playbackengine/qffmpegtimecontroller_p.h"
#include "playbackengine/qffmpegframe_p.h"

#include <chrono>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

class Renderer : public PlaybackEngineObject
{
    Q_OBJECT
public:
    using TimePoint = TimeController::TimePoint;
    using Clock = TimeController::Clock;
    Renderer(const TimeController &tc, const std::chrono::microseconds &seekPosTimeOffset = {});

    void syncSoft(TimePoint tp, qint64 trackPos);

    qint64 seekPosition() const;

    qint64 lastPosition() const;

    void setPlaybackRate(float rate);

    void doForceStep();

    bool isStepForced() const;

public slots:
    void setInitialPosition(TimePoint tp, qint64 trackPos);

    void onFinalFrameReceived();

    void render(Frame);

signals:
    void frameProcessed(Frame);

    void synchronized(Id id, TimePoint tp, qint64 pos);

    void forceStepDone();

    void loopChanged(Id id, qint64 offset, int index);

protected:
    bool setForceStepDone();

    void onPauseChanged() override;

    bool canDoNextStep() const override;

    virtual void onPlaybackRateChanged() { }

    struct RenderingResult
    {
        bool done = true;
        std::chrono::microseconds recheckInterval = std::chrono::microseconds(0);
    };

    virtual RenderingResult renderInternal(Frame frame) = 0;

    float playbackRate() const;

    std::chrono::microseconds frameDelay(const Frame &frame) const;

    void changeRendererTime(std::chrono::microseconds offset);

    template<typename Output, typename ChangeHandler>
    void setOutputInternal(QPointer<Output> &actual, Output *desired, ChangeHandler &&changeHandler)
    {
        const auto connectionType = thread() == QThread::currentThread()
                ? Qt::AutoConnection
                : Qt::BlockingQueuedConnection;
        auto doer = [desired, changeHandler, &actual]() {
            const auto prev = std::exchange(actual, desired);
            if (prev != desired)
                changeHandler(prev);
        };
        QMetaObject::invokeMethod(this, doer, connectionType);
    }

private:
    void doNextStep() override;

    int timerInterval() const override;

private:
    TimeController m_timeController;
    qint64 m_lastFrameEnd = 0;
    QAtomicInteger<qint64> m_lastPosition = 0;
    QAtomicInteger<qint64> m_seekPos = 0;

    int m_loopIndex = 0;
    QQueue<Frame> m_frames;

    QAtomicInteger<bool> m_isStepForced = false;
    std::optional<TimePoint> m_explicitNextFrameTime;
};

} // namespace QFFmpeg

QT_END_NAMESPACE

#endif // QFFMPEGRENDERER_P_H
