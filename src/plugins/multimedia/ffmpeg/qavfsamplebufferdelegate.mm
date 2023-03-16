// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qavfsamplebufferdelegate_p.h"

#define AVMediaType XAVMediaType

#include "qffmpeghwaccel_p.h"
#include "qavfhelpers_p.h"
#include "qffmpegvideobuffer_p.h"

#undef AVMediaType

#include <optional>

QT_USE_NAMESPACE

static void releaseHwFrame(void * /*opaque*/, uint8_t *data)
{
    CVPixelBufferRelease(CVPixelBufferRef(data));
}

namespace {

class CVImageVideoBuffer : public QAbstractVideoBuffer
{
public:
    CVImageVideoBuffer(CVImageBufferRef imageBuffer)
        : QAbstractVideoBuffer(QVideoFrame::NoHandle), m_buffer(imageBuffer)
    {
        CVPixelBufferRetain(imageBuffer);
    }

    ~CVImageVideoBuffer()
    {
        CVImageVideoBuffer::unmap();
        CVPixelBufferRelease(m_buffer);
    }

    CVImageVideoBuffer::MapData map(QVideoFrame::MapMode mode) override
    {
        MapData mapData;

        if (m_mode == QVideoFrame::NotMapped) {
            CVPixelBufferLockBaseAddress(
                    m_buffer, mode == QVideoFrame::ReadOnly ? kCVPixelBufferLock_ReadOnly : 0);
            m_mode = mode;
        }

        mapData.nPlanes = CVPixelBufferGetPlaneCount(m_buffer);
        Q_ASSERT(mapData.nPlanes <= 3);

        if (!mapData.nPlanes) {
            // single plane
            mapData.bytesPerLine[0] = CVPixelBufferGetBytesPerRow(m_buffer);
            mapData.data[0] = static_cast<uchar *>(CVPixelBufferGetBaseAddress(m_buffer));
            mapData.size[0] = CVPixelBufferGetDataSize(m_buffer);
            mapData.nPlanes = mapData.data[0] ? 1 : 0;
            return mapData;
        }

        // For a bi-planar or tri-planar format we have to set the parameters correctly:
        for (int i = 0; i < mapData.nPlanes; ++i) {
            mapData.bytesPerLine[i] = CVPixelBufferGetBytesPerRowOfPlane(m_buffer, i);
            mapData.size[i] = mapData.bytesPerLine[i] * CVPixelBufferGetHeightOfPlane(m_buffer, i);
            mapData.data[i] = static_cast<uchar *>(CVPixelBufferGetBaseAddressOfPlane(m_buffer, i));
        }

        return mapData;
    }

    QVideoFrame::MapMode mapMode() const override { return m_mode; }

    void unmap() override
    {
        if (m_mode != QVideoFrame::NotMapped) {
            CVPixelBufferUnlockBaseAddress(
                    m_buffer, m_mode == QVideoFrame::ReadOnly ? kCVPixelBufferLock_ReadOnly : 0);
            m_mode = QVideoFrame::NotMapped;
        }
    }

private:
    CVImageBufferRef m_buffer;
    QVideoFrame::MapMode m_mode = QVideoFrame::NotMapped;
};

}

// Make sure this is compatible with the layout used in ffmpeg's hwcontext_videotoolbox
static QFFmpeg::AVFrameUPtr allocHWFrame(AVBufferRef *hwContext, const CVPixelBufferRef &pixbuf)
{
    AVHWFramesContext *ctx = (AVHWFramesContext *)hwContext->data;
    auto frame = QFFmpeg::makeAVFrame();
    frame->hw_frames_ctx = av_buffer_ref(hwContext);
    frame->extended_data = frame->data;

    frame->buf[0] = av_buffer_create((uint8_t *)pixbuf, 1, releaseHwFrame, NULL, 0);
    frame->data[3] = (uint8_t *)pixbuf;
    CVPixelBufferRetain(pixbuf);
    frame->width = ctx->width;
    frame->height = ctx->height;
    frame->format = AV_PIX_FMT_VIDEOTOOLBOX;
    if (frame->width != (int)CVPixelBufferGetWidth(pixbuf)
        || frame->height != (int)CVPixelBufferGetHeight(pixbuf)) {

        // This can happen while changing camera format
        return nullptr;
    }
    return frame;
}

