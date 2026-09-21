// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>

#include <QtFFmpegMediaPluginImpl/private/qffmpeghwaccel_d3d11_p.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpeg_p.h>

#include "../shared/hwframecontext_p.h"

#include <QtMultimediaTestLib/private/qwindowsd3d11testdevicecontext_p.h>
#include <QtMultimediaTestLib/private/rhi_support_p.h>
#include <QtMultimedia/private/qvideotexturehelper_p.h>

#include <QtGui/rhi/qrhi.h>
#include <QtGui/qcolor.h>

#include <QtCore/qbytearrayview.h>

#include <functional>

extern "C" {
#include <libavutil/hwcontext.h>
#include <libavutil/hwcontext_d3d11va.h>
}

using namespace QFFmpeg;
using namespace QtMultimediaTest;

namespace {

constexpr QSize kFrameSize{ 64, 32 };

// Wraps a real D3D11 device/context as a synthetic AV_HWDEVICE_TYPE_D3D11VA device context, the
// same way the FFmpeg backend does when decoding, except that it wraps an externally-owned
// device rather than creating its own.
AVBufferUPtr createD3D11DeviceContext(const QWindowsD3D11TestDeviceContext &device)
{
    AVBufferRef *ref = av_hwdevice_ctx_alloc(AV_HWDEVICE_TYPE_D3D11VA);
    if (!ref)
        return {};

    auto *deviceCtx = reinterpret_cast<AVHWDeviceContext *>(ref->data);
    auto *d3d11Ctx = static_cast<AVD3D11VADeviceContext *>(deviceCtx->hwctx);

    // Deallocating the AVHWDeviceContext always releases this interface (see
    // hwcontext_d3d11va.h), regardless of who created it, so give it its own reference.
    d3d11Ctx->device = device.device.Get();
    d3d11Ctx->device->AddRef();

    if (av_hwdevice_ctx_init(ref) < 0) {
        av_buffer_unref(&ref);
        return {};
    }

    return AVBufferUPtr(ref);
}

// Creates a synthetic AV_PIX_FMT_D3D11 frames context with the given decoded (sw) pixel format
// and a small pool, wrapping the given AVHWDeviceContext, matching what
// D3D11TextureConverter::SetupDecoderTextures sets up for real decoding, so that a frame obtained
// from it is structurally identical to what the converter sees for a real hardware-decoded frame.
AVBufferUPtr createD3D11FramesContext(AVBufferRef *deviceCtx, QSize size, AVPixelFormat swFormat)
{
    AVBufferRef *ref = av_hwframe_ctx_alloc(deviceCtx);
    if (!ref)
        return {};

    auto *framesCtx = reinterpret_cast<AVHWFramesContext *>(ref->data);
    framesCtx->format = AV_PIX_FMT_D3D11;
    framesCtx->sw_format = swFormat;
    framesCtx->width = size.width();
    framesCtx->height = size.height();
    // Two slots: the "accepts old handles on second call" test needs two frames from the
    // pool alive at once.
    framesCtx->initial_pool_size = 2;

    auto *d3d11FramesCtx = static_cast<AVD3D11VAFramesContext *>(framesCtx->hwctx);
    // D3D11_BIND_DECODER is only valid for a format the driver actually supports as a video
    // decoder render target (planar YUV); requesting it for a packed RGB format like BGRA fails
    // texture creation outright, so only ask for it for the planar formats a real decode session
    // would use it for.
    d3d11FramesCtx->BindFlags = D3D11_BIND_SHADER_RESOURCE;
    if (swFormat == AV_PIX_FMT_NV12 || swFormat == AV_PIX_FMT_P010)
        d3d11FramesCtx->BindFlags |= D3D11_BIND_DECODER;
    d3d11FramesCtx->MiscFlags = D3D11_RESOURCE_MISC_SHARED;

    if (av_hwframe_ctx_init(ref) < 0) {
        av_buffer_unref(&ref);
        return {};
    }

    return AVBufferUPtr(ref);
}

// Gets a frame from the pool (matching the shape a real D3D11VA-decoded frame has: format,
// hw_frames_ctx, and data[0]/data[1] filled in by av_hwframe_get_buffer()) and fills its texture
// via "fill", for pixel-content verification. "fill" receives the pooled array texture and the
// array slice this particular frame occupies.
AVFrameUPtr createTestFrame(
        AVBufferRef *framesCtx, QSize size,
        const std::function<HRESULT(const ComPtr<ID3D11Texture2D> &, UINT arraySlice)> &fill)
{
    AVFrameUPtr frame = makeAVFrame();
    if (av_hwframe_get_buffer(framesCtx, frame.get(), 0) < 0)
        return {};

    const ComPtr<ID3D11Texture2D> texture = reinterpret_cast<ID3D11Texture2D *>(frame->data[0]);
    // frame->data[1] holds the pooled array texture's slice index for this frame (see
    // getAvFramePoolIndex in qffmpeghwaccel_d3d11.cpp) - fill that specific slice, not slice 0.
    const UINT arraySlice = static_cast<UINT>(reinterpret_cast<intptr_t>(frame->data[1]));
    if (fill(texture, arraySlice) != S_OK)
        return {};

    return frame;
}

AVFrameUPtr createNv12TestFrame(AVBufferRef *framesCtx,
                                const QWindowsD3D11TestDeviceContext &device, QSize size,
                                quint8 lumaValue, quint8 chromaU, quint8 chromaV)
{
    return createTestFrame(framesCtx, size,
                           [&](const ComPtr<ID3D11Texture2D> &texture, UINT slice) {
        return device.fillNV12Texture(texture, size, lumaValue, chromaU, chromaV, slice);
    });
}

AVFrameUPtr createP010TestFrame(AVBufferRef *framesCtx,
                                const QWindowsD3D11TestDeviceContext &device, QSize size,
                                quint16 lumaValue, quint16 chromaU, quint16 chromaV)
{
    return createTestFrame(framesCtx, size,
                           [&](const ComPtr<ID3D11Texture2D> &texture, UINT slice) {
        return device.fillP010Texture(texture, size, lumaValue, chromaU, chromaV, slice);
    });
}

AVFrameUPtr createBgraTestFrame(AVBufferRef *framesCtx,
                                const QWindowsD3D11TestDeviceContext &device, QSize size,
                                QByteArrayView texel)
{
    return createTestFrame(framesCtx, size,
                           [&](const ComPtr<ID3D11Texture2D> &texture, UINT slice) {
        return device.fillSinglePlaneTexture(texture, size, texel, slice);
    });
}

} // namespace

