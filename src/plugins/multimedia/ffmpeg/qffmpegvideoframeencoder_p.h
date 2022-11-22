// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QFFMPEGVIDEOFRAMEENCODER_P_H
#define QFFMPEGVIDEOFRAMEENCODER_P_H

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

#include "qffmpeghwaccel_p.h"
#include "qvideoframeformat.h"
#include "private/qplatformmediarecorder_p.h"

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

class VideoFrameEncoder
{
    class Data final
    {
    public:
        ~Data();
        QAtomicInt ref = 0;
        QMediaEncoderSettings settings;
        float frameRate = 0.;
        QSize sourceSize;

        std::unique_ptr<HWAccel> accel;
        const AVCodec *codec = nullptr;
        AVStream *stream = nullptr;
        AVCodecContext *codecContext = nullptr;
        SwsContext *converter = nullptr;
        AVPixelFormat sourceFormat = AV_PIX_FMT_NONE;
        AVPixelFormat sourceSWFormat = AV_PIX_FMT_NONE;
        AVPixelFormat targetFormat = AV_PIX_FMT_NONE;
        AVPixelFormat targetSWFormat = AV_PIX_FMT_NONE;
        bool sourceFormatIsHWFormat = false;
        bool targetFormatIsHWFormat = false;
        bool downloadFromHW = false;
        bool uploadToHW = false;
    };

    QExplicitlySharedDataPointer<Data> d;
public:
    VideoFrameEncoder() = default;
    VideoFrameEncoder(const QMediaEncoderSettings &encoderSettings, const QSize &sourceSize, float frameRate, AVPixelFormat sourceFormat, AVPixelFormat swFormat);
    ~VideoFrameEncoder();

    void initWithFormatContext(AVFormatContext *formatContext);
    bool open();

    bool isNull() const { return !d; }

    AVPixelFormat sourceFormat() const { return d ? d->sourceFormat : AV_PIX_FMT_NONE; }
    AVPixelFormat targetFormat() const { return d ? d->targetFormat : AV_PIX_FMT_NONE; }

    qint64 getPts(qint64 ms);

    int sendFrame(AVFrameUPtr frame);
    AVPacket *retrievePacket();
};


}

QT_END_NAMESPACE

#endif
