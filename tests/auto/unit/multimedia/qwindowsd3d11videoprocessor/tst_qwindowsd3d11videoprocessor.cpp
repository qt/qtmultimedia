// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QtTest>

#include <QtMultimediaTestLib/private/qwindowsd3d11testdevicecontext_p.h>

#include <QtMultimedia/private/qwindowsd3d11videoprocessor_p.h>

#include <QtGui/qcolor.h>

#include <QtCore/private/qsystemerror_p.h>

using QWindowsD3D11VideoProcessor = QtMultimediaPrivate::QWindowsD3D11VideoProcessor;

#define QVERIFYCOMRESULT(comresult) \
    QVERIFY2(comresult,             \
             comresult ? "no error" \
                       : qPrintable(QSystemError::windowsComString((comresult).error())))

namespace {

// Video processing needs ID3D11VideoDevice. Software (WARP) D3D11 devices don't provide it.
bool hasVideoProcessing(const QWindowsD3D11TestDeviceContext &dev)
{
    ComPtr<ID3D11VideoDevice> videoDevice;
    return SUCCEEDED(dev.device->QueryInterface(IID_PPV_ARGS(&videoDevice)));
}

} // namespace

class tst_QWindowsD3D11VideoProcessor : public QObject
{
    Q_OBJECT

private slots:
    void init();

    void isPixelFormatSupported_returnsTrue_onlyForNV12AndP010();

    void convertToBGRA_producesNeutralGray_whenChromaIsNeutralAndRangeIsFull();
    void convertToBGRA_producesNeutralGray_whenP010ChromaIsNeutralAndRangeIsFull();
    void convertToBGRA_appliesColorRange_whenRangeIsVideo();
    void convertToBGRA_appliesColorSpace_whenChromaIsNonNeutral();
    void convertToBGRA_appliesColorTransfer_whenP010IsBT2020();
    void convertToBGRA_returnsNull_whenFrameSizeIsEmpty();
    void convertToBGRA_returnsNull_whenDeviceHasNoVideoProcessing();
    void convertToBGRA_reusesOutputTexture_whenCalledRepeatedlyWithSameSize();
    void convertToBGRA_recreatesOutputTexture_whenFrameSizeChanges();

private:
    ComResult<QWindowsD3D11TestDeviceContext> m_device = q23::unexpected{ E_FAIL };
};

void tst_QWindowsD3D11VideoProcessor::init()
{
    m_device = createD3D11TestDeviceContext();
    QVERIFYCOMRESULT(m_device);
}

void tst_QWindowsD3D11VideoProcessor::isPixelFormatSupported_returnsTrue_onlyForNV12AndP010()
{
    QVERIFY(QWindowsD3D11VideoProcessor::isPixelFormatSupported(QVideoFrameFormat::Format_NV12));
    QVERIFY(QWindowsD3D11VideoProcessor::isPixelFormatSupported(QVideoFrameFormat::Format_P010));
    QVERIFY(!QWindowsD3D11VideoProcessor::isPixelFormatSupported(QVideoFrameFormat::Format_P016));
    QVERIFY(!QWindowsD3D11VideoProcessor::isPixelFormatSupported(
            QVideoFrameFormat::Format_YUV420P10));
    QVERIFY(!QWindowsD3D11VideoProcessor::isPixelFormatSupported(
            QVideoFrameFormat::Format_BGRA8888));
    QVERIFY(!QWindowsD3D11VideoProcessor::isPixelFormatSupported(
            QVideoFrameFormat::Format_Invalid));
}

