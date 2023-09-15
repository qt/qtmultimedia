// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

//TESTED_COMPONENT=src/multimedia

#include <QtTest/QtTest>

#include <qvideoframe.h>
#include <qvideoframeformat.h>
#include "private/qmemoryvideobuffer_p.h"
#include <QtGui/QImage>
#include <QtCore/QPointer>
#include <QtMultimedia/private/qtmultimedia-config_p.h>

// Adds an enum, and the stringized version
#define ADD_ENUM_TEST(x) \
    QTest::newRow(#x) \
        << QVideoFrame::x \
    << QString(QLatin1String(#x));


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
    void destructor();
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

    void isMapped();
    void isReadable();
    void isWritable();

    void image_data();
    void image();

    void emptyData();
};

class QtTestDummyVideoBuffer : public QObject, public QAbstractVideoBuffer
{
    Q_OBJECT
public:
    QtTestDummyVideoBuffer()
        : QAbstractVideoBuffer(QVideoFrame::NoHandle) {}
    explicit QtTestDummyVideoBuffer(QVideoFrame::HandleType type)
        : QAbstractVideoBuffer(type) {}

    [[nodiscard]] QVideoFrame::MapMode mapMode() const override { return QVideoFrame::NotMapped; }

    MapData map(QVideoFrame::MapMode) override { return {}; }
    void unmap() override {}
};

class QtTestVideoBuffer : public QAbstractVideoBuffer
{
public:
    QtTestVideoBuffer()
        : QAbstractVideoBuffer(QVideoFrame::NoHandle)
    {}
    explicit QtTestVideoBuffer(QVideoFrame::HandleType type)
        : QAbstractVideoBuffer(type)
    {}

    [[nodiscard]] QVideoFrame::MapMode mapMode() const override { return m_mapMode; }

    MapData map(QVideoFrame::MapMode mode) override
    {
        m_mapMode = mode;
        MapData mapData;
        int nBytes = m_numBytes;
        mapData.nPlanes = m_planeCount;
        for (int i = 0; i < m_planeCount; ++i) {
            mapData.data[i] = m_data[i];
            mapData.bytesPerLine[i] = m_bytesPerLine[i];
            if (i) {
                mapData.size[i-1] = m_data[i] - m_data[i-1];
                nBytes -= mapData.size[i-1];
            }
            mapData.size[i] = nBytes;
        }
        return mapData;
    }
    void unmap() override { m_mapMode = QVideoFrame::NotMapped; }

    uchar *m_data[4];
    int m_bytesPerLine[4];
    int m_planeCount = 0;
    int m_numBytes;
    QVideoFrame::MapMode m_mapMode = QVideoFrame::NotMapped;
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
    QTest::addColumn<int>("bytes");
    QTest::addColumn<int>("bytesPerLine");

    QTest::newRow("64x64 ARGB32")
            << QSize(64, 64)
            << QVideoFrameFormat::Format_ARGB8888;
    QTest::newRow("32x256 YUV420P")
            << QSize(32, 256)
            << QVideoFrameFormat::Format_YUV420P;
}

void tst_QVideoFrame::create()
{
    QFETCH(QSize, size);
    QFETCH(QVideoFrameFormat::PixelFormat, pixelFormat);

    QVideoFrame frame(QVideoFrameFormat(size, pixelFormat));

    QVERIFY(frame.isValid());
    QCOMPARE(frame.handleType(), QVideoFrame::NoHandle);
    QVERIFY(frame.videoBuffer() != nullptr);
    QCOMPARE(frame.videoBuffer()->textureHandle(0), 0u);
    QCOMPARE(frame.pixelFormat(), pixelFormat);
    QCOMPARE(frame.size(), size);
    QCOMPARE(frame.width(), size.width());
    QCOMPARE(frame.height(), size.height());
    QCOMPARE(frame.startTime(), qint64(-1));
    QCOMPARE(frame.endTime(), qint64(-1));
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
    QCOMPARE(frame.videoBuffer(), nullptr);
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

    QVideoFrame frame(new QtTestDummyVideoBuffer(handleType), QVideoFrameFormat(size, pixelFormat));

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
        QVideoFrame frame(nullptr, QVideoFrameFormat(QSize(1024,768), QVideoFrameFormat::Format_ARGB8888));
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

void tst_QVideoFrame::destructor()
{
    QPointer<QtTestDummyVideoBuffer> buffer = new QtTestDummyVideoBuffer;

    {
        QVideoFrame frame(buffer, QVideoFrameFormat(QSize(4, 1), QVideoFrameFormat::Format_ARGB8888));
    }

    QVERIFY(buffer.isNull());
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

    QPointer<QtTestDummyVideoBuffer> buffer = new QtTestDummyVideoBuffer(handleType);

    {
        QVideoFrame frame(buffer, QVideoFrameFormat(size, pixelFormat));
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

    QPointer<QtTestDummyVideoBuffer> buffer = new QtTestDummyVideoBuffer(handleType);

    QVideoFrame frame;
    {
        QVideoFrame otherFrame(buffer, QVideoFrameFormat(size, pixelFormat));
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

    QtTestVideoBuffer *planarBuffer = new QtTestVideoBuffer;
    planarBuffer->m_data[0] = bufferData;
    planarBuffer->m_data[1] = bufferData + 512;
    planarBuffer->m_data[2] = bufferData + 765;
    planarBuffer->m_bytesPerLine[0] = 64;
    planarBuffer->m_bytesPerLine[1] = 36;
    planarBuffer->m_bytesPerLine[2] = 36;
    planarBuffer->m_planeCount = 3;
    planarBuffer->m_numBytes = sizeof(bufferData);

    QTest::newRow("Planar")
        << QVideoFrame(planarBuffer, QVideoFrameFormat(QSize(64, 64), QVideoFrameFormat::Format_YUV420P))
        << (QList<int>() << 64 << 36 << 36)
        << (QList<int>() << 512 << 765);
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
}

void tst_QVideoFrame::formatConversion()
{
    QFETCH(QImage::Format, imageFormat);
    QFETCH(QVideoFrameFormat::PixelFormat, pixelFormat);

    if (imageFormat != QImage::Format_Invalid)
        QCOMPARE(QVideoFrameFormat::pixelFormatFromImageFormat(imageFormat), pixelFormat);

    if (pixelFormat != QVideoFrameFormat::Format_Invalid)
        QCOMPARE(QVideoFrameFormat::imageFormatFromPixelFormat(pixelFormat), imageFormat);
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
    QVideoFrame f(new QMemoryVideoBuffer(data, 600),
                  QVideoFrameFormat(QSize(800, 600), QVideoFrameFormat::Format_ARGB8888));
    QVERIFY(!f.map(QVideoFrame::ReadOnly));
}

QTEST_MAIN(tst_QVideoFrame)

#include "tst_qvideoframe.moc"
