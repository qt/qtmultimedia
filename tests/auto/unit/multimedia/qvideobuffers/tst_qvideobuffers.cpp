// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>

#include <private/qmemoryvideobuffer_p.h>
#include <private/qimagevideobuffer_p.h>
#include "qvideoframeformat.h"

using BufferPtr = std::shared_ptr<QAbstractVideoBuffer>;

class tst_QVideoBuffers : public QObject
{
    Q_OBJECT

public:
    tst_QVideoBuffers() {}
public slots:
    void initTestCase();

private slots:
    void map_returnsProperMappings_whenBufferIsNotMapped_data();
    void map_returnsProperMappings_whenBufferIsNotMapped();

    void map_returnsProperMappings_whenBufferIsMapped_data();
    void map_returnsProperMappings_whenBufferIsMapped();

    void mapMemoryOrImageBuffer_detachesDataDependingOnMode_data();
    void mapMemoryOrImageBuffer_detachesDataDependingOnMode();

    void unmap_resetsMappedState_whenBufferIsMapped_data();
    void unmap_resetsMappedState_whenBufferIsMapped();

    void imageBuffer_fixesInputImage_data();
    void imageBuffer_fixesInputImage();

private:
    void generateImageAndMemoryBuffersWithAllModes() const
    {
        QTest::addColumn<BufferPtr>("buffer");
        QTest::addColumn<QVideoFrame::MapMode>("mapMode");
        QTest::addColumn<const uint8_t *>("sourcePointer");

        static QMetaEnum modeEnum = QMetaEnum::fromType<QVideoFrame::MapMode>();
        QTEST_ASSERT(modeEnum.isValid());
        for (int i = 0; i < modeEnum.keyCount(); ++i) {
            const QVideoFrame::MapMode mode = QVideoFrame::MapMode(modeEnum.value(i));
            if (mode == QVideoFrame::NotMapped)
                continue;
            QTest::newRow(QStringLiteral("ImageBuffer, %1").arg(modeEnum.key(i)).toLocal8Bit().constData())
                    << createImageBuffer() << mode << m_image.constBits();
            QTest::newRow(QStringLiteral("MemoryBuffer, %1").arg(modeEnum.key(i)).toLocal8Bit().constData())
                    << createMemoryBuffer() << mode << reinterpret_cast<const uint8_t *>(m_byteArray.constData());
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

void tst_QVideoBuffers::map_returnsProperMappings_whenBufferIsNotMapped_data()
{
    generateImageAndMemoryBuffersWithAllModes();
}

void tst_QVideoBuffers::map_returnsProperMappings_whenBufferIsNotMapped()
{
    QFETCH(BufferPtr, buffer);
    QFETCH(QVideoFrame::MapMode, mapMode);

    auto mappedData = buffer->map(mapMode);

    QCOMPARE(mappedData.planeCount, 1);
    QVERIFY(mappedData.data[0]);
    QCOMPARE(mappedData.dataSize[0], 80);
    QCOMPARE(mappedData.bytesPerLine[0], 20);

    const auto data = reinterpret_cast<const char*>(mappedData.data[0]);
    QCOMPARE(QByteArray(data, mappedData.dataSize[0]), m_byteArray);
}

void tst_QVideoBuffers::map_returnsProperMappings_whenBufferIsMapped_data()
{
    generateImageAndMemoryBuffersWithAllModes();
}

void tst_QVideoBuffers::map_returnsProperMappings_whenBufferIsMapped()
{
    QFETCH(BufferPtr, buffer);
    QFETCH(QVideoFrame::MapMode, mapMode);

    auto mappedData1 = buffer->map(mapMode);
    auto mappedData2 = buffer->map(mapMode);

    QCOMPARE(mappedData1.planeCount, mappedData2.planeCount);
    QCOMPARE(mappedData1.data[0], mappedData2.data[0]);
    QCOMPARE(mappedData1.dataSize[0], mappedData2.dataSize[0]);
    QCOMPARE(mappedData1.bytesPerLine[0], mappedData2.bytesPerLine[0]);
}

void tst_QVideoBuffers::mapMemoryOrImageBuffer_detachesDataDependingOnMode_data()
{
    generateImageAndMemoryBuffersWithAllModes();
}

void tst_QVideoBuffers::mapMemoryOrImageBuffer_detachesDataDependingOnMode()
{
    QFETCH(BufferPtr, buffer);
    QFETCH(QVideoFrame::MapMode, mapMode);
    QFETCH(const uint8_t *, sourcePointer);

    auto mappedData = buffer->map(mapMode);
    QCOMPARE(mappedData.planeCount, 1);

    const bool isDetached = mappedData.data[0] != sourcePointer;
    const bool isWriteMode = (mapMode & QVideoFrame::WriteOnly) == QVideoFrame::WriteOnly;
    QCOMPARE(isDetached, isWriteMode);
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
    QCOMPARE(mappedData.planeCount, 1);

    const auto data = reinterpret_cast<const char*>(mappedData.data[0]);
    QCOMPARE(QByteArray(data, mappedData.dataSize[0]), m_byteArray);
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
