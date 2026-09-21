// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "../shared/hwframecontext_p.h"

#include <QtTest/qtest.h>
#include <QtMultimediaTestLib/private/rhi_support_p.h>

#include <QtFFmpegMediaPluginImpl/private/qffmpeghwaccel_videotoolbox_p.h>

#include <QtCore/qsize.h>

#include <QtGui/rhi/qrhi.h>
#include <QtGui/rhi/qrhi_platform.h>

extern "C" {
#include <libavutil/hwcontext.h>
}

#include <memory>
#include <optional>
#include <vector>

using namespace QFFmpeg;
using namespace QtMultimediaTest;
using namespace Qt::Literals;

using VideoToolboxFrameContext = HwFrameContext;

namespace {

void configureFramesContext(AVHWFramesContext &) { }

// Returns an error message if no usable VideoToolbox device is available on this machine.
q23::expected<VideoToolboxFrameContext, QString>
createVideoToolboxFrameContext(QSize size, quint8 lumaValue, quint8 chromaUValue,
                               quint8 chromaVValue)
{
    return createNv12HwFrameContext(AV_HWDEVICE_TYPE_VIDEOTOOLBOX, AV_PIX_FMT_VIDEOTOOLBOX, size,
                                    lumaValue, chromaUValue, chromaVValue, configureFramesContext);
}

// Same as above, but with a 10-bit P010 surface.
q23::expected<VideoToolboxFrameContext, QString>
createVideoToolboxP010FrameContext(QSize size, quint16 lumaValue, quint16 chromaUValue,
                                   quint16 chromaVValue)
{
    return createP010HwFrameContext(AV_HWDEVICE_TYPE_VIDEOTOOLBOX, AV_PIX_FMT_VIDEOTOOLBOX, size,
                                    lumaValue, chromaUValue, chromaVValue, configureFramesContext);
}

// Same as above, but with a single-plane BGRA8888 surface.
q23::expected<VideoToolboxFrameContext, QString>
createVideoToolboxBgraFrameContext(QSize size, QByteArray texel)
{
    return createSinglePlaneHwFrameContext(AV_HWDEVICE_TYPE_VIDEOTOOLBOX, AV_PIX_FMT_VIDEOTOOLBOX,
                                           size, std::move(texel), configureFramesContext);
}

q23::expected<VideoToolboxFrameContext, QString> createVideoToolboxBgra(QSize size)
{
    return createVideoToolboxBgraFrameContext(size, makeBytes({ 0x11, 0x22, 0x33, 0x44 }));
}

q23::expected<VideoToolboxFrameContext, QString> createVideoToolboxNv12(QSize size)
{
    return createVideoToolboxFrameContext(size, 0x40, 0x60, 0x80);
}

// Chosen so that a byte-swap or a missing "<< 6" shift would produce a different (and thus
// detectable) byte pattern.
q23::expected<VideoToolboxFrameContext, QString> createVideoToolboxP010(QSize size)
{
    return createVideoToolboxP010FrameContext(size, 0x141, 0x161, 0x181);
}

q23::expected<VideoToolboxFrameContext, QString>
createVideoToolboxFrame(QVideoFrameFormat::PixelFormat pixelFormat, QSize size)
{
    switch (pixelFormat) {
    case QVideoFrameFormat::Format_NV12:
        return createVideoToolboxNv12(size);
    case QVideoFrameFormat::Format_P010:
        return createVideoToolboxP010(size);
    case QVideoFrameFormat::Format_BGRA8888:
        return createVideoToolboxBgra(size);
    default:
        Q_UNREACHABLE_RETURN(q23::unexpected(u"unsupported pixel format"_s));
    }
}

} // namespace

class tst_QVideoToolboxTextureConverter : public QObject
{
    Q_OBJECT

private slots:
    void create_succeedsAndKeepsRhi_whenMetalIsAvailable();

    void createTextureHandles_roundTripsPixelData_forHwFrame_data();
    void createTextureHandles_roundTripsPixelData_forHwFrame();

    void map_returnsRealPixelData_forHwFrame_data();
    void map_returnsRealPixelData_forHwFrame();

    void createTextureHandles_outlivesSourceFrame_afterFrameContextIsDestroyed();
    void createTextureHandles_acceptsOldHandles_onSecondCall();
};