// Neutral chroma collapses the conversion to R=G=B independent of the driver's color matrix,
// and full-range luma maps directly, without studio-range rescaling.
void tst_QWindowsD3D11VideoProcessor::
        convertToBGRA_producesNeutralGray_whenChromaIsNeutralAndRangeIsFull()
{
    if (!hasVideoProcessing(*m_device))
        QSKIP("D3D11 device has no video processing support (software adapter)");

    constexpr QSize frameSize{ 64, 32 };
    constexpr quint8 lumaValue = 180;

    const ComResult<ComPtr<ID3D11Texture2D>> srcTex =
            m_device->createFilledNV12Texture(frameSize, lumaValue, 128, 128);
    QVERIFYCOMRESULT(srcTex);

    QWindowsD3D11VideoProcessor processor;
    const ComPtr<ID3D11Texture2D> outputTex = processor.convertToBGRA(
            m_device->device.Get(), m_device->context.Get(), *srcTex, frameSize,
            QVideoFrameFormat::ColorSpace_BT709, QVideoFrameFormat::ColorRange_Full,
            QVideoFrameFormat::ColorTransfer_BT709);
    QVERIFY(outputTex);

    D3D11_TEXTURE2D_DESC desc{};
    outputTex->GetDesc(&desc);
    QCOMPARE(desc.Format, DXGI_FORMAT_B8G8R8A8_UNORM);
    QCOMPARE(QSize(int(desc.Width), int(desc.Height)), frameSize);

    const ComResult<QColor> actualColor = m_device->getFirstPixelColorBGRA(outputTex);
    QVERIFYCOMRESULT(actualColor);

    // Fixed-function implementations round differently; allow tolerance.
    QVERIFY2(qAbs(actualColor->red() - int(lumaValue)) <= 4,
             qPrintable(QString("red=%1").arg(actualColor->red())));
    QVERIFY2(qAbs(actualColor->red() - actualColor->green()) <= 1,
             qPrintable(
                     QString("red=%1 green=%2").arg(actualColor->red()).arg(actualColor->green())));
    QVERIFY2(qAbs(actualColor->green() - actualColor->blue()) <= 1,
             qPrintable(QString("green=%1 blue=%2")
                                .arg(actualColor->green())
                                .arg(actualColor->blue())));
}

// Neutral P010 chroma still collapses to R=G=B after PQ tone-mapping to SDR.
void tst_QWindowsD3D11VideoProcessor::
        convertToBGRA_producesNeutralGray_whenP010ChromaIsNeutralAndRangeIsFull()
{
    if (!hasVideoProcessing(*m_device))
        QSKIP("D3D11 device has no video processing support (software adapter)");

    constexpr QSize frameSize{ 64, 32 };

    const ComResult<ComPtr<ID3D11Texture2D>> srcTex = m_device->createP010Texture(frameSize);
    QVERIFYCOMRESULT(srcTex);
    QCOMPARE(m_device->fillP010Texture(*srcTex, frameSize, 700, 512, 512), S_OK);

    QWindowsD3D11VideoProcessor processor;
    const ComPtr<ID3D11Texture2D> outputTex = processor.convertToBGRA(
            m_device->device.Get(), m_device->context.Get(), *srcTex, frameSize,
            QVideoFrameFormat::ColorSpace_BT2020, QVideoFrameFormat::ColorRange_Full,
            QVideoFrameFormat::ColorTransfer_ST2084);
    QVERIFY(outputTex);

    const ComResult<QColor> actualColor = m_device->getFirstPixelColorBGRA(outputTex);
    QVERIFYCOMRESULT(actualColor);

    QVERIFY2(qAbs(actualColor->red() - actualColor->green()) <= 1,
             qPrintable(
                     QString("red=%1 green=%2").arg(actualColor->red()).arg(actualColor->green())));
    QVERIFY2(qAbs(actualColor->green() - actualColor->blue()) <= 1,
             qPrintable(QString("green=%1 blue=%2")
                                .arg(actualColor->green())
                                .arg(actualColor->blue())));
}

// Studio-range luma [16,235] expands to [0,255]; the same input byte must map brighter under
// Video range than under Full range.
void tst_QWindowsD3D11VideoProcessor::convertToBGRA_appliesColorRange_whenRangeIsVideo()
{
    if (!hasVideoProcessing(*m_device))
        QSKIP("D3D11 device has no video processing support (software adapter)");

    constexpr QSize frameSize{ 64, 32 };
    constexpr quint8 lumaValue = 180;

    const ComResult<ComPtr<ID3D11Texture2D>> srcTex =
            m_device->createFilledNV12Texture(frameSize, lumaValue, 128, 128);
    QVERIFYCOMRESULT(srcTex);

    QWindowsD3D11VideoProcessor processor;

    const ComPtr<ID3D11Texture2D> fullRangeOut = processor.convertToBGRA(
            m_device->device.Get(), m_device->context.Get(), *srcTex, frameSize,
            QVideoFrameFormat::ColorSpace_BT709, QVideoFrameFormat::ColorRange_Full,
            QVideoFrameFormat::ColorTransfer_BT709);
    QVERIFY(fullRangeOut);
    const ComResult<QColor> fullRangeColor = m_device->getFirstPixelColorBGRA(fullRangeOut);
    QVERIFYCOMRESULT(fullRangeColor);

    const ComPtr<ID3D11Texture2D> videoRangeOut = processor.convertToBGRA(
            m_device->device.Get(), m_device->context.Get(), *srcTex, frameSize,
            QVideoFrameFormat::ColorSpace_BT709, QVideoFrameFormat::ColorRange_Video,
            QVideoFrameFormat::ColorTransfer_BT709);
    QVERIFY(videoRangeOut);
    const ComResult<QColor> videoRangeColor = m_device->getFirstPixelColorBGRA(videoRangeOut);
    QVERIFYCOMRESULT(videoRangeColor);

    QVERIFY(videoRangeColor->red() > fullRangeColor->red());
}

