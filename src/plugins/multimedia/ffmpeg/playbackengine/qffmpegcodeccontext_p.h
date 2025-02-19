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

#include <QtMultimedia/private/qmaybe_p.h>
#include <QtCore/qshareddata.h>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

class CodecContext
{
    struct Data : QSharedData
    {
        Data(AVCodecContextUPtr context, AVStream *avStream, AVFormatContext *formatContext,
             std::unique_ptr<QFFmpeg::HWAccel> hwAccel);
        AVCodecContextUPtr context;
        AVStream *stream = nullptr;
        AVFormatContext *formatContext = nullptr;
        AVRational pixelAspectRatio = { 0, 1 };
        std::unique_ptr<QFFmpeg::HWAccel> hwAccel;
    };

public:
    static QMaybe<CodecContext> create(AVStream *stream, AVFormatContext *formatContext);

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

    static QMaybe<CodecContext> create(AVStream *stream, AVFormatContext *formatContext,
                                       VideoCodecCreationPolicy videoCodecPolicy);
    CodecContext(Data *data) : d(data) { }
    QExplicitlySharedDataPointer<Data> d;
};

} // namespace QFFmpeg

QT_END_NAMESPACE

#endif // QFFMPEGCODECCONTEXT_P_H
