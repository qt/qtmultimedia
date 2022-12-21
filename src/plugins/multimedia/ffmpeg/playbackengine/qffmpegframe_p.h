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
#include "playbackengine/qffmpegcodec_p.h"
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
        Data(AVFrameUPtr f, const Codec &codec, qint64, const QObject *source)
            : codec(codec), frame(std::move(f)), source(source)
        {
            Q_ASSERT(frame);
            if (frame->pts != AV_NOPTS_VALUE)
                pts = codec.toUs(frame->pts);
            else
                pts = codec.toUs(frame->best_effort_timestamp);
            const auto &avgFrameRate = codec.stream()->avg_frame_rate;
            duration = avgFrameRate.num
                    ? (1000000 * avgFrameRate.den + avgFrameRate.num / 2) / avgFrameRate.num
                    : 0;
        }
        Data(const QString &text, qint64 pts, qint64 duration, const QObject *source)
            : text(text), pts(pts), duration(duration), source(source)
        {
        }

        QAtomicInt ref;
        std::optional<Codec> codec;
        AVFrameUPtr frame;
        QString text;
        qint64 pts = -1;
        qint64 duration = -1;
        QPointer<const QObject> source;
    };
    Frame() = default;

    Frame(AVFrameUPtr f, const Codec &codec, qint64 pts, const QObject *source = nullptr)
        : d(new Data(std::move(f), codec, pts, source))
    {
    }
    Frame(const QString &text, qint64 pts, qint64 duration, const QObject *source = nullptr)
        : d(new Data(text, pts, duration, source))
    {
    }
    bool isValid() const { return !!d; }

    AVFrame *avFrame() const { return data().frame.get(); }
    AVFrameUPtr takeAVFrame() { return std::move(data().frame); }
    const Codec *codec() const { return data().codec ? &data().codec.value() : nullptr; }
    qint64 pts() const { return data().pts; }
    qint64 duration() const { return data().duration; }
    qint64 end() const { return data().pts + data().duration; }
    QString text() const { return data().text; }
    const QObject *source() const { return data().source; };

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
