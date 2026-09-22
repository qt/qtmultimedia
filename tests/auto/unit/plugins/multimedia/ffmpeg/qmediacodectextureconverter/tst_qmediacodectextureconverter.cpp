// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtMultimediaTestLib/private/qfileutil_p.h>
#include <QtMultimediaTestLib/private/rhi_support_p.h>
#include <QtTest/qtest.h>

#include <QtFFmpegMediaPluginImpl/private/qffmpeg_p.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpeghwaccel_mediacodec_p.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpeghwaccel_p.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpegvideobuffer_p.h>

#include <QtMultimedia/qvideoframe.h>
#include <QtMultimedia/private/qvideoframe_p.h>

#include <QtGui/qguiapplication.h>
#include <QtGui/qoffscreensurface.h>
#include <QtGui/rhi/qrhi.h>

#include <QtCore/qfileinfo.h>
#include <QtCore/qregularexpression.h>
#include <QtCore/qtemporaryfile.h>
#include <QtCore/qurl.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include <chrono>
#include <memory>
#include <optional>

using namespace QFFmpeg;
using namespace QtMultimediaTest;
using namespace Qt::Literals;
using namespace std::chrono_literals;

namespace {

// MediaCodec's  AVMediaCodecBuffer-backed AVFrame can only come from a real, running AMediaCodec decode.
class MediaCodecDeviceContext
{
public:
    static std::optional<MediaCodecDeviceContext> create(const QUrl &url)
    {
        MediaCodecDeviceContext context;

        QString realFilePath;
        if (url.scheme() == "qrc"_L1) {
            const QString suffix = QFileInfo(url.path()).suffix();
            auto temporaryFile = copyResourceToTemporaryFile(url, u"XXXXXX."_s + suffix);
            if (!temporaryFile)
                return std::nullopt;

            realFilePath = temporaryFile->fileName();
            context.m_extractedResourceFile = std::move(temporaryFile);
        } else if (url.isLocalFile()) {
            realFilePath = url.toLocalFile();
        } else {
            return std::nullopt;
        }

        AVFormatContext *rawFormatContext = nullptr;
        const QByteArray path = realFilePath.toUtf8();
        if (avformat_open_input(&rawFormatContext, path.constData(), nullptr, nullptr) < 0)
            return std::nullopt;
        context.m_formatContext.reset(rawFormatContext);

        if (avformat_find_stream_info(context.m_formatContext.get(), nullptr) < 0)
            return std::nullopt;

        const AVCodec *unusedCodec = nullptr;
        const int streamIndex = av_find_best_stream(context.m_formatContext.get(),
                                                    AVMEDIA_TYPE_VIDEO, -1, -1, &unusedCodec, 0);
        if (streamIndex < 0)
            return std::nullopt;
        context.m_videoStreamIndex = streamIndex;

        const AVCodecParameters *codecParams =
                context.m_formatContext->streams[*context.m_videoStreamIndex]->codecpar;

        auto [codec, hwAccel] = HWAccel::findDecoderWithHwAccel(codecParams->codec_id);
        if (!codec || !hwAccel || hwAccel->deviceType() != AV_HWDEVICE_TYPE_MEDIACODEC)
            return std::nullopt;
        context.m_hwAccel = std::move(hwAccel);

        context.m_codecContext.reset(avcodec_alloc_context3(codec->get()));
        if (!context.m_codecContext)
            return std::nullopt;

        if (avcodec_parameters_to_context(context.m_codecContext.get(), codecParams) < 0)
            return std::nullopt;

        context.m_codecContext->hw_device_ctx =
                av_buffer_ref(context.m_hwAccel->hwDeviceContextAsBuffer());
        context.m_codecContext->get_format = &getFormat;

        if (avcodec_open2(context.m_codecContext.get(), codec->get(), nullptr) < 0)
            return std::nullopt;

        return context;
    }

    MediaCodecDeviceContext(MediaCodecDeviceContext &&) noexcept = default;
    MediaCodecDeviceContext &operator=(MediaCodecDeviceContext &&) noexcept = default;

