// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFFMPEGFRAME_P_H
#define QFFMPEGFRAME_P_H

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

#include "qffmpeg_p.h"
#include "playbackengine/qffmpegcodeccontext_p.h"
#include "playbackengine/qffmpegplaybackutils_p.h"
#include "playbackengine/qffmpegtime_p.h"
#include "QtCore/qsharedpointer.h"
#include "qpointer.h"
#include "qobject.h"

#include <optional>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

struct Frame
{
    struct Data
    {
        Data(const LoopOffset &offset, AVFrameUPtr f, const CodecContext &codecContext, quint64 sourceId)
            : loopOffset(offset),
              codecContext(codecContext),
              frame(std::move(f)),
              sourceId(sourceId)
        {
            Q_ASSERT(frame);
            if (frame->pts != AV_NOPTS_VALUE)
                startTime = codecContext.toTrackTime(frame->pts);
            else
                startTime = codecContext.toTrackTime(frame->best_effort_timestamp);

            if (auto frameDuration = getAVFrameDuration(*frame)) {
                duration = codecContext.toTrackTime(frameDuration);
            } else {
                // Estimate frame duration for audio stream
                if (codecContext.context()->codec_type == AVMEDIA_TYPE_AUDIO) {
                    if (frame->sample_rate)
                        duration =
                                TrackTime(qint64(1000000) * frame->nb_samples / frame->sample_rate);
                    else
                        duration = TrackTime(0); // Fallback
                } else {
                    // Estimate frame duration for video stream
                    const auto &avgFrameRate = codecContext.stream()->avg_frame_rate;
                    duration =
                            TrackTime(mul(qint64(1000000), { avgFrameRate.den, avgFrameRate.num })
                                              .value_or(0));
                }
            }
        }
        Data(const LoopOffset &offset, const QString &text, TrackTime pts, TrackTime duration,
             quint64 sourceId)
            : loopOffset(offset), text(text), startTime(pts), duration(duration), sourceId(sourceId)
        {
        }

        QAtomicInt ref;
        LoopOffset loopOffset;
        std::optional<CodecContext> codecContext;
        AVFrameUPtr frame;
        QString text;
        TrackTime startTime;
        TrackTime duration;
        quint64 sourceId = 0;
    };
    Frame() = default;

    Frame(const LoopOffset &offset, AVFrameUPtr f, const CodecContext &codecContext,
          quint64 sourceIndex)
        : d(new Data(offset, std::move(f), codecContext, sourceIndex))
    {
    }
    Frame(const LoopOffset &offset, const QString &text, TrackTime pts, TrackTime duration,
          quint64 sourceIndex)
        : d(new Data(offset, text, pts, duration, sourceIndex))
    {
    }
    bool isValid() const { return !!d; }

    AVFrame *avFrame() const { return data().frame.get(); }
    AVFrameUPtr takeAVFrame() { return std::move(data().frame); }
    const CodecContext *codecContext() const
    {
        return data().codecContext ? &data().codecContext.value() : nullptr;
    }
    TrackTime startTime() const { return data().startTime; }
    TrackTime duration() const { return data().duration; }
    TrackTime endTime() const { return data().startTime + data().duration; }
    QString text() const { return data().text; }
    quint64 sourceId() const { return data().sourceId; };
    const LoopOffset &loopOffset() const { return data().loopOffset; };
    TrackTime absolutePts() const { return startTime() + loopOffset().loopStartTimeUs; }
    TrackTime absoluteEnd() const { return endTime() + loopOffset().loopStartTimeUs; }

private:
    Data &data() const
    {
        Q_ASSERT(d);
        return *d;
    }

private:
    QExplicitlySharedDataPointer<Data> d;
};

} // namespace QFFmpeg

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QFFmpeg::Frame);

#endif // QFFMPEGFRAME_P_H
