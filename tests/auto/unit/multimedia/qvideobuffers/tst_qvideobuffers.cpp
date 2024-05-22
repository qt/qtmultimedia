// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QtTest>

#include <private/qmemoryvideobuffer_p.h>
#include <private/qimagevideobuffer_p.h>
#include "qvideoframeformat.h"

using BufferPtr = std::shared_ptr<QAbstractVideoBuffer>;
using MapModes = std::vector<QVideoFrame::MapMode>;

static const MapModes validMapModes = { QVideoFrame::ReadOnly, QVideoFrame::WriteOnly, QVideoFrame::ReadWrite };

class tst_QVideoBuffers : public QObject
{
    Q_OBJECT

public:
    tst_QVideoBuffers() {}
public slots:
    void initTestCase();

private slots:
    void map_changesMappedStateAndReturnsProperMappings_whenBufferIsNotMapped_data();
    void map_changesMappedStateAndReturnsProperMappings_whenBufferIsNotMapped();

    void map_doesNothing_whenBufferIsMapped_data();
    void map_doesNothing_whenBufferIsMapped();

    void mapMemoryBufferWithReadOnly_doesntDetachArray();

    void unmap_resetsMappedState_whenBufferIsMapped_data();
    void unmap_resetsMappedState_whenBufferIsMapped();

    void imageBuffer_fixesInputImage_data();
    void imageBuffer_fixesInputImage();

private:
    QString mapModeToString(QVideoFrame::MapMode mapMode) const
    {
        switch (mapMode) {
            case QVideoFrame::NotMapped:
                return QLatin1String("NotMapped");
            case QVideoFrame::ReadOnly:
                return QLatin1String("ReadOnly");
            case QVideoFrame::WriteOnly:
                return QLatin1String("WriteOnly");
            case QVideoFrame::ReadWrite:
                return QLatin1String("ReadWrite");
            default:
                return QLatin1String("Unknown");
        }
    }

    void generateImageAndMemoryBuffersWithAllModes(const MapModes& modes = validMapModes) const
    {
        QTest::addColumn<BufferPtr>("buffer");
        QTest::addColumn<QVideoFrame::MapMode>("mapMode");

        for (auto mode : modes) {
            QTest::newRow(QStringLiteral("ImageBuffer, %1").arg(mapModeToString(mode)).toUtf8().constData())
                    << createImageBuffer() << mode;
            QTest::newRow(QStringLiteral("MemoryBuffer, %1").arg(mapModeToString(mode)).toUtf8().constData())
                    << createMemoryBuffer() << mode;
        }
    }

    BufferPtr createImageBuffer() const
    {
        return std::make_shared<QImageVideoBuffer>(m_image);
    }

    BufferPtr createMemoryBuffer() const
    {
        return std::make_shared<QMemoryVideoBuffer>(m_byteArray, m_byteArray.size() / m_image.height());
    }

    QImage m_image = { QSize(5, 4), QImage::Format_RGBA8888 };
    QByteArray m_byteArray;
};


void tst_QVideoBuffers::initTestCase()
{
    m_image.fill(Qt::gray);
    m_image.setPixelColor(0, 0, Qt::green);
    m_image.setPixelColor(m_image.width() - 1, 0, Qt::blue);
    m_image.setPixelColor(0, m_image.height() - 1, Qt::red);

    m_byteArray.assign(m_image.constBits(), m_image.constBits() + m_image.sizeInBytes());
}

void tst_QVideoBuffers::map_changesMappedStateAndReturnsProperMappings_whenBufferIsNotMapped_data()
{
    generateImageAndMemoryBuffersWithAllModes();
}

void tst_QVideoBuffers::map_changesMappedStateAndReturnsProperMappings_whenBufferIsNotMapped()
{
    QFETCH(BufferPtr, buffer);
    QFETCH(QVideoFrame::MapMode, mapMode);

    auto mappedData = buffer->map(mapMode);

    QCOMPARE(mappedData.nPlanes, 1);
    QVERIFY(mappedData.data[0]);
    QCOMPARE(mappedData.size[0], 80);
    QCOMPARE(mappedData.bytesPerLine[0], 20);

    const auto data = reinterpret_cast<const char*>(mappedData.data[0]);
    QCOMPARE(QByteArray(data, mappedData.size[0]), m_byteArray);
}

void tst_QVideoBuffers::map_doesNothing_whenBufferIsMapped_data()
{
    generateImageAndMemoryBuffersWithAllModes();
}

void tst_QVideoBuffers::map_doesNothing_whenBufferIsMapped()
{
    QFETCH(BufferPtr, buffer);
    QFETCH(QVideoFrame::MapMode, mapMode);

    buffer->map(mapMode);
    auto mappedData = buffer->map(QVideoFrame::ReadOnly);
    QCOMPARE(mappedData.nPlanes, 0);
}

void tst_QVideoBuffers::mapMemoryBufferWithReadOnly_doesntDetachArray()
{
    auto buffer = createMemoryBuffer();

    auto mappedData = buffer->map(QVideoFrame::ReadOnly);
    QCOMPARE(mappedData.nPlanes, 1);
    QCOMPARE(mappedData.data[0], reinterpret_cast<const uchar *>(m_byteArray.constData()));
}

void tst_QVideoBuffers::unmap_resetsMappedState_whenBufferIsMapped_data()
{
    generateImageAndMemoryBuffersWithAllModes();
}

