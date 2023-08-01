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
public:
    static std::unique_ptr<VideoFrameEncoder> create(const QMediaEncoderSettings &encoderSettings,
                                                     const QSize &sourceSize, qreal sourceFrameRate,
                                                     AVPixelFormat sourceFormat,
                                                     AVPixelFormat sourceSWFormat,
                                                     AVFormatContext *formatContext);

    ~VideoFrameEncoder();

    bool open();

    AVPixelFormat sourceFormat() const { return m_sourceFormat; }
    AVPixelFormat targetFormat() const { return m_targetFormat; }

    qint64 getPts(qint64 ms) const;

    const AVRational &getTimeBase() const;

    int sendFrame(AVFrameUPtr frame);
    AVPacketUPtr retrievePacket();

private:
    VideoFrameEncoder() = default;

    void updateConversions();

    bool initCodec();

    bool initTargetFormats();

    bool initCodecContext(AVFormatContext *formatContext);

private:
    QMediaEncoderSettings m_settings;
    QSize m_sourceSize;

    std::unique_ptr<HWAccel> m_accel;
    const AVCodec *m_codec = nullptr;
    AVStream *m_stream = nullptr;
    AVCodecContextUPtr m_codecContext;
    std::unique_ptr<SwsContext, decltype(&sws_freeContext)> m_converter = { nullptr,
                                                                            &sws_freeContext };
    AVPixelFormat m_sourceFormat = AV_PIX_FMT_NONE;
    AVPixelFormat m_sourceSWFormat = AV_PIX_FMT_NONE;
    AVPixelFormat m_targetFormat = AV_PIX_FMT_NONE;
    AVPixelFormat m_targetSWFormat = AV_PIX_FMT_NONE;
    bool m_downloadFromHW = false;
    bool m_uploadToHW = false;

    AVRational m_codecFrameRate = { 0, 1 };

    int64_t m_prevPacketDts = AV_NOPTS_VALUE;
    int64_t m_packetDtsOffset = 0;
};
}

QT_END_NAMESPACE

#endif