class tst_QFFmpegD3D11TextureConverter : public QObject
{
    Q_OBJECT

private slots:
    void init();

    void create_succeedsAndKeepsRhi_whenRhiIsD3D11();

    void createTextureHandles_returnsNull_whenRhiIsNotD3D11();
    void createTextureHandles_returnsNull_whenRhiIsWarpD3D11();
    void createTextureHandles_returnsNull_whenFrameFormatIsNotD3D11();
    void createTextureHandles_returnsNull_whenFrameHasNoHwFramesCtx();

    void createTextureHandles_roundTripsPixelData_forHwFrame_data();
    void createTextureHandles_roundTripsPixelData_forHwFrame();

    void map_returnsRealPixelData_forHwFrame_data();
    void map_returnsRealPixelData_forHwFrame();

    void createTextureHandles_outlivesSourceFrame_afterFrameContextIsDestroyed();
    void createTextureHandles_acceptsOldHandles_onSecondCall();

private:
    ComResult<QWindowsD3D11TestDeviceContext> m_device = q23::unexpected{ E_FAIL };
    AVBufferUPtr m_deviceCtx;
    AVBufferUPtr m_framesCtx;
};

void tst_QFFmpegD3D11TextureConverter::init()
{
    m_device = createD3D11TestDeviceContext();
    if (!m_device)
        QSKIP("Could not create a D3D11 device on this system");

    m_deviceCtx = createD3D11DeviceContext(*m_device);
    if (!m_deviceCtx)
        QSKIP("Could not create a D3D11VA hw device context on this system (no DXVA-capable "
              "GPU/driver?)");

    m_framesCtx = createD3D11FramesContext(m_deviceCtx.get(), kFrameSize, AV_PIX_FMT_NV12);
    QVERIFY(m_framesCtx);
}

