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

#include <QtFFmpegMediaPluginImpl/private/qffmpeghwaccel_p.h>
#include <QtMultimedia/private/qplatformmediarecorder_p.h>
#include <QtMultimedia/private/qmultimediautils_p.h>

#include <unordered_set>

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
        VideoTransformation transform;
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
    VideoFrameEncoder(AVStream *stream, const Codec &codec, HWAccelUPtr hwAccel,
                      const SourceParams &sourceParams,
                      const QMediaEncoderSettings &encoderSettings);

    static AVStream *createStream(const SourceParams &sourceParams, AVFormatContext *formatContext);

    bool updateSourceFormatAndSize(const AVFrame *frame);

    void updateConversions();

    struct CreationResult
    {
        VideoFrameEncoderUPtr encoder;
        AVPixelFormat targetFormat = AV_PIX_FMT_NONE;
    };

    static CreationResult create(AVStream *stream, const Codec &codec, HWAccelUPtr hwAccel,
                                 const SourceParams &sourceParams,
                                 const QMediaEncoderSettings &encoderSettings,
                                 const AVPixelFormatSet &prohibitedTargetFormats = {});

    void initTargetSize();

    void initCodecFrameRate();

    bool initTargetFormats(const AVPixelFormatSet &prohibitedTargetFormats);

    void initStream();

    bool initCodecContext();

    bool open();

    qint64 estimateDuration(const AVPacket &packet, bool isFirstPacket);

private:
    QMediaEncoderSettings m_settings;
    AVStream *m_stream = nullptr;
    Codec m_codec;
    HWAccelUPtr m_accel;

    QSize m_sourceSize;
    QSize m_targetSize;

    qint64 m_lastPacketTime = AV_NOPTS_VALUE;
    AVCodecContextUPtr m_codecContext;
    SwsContextUPtr m_scaleContext;
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
} // namespace QFFmpeg

QT_END_NAMESPACE

#endif
