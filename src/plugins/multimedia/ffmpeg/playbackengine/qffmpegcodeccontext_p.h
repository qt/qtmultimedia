// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFFMPEGCODECCONTEXT_P_H
#define QFFMPEGCODECCONTEXT_P_H

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

#include <QtFFmpegMediaPluginImpl/private/qffmpeg_p.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpeghwaccel_p.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpegtime_p.h>

#include <QtCore/qshareddata.h>
#include <QtCore/private/qexpected_p.h>

QT_BEGIN_NAMESPACE

class QPlaybackOptions;

namespace QFFmpeg {

struct PlaybackCodecContextDeleter
{
    using Finalizer = void (*)(AVCodecContext *);

    PlaybackCodecContextDeleter();

    void operator()(AVCodecContext *context) const;
    void attach(AVCodecContext *context) const;
    static void setFinalizer(AVCodecContext *context, Finalizer finalizer);

private:
    struct State
    {
        AVCodecContext *context = nullptr;
        Finalizer finalizer = nullptr;
    };

    std::unique_ptr<State> m_state;
};

using PlaybackCodecContextUPtr = std::unique_ptr<AVCodecContext, PlaybackCodecContextDeleter>;

PlaybackCodecContextUPtr createPlaybackCodecContext(const AVCodec *codec);

class CodecContext
{
    struct Data : QSharedData
    {
        Data(PlaybackCodecContextUPtr context, AVStream *avStream, AVFormatContext *formatContext,
             std::unique_ptr<HWAccel> hwAccel);
        PlaybackCodecContextUPtr context;
        AVStream *stream = nullptr;
        AVFormatContext *formatContext = nullptr;
        AVRational pixelAspectRatio = { 0, 1 };
        std::unique_ptr<QFFmpeg::HWAccel> hwAccel;
    };

public:
    static q23::expected<CodecContext, QString> create(AVStream *stream, AVFormatContext *formatContext,
                                       const QPlaybackOptions &options);

    AVRational pixelAspectRatio(AVFrame *frame) const;

    AVCodecContext *context() const { return d->context.get(); }
    AVStream *stream() const { return d->stream; }
    uint streamIndex() const { return d->stream->index; }
    HWAccel *hwAccel() const { return d->hwAccel.get(); }
    TrackDuration toTrackDuration(AVStreamDuration duration) const
    {
        return QFFmpeg::toTrackDuration(duration, d->stream);
    }

    TrackPosition toTrackPosition(AVStreamPosition streamPosition) const
    {
        return QFFmpeg::toTrackPosition(streamPosition, d->stream, d->formatContext);
    }

private:
    enum VideoCodecCreationPolicy {
        Hw,
        Sw,
    };

    static q23::expected<CodecContext, QString> create(AVStream *stream, AVFormatContext *formatContext,
                                       const QPlaybackOptions &playbackOptions,
                                       VideoCodecCreationPolicy videoCodecPolicy);
    CodecContext(Data *data) : d(data) { }
    QExplicitlySharedDataPointer<Data> d;
};

} // namespace QFFmpeg

QT_END_NAMESPACE

#endif // QFFMPEGCODECCONTEXT_P_H