void tst_QVideoBuffers::unmap_resetsMappedState_whenBufferIsMapped()
{
    QFETCH(BufferPtr, buffer);
    QFETCH(QVideoFrame::MapMode, mapMode);

    buffer->map(mapMode);

    buffer->unmap();

    // Check buffer is valid and it's possible to map again
    auto mappedData = buffer->map(QVideoFrame::ReadOnly);
    QCOMPARE(mappedData.nPlanes, 1);

    const auto data = reinterpret_cast<const char*>(mappedData.data[0]);
    QCOMPARE(QByteArray(data, mappedData.size[0]), m_byteArray);
}

void tst_QVideoBuffers::imageBuffer_fixesInputImage_data()
{
    QTest::addColumn<QImage::Format>("inputImageFormat");
    QTest::addColumn<QImage::Format>("underlyingImageFormat");

    QTest::newRow("Format_RGB32 => Format_RGB32") << QImage::Format_RGB32 << QImage::Format_RGB32;
    QTest::newRow("Format_ARGB32 => Format_ARGB32")
            << QImage::Format_ARGB32 << QImage::Format_ARGB32;
    QTest::newRow("Format_ARGB32_Premultiplied => Format_ARGB32_Premultiplied")
            << QImage::Format_ARGB32_Premultiplied << QImage::Format_ARGB32_Premultiplied;
    QTest::newRow("Format_RGBA8888 => Format_RGBA8888")
            << QImage::Format_RGBA8888 << QImage::Format_RGBA8888;
    QTest::newRow("Format_RGBA8888_Premultiplied => Format_RGBA8888_Premultiplied")
            << QImage::Format_RGBA8888_Premultiplied << QImage::Format_RGBA8888_Premultiplied;
    QTest::newRow("Format_RGBX8888 => Format_RGBX8888")
            << QImage::Format_RGBX8888 << QImage::Format_RGBX8888;
    QTest::newRow("Format_Grayscale8 => Format_Grayscale8")
            << QImage::Format_Grayscale8 << QImage::Format_Grayscale8;
    QTest::newRow("Format_Grayscale16 => Format_Grayscale16")
            << QImage::Format_Grayscale16 << QImage::Format_Grayscale16;

    QTest::newRow("Format_ARGB8565_Premultiplied => Format_ARGB32_Premultiplied")
            << QImage::Format_ARGB8565_Premultiplied << QImage::Format_ARGB32_Premultiplied;
    QTest::newRow("Format_ARGB6666_Premultiplied => Format_ARGB32_Premultiplied")
            << QImage::Format_ARGB6666_Premultiplied << QImage::Format_ARGB32_Premultiplied;
    QTest::newRow("Format_ARGB8555_Premultiplied => Format_ARGB32_Premultiplied")
            << QImage::Format_ARGB8555_Premultiplied << QImage::Format_ARGB32_Premultiplied;
    QTest::newRow("Format_ARGB4444_Premultiplied => Format_ARGB32_Premultiplied")
            << QImage::Format_ARGB4444_Premultiplied << QImage::Format_ARGB32_Premultiplied;
    QTest::newRow("Format_A2BGR30_Premultiplied => Format_ARGB32_Premultiplied")
            << QImage::Format_A2BGR30_Premultiplied << QImage::Format_ARGB32_Premultiplied;
    QTest::newRow("Format_A2RGB30_Premultiplied => Format_ARGB32_Premultiplied")
            << QImage::Format_A2RGB30_Premultiplied << QImage::Format_ARGB32_Premultiplied;
    QTest::newRow("Format_RGBA64_Premultiplied => Format_ARGB32_Premultiplied")
            << QImage::Format_RGBA64_Premultiplied << QImage::Format_ARGB32_Premultiplied;
    QTest::newRow("Format_RGBA16FPx4_Premultiplied => Format_ARGB32_Premultiplied")
            << QImage::Format_RGBA16FPx4_Premultiplied << QImage::Format_ARGB32_Premultiplied;
    QTest::newRow("Format_RGBA32FPx4_Premultiplied => Format_ARGB32_Premultiplied")
            << QImage::Format_RGBA32FPx4_Premultiplied << QImage::Format_ARGB32_Premultiplied;

    QTest::newRow("Format_Alpha8 => Format_ARGB32")
            << QImage::Format_Alpha8 << QImage::Format_ARGB32;
    QTest::newRow("Format_RGBA64 => Format_ARGB32")
            << QImage::Format_RGBA64 << QImage::Format_ARGB32;
    QTest::newRow("Format_RGBA16FPx4 => Format_ARGB32")
            << QImage::Format_RGBA16FPx4 << QImage::Format_ARGB32;
    QTest::newRow("Format_RGBA32FPx4 => Format_ARGB32")
            << QImage::Format_RGBA32FPx4 << QImage::Format_ARGB32;
}

void tst_QVideoBuffers::imageBuffer_fixesInputImage()
{
    QFETCH(QImage::Format, inputImageFormat);
    QFETCH(QImage::Format, underlyingImageFormat);

    m_image.convertTo(inputImageFormat);
    QImageVideoBuffer buffer(m_image);

    auto underlyingImage = buffer.underlyingImage();

    QCOMPARE(underlyingImage.format(), underlyingImageFormat);
    QCOMPARE_NE(QVideoFrameFormat::pixelFormatFromImageFormat(underlyingImage.format()),
                QVideoFrameFormat::Format_Invalid);
    QCOMPARE(m_image.convertedTo(underlyingImageFormat), underlyingImage);

    if (inputImageFormat == underlyingImageFormat)
        QCOMPARE(m_image.constBits(), underlyingImage.constBits());
}

QTEST_APPLESS_MAIN(tst_QVideoBuffers);

#include "tst_qvideobuffers.moc"