void tst_QFFmpegD3D11TextureConverter::create_succeedsAndKeepsRhi_whenRhiIsD3D11()
{
    std::unique_ptr<QRhi> rhi = createOffscreenD3D11Rhi();
    if (!rhi)
        QSKIP("Could not create a D3D11 QRhi on this system");

    std::shared_ptr<D3D11TextureConverter> converter = D3D11TextureConverter::create(rhi.get());
    QVERIFY(converter);
    QCOMPARE(converter->rhi, rhi.get());
}

void tst_QFFmpegD3D11TextureConverter::createTextureHandles_returnsNull_whenRhiIsNotD3D11()
{
    std::unique_ptr<QRhi> rhi = createNullRhi();
    QVERIFY(rhi);

    std::shared_ptr<D3D11TextureConverter> converter = D3D11TextureConverter::create(rhi.get());
    QVERIFY(!converter);
}

void tst_QFFmpegD3D11TextureConverter::createTextureHandles_returnsNull_whenRhiIsWarpD3D11()
{
    // A WARP (software) D3D11 device lives on a different adapter than the source device, so
    // sharing textures with it would either fail or, worse, stall for the keyed mutex timeout —
    // create() must reject it (via isRhiBackendSupported()'s CpuDevice check) rather than
    // attempting a doomed shared-texture copy.
    const ComResult<QWindowsD3D11TestDeviceContext> warpDevice = createWarpD3D11TestDeviceContext();
    if (!warpDevice)
        QSKIP("Could not create a WARP D3D11 device on this system");

    std::unique_ptr<QRhi> rhi = createWarpD3D11Rhi(*warpDevice);
    if (!rhi)
        QSKIP("Could not create a D3D11 QRhi wrapping a WARP device on this system");
    QCOMPARE_EQ(rhi->driverInfo().deviceType, QRhiDriverInfo::CpuDevice);

    std::shared_ptr<D3D11TextureConverter> converter = D3D11TextureConverter::create(rhi.get());
    QVERIFY(!converter);
}

void tst_QFFmpegD3D11TextureConverter::createTextureHandles_returnsNull_whenFrameFormatIsNotD3D11()
{
    std::unique_ptr<QRhi> rhi = createOffscreenD3D11Rhi();
    if (!rhi)
        QSKIP("Could not create a D3D11 QRhi on this system");

    std::shared_ptr<D3D11TextureConverter> converter = D3D11TextureConverter::create(rhi.get());
    QVERIFY(converter);

    // A plain software frame: correct-looking pixel format, but no hw_frames_ctx and the wrong
    // AVPixelFormat, so this must be rejected rather than dereferencing anything hw-specific.
    AVFrameUPtr frame = makeAVFrame();
    frame->format = AV_PIX_FMT_NV12;
    frame->width = kFrameSize.width();
    frame->height = kFrameSize.height();

    QVERIFY(!converter->createTextureHandles(frame.get(), nullptr));
}

void tst_QFFmpegD3D11TextureConverter::createTextureHandles_returnsNull_whenFrameHasNoHwFramesCtx()
{
    std::unique_ptr<QRhi> rhi = createOffscreenD3D11Rhi();
    if (!rhi)
        QSKIP("Could not create a D3D11 QRhi on this system");

    std::shared_ptr<D3D11TextureConverter> converter = D3D11TextureConverter::create(rhi.get());
    QVERIFY(converter);

    // The right pixel format, but no hw_frames_ctx at all: this must be rejected too, distinct
    // from the wrong-pixel-format case above.
    AVFrameUPtr frame = makeAVFrame();
    frame->format = AV_PIX_FMT_D3D11;
    frame->width = kFrameSize.width();
    frame->height = kFrameSize.height();

    QVERIFY(!converter->createTextureHandles(frame.get(), nullptr));
}