void tst_QVideoToolboxTextureConverter::create_succeedsAndKeepsRhi_whenMetalIsAvailable()
{
    std::unique_ptr<QRhi> rhi = createOffscreenMetalRhi();
    if (!rhi)
        QSKIP("Could not create a Metal QRhi backend on this system");

    std::shared_ptr<VideoToolBoxTextureConverter> converter =
            VideoToolBoxTextureConverter::create(rhi.get());
    QVERIFY(converter);
    if (!converter->rhi)
        QSKIP("VideoToolbox/Metal texture cache creation failed on this system");

    QCOMPARE(converter->rhi, rhi.get());
}

void tst_QVideoToolboxTextureConverter::createTextureHandles_roundTripsPixelData_forHwFrame_data()
{
    QTest::addColumn<QVideoFrameFormat::PixelFormat>("pixelFormat");

    QTest::newRow("NV12") << QVideoFrameFormat::Format_NV12;
    QTest::newRow("P010") << QVideoFrameFormat::Format_P010;
    QTest::newRow("BGRA") << QVideoFrameFormat::Format_BGRA8888;
}

// GPU-import coverage for the 8-bit (NV12) and 10-bit (P010) bi-planar CVPixelBuffer formats
// VideoToolbox decode produces, and the single-plane BGRA8888 CVPixelBuffer shape.
void tst_QVideoToolboxTextureConverter::createTextureHandles_roundTripsPixelData_forHwFrame()
{
    QFETCH(QVideoFrameFormat::PixelFormat, pixelFormat);
    std::vector<QRhiTexture::Format> planeFormats;
    switch (pixelFormat) {
    case QVideoFrameFormat::Format_NV12:
        planeFormats = { QRhiTexture::R8, QRhiTexture::RG8 };
        break;
    case QVideoFrameFormat::Format_P010:
        planeFormats = { QRhiTexture::R16, QRhiTexture::RG16 };
        break;
    case QVideoFrameFormat::Format_BGRA8888:
        planeFormats = { QRhiTexture::BGRA8 };
        break;
    default:
        Q_UNREACHABLE();
    }

    std::unique_ptr<QRhi> rhi = createOffscreenMetalRhi();
    if (!rhi)
        QSKIP("Could not create a Metal QRhi backend on this system");

    std::shared_ptr<VideoToolBoxTextureConverter> converter =
            VideoToolBoxTextureConverter::create(rhi.get());
    if (!converter->rhi)
        QSKIP("VideoToolbox/Metal texture cache creation failed on this system");

    const QSize frameSize(64, 64);
    q23::expected<VideoToolboxFrameContext, QString> frameContext =
            createVideoToolboxFrame(pixelFormat, frameSize);
    if (!frameContext)
        QSKIP(qPrintable(u"Could not create a VideoToolbox hw frame on this system: "_s
                         + frameContext.error()));

    QVideoFrameTexturesHandlesUPtr handles =
            converter->createTextureHandles(frameContext->hwFrame.get(), nullptr);
    if (!handles)
        QSKIP("VideoToolbox texture import failed on this system; see qWarning above for details");

    const int nPlanes = int(planeFormats.size());
    if (nPlanes > 1)
        QCOMPARE_NE(handles->textureHandle(*rhi, 0), handles->textureHandle(*rhi, 1));

    const auto *desc = QVideoTextureHelper::textureDescription(pixelFormat);
    for (int plane = 0; plane < nPlanes; ++plane) {
        const quint64 handle = handles->textureHandle(*rhi, plane);
        QCOMPARE_NE(handle, quint64(0));

        const QSize planeSize(desc->widthForPlane(frameSize.width(), plane),
                              desc->heightForPlane(frameSize.height(), plane));
        const auto readback = readBackPlane(*rhi, handle, planeSize, planeFormats[plane]);
        if (!readback)
            QFAIL(qPrintable(u"Failed to read back plane %1: "_s.arg(plane) + readback.error()));
        QCOMPARE(*readback,
                 frameContext->planeTexels[plane].repeated(planeSize.width() * planeSize.height()));
    }
}

void tst_QVideoToolboxTextureConverter::map_returnsRealPixelData_forHwFrame_data()
{
    QTest::addColumn<QVideoFrameFormat::PixelFormat>("pixelFormat");

    QTest::newRow("NV12") << QVideoFrameFormat::Format_NV12;
    QTest::newRow("P010") << QVideoFrameFormat::Format_P010;
    QTest::newRow("BGRA") << QVideoFrameFormat::Format_BGRA8888;
}