    AVFrameUPtr decodeNextFrame()
    {
        AVFrameUPtr frame = makeAVFrame();
        AVPacketUPtr packet(av_packet_alloc());

        while (true) {
            const int receiveResult = avcodec_receive_frame(m_codecContext.get(), frame.get());
            if (receiveResult == 0) {
                // QTBUG-108446: the MediaCodec decoder never attaches an AVHWFramesContext to its
                // output frames (compare qffmpegvideorenderer.cpp)
                if (!frame->hw_frames_ctx) {
                    if (!m_hwAccel->hwFramesContext())
                        m_hwAccel->createFramesContext(AVPixelFormat(frame->format),
                                                       { frame->width, frame->height });
                    if (m_hwAccel->hwFramesContext())
                        frame->hw_frames_ctx = av_buffer_ref(m_hwAccel->hwFramesContextAsBuffer());
                }
                Q_ASSERT(frame->format == AV_PIX_FMT_MEDIACODEC);
                return frame;
            }
            if (receiveResult != AVERROR(EAGAIN))
                return nullptr; // EOF or a real decode error

            int readResult = 0;
            do {
                av_packet_unref(packet.get());
                readResult = av_read_frame(m_formatContext.get(), packet.get());
            } while (readResult == 0 && packet->stream_index != *m_videoStreamIndex);

            if (readResult < 0) {
                // Flush: signal EOF to the decoder and let the next receive_frame() drain it.
                avcodec_send_packet(m_codecContext.get(), nullptr);
                continue;
            }

            if (avcodec_send_packet(m_codecContext.get(), packet.get()) < 0)
                return nullptr;
        }
    }

private:
    MediaCodecDeviceContext() = default;

    AVDemuxerContextUPtr m_formatContext;
    AVCodecContextUPtr m_codecContext;
    HWAccelUPtr m_hwAccel;
    std::optional<int> m_videoStreamIndex;
    // Kept alive for the object's lifetime when filePath was a resource path; the demuxer reads
    // from this file lazily throughout decodeNextFrame(), not just at open time.
    std::unique_ptr<QTemporaryFile> m_extractedResourceFile;
};

} // namespace

class tst_QMediaCodecTextureConverter : public QObject
{
    Q_OBJECT

private slots:
    void create_succeedsAndKeepsRhi_whenGlIsAvailable();
    void createTextureHandles_returnsRedTexture_forRealDecodedFrame_data();
    void createTextureHandles_returnsRedTexture_forRealDecodedFrame();
    void createTextureHandles_reusesTexture_onSecondRealFrame();
    void createTextureHandles_returnsNull_forFrameWithoutHwFramesCtx();
    void map_returnsFalse_forRealDecodedFrame();

private:
    // The MediaCodec decode is asynchronous: av_mediacodec_release_buffer() only requests that
    // the buffer be rendered to the Surface, and AndroidSurfaceTexture::updateTexImage() can
    // legitimately be a no-op if that render hasn't landed yet. This is an inherent property of
    // the SurfaceTexture API, not test flakiness -- poll with a bounded retry instead of asserting
    // on the first attempt.
    std::optional<QByteArray> pollReadback(QRhi &rhi, quint64 textureHandle, QSize size);
};

std::optional<QByteArray>
tst_QMediaCodecTextureConverter::pollReadback(QRhi &rhi, quint64 textureHandle, QSize size)
{
    constexpr int maxAttempts = 50;
    constexpr auto retryDelay = 20ms;
    for (int attempt = 0; attempt < maxAttempts; ++attempt) {
        auto result = readBackExternalOesTexture(rhi, textureHandle, size);
        if (result)
            return *result;
        QTest::qWait(retryDelay);
    }
    return std::nullopt;
}

