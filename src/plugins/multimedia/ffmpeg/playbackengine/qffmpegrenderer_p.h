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
    Renderer(const TimeController &tc, const std::chrono::microseconds &seekPosTimeOffset = {});

    void syncSoft(TimePoint tp, qint64 trackPos);

    qint64 seekPosition() const;

    qint64 lastPosition() const;

    void setPlaybackRate(float rate);

    void doForceStep();

    bool isStepForced() const;

public slots:
    void onFinalFrameReceived();

    void render(Frame);

signals:
    void frameProcessed(Frame);

    void synchronized(TimePoint tp, qint64 pos);

    void forceStepDone();

protected:
    bool setForceStepDone();

    void onPauseChanged() override;

    bool canDoNextStep() const override;

    virtual void onPlaybackRateChanged() { }

    struct RenderingResult
    {
        std::chrono::microseconds timeLeft = {};
    };

    virtual RenderingResult renderInternal(Frame frame) = 0;

    float playbackRate() const;

private:
    void doNextStep() override;

    int timerInterval() const override;

private:
    TimeController m_timeController;
    std::atomic<qint64> m_lastPosition = 0;
    std::atomic<qint64> m_seekPos = 0;
    QQueue<Frame> m_frames;

    std::atomic_bool m_isStepForced = false;
};

} // namespace QFFmpeg

QT_END_NAMESPACE

#endif // QFFMPEGRENDERER_P_H