// End-to-end coverage for the CPU (QVideoFrame::map()) fallback path on a real VideoToolbox hw
// frame, constructed and wrapped exactly as QFFmpeg::VideoRenderer does: map() always goes
// through FFmpeg's own av_hwframe_transfer_data(), independent of GPU texture import.
void tst_QVideoToolboxTextureConverter::map_returnsRealPixelData_forHwFrame()
{
    QFETCH(QVideoFrameFormat::PixelFormat, pixelFormat);

    const QSize frameSize(64, 64);
    q23::expected<VideoToolboxFrameContext, QString> frameContext =
            createVideoToolboxFrame(pixelFormat, frameSize);
    if (!frameContext)
        QSKIP(qPrintable(u"Could not create a VideoToolbox hw frame on this system: "_s
                         + frameContext.error()));

    const q23::expected<void, QString> result =
            verifyMappedHwFrame(std::move(frameContext->hwFrame), frameContext->planeTexels);
    QVERIFY2(result.has_value(), result ? "" : qPrintable(result.error()));
}

void tst_QVideoToolboxTextureConverter::
        createTextureHandles_outlivesSourceFrame_afterFrameContextIsDestroyed()
{
    std::unique_ptr<QRhi> rhi = createOffscreenMetalRhi();
    if (!rhi)
        QSKIP("Could not create a Metal QRhi backend on this system");

    std::shared_ptr<VideoToolBoxTextureConverter> converter =
            VideoToolBoxTextureConverter::create(rhi.get());
    if (!converter->rhi)
        QSKIP("VideoToolbox/Metal texture cache creation failed on this system");

    const QSize frameSize(64, 64);
    q23::expected<VideoToolboxFrameContext, QString> frameContextResult =
            createVideoToolboxFrameContext(frameSize, 0x10, 0x20, 0x30);
    if (!frameContextResult)
        QSKIP(qPrintable(u"Could not create a VideoToolbox hw frame on this system: "_s
                         + frameContextResult.error()));
    std::optional<VideoToolboxFrameContext> frameContext = std::move(*frameContextResult);

    QVideoFrameTexturesHandlesUPtr handles =
            converter->createTextureHandles(frameContext->hwFrame.get(), nullptr);
    if (!handles)
        QSKIP("VideoToolbox texture import failed on this system; see qWarning above for details");

    const quint64 lumaHandle = handles->textureHandle(*rhi, 0);
    QVERIFY(lumaHandle != 0);

    // VideoToolBoxTextureHandles retains its own CVPixelBuffer ref, independent of the AVFrame.
    frameContext.reset();

    const auto lumaReadback = readBackPlane(*rhi, lumaHandle, frameSize, QRhiTexture::R8);
    if (!lumaReadback)
        QFAIL(qPrintable(u"Failed to read back luma plane: "_s + lumaReadback.error()));
    QCOMPARE(*lumaReadback, QByteArray(frameSize.width() * frameSize.height(), char(0x10)));
}

void tst_QVideoToolboxTextureConverter::createTextureHandles_acceptsOldHandles_onSecondCall()
{
    std::unique_ptr<QRhi> rhi = createOffscreenMetalRhi();
    if (!rhi)
        QSKIP("Could not create a Metal QRhi backend on this system");

    std::shared_ptr<VideoToolBoxTextureConverter> converter =
            VideoToolBoxTextureConverter::create(rhi.get());
    if (!converter->rhi)
        QSKIP("VideoToolbox/Metal texture cache creation failed on this system");

    const QSize frameSize(64, 64);
    q23::expected<VideoToolboxFrameContext, QString> frameContext =
            createVideoToolboxFrameContext(frameSize, 0x10, 0x20, 0x30);
    if (!frameContext)
        QSKIP(qPrintable(u"Could not create a VideoToolbox hw frame on this system: "_s
                         + frameContext.error()));

    QVideoFrameTexturesHandlesUPtr firstHandles =
            converter->createTextureHandles(frameContext->hwFrame.get(), nullptr);
    if (!firstHandles)
        QSKIP("VideoToolbox texture import failed on this system; see qWarning above for details");

    q23::expected<VideoToolboxFrameContext, QString> secondFrameContext =
            createVideoToolboxFrameContext(frameSize, 0x50, 0x60, 0x70);
    QVERIFY(secondFrameContext);

    QVideoFrameTexturesHandlesUPtr secondHandles = converter->createTextureHandles(
            secondFrameContext->hwFrame.get(), std::move(firstHandles));
    QVERIFY(secondHandles);
    QVERIFY(secondHandles->textureHandle(*rhi, 0) != 0);
}

QTEST_MAIN(tst_QVideoToolboxTextureConverter)

#include "tst_qvideotoolboxtextureconverter.moc"