@implementation QAVFSampleBufferDelegate {
@private
    std::function<void(const QVideoFrame &)> frameHandler;
    AVBufferRef *hwFramesContext;
    std::unique_ptr<QFFmpeg::HWAccel> m_accel;
    qint64 startTime;
    std::optional<qint64> baseTime;
    qreal frameRate;
}

static QVideoFrame createHwVideoFrame(QAVFSampleBufferDelegate &delegate,
                                      CVImageBufferRef imageBuffer, QVideoFrameFormat format)
{
    Q_ASSERT(delegate.baseTime);

    if (!delegate.m_accel)
        return {};

    auto avFrame = allocHWFrame(delegate.m_accel->hwFramesContextAsBuffer(), imageBuffer);
    if (!avFrame)
        return {};

#ifdef USE_SW_FRAMES
    {
        auto swFrame = QFFmpeg::makeAVFrame();
        /* retrieve data from GPU to CPU */
        const int ret = av_hwframe_transfer_data(swFrame.get(), avFrame.get(), 0);
        if (ret < 0) {
            qWarning() << "Error transferring the data to system memory:" << ret;
        } else {
            avFrame = std::move(swFrame);
        }
    }
#endif

    avFrame->pts = delegate.startTime - *delegate.baseTime;

    return QVideoFrame(new QFFmpegVideoBuffer(std::move(avFrame)), format);
}

- (instancetype)initWithFrameHandler:(std::function<void(const QVideoFrame &)>)handler
{
    if (!(self = [super init]))
        return nil;

    Q_ASSERT(handler);

    frameHandler = std::move(handler);
    hwFramesContext = nullptr;
    startTime = 0;
    frameRate = 0.;
    return self;
}

- (void)captureOutput:(AVCaptureOutput *)captureOutput
        didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
               fromConnection:(AVCaptureConnection *)connection
{
    Q_UNUSED(connection);
    Q_UNUSED(captureOutput);

    // NB: on iOS captureOutput/connection can be nil (when recording a video -
    // avfmediaassetwriter).

    CVImageBufferRef imageBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);

    if (!imageBuffer) {
        qWarning() << "Cannot get image buffer from sample buffer";
        return;
    }

    const CMTime time = CMSampleBufferGetPresentationTimeStamp(sampleBuffer);
    const qint64 frameTime = time.timescale ? time.value * 1000000 / time.timescale : 0;
    if (!baseTime) {
        baseTime = frameTime;
        startTime = frameTime;
    }

    QVideoFrameFormat format = QAVFHelpers::videoFormatForImageBuffer(imageBuffer);
    if (!format.isValid()) {
        qWarning() << "Cannot get get video format for image buffer"
                   << CVPixelBufferGetWidth(imageBuffer) << 'x'
                   << CVPixelBufferGetHeight(imageBuffer);
        return;
    }

    format.setFrameRate(frameRate);

    auto frame = createHwVideoFrame(*self, imageBuffer, format);
    if (!frame.isValid())
        frame = QVideoFrame(new CVImageVideoBuffer(imageBuffer), format);

    frame.setStartTime(startTime - *baseTime);
    frame.setEndTime(frameTime - *baseTime);
    startTime = frameTime;

    frameHandler(frame);
}

- (void)setHWAccel:(std::unique_ptr<QFFmpeg::HWAccel> &&)accel
{
    m_accel = std::move(accel);
}

- (void)setVideoFormatFrameRate:(qreal)rate
{
    frameRate = rate;
}

@end
