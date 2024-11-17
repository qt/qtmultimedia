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

#include "qffmpeg_p.h"
#include "qffmpeghwaccel_p.h"

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
    qint64 toMs(qint64 ts) const { return timeStampMs(ts, d->stream->time_base).value_or(0); }
    qint64 toUs(qint64 ts) const { return timeStampUs(ts, d->stream->time_base).value_or(0); }

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