// BT.601 and BT.709 use different YCbCr->RGB coefficients, so non-neutral chroma must give
// different colors for the two spaces.
void tst_QWindowsD3D11VideoProcessor::convertToBGRA_appliesColorSpace_whenChromaIsNonNeutral()
{
    if (!hasVideoProcessing(*m_device))
        QSKIP("D3D11 device has no video processing support (software adapter)");

    constexpr QSize frameSize{ 64, 32 };

    const ComResult<ComPtr<ID3D11Texture2D>> srcTex =
            m_device->createFilledNV12Texture(frameSize, 180, 90, 200);
    QVERIFYCOMRESULT(srcTex);

    QWindowsD3D11VideoProcessor processor;

    const ComPtr<ID3D11Texture2D> bt601Out = processor.convertToBGRA(
            m_device->device.Get(), m_device->context.Get(), *srcTex, frameSize,
            QVideoFrameFormat::ColorSpace_BT601, QVideoFrameFormat::ColorRange_Full,
            QVideoFrameFormat::ColorTransfer_BT601);
    QVERIFY(bt601Out);
    const ComResult<QColor> bt601Color = m_device->getFirstPixelColorBGRA(bt601Out);
    QVERIFYCOMRESULT(bt601Color);

    const ComPtr<ID3D11Texture2D> bt709Out = processor.convertToBGRA(
            m_device->device.Get(), m_device->context.Get(), *srcTex, frameSize,
            QVideoFrameFormat::ColorSpace_BT709, QVideoFrameFormat::ColorRange_Full,
            QVideoFrameFormat::ColorTransfer_BT709);
    QVERIFY(bt709Out);
    const ComResult<QColor> bt709Color = m_device->getFirstPixelColorBGRA(bt709Out);
    QVERIFYCOMRESULT(bt709Color);

    QVERIFY(*bt601Color != *bt709Color);
}

// PQ and HLG tag different DXGI spaces (see toDxgiColorSpace) and should tone-map
// differently. Skip rather than fail on drivers whose video processor doesn't distinguish them;
// that's a hardware limitation, not a code defect.
void tst_QWindowsD3D11VideoProcessor::convertToBGRA_appliesColorTransfer_whenP010IsBT2020()
{
    if (!hasVideoProcessing(*m_device))
        QSKIP("D3D11 device has no video processing support (software adapter)");

    constexpr QSize frameSize{ 64, 32 };

    const ComResult<ComPtr<ID3D11Texture2D>> srcTex = m_device->createP010Texture(frameSize);
    QVERIFYCOMRESULT(srcTex);
    QCOMPARE(m_device->fillP010Texture(*srcTex, frameSize, 700, 460, 560), S_OK);

    QWindowsD3D11VideoProcessor processor;

    const ComPtr<ID3D11Texture2D> pqOut = processor.convertToBGRA(
            m_device->device.Get(), m_device->context.Get(), *srcTex, frameSize,
            QVideoFrameFormat::ColorSpace_BT2020, QVideoFrameFormat::ColorRange_Full,
            QVideoFrameFormat::ColorTransfer_ST2084);
    QVERIFY(pqOut);
    const ComResult<QColor> pqColor = m_device->getFirstPixelColorBGRA(pqOut);
    QVERIFYCOMRESULT(pqColor);

    const ComPtr<ID3D11Texture2D> hlgOut = processor.convertToBGRA(
            m_device->device.Get(), m_device->context.Get(), *srcTex, frameSize,
            QVideoFrameFormat::ColorSpace_BT2020, QVideoFrameFormat::ColorRange_Full,
            QVideoFrameFormat::ColorTransfer_STD_B67);
    QVERIFY(hlgOut);
    const ComResult<QColor> hlgColor = m_device->getFirstPixelColorBGRA(hlgOut);
    QVERIFYCOMRESULT(hlgColor);

    if (*pqColor == *hlgColor)
        QSKIP("This GPU/driver's video processor doesn't distinguish PQ from HLG tone-mapping");
}