void tst_QFFmpegD3D11TextureConverter::createTextureHandles_roundTripsPixelData_forHwFrame_data()
{
    QTest::addColumn<QVideoFrameFormat::PixelFormat>("pixelFormat");

    QTest::newRow("NV12") << QVideoFrameFormat::Format_NV12;
    QTest::newRow("P010") << QVideoFrameFormat::Format_P010;
    QTest::newRow("BGRA") << QVideoFrameFormat::Format_BGRA8888;
}

// GPU-import coverage across the three shapes a D3D11VA-decoded frame can take on this branch:
// two multi-plane formats (8-bit NV12, 10-bit P010) and a single-plane packed format (BGRA8888).
// D3D11TextureConverter::createTextureHandles() imports the raw D3D11 texture without inspecting
// its pixel format at all, so this exercises that the importer is genuinely format-agnostic, not
// just NV12-shaped.
void tst_QFFmpegD3D11TextureConverter::createTextureHandles_roundTripsPixelData_forHwFrame()
{
    QFETCH(QVideoFrameFormat::PixelFormat, pixelFormat);

    std::unique_ptr<QRhi> rhi = createOffscreenD3D11Rhi();
    if (!rhi)
        QSKIP("Could not create a D3D11 QRhi on this system");

    AVBufferUPtr framesCtx;
    AVFrameUPtr frame;
    QList<QByteArray> planeTexels;

    switch (pixelFormat) {
    case QVideoFrameFormat::Format_NV12: {
        framesCtx = createD3D11FramesContext(m_deviceCtx.get(), kFrameSize, AV_PIX_FMT_NV12);
        QVERIFY(framesCtx);
        frame = createNv12TestFrame(framesCtx.get(), *m_device, kFrameSize, 0x40, 0x60, 0x80);
        planeTexels = { QByteArray(1, char(0x40)), makeBytes({ 0x60, 0x80 }) };
        break;
    }
    case QVideoFrameFormat::Format_P010: {
        framesCtx = createD3D11FramesContext(m_deviceCtx.get(), kFrameSize, AV_PIX_FMT_P010);
        QVERIFY(framesCtx);
        // Chosen so that a byte-swap or a missing "<< 6" shift would produce a different (and
        // thus detectable) byte pattern.
        frame = createP010TestFrame(framesCtx.get(), *m_device, kFrameSize, 0x141, 0x161, 0x181);
        const auto toBytes = [](quint16 value) {
            const quint16 shifted = quint16(value << 6);
            return QByteArray(reinterpret_cast<const char *>(&shifted), sizeof(shifted));
        };
        planeTexels = { toBytes(0x141), toBytes(0x161) + toBytes(0x181) };
        break;
    }
    case QVideoFrameFormat::Format_BGRA8888: {
        framesCtx = createD3D11FramesContext(m_deviceCtx.get(), kFrameSize, AV_PIX_FMT_BGRA);
        QVERIFY(framesCtx);
        const QByteArray texel = makeBytes({ 0x11, 0x22, 0x33, 0x44 });
        frame = createBgraTestFrame(framesCtx.get(), *m_device, kFrameSize, texel);
        planeTexels = { texel };
        break;
    }
    default:
        Q_UNREACHABLE();
    }
    QVERIFY(frame);

    std::shared_ptr<D3D11TextureConverter> converter = D3D11TextureConverter::create(rhi.get());
    QVERIFY(converter);

    QVideoFrameTexturesHandlesUPtr handles = converter->createTextureHandles(frame.get(), nullptr);
    QVERIFY(handles);

    const auto *desc = QVideoTextureHelper::textureDescription(pixelFormat);
    QCOMPARE(desc->nplanes, int(planeTexels.size()));

    QVideoFrameTexturesUPtr textures = QVideoTextureHelper::createTexturesFromHandles(
            std::move(handles), *rhi, pixelFormat, kFrameSize);
    QVERIFY(textures);

    const ComResult<QWindowsD3D11TestDeviceContext> rhiDevice = wrapRhiD3D11Device(*rhi);
    QVERIFY(rhiDevice);

    int rowOffset = 0;
    for (int plane = 0; plane < desc->nplanes; ++plane) {
        QRhiTexture *planeTexture = textures->texture(plane);
        QVERIFY(planeTexture);

        const int width = desc->widthForPlane(kFrameSize.width(), plane);
        const int height = desc->heightForPlane(kFrameSize.height(), plane);
        QCOMPARE(planeTexture->pixelSize(), QSize(width, height));

        // The texture lives on the RHI's own device, not the fixture device, so read it back via
        // a device context wrapping the RHI's device. All planes of a D3D11-imported multi-plane
        // frame share one physical resource, so read starting at this plane's row offset within
        // it.
        const quint64 handle = planeTexture->nativeTexture().object;
        const ComPtr<ID3D11Texture2D> nativeTexture = reinterpret_cast<ID3D11Texture2D *>(handle);
        const ComResult<QByteArray> readback = rhiDevice->readPlaneRows(
                nativeTexture, rowOffset, width, height, planeTexels[plane].size());
        QVERIFY(readback);
        QCOMPARE(*readback, planeTexels[plane].repeated(width * height));

        rowOffset += height;
    }
}

