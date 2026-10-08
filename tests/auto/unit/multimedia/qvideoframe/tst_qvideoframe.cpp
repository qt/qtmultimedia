// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>

#include <qvideoframe.h>
#include <qvideoframeformat.h>
#include "QtTest/qtestcase.h"
#include "private/qmemoryvideobuffer_p.h"
#include "private/qhwvideobuffer_p.h"
#include "private/qvideoframe_p.h"
#include <QtGui/QImage>
#include <QtCore/QPointer>
#include <QtCore/qset.h>
#include <QtMultimedia/private/qtmultimedia-config_p.h>
#include "private/qvideoframeconverter_p.h"
#include <private/mediabackendutils_p.h>

// Adds an enum, and the stringized version
#define ADD_ENUM_TEST(x) \
    QTest::newRow(#x) \
        << QVideoFrame::x \
    << QString(QLatin1String(#x));


// Image used for testing conversion from QImage to QVideoFrame
QImage createTestImage(QImage::Format format)
{
    // +---+---+---+
    // | r | g | b |
    // | b | r | g |
    // +---+---+---+
    QImage image{ { 3, 2 }, QImage::Format_ARGB32 };
    image.setPixelColor(0, 0, QColor(Qt::red));
    image.setPixelColor(1, 0, QColor(Qt::green));
    image.setPixelColor(2, 0, QColor(Qt::blue));
    image.setPixelColor(0, 1, QColor(Qt::blue));
    image.setPixelColor(1, 1, QColor(Qt::red));
    image.setPixelColor(2, 1, QColor(Qt::green));
    return image.convertToFormat(format);
}

// clang-format off

// Convert a QVideoFrame pixel value from raw format to QRgb
// Only works with little-endian byte ordering
QRgb swizzle(uint value, QVideoFrameFormat::PixelFormat format)
{
    switch (format) {
    case QVideoFrameFormat::Format_ARGB8888:
    case QVideoFrameFormat::Format_ARGB8888_Premultiplied:
    case QVideoFrameFormat::Format_XRGB8888:
        QTEST_ASSERT(false); // not implemented
        return 0;
    case QVideoFrameFormat::Format_BGRA8888:
    case QVideoFrameFormat::Format_BGRA8888_Premultiplied:
    case QVideoFrameFormat::Format_BGRX8888:
        return value;
    case QVideoFrameFormat::Format_ABGR8888:
    case QVideoFrameFormat::Format_XBGR8888:
        QTEST_ASSERT(false); // not implemented
        return 0;
    case QVideoFrameFormat::Format_RGBA8888:
    case QVideoFrameFormat::Format_RGBX8888:
        return (((value >> 24) & 0xff) << 24)   // a -> a
                | ((value & 0xff) << 16)        // b -> r
                | (((value >> 8) & 0xff) << 8)  // g -> g
                | ((value >> 16) & 0xff);       // r -> b
    default:
        qWarning() << "Unsupported format";
        return 0;
    }
}

std::vector<QRgb> swizzle(const std::vector<uint> &pixels, QVideoFrameFormat::PixelFormat format)
{
    std::vector<QRgb> rgba(pixels.size());
    std::transform(pixels.begin(), pixels.end(), rgba.begin(),
                   [format](uint value) {
                       return swizzle(value, format);
                   });
    return rgba;
}

// clang-format on

std::optional<std::vector<QRgb>> getPixels(QVideoFrame &frame)
{
    if (!frame.map(QVideoFrame::ReadOnly))
        return std::nullopt;

    const uint *mappedPixels = reinterpret_cast<const uint *>(frame.bits(0));
    const unsigned long long stride = frame.bytesPerLine(0) / sizeof(QRgb);

    std::vector<uint> pixels;
    for (int j = 0; j < frame.size().height(); ++j) {
        for (int i = 0; i < frame.size().width(); ++i) {
            pixels.push_back(mappedPixels[i + j * stride]);
        }
    }

    frame.unmap();

    return swizzle(pixels, frame.pixelFormat());
}

bool compareEq(QVideoFrame &frame, const QImage &image)
{
    if (frame.size() != image.size()) {
        qDebug() << "Size mismatch";
        return false;
    }

    const std::vector<QRgb> expectedPixels = { image.pixel(0, 0), image.pixel(1, 0), image.pixel(2, 0),
                                               image.pixel(0, 1), image.pixel(1, 1), image.pixel(2, 1) };

    const std::optional<std::vector<QRgb>> actualPixels = getPixels(frame);
    if (!actualPixels) {
        qDebug() << "Failed to read pixels from frame";
        return false;
    }

    for (size_t i = 0; i < expectedPixels.size(); ++i) {
        if (expectedPixels[i] != actualPixels->at(i)) {
            qDebug() << "Pixel difference at element" << i << ":" << Qt::hex << expectedPixels[i]
                     << "vs" << actualPixels->at(i);
            return false;
        }
    }
    return true;
}

QSet s_pixelFormats{ QVideoFrameFormat::Format_ARGB8888,
                     QVideoFrameFormat::Format_ARGB8888_Premultiplied,
                     QVideoFrameFormat::Format_XRGB8888,
                     QVideoFrameFormat::Format_BGRA8888,
                     QVideoFrameFormat::Format_BGRA8888_Premultiplied,
                     QVideoFrameFormat::Format_BGRX8888,
                     QVideoFrameFormat::Format_ABGR8888,
                     QVideoFrameFormat::Format_XBGR8888,
                     QVideoFrameFormat::Format_RGBA8888,
                     QVideoFrameFormat::Format_RGBX8888,
                     QVideoFrameFormat::Format_NV12,
                     QVideoFrameFormat::Format_NV21,
                     QVideoFrameFormat::Format_IMC1,
                     QVideoFrameFormat::Format_IMC2,
                     QVideoFrameFormat::Format_IMC3,
                     QVideoFrameFormat::Format_IMC4,
                     QVideoFrameFormat::Format_AYUV,
                     QVideoFrameFormat::Format_AYUV_Premultiplied,
                     QVideoFrameFormat::Format_YV12,
                     QVideoFrameFormat::Format_YUV420P,
                     QVideoFrameFormat::Format_YUV422P,
                     QVideoFrameFormat::Format_UYVY,
                     QVideoFrameFormat::Format_YUYV,
                     QVideoFrameFormat::Format_Y8,
                     QVideoFrameFormat::Format_Y16,
                     QVideoFrameFormat::Format_P010,
                     QVideoFrameFormat::Format_P016,
                     QVideoFrameFormat::Format_YUV420P10 };

class tst_QVideoFrame : public QObject
{
    Q_OBJECT
public:
    tst_QVideoFrame();
    ~tst_QVideoFrame() override;

public slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

private slots:
    void create_data();
    void create();
    void createInvalid_data();
    void createInvalid();
    void createFromBuffer_data();
    void createFromBuffer();
    void createFromImage_data();
    void createNull();
    void destructor_deletesVideoBuffer();
    void destructorOfPrivateData_unmapsAndDeletesVideoBuffer_whenNoMoreFramesCopies();
    void copy_data();
    void copy();
    void assign_data();
    void assign();
    void map_data();
    void map();
    void mapPlanes_data();
    void mapPlanes();
    void formatConversion_data();
    void formatConversion();

    void qImageFromVideoFrame_doesNotCrash_whenCalledWithEvenAndOddSizedFrames_data();
    void qImageFromVideoFrame_doesNotCrash_whenCalledWithEvenAndOddSizedFrames();

    void qImageFromVideoFrame_emptyJPEG();
    void qImageFromVideoFrame_goodJPEG();
    void qImageFromVideoFrame_goodJPEGWithExtraData();
    void qImageFromVideoFrame_badJPEG();

    void isMapped();
    void isReadable();
    void isWritable();

    void image_data();
    void image();

    void emptyData();

    void mirrored_doesntTakeValue_fromVideoFrameFormat();
    void rotation_doesntTakeValue_fromVideoFrameFormat();
    void streamFrameRate_takesValue_fromVideoFrameFormat();

    void constructor_createsInvalidFrame_whenCalledWithNullImage();
    void constructor_createsInvalidFrame_whenCalledWithEmptyImage();
    void constructor_createsInvalidFrame_whenCalledWithInvalidImageFormat();
    void constructor_createsFrameWithCorrectFormat_whenCalledWithSupportedImageFormats_data();
    void constructor_createsFrameWithCorrectFormat_whenCalledWithSupportedImageFormats();
    void constructor_copiesImageData_whenCalledWithRGBFormats_data();
    void constructor_copiesImageData_whenCalledWithRGBFormats();
};

class QtTestVideoBuffer : public QObject, public QHwVideoBuffer
{
    Q_OBJECT
public:
    QtTestVideoBuffer() : QHwVideoBuffer(QVideoFrame::NoHandle) { }
    explicit QtTestVideoBuffer(QVideoFrame::HandleType type) : QHwVideoBuffer(type) { }
    ~QtTestVideoBuffer() override { QTEST_ASSERT(!m_mapObject); }

    MapData map(QVideoFrame::MapMode) override
    {
        m_mapObject = std::make_unique<QObject>();
        MapData mapData;
        int nBytes = m_numBytes;
        mapData.planeCount = m_planeCount;
        for (int i = 0; i < m_planeCount; ++i) {
            mapData.data[i] = m_data[i];
            mapData.bytesPerLine[i] = m_bytesPerLine[i];
            if (i) {
                mapData.dataSize[i-1] = m_data[i] - m_data[i-1];
                nBytes -= mapData.dataSize[i-1];
            }
            mapData.dataSize[i] = nBytes;
        }
        return mapData;
    }

    void unmap() override { m_mapObject.reset(); }

    uchar *m_data[4];
    int m_bytesPerLine[4];
    int m_planeCount = 1;
    int m_numBytes;
    std::unique_ptr<QObject> m_mapObject;
};

tst_QVideoFrame::tst_QVideoFrame()
{
}

tst_QVideoFrame::~tst_QVideoFrame()
{
}

void tst_QVideoFrame::initTestCase()
{
}

void tst_QVideoFrame::cleanupTestCase()
{
}

void tst_QVideoFrame::init()
{
}

void tst_QVideoFrame::cleanup()
{
}

void tst_QVideoFrame::create_data()
{
    QTest::addColumn<QSize>("size");
    QTest::addColumn<QVideoFrameFormat::PixelFormat>("pixelFormat");
    QTest::addColumn<int>("bytesPerLine");

    QTest::newRow("64x64 ARGB32")
            << QSize(64, 64)
            << QVideoFrameFormat::Format_ARGB8888
            << 64*4;
    QTest::newRow("32x256 YUV420P")
            << QSize(32, 256)
            << QVideoFrameFormat::Format_YUV420P
            << 32;
    QTest::newRow("32x256 UYVY")
            << QSize(32, 256)
            << QVideoFrameFormat::Format_UYVY
            << 32*2;
}

void tst_QVideoFrame::create()
{
    QFETCH(QSize, size);
    QFETCH(QVideoFrameFormat::PixelFormat, pixelFormat);
    QFETCH(int, bytesPerLine);

    QVideoFrame frame(QVideoFrameFormat(size, pixelFormat));

    QVERIFY(frame.isValid());
    QCOMPARE(frame.handleType(), QVideoFrame::NoHandle);
    QCOMPARE(QVideoFramePrivate::hwBuffer(frame), nullptr);
    QCOMPARE_NE(QVideoFramePrivate::buffer(frame), nullptr);
    QCOMPARE(frame.pixelFormat(), pixelFormat);
    QCOMPARE(frame.size(), size);
    QCOMPARE(frame.width(), size.width());
    QCOMPARE(frame.height(), size.height());
    QCOMPARE(frame.startTime(), qint64(-1));
    QCOMPARE(frame.endTime(), qint64(-1));
    frame.map(QVideoFrame::ReadOnly);
    QCOMPARE(frame.bytesPerLine(0), bytesPerLine);
    frame.unmap();
}

void tst_QVideoFrame::createInvalid_data()
{
    QTest::addColumn<QSize>("size");
    QTest::addColumn<QVideoFrameFormat::PixelFormat>("pixelFormat");

    QTest::newRow("0x64 ARGB32 0 size")
            << QSize(0, 64)
            << QVideoFrameFormat::Format_ARGB8888;
    QTest::newRow("32x0 YUV420P 0 size")
            << QSize(32, 0)
            << QVideoFrameFormat::Format_YUV420P;
}

void tst_QVideoFrame::createInvalid()
{
    QFETCH(QSize, size);
    QFETCH(QVideoFrameFormat::PixelFormat, pixelFormat);

    QVideoFrame frame(QVideoFrameFormat(size, pixelFormat));

    QVERIFY(!frame.isValid());
    QCOMPARE(frame.handleType(), QVideoFrame::NoHandle);
    QCOMPARE(QVideoFramePrivate::buffer(frame), nullptr);
    QCOMPARE(frame.pixelFormat(), pixelFormat);
    QCOMPARE(frame.size(), size);
    QCOMPARE(frame.width(), size.width());
    QCOMPARE(frame.height(), size.height());
    QCOMPARE(frame.startTime(), qint64(-1));
    QCOMPARE(frame.endTime(), qint64(-1));
}

void tst_QVideoFrame::createFromBuffer_data()
{
    QTest::addColumn<QVideoFrame::HandleType>("handleType");
    QTest::addColumn<QSize>("size");
    QTest::addColumn<QVideoFrameFormat::PixelFormat>("pixelFormat");

    QTest::newRow("64x64 ARGB32 no handle")
            << QVideoFrame::NoHandle
            << QSize(64, 64)
            << QVideoFrameFormat::Format_ARGB8888;
    QTest::newRow("64x64 ARGB32 gl handle")
            << QVideoFrame::RhiTextureHandle
            << QSize(64, 64)
            << QVideoFrameFormat::Format_ARGB8888;
}

void tst_QVideoFrame::createFromBuffer()
{
    QFETCH(QVideoFrame::HandleType, handleType);
    QFETCH(QSize, size);
    QFETCH(QVideoFrameFormat::PixelFormat, pixelFormat);

    QVideoFrame frame = QVideoFramePrivate::createFrame(
            std::make_unique<QtTestVideoBuffer>(handleType), QVideoFrameFormat(size, pixelFormat));

    QVERIFY(frame.isValid());
    QCOMPARE(frame.handleType(), handleType);
    QCOMPARE(frame.pixelFormat(), pixelFormat);
    QCOMPARE(frame.size(), size);
    QCOMPARE(frame.width(), size.width());
    QCOMPARE(frame.height(), size.height());
    QCOMPARE(frame.startTime(), qint64(-1));
    QCOMPARE(frame.endTime(), qint64(-1));
}

void tst_QVideoFrame::createFromImage_data()
{
    QTest::addColumn<QSize>("size");
    QTest::addColumn<QImage::Format>("imageFormat");
    QTest::addColumn<QVideoFrameFormat::PixelFormat>("pixelFormat");

    QTest::newRow("64x64 RGB32")
            << QSize(64, 64)
            << QImage::Format_RGB32
            << QVideoFrameFormat::Format_XRGB8888;
    QTest::newRow("19x46 ARGB32_Premultiplied")
            << QSize(19, 46)
            << QImage::Format_ARGB32_Premultiplied
            << QVideoFrameFormat::Format_ARGB8888_Premultiplied;
}

void tst_QVideoFrame::createNull()
{
    // Default ctor
    {
        QVideoFrame frame;

        QVERIFY(!frame.isValid());
        QCOMPARE(frame.handleType(), QVideoFrame::NoHandle);
        QCOMPARE(frame.pixelFormat(), QVideoFrameFormat::Format_Invalid);
        QCOMPARE(frame.size(), QSize());
        QCOMPARE(frame.width(), -1);
        QCOMPARE(frame.height(), -1);
        QCOMPARE(frame.startTime(), qint64(-1));
        QCOMPARE(frame.endTime(), qint64(-1));
        QCOMPARE(frame.mapMode(), QVideoFrame::NotMapped);
        QVERIFY(!frame.map(QVideoFrame::ReadOnly));
        QVERIFY(!frame.map(QVideoFrame::ReadWrite));
        QVERIFY(!frame.map(QVideoFrame::WriteOnly));
        QCOMPARE(frame.isMapped(), false);
        frame.unmap(); // Shouldn't crash
        QCOMPARE(frame.isReadable(), false);
        QCOMPARE(frame.isWritable(), false);
    }

    // Null buffer (shouldn't crash)
    {
        QVideoFrame frame = QVideoFramePrivate::createFrame(
                std::unique_ptr<QHwVideoBuffer>(),
                QVideoFrameFormat(QSize(1024, 768), QVideoFrameFormat::Format_ARGB8888));
        QVERIFY(!frame.isValid());
        QCOMPARE(frame.handleType(), QVideoFrame::NoHandle);
        QCOMPARE(frame.pixelFormat(), QVideoFrameFormat::Format_ARGB8888);
        QCOMPARE(frame.size(), QSize(1024, 768));
        QCOMPARE(frame.width(), 1024);
        QCOMPARE(frame.height(), 768);
        QCOMPARE(frame.startTime(), qint64(-1));
        QCOMPARE(frame.endTime(), qint64(-1));
        QCOMPARE(frame.mapMode(), QVideoFrame::NotMapped);
        QVERIFY(!frame.map(QVideoFrame::ReadOnly));
        QVERIFY(!frame.map(QVideoFrame::ReadWrite));
        QVERIFY(!frame.map(QVideoFrame::WriteOnly));
        QCOMPARE(frame.isMapped(), false);
        frame.unmap(); // Shouldn't crash
        QCOMPARE(frame.isReadable(), false);
        QCOMPARE(frame.isWritable(), false);
    }
}

void tst_QVideoFrame::destructor_deletesVideoBuffer()
{
    QPointer buffer(new QtTestVideoBuffer);

    {
        QVideoFrame frame = QVideoFramePrivate::createFrame(
                std::unique_ptr<QHwVideoBuffer>(buffer),
                QVideoFrameFormat(QSize(4, 1), QVideoFrameFormat::Format_ARGB8888));
    }

    QVERIFY(buffer.isNull());
}

void tst_QVideoFrame::destructorOfPrivateData_unmapsAndDeletesVideoBuffer_whenNoMoreFramesCopies()
{
    uchar bufferData[16] = {
        0,
    };

    QPointer buffer(new QtTestVideoBuffer);
    buffer->m_data[0] = bufferData;
    buffer->m_bytesPerLine[0] = 16;
    buffer->m_planeCount = 1;
    buffer->m_numBytes = sizeof(bufferData);

    QVideoFrame frame1 = QVideoFramePrivate::createFrame(
            std::unique_ptr<QHwVideoBuffer>(buffer),
            QVideoFrameFormat(QSize(4, 1), QVideoFrameFormat::Format_ARGB8888));

    frame1.map(QVideoFrame::ReadOnly);

    QVERIFY(buffer);
    QPointer mapObject(buffer->m_mapObject.get());

    QVideoFrame frame2 = frame1;

    frame1 = {};

    // check if the buffer and the map object are still alive
    QVERIFY(mapObject);
    QVERIFY(buffer);

    frame2 = {};

    // check if the buffer and the map object have been deleted
    QVERIFY(!mapObject);
    QVERIFY(!buffer);
}

void tst_QVideoFrame::copy_data()
{
    QTest::addColumn<QVideoFrame::HandleType>("handleType");
    QTest::addColumn<QSize>("size");
    QTest::addColumn<QVideoFrameFormat::PixelFormat>("pixelFormat");
    QTest::addColumn<qint64>("startTime");
    QTest::addColumn<qint64>("endTime");

    QTest::newRow("64x64 ARGB32")
            << QVideoFrame::RhiTextureHandle
            << QSize(64, 64)
            << QVideoFrameFormat::Format_ARGB8888
            << qint64(63641740)
            << qint64(63641954);
    QTest::newRow("64x64 ARGB32")
            << QVideoFrame::RhiTextureHandle
            << QSize(64, 64)
            << QVideoFrameFormat::Format_ARGB8888
            << qint64(63641740)
            << qint64(63641954);
    QTest::newRow("32x256 YUV420P")
            << QVideoFrame::NoHandle
            << QSize(32, 256)
            << QVideoFrameFormat::Format_YUV420P
            << qint64(12345)
            << qint64(12389);
    QTest::newRow("1052x756 ARGB32")
            << QVideoFrame::NoHandle
            << QSize(1052, 756)
            << QVideoFrameFormat::Format_ARGB8888
            << qint64(12345)
            << qint64(12389);
    QTest::newRow("32x256 YUV420P")
            << QVideoFrame::NoHandle
            << QSize(32, 256)
            << QVideoFrameFormat::Format_YUV420P
            << qint64(12345)
            << qint64(12389);
}

void tst_QVideoFrame::copy()
{
    QFETCH(QVideoFrame::HandleType, handleType);
    QFETCH(QSize, size);
    QFETCH(QVideoFrameFormat::PixelFormat, pixelFormat);
    QFETCH(qint64, startTime);
    QFETCH(qint64, endTime);

    QPointer<QtTestVideoBuffer> buffer = new QtTestVideoBuffer(handleType);

    {
        QVideoFrame frame = QVideoFramePrivate::createFrame(std::unique_ptr<QHwVideoBuffer>(buffer),
                                                            QVideoFrameFormat(size, pixelFormat));
        frame.setStartTime(startTime);
        frame.setEndTime(endTime);

        QVERIFY(frame.isValid());
        QCOMPARE(frame.handleType(), handleType);
        QCOMPARE(frame.pixelFormat(), pixelFormat);
        QCOMPARE(frame.size(), size);
        QCOMPARE(frame.width(), size.width());
        QCOMPARE(frame.height(), size.height());
        QCOMPARE(frame.startTime(), startTime);
        QCOMPARE(frame.endTime(), endTime);

        {
            QVideoFrame otherFrame(frame);

            QVERIFY(!buffer.isNull());

            QVERIFY(otherFrame.isValid());
            QCOMPARE(otherFrame.handleType(), handleType);
            QCOMPARE(otherFrame.pixelFormat(), pixelFormat);
            QCOMPARE(otherFrame.size(), size);
            QCOMPARE(otherFrame.width(), size.width());
            QCOMPARE(otherFrame.height(), size.height());
            QCOMPARE(otherFrame.startTime(), startTime);
            QCOMPARE(otherFrame.endTime(), endTime);

            otherFrame.setEndTime(-1);

            QVERIFY(!buffer.isNull());

            QVERIFY(otherFrame.isValid());
            QCOMPARE(otherFrame.handleType(), handleType);
            QCOMPARE(otherFrame.pixelFormat(), pixelFormat);
            QCOMPARE(otherFrame.size(), size);
            QCOMPARE(otherFrame.width(), size.width());
            QCOMPARE(otherFrame.height(), size.height());
            QCOMPARE(otherFrame.startTime(), startTime);
            QCOMPARE(otherFrame.endTime(), qint64(-1));
        }

        QVERIFY(!buffer.isNull());

        QVERIFY(frame.isValid());
        QCOMPARE(frame.handleType(), handleType);
        QCOMPARE(frame.pixelFormat(), pixelFormat);
        QCOMPARE(frame.size(), size);
        QCOMPARE(frame.width(), size.width());
        QCOMPARE(frame.height(), size.height());
        QCOMPARE(frame.startTime(), startTime);
        QCOMPARE(frame.endTime(), qint64(-1));  // Explicitly shared.
    }

    QVERIFY(buffer.isNull());
}

void tst_QVideoFrame::assign_data()
{
    QTest::addColumn<QVideoFrame::HandleType>("handleType");
    QTest::addColumn<QSize>("size");
    QTest::addColumn<QVideoFrameFormat::PixelFormat>("pixelFormat");
    QTest::addColumn<qint64>("startTime");
    QTest::addColumn<qint64>("endTime");

    QTest::newRow("64x64 ARGB32")
            << QVideoFrame::RhiTextureHandle
            << QSize(64, 64)
            << QVideoFrameFormat::Format_ARGB8888
            << qint64(63641740)
            << qint64(63641954);
    QTest::newRow("32x256 YUV420P")
            << QVideoFrame::NoHandle
            << QSize(32, 256)
            << QVideoFrameFormat::Format_YUV420P
            << qint64(12345)
            << qint64(12389);
}

void tst_QVideoFrame::assign()
{
    QFETCH(QVideoFrame::HandleType, handleType);
    QFETCH(QSize, size);
    QFETCH(QVideoFrameFormat::PixelFormat, pixelFormat);
    QFETCH(qint64, startTime);
    QFETCH(qint64, endTime);

    QPointer<QtTestVideoBuffer> buffer = new QtTestVideoBuffer(handleType);

    QVideoFrame frame;
    {
        QVideoFrame otherFrame = QVideoFramePrivate::createFrame(
                std::unique_ptr<QHwVideoBuffer>(buffer), QVideoFrameFormat(size, pixelFormat));
        otherFrame.setStartTime(startTime);
        otherFrame.setEndTime(endTime);

        frame = otherFrame;

        QVERIFY(!buffer.isNull());

        QVERIFY(otherFrame.isValid());
        QCOMPARE(otherFrame.handleType(), handleType);
        QCOMPARE(otherFrame.pixelFormat(), pixelFormat);
        QCOMPARE(otherFrame.size(), size);
        QCOMPARE(otherFrame.width(), size.width());
        QCOMPARE(otherFrame.height(), size.height());
        QCOMPARE(otherFrame.startTime(), startTime);
        QCOMPARE(otherFrame.endTime(), endTime);

        otherFrame.setStartTime(-1);

        QVERIFY(!buffer.isNull());

        QVERIFY(otherFrame.isValid());
        QCOMPARE(otherFrame.handleType(), handleType);
        QCOMPARE(otherFrame.pixelFormat(), pixelFormat);
        QCOMPARE(otherFrame.size(), size);
        QCOMPARE(otherFrame.width(), size.width());
        QCOMPARE(otherFrame.height(), size.height());
        QCOMPARE(otherFrame.startTime(), qint64(-1));
        QCOMPARE(otherFrame.endTime(), endTime);
    }

    QVERIFY(!buffer.isNull());

    QVERIFY(frame.isValid());
    QCOMPARE(frame.handleType(), handleType);
    QCOMPARE(frame.pixelFormat(), pixelFormat);
    QCOMPARE(frame.size(), size);
    QCOMPARE(frame.width(), size.width());
    QCOMPARE(frame.height(), size.height());
    QCOMPARE(frame.startTime(), qint64(-1));
    QCOMPARE(frame.endTime(), endTime);

    frame = QVideoFrame();

    QVERIFY(buffer.isNull());

    QVERIFY(!frame.isValid());
    QCOMPARE(frame.handleType(), QVideoFrame::NoHandle);
    QCOMPARE(frame.pixelFormat(), QVideoFrameFormat::Format_Invalid);
    QCOMPARE(frame.size(), QSize());
    QCOMPARE(frame.width(), -1);
    QCOMPARE(frame.height(), -1);
    QCOMPARE(frame.startTime(), qint64(-1));
    QCOMPARE(frame.endTime(), qint64(-1));
}

void tst_QVideoFrame::map_data()
{
    QTest::addColumn<QSize>("size");
    QTest::addColumn<QVideoFrameFormat::PixelFormat>("pixelFormat");
    QTest::addColumn<QVideoFrame::MapMode>("mode");

    QTest::newRow("read-only")
            << QSize(64, 64)
            << QVideoFrameFormat::Format_ARGB8888
            << QVideoFrame::ReadOnly;

    QTest::newRow("write-only")
            << QSize(64, 64)
            << QVideoFrameFormat::Format_ARGB8888
            << QVideoFrame::WriteOnly;

    QTest::newRow("read-write")
            << QSize(64, 64)
            << QVideoFrameFormat::Format_ARGB8888
            << QVideoFrame::ReadWrite;
}

void tst_QVideoFrame::map()
{
    QFETCH(QSize, size);
    QFETCH(QVideoFrameFormat::PixelFormat, pixelFormat);
    QFETCH(QVideoFrame::MapMode, mode);

    QVideoFrame frame(QVideoFrameFormat(size, pixelFormat));

    QVERIFY(!frame.bits(0));
    QCOMPARE(frame.mappedBytes(0), 0);
    QCOMPARE(frame.bytesPerLine(0), 0);
    QCOMPARE(frame.mapMode(), QVideoFrame::NotMapped);

    QVERIFY(frame.map(mode));

    // Mapping multiple times is allowed in ReadOnly mode
    if (mode == QVideoFrame::ReadOnly) {
        const uchar *bits = frame.bits(0);

        QVERIFY(frame.map(QVideoFrame::ReadOnly));
        QVERIFY(frame.isMapped());
        QCOMPARE(frame.bits(0), bits);

        frame.unmap();
        //frame should still be mapped after the first nested unmap
        QVERIFY(frame.isMapped());
        QCOMPARE(frame.bits(0), bits);

        //re-mapping in Write or ReadWrite modes should fail
        QVERIFY(!frame.map(QVideoFrame::WriteOnly));
        QVERIFY(!frame.map(QVideoFrame::ReadWrite));
    } else {
        // Mapping twice in ReadWrite or WriteOnly modes should fail, but leave it mapped (and the mode is ignored)
        QVERIFY(!frame.map(mode));
        QVERIFY(!frame.map(QVideoFrame::ReadOnly));
    }

    QVERIFY(frame.bits(0));
    QCOMPARE(frame.mapMode(), mode);

    frame.unmap();

    QVERIFY(!frame.bits(0));
    QCOMPARE(frame.mappedBytes(0), 0);
    QCOMPARE(frame.bytesPerLine(0), 0);
    QCOMPARE(frame.mapMode(), QVideoFrame::NotMapped);
}

void tst_QVideoFrame::mapPlanes_data()
{
    QTest::addColumn<QVideoFrame>("frame");

    // Distance between subsequent lines within a color plane in bytes
    QTest::addColumn<QList<int> >("strides");

    // Distance from first pixel of first color plane to first pixel
    // of n'th plane in bytes
    QTest::addColumn<QList<int> >("offsets");

    static uchar bufferData[1024];

    auto planarBuffer = std::make_unique<QtTestVideoBuffer>();
    planarBuffer->m_data[0] = bufferData;
    planarBuffer->m_data[1] = bufferData + 512;
    planarBuffer->m_data[2] = bufferData + 765;
    planarBuffer->m_bytesPerLine[0] = 64;
    planarBuffer->m_bytesPerLine[1] = 36;
    planarBuffer->m_bytesPerLine[2] = 36;
    planarBuffer->m_planeCount = 3;
    planarBuffer->m_numBytes = sizeof(bufferData);

    QTest::newRow("Planar") << QVideoFramePrivate::createFrame(
            std::move(planarBuffer),
            QVideoFrameFormat(QSize(64, 64), QVideoFrameFormat::Format_YUV420P))
                            << (QList<int>() << 64 << 36 << 36) << (QList<int>() << 512 << 765);
    QTest::newRow("Format_YUV420P")
        << QVideoFrame(QVideoFrameFormat(QSize(60, 64), QVideoFrameFormat::Format_YUV420P))
        << (QList<int>() << 64 << 32 << 32)
        << (QList<int>() << 4096 << 5120);
    QTest::newRow("Format_YUV422P")
        << QVideoFrame(QVideoFrameFormat(QSize(60, 64), QVideoFrameFormat::Format_YUV422P))
        << (QList<int>() << 64 << 64 / 2 << 64 / 2)
        << (QList<int>() << 64 * 64 << 64 * 64 + 64 / 2 * 64);
    QTest::newRow("Format_YV12")
        << QVideoFrame(QVideoFrameFormat(QSize(60, 64), QVideoFrameFormat::Format_YV12))
        << (QList<int>() << 64 << 32 << 32)
        << (QList<int>() << 4096 << 5120);
    QTest::newRow("Format_NV12")
        << QVideoFrame(QVideoFrameFormat(QSize(60, 64), QVideoFrameFormat::Format_NV12))
        << (QList<int>() << 64 << 64)
        << (QList<int>() << 4096);
    QTest::newRow("Format_NV21")
        << QVideoFrame(QVideoFrameFormat(QSize(60, 64), QVideoFrameFormat::Format_NV21))
        << (QList<int>() << 64 << 64)
        << (QList<int>() << 4096);
    QTest::newRow("Format_IMC2")
        << QVideoFrame(QVideoFrameFormat(QSize(60, 64), QVideoFrameFormat::Format_IMC2))
        << (QList<int>() << 64 << 64)
        << (QList<int>() << 4096);
    QTest::newRow("Format_IMC4")
        << QVideoFrame(QVideoFrameFormat(QSize(60, 64), QVideoFrameFormat::Format_IMC4))
        << (QList<int>() << 64 << 64)
        << (QList<int>() << 4096);
    QTest::newRow("Format_IMC1")
        << QVideoFrame(QVideoFrameFormat(QSize(60, 64), QVideoFrameFormat::Format_IMC1))
        << (QList<int>() << 64 << 64 << 64)
        << (QList<int>() << 4096 << 6144);
    QTest::newRow("Format_IMC3")
        << QVideoFrame(QVideoFrameFormat(QSize(60, 64), QVideoFrameFormat::Format_IMC3))
        << (QList<int>() << 64 << 64 << 64)
        << (QList<int>() << 4096 << 6144);
    QTest::newRow("Format_ARGB32")
        << QVideoFrame(QVideoFrameFormat(QSize(60, 64), QVideoFrameFormat::Format_ARGB8888))
        << (QList<int>() << 240)
        << (QList<int>());
}

void tst_QVideoFrame::mapPlanes()
{
    QFETCH(QVideoFrame, frame);
    QFETCH(QList<int>, strides);
    QFETCH(QList<int>, offsets);

    QCOMPARE(strides.size(), offsets.size() + 1);

    QCOMPARE(frame.map(QVideoFrame::ReadOnly), true);
    QCOMPARE(frame.planeCount(), strides.size());

    QVERIFY(strides.size() > 0);
    QCOMPARE(frame.bytesPerLine(0), strides.at(0));
    QVERIFY(frame.bits(0));

    if (strides.size() > 1) {
        QCOMPARE(frame.bytesPerLine(1), strides.at(1));
        QCOMPARE(int(frame.bits(1) - frame.bits(0)), offsets.at(0));
    }
    if (strides.size() > 2) {
        QCOMPARE(frame.bytesPerLine(2), strides.at(2));
        QCOMPARE(int(frame.bits(2) - frame.bits(0)), offsets.at(1));
    }
    if (strides.size() > 3) {
        QCOMPARE(frame.bytesPerLine(3), strides.at(3));
        QCOMPARE(int(frame.bits(3) - frame.bits(0)), offsets.at(0));
    }

    frame.unmap();
}

void tst_QVideoFrame::formatConversion_data()
{
    QTest::addColumn<QImage::Format>("imageFormat");
    QTest::addColumn<QVideoFrameFormat::PixelFormat>("pixelFormat");

#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    QTest::newRow("QImage::Format_RGB32 | QVideoFrameFormat::Format_BGRX8888")
            << QImage::Format_RGB32
            << QVideoFrameFormat::Format_BGRX8888;
    QTest::newRow("QImage::Format_ARGB32 | QVideoFrameFormat::Format_BGRA8888")
            << QImage::Format_ARGB32
            << QVideoFrameFormat::Format_BGRA8888;
    QTest::newRow("QImage::Format_ARGB32_Premultiplied | QVideoFrameFormat::Format_BGRA8888_Premultiplied")
            << QImage::Format_ARGB32_Premultiplied
            << QVideoFrameFormat::Format_BGRA8888_Premultiplied;
    QTest::newRow("QVideoFrameFormat::Format_ARGB8888")
        << QImage::Format_Invalid
        << QVideoFrameFormat::Format_ARGB8888;
    QTest::newRow("QVideoFrameFormat::Format_ARGB8888_Premultiplied")
        << QImage::Format_Invalid
        << QVideoFrameFormat::Format_ARGB8888_Premultiplied;
#else
    QTest::newRow("QImage::Format_RGB32 | QVideoFrameFormat::Format_XRGB8888")
        << QImage::Format_RGB32
        << QVideoFrameFormat::Format_XRGB8888;
    QTest::newRow("QImage::Format_ARGB32 | QVideoFrameFormat::Format_ARGB8888")
        << QImage::Format_ARGB32
        << QVideoFrameFormat::Format_ARGB8888;
    QTest::newRow("QImage::Format_ARGB32_Premultiplied | QVideoFrameFormat::Format_ARGB8888_Premultiplied")
        << QImage::Format_ARGB32_Premultiplied
        << QVideoFrameFormat::Format_ARGB8888_Premultiplied;
    QTest::newRow("QVideoFrameFormat::Format_BGRA8888")
        << QImage::Format_Invalid
        << QVideoFrameFormat::Format_BGRA8888;
    QTest::newRow("QVideoFrameFormat::Format_BGRA8888_Premultiplied")
        << QImage::Format_Invalid
        << QVideoFrameFormat::Format_BGRA8888_Premultiplied;
#endif

    QTest::newRow("QImage::Format_MonoLSB")
            << QImage::Format_MonoLSB
            << QVideoFrameFormat::Format_Invalid;
    QTest::newRow("QImage::Format_Indexed8")
            << QImage::Format_Indexed8
            << QVideoFrameFormat::Format_Invalid;
    QTest::newRow("QImage::Format_ARGB6666_Premultiplied")
            << QImage::Format_ARGB6666_Premultiplied
            << QVideoFrameFormat::Format_Invalid;
    QTest::newRow("QImage::Format_ARGB8555_Premultiplied")
            << QImage::Format_ARGB8555_Premultiplied
            << QVideoFrameFormat::Format_Invalid;
    QTest::newRow("QImage::Format_RGB666")
            << QImage::Format_RGB666
            << QVideoFrameFormat::Format_Invalid;
    QTest::newRow("QImage::Format_RGB444")
            << QImage::Format_RGB444
            << QVideoFrameFormat::Format_Invalid;
    QTest::newRow("QImage::Format_ARGB4444_Premultiplied")
            << QImage::Format_ARGB4444_Premultiplied
            << QVideoFrameFormat::Format_Invalid;

    QTest::newRow("QVideoFrameFormat::Format_BGR32")
            << QImage::Format_Invalid
            << QVideoFrameFormat::Format_XBGR8888;
    QTest::newRow("QVideoFrameFormat::Format_AYUV")
            << QImage::Format_Invalid
            << QVideoFrameFormat::Format_AYUV;
    QTest::newRow("QVideoFrameFormat::Format_AYUV_Premultiplied")
            << QImage::Format_Invalid
            << QVideoFrameFormat::Format_AYUV_Premultiplied;
    QTest::newRow("QVideoFrameFormat::Format_YUV420P")
            << QImage::Format_Invalid
            << QVideoFrameFormat::Format_YUV420P;
    QTest::newRow("QVideoFrameFormat::Format_YV12")
            << QImage::Format_Invalid
            << QVideoFrameFormat::Format_YV12;
    QTest::newRow("QVideoFrameFormat::Format_UYVY")
            << QImage::Format_Invalid
            << QVideoFrameFormat::Format_UYVY;
    QTest::newRow("QVideoFrameFormat::Format_YUYV")
            << QImage::Format_Invalid
            << QVideoFrameFormat::Format_YUYV;
    QTest::newRow("QVideoFrameFormat::Format_NV12")
            << QImage::Format_Invalid
            << QVideoFrameFormat::Format_NV12;
    QTest::newRow("QVideoFrameFormat::Format_NV21")
            << QImage::Format_Invalid
            << QVideoFrameFormat::Format_NV21;
    QTest::newRow("QVideoFrameFormat::Format_IMC1")
            << QImage::Format_Invalid
            << QVideoFrameFormat::Format_IMC1;
    QTest::newRow("QVideoFrameFormat::Format_IMC2")
            << QImage::Format_Invalid
            << QVideoFrameFormat::Format_IMC2;
    QTest::newRow("QVideoFrameFormat::Format_IMC3")
            << QImage::Format_Invalid
            << QVideoFrameFormat::Format_IMC3;
    QTest::newRow("QVideoFrameFormat::Format_IMC4")
            << QImage::Format_Invalid
            << QVideoFrameFormat::Format_IMC4;
    QTest::newRow("QVideoFrameFormat::Format_Y8")
            << QImage::Format_Grayscale8
            << QVideoFrameFormat::Format_Y8;
    QTest::newRow("QVideoFrameFormat::Format_Y16")
            << QImage::Format_Grayscale16
            << QVideoFrameFormat::Format_Y16;
    QTest::newRow("QVideoFrameFormat::Format_Jpeg")
            << QImage::Format_Invalid
            << QVideoFrameFormat::Format_Jpeg;
    QTest::newRow("QVideoFrameFormat::Format_RGBX8888")
            << QImage::Format_RGBX8888 << QVideoFrameFormat::Format_RGBX8888;
    QTest::newRow("QImage::Format_RGBA8888_Premultiplied => QVideoFrameFormat::Format_RGBX8888 "
                  "(workaround)")
            << QImage::Format_RGBA8888_Premultiplied << QVideoFrameFormat::Format_RGBX8888;
}

void tst_QVideoFrame::formatConversion()
{
    QFETCH(QImage::Format, imageFormat);
    QFETCH(QVideoFrameFormat::PixelFormat, pixelFormat);

    if (imageFormat != QImage::Format_Invalid)
        QCOMPARE(QVideoFrameFormat::pixelFormatFromImageFormat(imageFormat), pixelFormat);

    if (imageFormat == QImage::Format_RGBA8888_Premultiplied) {
        qWarning() << "Workaround: convert QImage::Format_RGBA8888_Premultiplied to "
                      "QVideoFrameFormat::Format_RGBX8888; to be removed in 6.8";
        return;
    }

    if (pixelFormat != QVideoFrameFormat::Format_Invalid)
        QCOMPARE(QVideoFrameFormat::imageFormatFromPixelFormat(pixelFormat), imageFormat);
}

void tst_QVideoFrame::qImageFromVideoFrame_doesNotCrash_whenCalledWithEvenAndOddSizedFrames_data() {
    QTest::addColumn<QSize>("size");
    QTest::addColumn<QVideoFrameFormat::PixelFormat>("pixelFormat");
    QTest::addColumn<bool>("forceCpuConversion");
    QTest::addColumn<bool>("supportedOnPlatform");

    const std::vector<QSize> sizes{
        // Even sized
        { 2, 2 },
        { 2, 10 },
        { 10, 2 },
        { 640, 480 },
        { 4096, 2160 },
        // Odd sized
        { 0, 0 },
        { 3, 3 },
        { 2, 3 },
        { 3, 2 },
        { 641, 480 },
        { 640, 481 },
        // TODO: Crashes
        // { 1, 1 } // TODO: Division by zero in QVideoFrame::map (Debug)
        // { 1, 2 } // TODO: D3D validation error in QRhiD3D11::executeCommandBuffer
        // { 2, 1 } // TODO: D3D validation error in QRhiD3D11::executeCommandBuffer
    };

    for (const QSize &size : sizes) {
        for (const QVideoFrameFormat::PixelFormat pixelFormat : s_pixelFormats) {
            QList<bool> cpuChoices = { true };
            if (isRhiRenderingSupported())
                cpuChoices.push_back(false); // Only run tests on GPU if RHI is supported

            for (const bool forceCpu : cpuChoices) {

                if (pixelFormat == QVideoFrameFormat::Format_YUV420P10 && forceCpu)
                    continue; // TODO: Cpu conversion not implemented

                QString name = QStringLiteral("%1x%2_%3%4")
                               .arg(size.width())
                               .arg(size.height())
                               .arg(QVideoFrameFormat::pixelFormatToString(pixelFormat))
                               .arg(forceCpu ? "_cpu" : "");

                QTest::addRow("%s", name.toLatin1().data())
                        << size << pixelFormat << forceCpu;
            }
        }
    }
}

void tst_QVideoFrame::qImageFromVideoFrame_doesNotCrash_whenCalledWithEvenAndOddSizedFrames() {
    QFETCH(const QSize, size);
    QFETCH(const QVideoFrameFormat::PixelFormat, pixelFormat);
    QFETCH(const bool, forceCpuConversion);

    const QVideoFrameFormat format{ size, pixelFormat };
    const QVideoFrame frame{ format };
    const QImage actual = qImageFromVideoFrame(frame, forceCpuConversion);

    QCOMPARE_EQ(actual.isNull(), size.isEmpty());
}

void tst_QVideoFrame::qImageFromVideoFrame_emptyJPEG()
{
    QByteArray byteArray;
    auto memoryBuffer = std::make_unique<QMemoryVideoBuffer>(byteArray, byteArray.size());

    QVideoFrame frame = QVideoFramePrivate::createFrame(
            std::move(memoryBuffer),
            QVideoFrameFormat(QSize(144, 110), QVideoFrameFormat::Format_Jpeg));

    QImage img = qImageFromVideoFrame(frame, false);
    QCOMPARE(img.isNull(), true);
}

static QByteArray umbrellasJPEG()
{
    static const QByteArray cachedData = [] {
        QString input = QFINDTESTDATA("umbrellas.jpg");
        auto file = QFile(input);
        if (!file.open(QFile::ReadOnly))
            return QByteArray{};
        return file.readAll();
    }();
    return cachedData;
}

void tst_QVideoFrame::qImageFromVideoFrame_goodJPEG()
{
    QByteArray byteArray = umbrellasJPEG();
    auto memoryBuffer = std::make_unique<QMemoryVideoBuffer>(byteArray, byteArray.size());

    QVideoFrame frame = QVideoFramePrivate::createFrame(
            std::move(memoryBuffer),
            QVideoFrameFormat(QSize(144, 110), QVideoFrameFormat::Format_Jpeg));

    QImage img = qImageFromVideoFrame(frame, false);
    QCOMPARE(img.size(), QSize(144, 110));
}

void tst_QVideoFrame::qImageFromVideoFrame_goodJPEGWithExtraData()
{
    QByteArray byteArray = umbrellasJPEG();
    byteArray += "Yada";

    auto memoryBuffer = std::make_unique<QMemoryVideoBuffer>(byteArray, byteArray.size());

    QVideoFrame frame = QVideoFramePrivate::createFrame(
            std::move(memoryBuffer),
            QVideoFrameFormat(QSize(144, 110), QVideoFrameFormat::Format_Jpeg));

    QImage img = qImageFromVideoFrame(frame, false);
    QCOMPARE(img.size(), QSize(144, 110));
}

void tst_QVideoFrame::qImageFromVideoFrame_badJPEG()
{
    QByteArray byteArray = umbrellasJPEG();
    byteArray = byteArray.first(10'000);

    auto memoryBuffer = std::make_unique<QMemoryVideoBuffer>(byteArray, byteArray.size());

    QVideoFrame frame = QVideoFramePrivate::createFrame(
            std::move(memoryBuffer),
            QVideoFrameFormat(QSize(144, 110), QVideoFrameFormat::Format_Jpeg));

    QTest::ignoreMessage(QtMsgType::QtWarningMsg,
                         QRegularExpression(u".*JPEG data does not contain EOI marker.*"_s));

    QImage img = qImageFromVideoFrame(frame, false);
    QCOMPARE(img.isNull(), true);
}

#define TEST_MAPPED(frame, mode) \
do { \
    QVERIFY(frame.bits(0)); \
    QVERIFY(frame.isMapped()); \
    QCOMPARE(frame.mappedBytes(0), 16384); \
    QCOMPARE(frame.bytesPerLine(0), 256); \
    QCOMPARE(frame.mapMode(), mode); \
} while (0)

#define TEST_UNMAPPED(frame) \
do { \
    QVERIFY(!frame.bits(0)); \
    QVERIFY(!frame.isMapped()); \
    QCOMPARE(frame.mappedBytes(0), 0); \
    QCOMPARE(frame.bytesPerLine(0), 0); \
    QCOMPARE(frame.mapMode(), QVideoFrame::NotMapped); \
} while (0)

void tst_QVideoFrame::isMapped()
{
    QVideoFrame frame(QVideoFrameFormat(QSize(64, 64), QVideoFrameFormat::Format_ARGB8888));
    const QVideoFrame& constFrame(frame);

    TEST_UNMAPPED(frame);
    TEST_UNMAPPED(constFrame);

    QVERIFY(frame.map(QVideoFrame::ReadOnly));
    TEST_MAPPED(frame, QVideoFrame::ReadOnly);
    TEST_MAPPED(constFrame, QVideoFrame::ReadOnly);
    frame.unmap();
    TEST_UNMAPPED(frame);
    TEST_UNMAPPED(constFrame);

    QVERIFY(frame.map(QVideoFrame::WriteOnly));
    TEST_MAPPED(frame, QVideoFrame::WriteOnly);
    TEST_MAPPED(constFrame, QVideoFrame::WriteOnly);
    frame.unmap();
    TEST_UNMAPPED(frame);
    TEST_UNMAPPED(constFrame);

    QVERIFY(frame.map(QVideoFrame::ReadWrite));
    TEST_MAPPED(frame, QVideoFrame::ReadWrite);
    TEST_MAPPED(constFrame, QVideoFrame::ReadWrite);
    frame.unmap();
    TEST_UNMAPPED(frame);
    TEST_UNMAPPED(constFrame);
}

void tst_QVideoFrame::isReadable()
{
    QVideoFrame frame(QVideoFrameFormat(QSize(64, 64), QVideoFrameFormat::Format_ARGB8888));

    QVERIFY(!frame.isMapped());
    QVERIFY(!frame.isReadable());

    QVERIFY(frame.map(QVideoFrame::ReadOnly));
    QVERIFY(frame.isMapped());
    QVERIFY(frame.isReadable());
    frame.unmap();

    QVERIFY(frame.map(QVideoFrame::WriteOnly));
    QVERIFY(frame.isMapped());
    QVERIFY(!frame.isReadable());
    frame.unmap();

    QVERIFY(frame.map(QVideoFrame::ReadWrite));
    QVERIFY(frame.isMapped());
    QVERIFY(frame.isReadable());
    frame.unmap();
}

void tst_QVideoFrame::isWritable()
{
    QVideoFrame frame(QVideoFrameFormat(QSize(64, 64), QVideoFrameFormat::Format_ARGB8888));

    QVERIFY(!frame.isMapped());
    QVERIFY(!frame.isWritable());

    QVERIFY(frame.map(QVideoFrame::ReadOnly));
    QVERIFY(frame.isMapped());
    QVERIFY(!frame.isWritable());
    frame.unmap();

    QVERIFY(frame.map(QVideoFrame::WriteOnly));
    QVERIFY(frame.isMapped());
    QVERIFY(frame.isWritable());
    frame.unmap();

    QVERIFY(frame.map(QVideoFrame::ReadWrite));
    QVERIFY(frame.isMapped());
    QVERIFY(frame.isWritable());
    frame.unmap();
}

void tst_QVideoFrame::image_data()
{
    QTest::addColumn<QSize>("size");
    QTest::addColumn<QVideoFrameFormat::PixelFormat>("pixelFormat");

    QTest::newRow("64x64 ARGB32")
            << QSize(64, 64)
            << QVideoFrameFormat::Format_ARGB8888;

    QTest::newRow("64x64 ARGB32_Premultiplied")
            << QSize(64, 64)
            << QVideoFrameFormat::Format_ARGB8888_Premultiplied;

    QTest::newRow("64x64 RGB32")
            << QSize(64, 64)
            << QVideoFrameFormat::Format_XRGB8888;

    QTest::newRow("64x64 BGRA32")
            << QSize(64, 64)
            << QVideoFrameFormat::Format_BGRA8888;

    QTest::newRow("64x64 BGRA32_Premultiplied")
            << QSize(64, 64)
            << QVideoFrameFormat::Format_BGRA8888_Premultiplied;

    QTest::newRow("64x64 BGR32")
            << QSize(64, 64)
            << QVideoFrameFormat::Format_XBGR8888;

    QTest::newRow("64x64 AYUV")
            << QSize(64, 64)
            << QVideoFrameFormat::Format_AYUV;

    QTest::newRow("64x64 YUV420P")
            << QSize(64, 64)
            << QVideoFrameFormat::Format_YUV420P;

    QTest::newRow("64x64 YV12")
            << QSize(64, 64)
            << QVideoFrameFormat::Format_YV12;

    QTest::newRow("64x64 UYVY")
            << QSize(64, 64)
            << QVideoFrameFormat::Format_UYVY;

    QTest::newRow("64x64 YUYV")
            << QSize(64, 64)
            << QVideoFrameFormat::Format_YUYV;

    QTest::newRow("64x64 NV12")
            << QSize(64, 64)
            << QVideoFrameFormat::Format_NV12;

    QTest::newRow("64x64 NV21")
            << QSize(64, 64)
            << QVideoFrameFormat::Format_NV21;
}

void tst_QVideoFrame::image()
{
    QFETCH(QSize, size);
    QFETCH(QVideoFrameFormat::PixelFormat, pixelFormat);

    QVideoFrame frame(QVideoFrameFormat(size, pixelFormat));
    QImage img = frame.toImage();

    QVERIFY(!img.isNull());
    QCOMPARE(img.size(), size);
}

void tst_QVideoFrame::emptyData()
{
    QByteArray data(nullptr, 0);
    QVideoFrame f = QVideoFramePrivate::createFrame(
            std::make_unique<QMemoryVideoBuffer>(data, 600),
            QVideoFrameFormat(QSize(800, 600), QVideoFrameFormat::Format_ARGB8888));
    QVERIFY(!f.map(QVideoFrame::ReadOnly));
}

void tst_QVideoFrame::mirrored_doesntTakeValue_fromVideoFrameFormat()
{
    QVideoFrameFormat format(QSize(10, 20), QVideoFrameFormat::Format_ARGB8888);
    format.setMirrored(true);

    QVideoFrame frame(format);
    QVERIFY(!frame.mirrored());

    frame.setMirrored(false);
    frame.setRotation(QtVideo::Rotation::Clockwise180);
    QVERIFY(!frame.mirrored());
    QVERIFY(frame.surfaceFormat().isMirrored());
}

void tst_QVideoFrame::rotation_doesntTakeValue_fromVideoFrameFormat()
{
    QVideoFrameFormat format(QSize(10, 20), QVideoFrameFormat::Format_ARGB8888);
    format.setRotation(QtVideo::Rotation::Clockwise270);

    QVideoFrame frame(format);
    QCOMPARE(frame.rotation(), QtVideo::Rotation::None);

    frame.setRotation(QtVideo::Rotation::Clockwise180);

    QCOMPARE(frame.rotation(), QtVideo::Rotation::Clockwise180);
    QCOMPARE(frame.surfaceFormat().rotation(), QtVideo::Rotation::Clockwise270);
}

void tst_QVideoFrame::streamFrameRate_takesValue_fromVideoFrameFormat()
{
    QVideoFrameFormat format(QSize(10, 20), QVideoFrameFormat::Format_ARGB8888);
    format.setStreamFrameRate(20.);

    QVideoFrame frame(format);
    QCOMPARE(frame.streamFrameRate(), 20.);

    frame.setStreamFrameRate(25.);

    QCOMPARE(frame.streamFrameRate(), 25.);
    QCOMPARE(frame.surfaceFormat().streamFrameRate(), 25.);
}

void tst_QVideoFrame::constructor_createsInvalidFrame_whenCalledWithNullImage()
{
    const QVideoFrame frame{ QImage{} };
    QVERIFY(!frame.isValid());
}

void tst_QVideoFrame::constructor_createsInvalidFrame_whenCalledWithEmptyImage()
{
    {
        const QImage image{ QSize{}, QImage::Format_RGB32 };
        const QVideoFrame frame{ image };

        QVERIFY(!frame.isValid());
    }

    {
        const QImage image{ { 0, 0 }, QImage::Format_RGB32 };
        const QVideoFrame frame{ image };

        QVERIFY(!frame.isValid());
    }

    {
        const QImage image{ { 1, 0 }, QImage::Format_RGB32 };
        const QVideoFrame frame{ image };

        QVERIFY(!frame.isValid());
    }

    {
        const QImage image{ { 0, 1 }, QImage::Format_RGB32 };
        const QVideoFrame frame{ image };

        QVERIFY(!frame.isValid());
    }
}

void tst_QVideoFrame::constructor_createsInvalidFrame_whenCalledWithInvalidImageFormat()
{
    const QImage image{ { 1, 1 }, QImage::Format_Invalid};
    const QVideoFrame frame{ image };

    QVERIFY(!frame.isValid());
}

// clang-format off
void tst_QVideoFrame::constructor_createsFrameWithCorrectFormat_whenCalledWithSupportedImageFormats_data()
{
    QTest::addColumn<QImage::Format>("imageFormat");
    QTest::addColumn<QVideoFrameFormat::PixelFormat>("expectedFrameFormat");

    // Formats that do not require conversion
    QTest::newRow("Format_RGB32") << QImage::Format_RGB32 << QVideoFrameFormat::Format_BGRX8888;
    QTest::newRow("Format_ARGB32") << QImage::Format_ARGB32 << QVideoFrameFormat::Format_BGRA8888;
    QTest::newRow("Format_ARGB32_Premultiplied") << QImage::Format_ARGB32_Premultiplied << QVideoFrameFormat::Format_BGRA8888_Premultiplied;
    QTest::newRow("Format_RGBA8888") << QImage::Format_RGBA8888 << QVideoFrameFormat::Format_RGBA8888;
    QTest::newRow("Format_RGBA8888_Premultiplied") << QImage::Format_RGBA8888_Premultiplied << QVideoFrameFormat::Format_RGBX8888;
    QTest::newRow("Format_RGBX8888") << QImage::Format_RGBX8888 << QVideoFrameFormat::Format_RGBX8888;
    QTest::newRow("Format_Grayscale8") << QImage::Format_Grayscale8 << QVideoFrameFormat::Format_Y8;
    QTest::newRow("Format_Grayscale16") << QImage::Format_Grayscale16 << QVideoFrameFormat::Format_Y16;

    // Formats that require conversion of input image
    QTest::newRow("Format_Mono") << QImage::Format_Mono << QVideoFrameFormat::Format_BGRX8888;
    QTest::newRow("Format_MonoLSB") << QImage::Format_MonoLSB << QVideoFrameFormat::Format_BGRX8888;
    QTest::newRow("Format_Indexed8") << QImage::Format_Indexed8 << QVideoFrameFormat::Format_BGRX8888;
    QTest::newRow("Format_RGB16") << QImage::Format_RGB16 << QVideoFrameFormat::Format_BGRX8888;
    QTest::newRow("Format_ARGB8565_Premultiplied") << QImage::Format_ARGB8565_Premultiplied << QVideoFrameFormat::Format_BGRA8888_Premultiplied;
    QTest::newRow("Format_RGB666") << QImage::Format_RGB666 << QVideoFrameFormat::Format_BGRX8888;
    QTest::newRow("Format_ARGB6666_Premultiplied") << QImage::Format_ARGB6666_Premultiplied << QVideoFrameFormat::Format_BGRA8888_Premultiplied;
    QTest::newRow("Format_RGB555") << QImage::Format_RGB555 << QVideoFrameFormat::Format_BGRX8888;
    QTest::newRow("Format_ARGB8555_Premultiplied") << QImage::Format_ARGB8555_Premultiplied << QVideoFrameFormat::Format_BGRA8888_Premultiplied;
    QTest::newRow("Format_RGB888") << QImage::Format_RGB888 << QVideoFrameFormat::Format_BGRX8888;
    QTest::newRow("Format_RGB444") << QImage::Format_RGB444 << QVideoFrameFormat::Format_BGRX8888;
    QTest::newRow("Format_ARGB4444_Premultiplied") << QImage::Format_ARGB4444_Premultiplied << QVideoFrameFormat::Format_BGRA8888_Premultiplied;
    QTest::newRow("Format_BGR30") << QImage::Format_BGR30 << QVideoFrameFormat::Format_BGRX8888;
    QTest::newRow("Format_A2BGR30_Premultiplied") << QImage::Format_A2BGR30_Premultiplied << QVideoFrameFormat::Format_BGRA8888_Premultiplied;
    QTest::newRow("Format_RGB30") << QImage::Format_RGB30 << QVideoFrameFormat::Format_BGRX8888;
    QTest::newRow("Format_A2RGB30_Premultiplied") << QImage::Format_A2RGB30_Premultiplied << QVideoFrameFormat::Format_BGRA8888_Premultiplied;
    QTest::newRow("Format_Alpha8") << QImage::Format_Alpha8 << QVideoFrameFormat::Format_BGRA8888;
    QTest::newRow("Format_RGBX64") << QImage::Format_RGBX64 << QVideoFrameFormat::Format_BGRX8888;
    QTest::newRow("Format_RGBA64") << QImage::Format_RGBA64 << QVideoFrameFormat::Format_BGRA8888;
    QTest::newRow("Format_RGBA64_Premultiplied") << QImage::Format_RGBA64_Premultiplied << QVideoFrameFormat::Format_BGRA8888_Premultiplied;
    QTest::newRow("Format_BGR888") << QImage::Format_BGR888 << QVideoFrameFormat::Format_BGRX8888;
    QTest::newRow("Format_RGBX16FPx4") << QImage::Format_RGBX16FPx4 << QVideoFrameFormat::Format_BGRX8888;
    QTest::newRow("Format_RGBA16FPx4") << QImage::Format_RGBA16FPx4 << QVideoFrameFormat::Format_BGRA8888;
    QTest::newRow("Format_RGBA16FPx4_Premultiplied") << QImage::Format_RGBA16FPx4_Premultiplied << QVideoFrameFormat::Format_BGRA8888_Premultiplied;
    QTest::newRow("Format_RGBX32FPx4") << QImage::Format_RGBX32FPx4 << QVideoFrameFormat::Format_BGRX8888;
    QTest::newRow("Format_RGBA32FPx4") << QImage::Format_RGBA32FPx4 << QVideoFrameFormat::Format_BGRA8888;
    QTest::newRow("Format_RGBA32FPx4_Premultiplied") << QImage::Format_RGBA32FPx4_Premultiplied << QVideoFrameFormat::Format_BGRA8888_Premultiplied;
}
// clang-format on

void tst_QVideoFrame::constructor_createsFrameWithCorrectFormat_whenCalledWithSupportedImageFormats()
{
    QFETCH(const QImage::Format, imageFormat);
    QFETCH(QVideoFrameFormat::PixelFormat, expectedFrameFormat);

    const QImage image{ { 1, 1 }, imageFormat };
    const QVideoFrame frame{ image };

    QVERIFY(frame.isValid());
    QCOMPARE_EQ(frame.pixelFormat(), expectedFrameFormat);
}

// clang-format off
void tst_QVideoFrame::constructor_copiesImageData_whenCalledWithRGBFormats_data()
{
    QTest::addColumn<QImage::Format>("imageFormat");

    // Formats that do not require image conversion
    QTest::newRow("Format_RGB32") << QImage::Format_RGB32;
    QTest::newRow("Format_RGBX8888") << QImage::Format_RGBX8888;
    QTest::newRow("Format_ARGB32") << QImage::Format_ARGB32;
    QTest::newRow("Format_ARGB32_Premultiplied") << QImage::Format_ARGB32_Premultiplied;
    QTest::newRow("Format_RGBA8888") << QImage::Format_RGBA8888;
    QTest::newRow("Format_RGBA8888_Premultiplied") << QImage::Format_RGBA8888_Premultiplied;

    // Formats that require image conversion
    QTest::newRow("Format_Mono") << QImage::Format_Mono;
    QTest::newRow("Format_MonoLSB") << QImage::Format_MonoLSB;
    QTest::newRow("Format_Indexed8") << QImage::Format_Indexed8;
    QTest::newRow("Format_RGB16") << QImage::Format_RGB16;
    QTest::newRow("Format_ARGB8565_Premultiplied") << QImage::Format_ARGB8565_Premultiplied;
    QTest::newRow("Format_RGB666") << QImage::Format_RGB666;
    QTest::newRow("Format_ARGB6666_Premultiplied") << QImage::Format_ARGB6666_Premultiplied;
    QTest::newRow("Format_RGB555") << QImage::Format_RGB555;
    QTest::newRow("Format_ARGB8555_Premultiplied") << QImage::Format_ARGB8555_Premultiplied;
    QTest::newRow("Format_RGB888") << QImage::Format_RGB888;
    QTest::newRow("Format_RGB444") << QImage::Format_RGB444;
    QTest::newRow("Format_ARGB4444_Premultiplied") << QImage::Format_ARGB4444_Premultiplied;
    QTest::newRow("Format_BGR30") << QImage::Format_BGR30;
    QTest::newRow("Format_A2BGR30_Premultiplied") << QImage::Format_A2BGR30_Premultiplied;
    QTest::newRow("Format_RGB30") << QImage::Format_RGB30;
    QTest::newRow("Format_A2RGB30_Premultiplied") << QImage::Format_A2RGB30_Premultiplied;
    QTest::newRow("Format_Alpha8") << QImage::Format_Alpha8;
    QTest::newRow("Format_RGBX64") << QImage::Format_RGBX64;
    QTest::newRow("Format_RGBA64") << QImage::Format_RGBA64;
    QTest::newRow("Format_RGBA64_Premultiplied") << QImage::Format_RGBA64_Premultiplied;
    QTest::newRow("Format_BGR888") << QImage::Format_BGR888;
    QTest::newRow("Format_RGBX16FPx4") << QImage::Format_RGBX16FPx4;
    QTest::newRow("Format_RGBA16FPx4") << QImage::Format_RGBA16FPx4;
    QTest::newRow("Format_RGBA16FPx4_Premultiplied") << QImage::Format_RGBA16FPx4_Premultiplied;
    QTest::newRow("Format_RGBX32FPx4") << QImage::Format_RGBX32FPx4;
    QTest::newRow("Format_RGBA32FPx4") << QImage::Format_RGBA32FPx4;
    QTest::newRow("Format_RGBA32FPx4_Premultiplied") << QImage::Format_RGBA32FPx4_Premultiplied;
}

// clang-format on

void tst_QVideoFrame::constructor_copiesImageData_whenCalledWithRGBFormats()
{
    QFETCH(const QImage::Format, imageFormat);

    // Arrange
    const QImage image{ createTestImage(imageFormat) };

    // Act
    QVideoFrame frame{ image };

    // Assert
    QVERIFY(compareEq(frame, image));
}

QTEST_MAIN(tst_QVideoFrame)

#include "tst_qvideoframe.moc"