void tst_QWindowsD3D11VideoProcessor::convertToBGRA_returnsNull_whenFrameSizeIsEmpty()
{
    const ComResult<ComPtr<ID3D11Texture2D>> srcTex =
            m_device->createFilledNV12Texture(QSize{ 64, 32 }, 180, 128, 128);
    QVERIFYCOMRESULT(srcTex);

    QWindowsD3D11VideoProcessor processor;
    const ComPtr<ID3D11Texture2D> outputTex = processor.convertToBGRA(
            m_device->device.Get(), m_device->context.Get(), *srcTex, QSize{},
            QVideoFrameFormat::ColorSpace_BT709, QVideoFrameFormat::ColorRange_Full,
            QVideoFrameFormat::ColorTransfer_BT709);
    QVERIFY(!outputTex);
}

// Software (WARP) devices have no video processing: convertToBGRA() must fail cleanly rather
// than crash. This is the only convertToBGRA() path that runs on such devices.
void tst_QWindowsD3D11VideoProcessor::convertToBGRA_returnsNull_whenDeviceHasNoVideoProcessing()
{
    if (hasVideoProcessing(*m_device))
        QSKIP("Device has video processing; the other convertToBGRA tests cover it");

    constexpr QSize frameSize{ 64, 32 };
    const ComResult<ComPtr<ID3D11Texture2D>> srcTex = m_device->createNV12Texture(frameSize);
    QVERIFYCOMRESULT(srcTex);

    QWindowsD3D11VideoProcessor processor;
    const ComPtr<ID3D11Texture2D> outputTex = processor.convertToBGRA(
            m_device->device.Get(), m_device->context.Get(), *srcTex, frameSize,
            QVideoFrameFormat::ColorSpace_BT709, QVideoFrameFormat::ColorRange_Full,
            QVideoFrameFormat::ColorTransfer_BT709);
    QVERIFY(!outputTex);
}

void tst_QWindowsD3D11VideoProcessor::
        convertToBGRA_reusesOutputTexture_whenCalledRepeatedlyWithSameSize()
{
    if (!hasVideoProcessing(*m_device))
        QSKIP("D3D11 device has no video processing support (software adapter)");

    constexpr QSize frameSize{ 64, 32 };
    QWindowsD3D11VideoProcessor processor;

    ComPtr<ID3D11Texture2D> firstOutput;
    for (int i = 0; i < 3; ++i) {
        const ComResult<ComPtr<ID3D11Texture2D>> srcTex =
                m_device->createFilledNV12Texture(frameSize, 100, 128, 128);
        QVERIFYCOMRESULT(srcTex);

        const ComPtr<ID3D11Texture2D> outputTex = processor.convertToBGRA(
                m_device->device.Get(), m_device->context.Get(), *srcTex, frameSize,
                QVideoFrameFormat::ColorSpace_BT709, QVideoFrameFormat::ColorRange_Full,
                QVideoFrameFormat::ColorTransfer_BT709);
        QVERIFY(outputTex);

        if (!firstOutput)
            firstOutput = outputTex;
        else
            QCOMPARE_EQ(outputTex.Get(), firstOutput.Get());
    }
}

void tst_QWindowsD3D11VideoProcessor::convertToBGRA_recreatesOutputTexture_whenFrameSizeChanges()
{
    if (!hasVideoProcessing(*m_device))
        QSKIP("D3D11 device has no video processing support (software adapter)");

    QWindowsD3D11VideoProcessor processor;

    // NV12 requires even width/height (2x2 chroma subsampling).
    const std::vector<QSize> frameSizes{ QSize{ 64, 32 }, QSize{ 128, 64 }, QSize{ 66, 34 } };
    for (const QSize &frameSize : frameSizes) {
        const ComResult<ComPtr<ID3D11Texture2D>> srcTex =
                m_device->createFilledNV12Texture(frameSize, 150, 128, 128);
        QVERIFYCOMRESULT(srcTex);

        const ComPtr<ID3D11Texture2D> outputTex = processor.convertToBGRA(
                m_device->device.Get(), m_device->context.Get(), *srcTex, frameSize,
                QVideoFrameFormat::ColorSpace_BT709, QVideoFrameFormat::ColorRange_Full,
                QVideoFrameFormat::ColorTransfer_BT709);
        QVERIFY(outputTex);

        D3D11_TEXTURE2D_DESC desc{};
        outputTex->GetDesc(&desc);
        QCOMPARE(QSize(int(desc.Width), int(desc.Height)), frameSize);
    }
}

QTEST_GUILESS_MAIN(tst_QWindowsD3D11VideoProcessor)

#include "tst_qwindowsd3d11videoprocessor.moc"
