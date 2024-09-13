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
#include "private/qmultimediautils_p.h"

QT_BEGIN_NAMESPACE

class QMediaEncoderSettings;

namespace QFFmpeg {

class VideoFrameEncoder;
using VideoFrameEncoderUPtr = std::unique_ptr<VideoFrameEncoder>;

class VideoFrameEncoder
{
public:
    struct SourceParams
    {
        QSize size;
        AVPixelFormat format = AV_PIX_FMT_NONE;
        AVPixelFormat swFormat = AV_PIX_FMT_NONE;
        NormalizedFrameTransformation transform;
        qreal frameRate = 0.;
        AVColorTransferCharacteristic colorTransfer = AVCOL_TRC_UNSPECIFIED;
        AVColorSpace colorSpace = AVCOL_SPC_UNSPECIFIED;
        AVColorRange colorRange = AVCOL_RANGE_UNSPECIFIED;
    };
    static VideoFrameEncoderUPtr create(const QMediaEncoderSettings &encoderSettings,
                                        const SourceParams &sourceParams,
                                        AVFormatContext *formatContext);

    ~VideoFrameEncoder();

    AVPixelFormat sourceFormat() const { return m_sourceFormat; }
    AVPixelFormat targetFormat() const { return m_targetFormat; }

    qreal codecFrameRate() const;

    qint64 getPts(qint64 ms) const;

    const AVRational &getTimeBase() const;

    int sendFrame(AVFrameUPtr inputFrame);
    AVPacketUPtr retrievePacket();

private:
    VideoFrameEncoder(AVStream *stream, const AVCodec *codec, HWAccelUPtr hwAccel,
                      const SourceParams &sourceParams,
                      const QMediaEncoderSettings &encoderSettings);

    static AVStream *createStream(const SourceParams &sourceParams, AVFormatContext *formatContext);

    bool updateSourceFormatAndSize(const AVFrame *frame);

    void updateConversions();

    static VideoFrameEncoderUPtr create(AVStream *stream, const AVCodec *codec, HWAccelUPtr hwAccel,
                                        const SourceParams &sourceParams,
                                        const QMediaEncoderSettings &encoderSettings);

    void initTargetSize();

    void initCodecFrameRate();

    bool initTargetFormats();

    void initStream();

    bool initCodecContext();

    bool open();

    qint64 estimateDuration(const AVPacket &packet, bool isFirstPacket);

private:
    QMediaEncoderSettings m_settings;
    AVStream *m_stream = nullptr;
    const AVCodec *m_codec = nullptr;
    HWAccelUPtr m_accel;

    QSize m_sourceSize;
    QSize m_targetSize;

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
