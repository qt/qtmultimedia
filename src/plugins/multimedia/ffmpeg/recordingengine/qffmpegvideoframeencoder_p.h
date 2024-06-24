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
#include "private/qplatformmediarecorder_p.h"

QT_BEGIN_NAMESPACE

class QMediaEncoderSettings;

namespace QFFmpeg {

class VideoFrameEncoder
{
public:
    struct SourceParams
    {
        QSize size;
        AVPixelFormat format = AV_PIX_FMT_NONE;
        AVPixelFormat swFormat = AV_PIX_FMT_NONE;
        QtVideo::Rotation rotation = QtVideo::Rotation::None;
        bool xMirrored = false;
        bool yMirrored = false;
        qreal frameRate = 0.;
        AVColorTransferCharacteristic colorTransfer = AVCOL_TRC_UNSPECIFIED;
        AVColorSpace colorSpace = AVCOL_SPC_UNSPECIFIED;
        AVColorRange colorRange = AVCOL_RANGE_UNSPECIFIED;
    };
    static std::unique_ptr<VideoFrameEncoder> create(const QMediaEncoderSettings &encoderSettings,
                                                     const SourceParams &sourceParams,
                                                     AVFormatContext *formatContext);

    ~VideoFrameEncoder();

    bool open();

    AVPixelFormat sourceFormat() const { return m_sourceFormat; }
    AVPixelFormat targetFormat() const { return m_targetFormat; }

    qreal codecFrameRate() const;

    qint64 getPts(qint64 ms) const;

    const AVRational &getTimeBase() const;

    int sendFrame(AVFrameUPtr inputFrame);
    AVPacketUPtr retrievePacket();

    const QMediaEncoderSettings &settings() { return m_settings; }

private:
    VideoFrameEncoder() = default;

    bool updateSourceFormatAndSize(const AVFrame *frame);

    void updateConversions();

    bool initCodec();

    void initTargetSize();

    void initCodecFrameRate();

    bool initTargetFormats();

    bool initCodecContext(const SourceParams &sourceParams, AVFormatContext *formatContext);

    qint64 estimateDuration(const AVPacket &packet, bool isFirstPacket);

private:
    QMediaEncoderSettings m_settings;
    QSize m_sourceSize;
    QSize m_targetSize;

    std::unique_ptr<HWAccel> m_accel;
    const AVCodec *m_codec = nullptr;
    AVStream *m_stream = nullptr;
    qint64 m_lastPacketTime = AV_NOPTS_VALUE;
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
