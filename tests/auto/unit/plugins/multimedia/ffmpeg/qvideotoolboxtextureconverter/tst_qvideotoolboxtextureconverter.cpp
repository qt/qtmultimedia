// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "../shared/nv12hwframecontext_p.h"

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

using namespace QFFmpeg;
using namespace QtMultimediaTest;
using namespace Qt::Literals;

using VideoToolboxFrameContext = Nv12HwFrameContext;

namespace {

// Returns nullopt if no usable VideoToolbox device is available on this machine.
std::optional<VideoToolboxFrameContext> createVideoToolboxFrameContext(QSize size, quint8 lumaValue,
                                                                       quint8 chromaUValue,
                                                                       quint8 chromaVValue)
{
    return createNv12HwFrameContext(AV_HWDEVICE_TYPE_VIDEOTOOLBOX, AV_PIX_FMT_VIDEOTOOLBOX, size,
                                    lumaValue, chromaUValue, chromaVValue, [](AVHWFramesContext &) {
    });
}

} // namespace

class tst_QVideoToolboxTextureConverter : public QObject
{
    Q_OBJECT

private slots:
    void create_succeedsAndKeepsRhi_whenMetalIsAvailable();
    void createTextureHandles_roundTripsPixelData_forNv12HwFrame();
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

void tst_QVideoToolboxTextureConverter::createTextureHandles_roundTripsPixelData_forNv12HwFrame()
{
    std::unique_ptr<QRhi> rhi = createOffscreenMetalRhi();
    if (!rhi)
        QSKIP("Could not create a Metal QRhi backend on this system");

    std::shared_ptr<VideoToolBoxTextureConverter> converter =
            VideoToolBoxTextureConverter::create(rhi.get());
    if (!converter->rhi)
        QSKIP("VideoToolbox/Metal texture cache creation failed on this system");

    const QSize frameSize(64, 64);
    std::optional<VideoToolboxFrameContext> frameContext =
            createVideoToolboxFrameContext(frameSize, 0x40, 0x60, 0x80);
    if (!frameContext)
        QSKIP("Could not create a VideoToolbox hw frame on this system");

    QVideoFrameTexturesHandlesUPtr handles =
            converter->createTextureHandles(frameContext->hwFrame.get(), nullptr);
    if (!handles)
        QSKIP("VideoToolbox texture import failed on this system; see qWarning above for details");

    // NV12 has two planes: luma (R8) and interleaved chroma (RG8).
    const quint64 lumaHandle = handles->textureHandle(*rhi, 0);
    const quint64 chromaHandle = handles->textureHandle(*rhi, 1);
    QVERIFY(lumaHandle != 0);
    QVERIFY(chromaHandle != 0);
    QVERIFY(lumaHandle != chromaHandle);

    const auto lumaReadback = readBackPlane(*rhi, lumaHandle, frameSize, QRhiTexture::R8);
    if (!lumaReadback)
        QFAIL(qPrintable(u"Failed to read back luma plane: "_s + lumaReadback.error()));
    QCOMPARE(*lumaReadback, QByteArray(frameSize.width() * frameSize.height(), char(0x40)));

    const QSize chromaSize(frameSize.width() / 2, frameSize.height() / 2);
    const auto chromaReadback = readBackPlane(*rhi, chromaHandle, chromaSize, QRhiTexture::RG8);
    if (!chromaReadback)
        QFAIL(qPrintable(u"Failed to read back chroma plane: "_s + chromaReadback.error()));
    QByteArray chromaTexel;
    chromaTexel += char(0x60);
    chromaTexel += char(0x80);
    const QByteArray expectedChroma =
            chromaTexel.repeated(chromaSize.width() * chromaSize.height());
    QCOMPARE(*chromaReadback, expectedChroma);
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
    std::optional<VideoToolboxFrameContext> frameContext =
            createVideoToolboxFrameContext(frameSize, 0x10, 0x20, 0x30);
    if (!frameContext)
        QSKIP("Could not create a VideoToolbox hw frame on this system");

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
    std::optional<VideoToolboxFrameContext> frameContext =
            createVideoToolboxFrameContext(frameSize, 0x10, 0x20, 0x30);
    if (!frameContext)
        QSKIP("Could not create a VideoToolbox hw frame on this system");

    QVideoFrameTexturesHandlesUPtr firstHandles =
            converter->createTextureHandles(frameContext->hwFrame.get(), nullptr);
    if (!firstHandles)
        QSKIP("VideoToolbox texture import failed on this system; see qWarning above for details");

    std::optional<VideoToolboxFrameContext> secondFrameContext =
            createVideoToolboxFrameContext(frameSize, 0x50, 0x60, 0x70);
    QVERIFY(secondFrameContext);

    QVideoFrameTexturesHandlesUPtr secondHandles = converter->createTextureHandles(
            secondFrameContext->hwFrame.get(), std::move(firstHandles));
    QVERIFY(secondHandles);
    QVERIFY(secondHandles->textureHandle(*rhi, 0) != 0);
}

QTEST_MAIN(tst_QVideoToolboxTextureConverter)

#include "tst_qvideotoolboxtextureconverter.moc"