void tst_QMediaCodecTextureConverter::create_succeedsAndKeepsRhi_whenGlIsAvailable()
{
    q23::expected<OffscreenGlRhi, QString> offscreenRhi = createOffscreenGlRhi();
    if (!offscreenRhi)
        QSKIP(qPrintable(u"Could not create an OpenGL ES2 QRhi backend on this system: "_s
                         + offscreenRhi.error()));
    auto &[fallbackSurface, rhi] = *offscreenRhi;

    std::shared_ptr converter = MediaCodecTextureConverter::create(rhi.get());
    QVERIFY(converter);
    QCOMPARE(converter->rhi, rhi.get());
}

void tst_QMediaCodecTextureConverter::
        createTextureHandles_returnsRedTexture_forRealDecodedFrame_data()
{
    QTest::addColumn<QUrl>("sourceUrl");

    QTest::newRow("NV12") << QUrl(u"qrc:/testdata/one_red_frame.mp4"_s);
    QTest::newRow("P010") << QUrl(u"qrc:/testdata/one_red_frame_p010.mp4"_s);
}

void tst_QMediaCodecTextureConverter::createTextureHandles_returnsRedTexture_forRealDecodedFrame()
{
    QFETCH(QUrl, sourceUrl);

    q23::expected<OffscreenGlRhi, QString> offscreenRhi = createOffscreenGlRhi();
    if (!offscreenRhi)
        QSKIP(qPrintable(u"Could not create an OpenGL ES2 QRhi backend on this system: "_s
                         + offscreenRhi.error()));
    auto &[fallbackSurface, rhi] = *offscreenRhi;

    // AndroidSurfaceTexture's constructor calls the deprecated android.graphics.SurfaceTexture(int
    // texName) constructor, which requires a GL context to be current on the calling thread; this
    // must be current before MediaCodecTextureConverter::setupDecoderSurface() runs, which happens
    // synchronously inside avcodec_open2()/decodeNextFrame() via the get_format callback -- not
    // when createTextureHandles() is called afterwards.
    rhi->makeThreadLocalNativeContextCurrent();

    std::optional<MediaCodecDeviceContext> deviceContext =
            MediaCodecDeviceContext::create(sourceUrl);
    if (!deviceContext)
        QSKIP("Could not open a MediaCodec hw decode session on this device");

    AVFrameUPtr frame = deviceContext->decodeNextFrame();
    QVERIFY(frame);
    QCOMPARE(frame->format, AV_PIX_FMT_MEDIACODEC);

    std::shared_ptr converter = MediaCodecTextureConverter::create(rhi.get());
    QVideoFrameTexturesHandlesUPtr handles = converter->createTextureHandles(frame.get(), nullptr);
    QVERIFY(handles);

    const quint64 textureHandle = handles->textureHandle(*rhi, 0);
    QVERIFY(textureHandle != 0);

    const QSize frameSize(frame->width, frame->height);
    std::optional<QByteArray> pixels = pollReadback(*rhi, textureHandle, frameSize);
    if (!pixels)
        QFAIL("Timed out waiting for the decoded buffer to be rendered to the surface");

    // Sanity-check a handful of pixels rather than the whole frame: exact color reproduction
    // through a real hw video decoder is not bit-exact (chroma subsampling/color conversion), so
    // only assert "clearly red", not an exact RGBA match.
    const int stride = frameSize.width() * 4;
    for (int y : { 0, frameSize.height() / 2, frameSize.height() - 1 }) {
        const uchar *pixel = reinterpret_cast<const uchar *>(pixels->constData() + y * stride);
        QVERIFY2(pixel[0] > 150 && pixel[1] < 100 && pixel[2] < 100,
                 qPrintable(u"Pixel at row %1 was not clearly red: (%2, %3, %4)"_s.arg(y)
                                    .arg(pixel[0])
                                    .arg(pixel[1])
                                    .arg(pixel[2])));
    }
}