void tst_QFFmpegD3D11TextureConverter::map_returnsRealPixelData_forHwFrame_data()
{
    QTest::addColumn<QVideoFrameFormat::PixelFormat>("pixelFormat");

    QTest::newRow("NV12") << QVideoFrameFormat::Format_NV12;
    QTest::newRow("P010") << QVideoFrameFormat::Format_P010;
    QTest::newRow("BGRA") << QVideoFrameFormat::Format_BGRA8888;
}

// End-to-end coverage for the CPU (QVideoFrame::map()) fallback path on a real D3D11VA hw frame,
// constructed and wrapped exactly as QFFmpeg::VideoRenderer does. map() always goes through
// FFmpeg's own av_hwframe_transfer_data(), independent of GPU texture import, across the same
// three pixel-format shapes exercised above.
void tst_QFFmpegD3D11TextureConverter::map_returnsRealPixelData_forHwFrame()
{
    QFETCH(QVideoFrameFormat::PixelFormat, pixelFormat);

    AVBufferUPtr framesCtx;
    AVFrameUPtr frame;
    QList<QByteArray> planeTexels;

    switch (pixelFormat) {
    case QVideoFrameFormat::Format_NV12: {
        framesCtx = createD3D11FramesContext(m_deviceCtx.get(), kFrameSize, AV_PIX_FMT_NV12);
        QVERIFY(framesCtx);
        frame = createNv12TestFrame(framesCtx.get(), *m_device, kFrameSize, 0x40, 0x60, 0x80);
        planeTexels = { QByteArray(1, char(0x40)), makeBytes({ 0x60, 0x80 }) };
        break;
    }
    case QVideoFrameFormat::Format_P010: {
        framesCtx = createD3D11FramesContext(m_deviceCtx.get(), kFrameSize, AV_PIX_FMT_P010);
        QVERIFY(framesCtx);
        frame = createP010TestFrame(framesCtx.get(), *m_device, kFrameSize, 0x141, 0x161, 0x181);
        const auto toBytes = [](quint16 value) {
            const quint16 shifted = quint16(value << 6);
            return QByteArray(reinterpret_cast<const char *>(&shifted), sizeof(shifted));
        };
        planeTexels = { toBytes(0x141), toBytes(0x161) + toBytes(0x181) };
        break;
    }
    case QVideoFrameFormat::Format_BGRA8888: {
        framesCtx = createD3D11FramesContext(m_deviceCtx.get(), kFrameSize, AV_PIX_FMT_BGRA);
        QVERIFY(framesCtx);
        const QByteArray texel = makeBytes({ 0x11, 0x22, 0x33, 0x44 });
        frame = createBgraTestFrame(framesCtx.get(), *m_device, kFrameSize, texel);
        planeTexels = { texel };
        break;
    }
    default:
        Q_UNREACHABLE();
    }
    QVERIFY(frame);

    const q23::expected<void, QString> result = verifyMappedHwFrame(std::move(frame), planeTexels);
    QVERIFY2(result.has_value(), result ? "" : qPrintable(result.error()));
}

