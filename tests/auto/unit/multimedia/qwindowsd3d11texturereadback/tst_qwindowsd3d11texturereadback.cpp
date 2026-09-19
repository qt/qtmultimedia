// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QtTest>

#include <QtMultimediaTestLib/private/qwindowsd3d11testdevicecontext_p.h>

#include <QtMultimedia/qvideoframeformat.h>
#include <QtMultimedia/private/qwindowsd3d11texturereadback_p.h>

#include <QtGui/qcolor.h>

#include <QtCore/private/qsystemerror_p.h>

using namespace QtMultimediaPrivate;

// Helper macro to verify q23::expected<T, HRESULT>
#define QVERIFYCOMRESULT(comresult) \
    QVERIFY2(comresult,             \
             comresult ? "no error" \
                       : qPrintable(QSystemError::windowsComString((comresult).error())))

class tst_QWindowsD3D11TextureReadback : public QObject
{
    Q_OBJECT

private slots:
    void init();

    void map_returnsPixelData_forRgbaTexture();
    void map_returnsPixelData_forPlanarTexture_data();
    void map_returnsPixelData_forPlanarTexture();
    void map_returnsEmpty_whenAlreadyMapped();
    void map_succeedsAgain_afterUnmap();
    void unmap_isSafeToCall_whenNotMapped();

private:
    ComResult<QWindowsD3D11TestDeviceContext> m_device = q23::unexpected{ E_FAIL };
};

void tst_QWindowsD3D11TextureReadback::init()
{
    m_device = createD3D11TestDeviceContext();
    QVERIFYCOMRESULT(m_device);
}

void tst_QWindowsD3D11TextureReadback::map_returnsPixelData_forRgbaTexture()
{
    constexpr QSize frameSize{ 64, 32 };
    const ComResult<ComPtr<ID3D11Texture2D>> texture =
            m_device->createTextureArray(frameSize, { Qt::red });
    QVERIFYCOMRESULT(texture);

    QWindowsD3D11TextureReadback readback;
    const QAbstractVideoBuffer::MapData data = readback.map(m_device->device, *texture, frameSize);

    QCOMPARE(data.planeCount, 1);
    QVERIFY(data.data[0]);
    QVERIFY(data.bytesPerLine[0] >= frameSize.width() * 4);
    QVERIFY(data.dataSize[0] >= data.bytesPerLine[0] * frameSize.height());

    // Verify the mapped bytes match the texture's actual (red) contents.
    const unsigned char *pixel = data.data[0];
    QCOMPARE(pixel[0], 0xff); // R
    QCOMPARE(pixel[1], 0x00); // G
    QCOMPARE(pixel[2], 0x00); // B
    QCOMPARE(pixel[3], 0xff); // A

    readback.unmap();
}

void tst_QWindowsD3D11TextureReadback::map_returnsPixelData_forPlanarTexture_data()
{
    QTest::addColumn<QVideoFrameFormat::PixelFormat>("pixelFormat");

    QTest::newRow("NV12") << QVideoFrameFormat::Format_NV12;
    QTest::newRow("P010") << QVideoFrameFormat::Format_P010;
}

void tst_QWindowsD3D11TextureReadback::map_returnsPixelData_forPlanarTexture()
{
    // QWindowsD3D11TextureReadback doesn't special-case pixel format: verify the single-plane
    // raw-byte readback also works for semi-planar 8-bit (NV12) and 10-bit (P010) formats, not
    // just packed RGBA.
    QFETCH(QVideoFrameFormat::PixelFormat, pixelFormat);

    constexpr QSize frameSize{ 64, 32 };
    int bytesPerSample = 0;

    ComResult<ComPtr<ID3D11Texture2D>> texture = q23::unexpected{ E_FAIL };
    switch (pixelFormat) {
    case QVideoFrameFormat::Format_NV12: {
        bytesPerSample = 1;
        texture = m_device->createNV12Texture(frameSize);
        QVERIFYCOMRESULT(texture);
        auto result = m_device->fillNV12Texture(*texture, frameSize, 0x40, 0x60, 0x80);
        if (FAILED(result))
            QSKIP("Cannot fill NV12 Texture");
        break;
    }
    case QVideoFrameFormat::Format_P010: {
        bytesPerSample = 2;
        if (!m_device->isTextureFormatSupported(DXGI_FORMAT_P010))
            QSKIP("P010 Texture2D not supported by D3D11 device");
        texture = m_device->createP010Texture(frameSize);
        QVERIFYCOMRESULT(texture);
        auto result = m_device->fillP010Texture(*texture, frameSize, 0x100, 0x180, 0x200);
        if (FAILED(result))
            QSKIP("Cannot fill P010 Texture");
        break;
    }
    default:
        QFAIL("unexpected pixel format");
    }

    QWindowsD3D11TextureReadback readback;
    const QAbstractVideoBuffer::MapData data = readback.map(m_device->device, *texture, frameSize);

    QCOMPARE(data.planeCount, 1);
    QVERIFY(data.data[0]);
    QVERIFY(data.bytesPerLine[0] >= frameSize.width() * bytesPerSample);

    // First sample of the mapped region is the top-left luma sample.
    switch (pixelFormat) {
    case QVideoFrameFormat::Format_NV12:
        QCOMPARE(data.data[0][0], quint8(0x40));
        break;
    case QVideoFrameFormat::Format_P010:
        QCOMPARE(*reinterpret_cast<const quint16 *>(data.data[0]), quint16(0x100 << 6));
        break;
    default:
        QFAIL("unexpected pixel format");
    }

    readback.unmap();
}

void tst_QWindowsD3D11TextureReadback::map_returnsEmpty_whenAlreadyMapped()
{
    constexpr QSize frameSize{ 64, 32 };
    const ComResult<ComPtr<ID3D11Texture2D>> texture =
            m_device->createTextureArray(frameSize, { Qt::blue });
    QVERIFYCOMRESULT(texture);

    QWindowsD3D11TextureReadback readback;
    const QAbstractVideoBuffer::MapData firstMap =
            readback.map(m_device->device, *texture, frameSize);
    QCOMPARE(firstMap.planeCount, 1);

    // Only one mapping may be active at a time.
    const QAbstractVideoBuffer::MapData secondMap =
            readback.map(m_device->device, *texture, frameSize);
    QCOMPARE(secondMap.planeCount, 0);

    readback.unmap();
}

void tst_QWindowsD3D11TextureReadback::map_succeedsAgain_afterUnmap()
{
    constexpr QSize frameSize{ 64, 32 };
    QWindowsD3D11TextureReadback readback;

    const std::vector<QColor> colors{ Qt::green, Qt::yellow };
    for (const QColor &color : colors) {
        const ComResult<ComPtr<ID3D11Texture2D>> texture =
                m_device->createTextureArray(frameSize, { color });
        QVERIFYCOMRESULT(texture);

        const QAbstractVideoBuffer::MapData data =
                readback.map(m_device->device, *texture, frameSize);
        QCOMPARE(data.planeCount, 1);
        QVERIFY(data.data[0]);

        readback.unmap();
    }
}

void tst_QWindowsD3D11TextureReadback::unmap_isSafeToCall_whenNotMapped()
{
    QWindowsD3D11TextureReadback readback;
    readback.unmap(); // must not crash
}

QTEST_MAIN(tst_QWindowsD3D11TextureReadback)

#include "tst_qwindowsd3d11texturereadback.moc"
