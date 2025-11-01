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

#include <QtFFmpegMediaPluginImpl/private/qffmpegplaybackengineobject_p.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpegtimecontroller_p.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpegframe_p.h>

#include <QtCore/qpointer.h>
#include <QtCore/qqueue.h>

#include <chrono>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

class Renderer : public PlaybackEngineObject
{
    Q_OBJECT
public:
    using TimePoint = SteadyClock::time_point;

    Renderer(const PlaybackEngineObjectID &id, const TimeController &tc);

    TrackPosition seekPosition() const;

    TrackPosition lastPosition() const;

    void setPlaybackRate(float rate);

    void doForceStep();

    bool isStepForced() const;

    void setTimeController(const TimeController &tc);

    void seek(quint64 sessionId, const TimeController &tc, const LoopOffset &offset);

public slots:

    void onFinalFrameReceived(PlaybackEngineObjectID sourceID);

    void render(Frame);

signals:
    void frameProcessed(Frame);

    void synchronized(PlaybackEngineObjectID id, TimePoint tp, TrackPosition pos);

    void forceStepDone();

    void loopChanged(PlaybackEngineObjectID id, TrackPosition offset, int index);

protected:
    bool setForceStepDone();

    void onPauseChanged() override;

    bool canDoNextStep() const override;

    TimePoint nextTimePoint() const override;

    virtual void seekInternal() { }

    virtual void onPlaybackRateChanged() { }

    struct RenderingResult
    {
        bool done = true;
        std::chrono::microseconds recheckInterval = std::chrono::microseconds(0);
    };

    virtual RenderingResult renderInternal(Frame frame) = 0;

    float playbackRate() const;

    std::chrono::microseconds frameDelay(const Frame &frame,
                                         TimePoint timePoint = SteadyClock::now()) const;

    void changeRendererTime(std::chrono::microseconds offset);

    template<typename Output, typename ChangeHandler>
    void setOutputInternal(QPointer<Output> &actual, Output *desired, ChangeHandler &&changeHandler)
    {
        const auto connectionType =
                thread()->isCurrentThread() ? Qt::AutoConnection : Qt::BlockingQueuedConnection;
        auto doer = [desired, changeHandler, &actual]() {
            const auto prev = std::exchange(actual, desired);
            if (prev != desired)
                changeHandler(prev);
        };
        QMetaObject::invokeMethod(this, doer, connectionType);
    }

private:
    void doNextStep() override;

private:
    struct SessionContext
    {
        TimeController timeController;
        int loopIndex = 0;
        TrackPosition lastFrameEnd = timeController.currentPosition();
        QAtomicInteger<qint64> lastPosition = lastFrameEnd.get();
        QAtomicInteger<qint64> seekPos = lastFrameEnd.get();
        QQueue<Frame> frames = {};
        std::optional<TimePoint> explicitNextFrameTime = {};
    };

    SessionContext m_sessionCtx;
    QAtomicInteger<bool> m_isStepForced = false;
};

} // namespace QFFmpeg

QT_END_NAMESPACE

#endif // QFFMPEGRENDERER_P_H
