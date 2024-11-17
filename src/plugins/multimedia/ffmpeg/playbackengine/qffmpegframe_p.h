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
#include "playbackengine/qffmpegpositionwithoffset_p.h"
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
        Data(const LoopOffset &offset, AVFrameUPtr f, const CodecContext &codecContext, qint64,
             quint64 sourceId)
            : loopOffset(offset),
              codecContext(codecContext),
              frame(std::move(f)),
              sourceId(sourceId)
        {
            Q_ASSERT(frame);
            if (frame->pts != AV_NOPTS_VALUE)
                pts = codecContext.toUs(frame->pts);
            else
                pts = codecContext.toUs(frame->best_effort_timestamp);

            if (frame->sample_rate && codecContext.context()->codec_type == AVMEDIA_TYPE_AUDIO)
                duration = qint64(1000000) * frame->nb_samples / frame->sample_rate;

            if (auto frameDuration = getAVFrameDuration(*frame)) {
                duration = codecContext.toUs(frameDuration);
            } else {
                const auto &avgFrameRate = codecContext.stream()->avg_frame_rate;
                duration = mul(qint64(1000000), { avgFrameRate.den, avgFrameRate.num }).value_or(0);
            }
        }
        Data(const LoopOffset &offset, const QString &text, qint64 pts, qint64 duration,
             quint64 sourceId)
            : loopOffset(offset), text(text), pts(pts), duration(duration), sourceId(sourceId)
        {
        }

        QAtomicInt ref;
        LoopOffset loopOffset;
        std::optional<CodecContext> codecContext;
        AVFrameUPtr frame;
        QString text;
        qint64 pts = -1;
        qint64 duration = -1;
        quint64 sourceId = 0;
    };
    Frame() = default;

    Frame(const LoopOffset &offset, AVFrameUPtr f, const CodecContext &codecContext, qint64 pts,
          quint64 sourceIndex)
        : d(new Data(offset, std::move(f), codecContext, pts, sourceIndex))
    {
    }
    Frame(const LoopOffset &offset, const QString &text, qint64 pts, qint64 duration,
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
    qint64 pts() const { return data().pts; }
    qint64 duration() const { return data().duration; }
    qint64 end() const { return data().pts + data().duration; }
    QString text() const { return data().text; }
    quint64 sourceId() const { return data().sourceId; };
    const LoopOffset &loopOffset() const { return data().loopOffset; };
    qint64 absolutePts() const { return pts() + loopOffset().pos; }
    qint64 absoluteEnd() const { return end() + loopOffset().pos; }

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