void tst_QFFmpegD3D11TextureConverter::
        createTextureHandles_outlivesSourceFrame_afterFrameContextIsDestroyed()
{
    std::unique_ptr<QRhi> rhi = createOffscreenD3D11Rhi();
    if (!rhi)
        QSKIP("Could not create a D3D11 QRhi on this system");

    constexpr quint8 lumaValue = 90;
    AVFrameUPtr frame =
            createNv12TestFrame(m_framesCtx.get(), *m_device, kFrameSize, lumaValue, 128, 128);
    QVERIFY(frame);

    std::shared_ptr<D3D11TextureConverter> converter = D3D11TextureConverter::create(rhi.get());
    QVERIFY(converter);

    QVideoFrameTexturesHandlesUPtr handles = converter->createTextureHandles(frame.get(), nullptr);
    QVERIFY(handles);

    const quint64 lumaHandle = handles->textureHandle(*rhi, 0);
    QVERIFY(lumaHandle != 0);

    // The output texture is the bridge's own copy; it must remain valid after the source frame
    // (and its pooled hw texture) is released.
    frame.reset();
    m_framesCtx.reset();
    m_deviceCtx.reset();

    const ComResult<QWindowsD3D11TestDeviceContext> rhiDevice = wrapRhiD3D11Device(*rhi);
    QVERIFY(rhiDevice);

    const ComPtr<ID3D11Texture2D> nativeTexture = reinterpret_cast<ID3D11Texture2D *>(lumaHandle);
    const ComResult<QColor> actualColor = rhiDevice->getFirstPixelColor(nativeTexture);
    QVERIFY(actualColor);
    QCOMPARE(actualColor->red(), lumaValue);
}

void tst_QFFmpegD3D11TextureConverter::createTextureHandles_acceptsOldHandles_onSecondCall()
{
    std::unique_ptr<QRhi> rhi = createOffscreenD3D11Rhi();
    if (!rhi)
        QSKIP("Could not create a D3D11 QRhi on this system");

    AVFrameUPtr firstFrame =
            createNv12TestFrame(m_framesCtx.get(), *m_device, kFrameSize, 40, 128, 128);
    QVERIFY(firstFrame);

    std::shared_ptr<D3D11TextureConverter> converter = D3D11TextureConverter::create(rhi.get());
    QVERIFY(converter);

    QVideoFrameTexturesHandlesUPtr firstHandles =
            converter->createTextureHandles(firstFrame.get(), nullptr);
    QVERIFY(firstHandles);

    constexpr quint8 secondLumaValue = 210;
    AVFrameUPtr secondFrame = createNv12TestFrame(m_framesCtx.get(), *m_device, kFrameSize,
                                                  secondLumaValue, 128, 128);
    QVERIFY(secondFrame);

    QVideoFrameTexturesHandlesUPtr secondHandles =
            converter->createTextureHandles(secondFrame.get(), std::move(firstHandles));
    QVERIFY(secondHandles);

    const quint64 secondLumaHandle = secondHandles->textureHandle(*rhi, 0);
    QVERIFY(secondLumaHandle != 0);

    const ComResult<QWindowsD3D11TestDeviceContext> rhiDevice = wrapRhiD3D11Device(*rhi);
    QVERIFY(rhiDevice);

    const ComPtr<ID3D11Texture2D> nativeTexture =
            reinterpret_cast<ID3D11Texture2D *>(secondLumaHandle);
    const ComResult<QColor> actualColor = rhiDevice->getFirstPixelColor(nativeTexture);
    QVERIFY(actualColor);
    QCOMPARE(actualColor->red(), secondLumaValue);
}

QTEST_MAIN(tst_QFFmpegD3D11TextureConverter)

#include "tst_qffmpegd3d11textureconverter.moc"