void tst_QMediaCodecTextureConverter::createTextureHandles_reusesTexture_onSecondRealFrame()
{
    q23::expected<OffscreenGlRhi, QString> offscreenRhi = createOffscreenGlRhi();
    if (!offscreenRhi)
        QSKIP(qPrintable(u"Could not create an OpenGL ES2 QRhi backend on this system: "_s
                         + offscreenRhi.error()));
    auto &[fallbackSurface, rhi] = *offscreenRhi;

    rhi->makeThreadLocalNativeContextCurrent();

    std::optional<MediaCodecDeviceContext> deviceContext =
            MediaCodecDeviceContext::create(QUrl(u"qrc:/testdata/colors.mp4"_s));
    if (!deviceContext)
        QSKIP("Could not open a MediaCodec hw decode session on this device");

    std::shared_ptr converter = MediaCodecTextureConverter::create(rhi.get());

    AVFrameUPtr firstFrame = deviceContext->decodeNextFrame();
    QVERIFY(firstFrame);
    QVideoFrameTexturesHandlesUPtr firstHandles =
            converter->createTextureHandles(firstFrame.get(), nullptr);
    QVERIFY(firstHandles);
    const quint64 firstHandle = firstHandles->textureHandle(*rhi, 0);
    QVERIFY(firstHandle != 0);

    AVFrameUPtr secondFrame = deviceContext->decodeNextFrame();
    QVERIFY(secondFrame);
    QVideoFrameTexturesHandlesUPtr secondHandles =
            converter->createTextureHandles(secondFrame.get(), std::move(firstHandles));
    QVERIFY(secondHandles);
    const quint64 secondHandle = secondHandles->textureHandle(*rhi, 0);
    QVERIFY(secondHandle != 0);

    // Same decoder Surface/AndroidSurfaceTexture across both calls, so the underlying QRhiTexture
    // is expected to be reused rather than recreated (only invalidated when
    // AndroidSurfaceTexture::index() changes, which doesn't happen here).
    QCOMPARE(secondHandle, firstHandle);
}

void tst_QMediaCodecTextureConverter::map_returnsFalse_forRealDecodedFrame()
{
    std::optional<MediaCodecDeviceContext> deviceContext =
            MediaCodecDeviceContext::create(QUrl(u"qrc:/testdata/one_red_frame.mp4"_s));
    if (!deviceContext)
        QSKIP("Could not open a MediaCodec hw decode session on this device");

    AVFrameUPtr frame = deviceContext->decodeNextFrame();
    QVERIFY(frame);
    QCOMPARE(frame->format, AV_PIX_FMT_MEDIACODEC);

    auto buffer = std::make_unique<QFFmpegVideoBuffer>(std::move(frame));
    QVideoFrameFormat format(buffer->size(), buffer->pixelFormat());
    QVideoFrame videoFrame = QVideoFramePrivate::createFrame(std::move(buffer), format);

    QTest::ignoreMessage(QtWarningMsg,
                         QRegularExpression(u"Error transferring the data to system memory.*"_s));
    QVERIFY(!videoFrame.map(QVideoFrame::ReadOnly));
}

void tst_QMediaCodecTextureConverter::createTextureHandles_returnsNull_forFrameWithoutHwFramesCtx()
{
    q23::expected<OffscreenGlRhi, QString> offscreenRhi = createOffscreenGlRhi();
    if (!offscreenRhi)
        QSKIP(qPrintable(u"Could not create an OpenGL ES2 QRhi backend on this system: "_s
                         + offscreenRhi.error()));
    auto &[fallbackSurface, rhi] = *offscreenRhi;

    std::shared_ptr converter = MediaCodecTextureConverter::create(rhi.get());

    AVFrameUPtr frame = makeAVFrame();
    frame->width = 64;
    frame->height = 64;
    frame->format = AV_PIX_FMT_YUV420P;
    // Deliberately no hw_frames_ctx: getTextureSurface() bails out on that check alone, so this
    // needs no real decode.

    QVideoFrameTexturesHandlesUPtr handles = converter->createTextureHandles(frame.get(), nullptr);
    QVERIFY(!handles);
}

QTEST_MAIN(tst_QMediaCodecTextureConverter)

#include "tst_qmediacodectextureconverter.moc"
