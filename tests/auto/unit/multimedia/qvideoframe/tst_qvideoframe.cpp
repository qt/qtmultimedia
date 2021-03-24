/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

//TESTED_COMPONENT=src/multimedia

#include <QtTest/QtTest>

#include <qvideoframe.h>
#include <qvideosurfaceformat.h>
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
    void createFromImage();
    void createFromIncompatibleImage();
    void createNull();
    void destructor();
    void copy_data();
    void copy();
    void assign_data();
    void assign();
    void map_data();
    void map();
    void mapImage_data();
    void mapImage();
    void mapPlanes_data();
    void mapPlanes();
    void imageDetach();
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
        mapData.nBytes = m_numBytes;
        mapData.nPlanes = m_planeCount;
        for (int i = 0; i < m_planeCount; ++i) {
            mapData.data[i] = m_data[i];
            mapData.bytesPerLine[i] = m_bytesPerLine[i];
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
    QTest::addColumn<QVideoSurfaceFormat::PixelFormat>("pixelFormat");
    QTest::addColumn<int>("bytes");
    QTest::addColumn<int>("bytesPerLine");

    QTest::newRow("64x64 ARGB32")
            << QSize(64, 64)
            << QVideoSurfaceFormat::Format_ARGB32
            << 16384
            << 256;
    QTest::newRow("32x256 YUV420P")
            << QSize(32, 256)
            << QVideoSurfaceFormat::Format_YUV420P
            << 13288
            << 32;
}

void tst_QVideoFrame::create()
{
    QFETCH(QSize, size);
    QFETCH(QVideoSurfaceFormat::PixelFormat, pixelFormat);
    QFETCH(int, bytes);
    QFETCH(int, bytesPerLine);

    QVideoFrame frame(bytes, bytesPerLine, QVideoSurfaceFormat(size, pixelFormat));

    QVERIFY(frame.isValid());
    QCOMPARE(frame.handleType(), QVideoFrame::NoHandle);
    QCOMPARE(frame.textureHandle(0), 0);
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
    QTest::addColumn<QVideoSurfaceFormat::PixelFormat>("pixelFormat");
    QTest::addColumn<int>("bytes");
    QTest::addColumn<int>("bytesPerLine");

    QTest::newRow("64x64 ARGB32 0 size")
            << QSize(64, 64)
            << QVideoSurfaceFormat::Format_ARGB32
            << 0
            << 45;
    QTest::newRow("32x256 YUV420P negative size")
            << QSize(32, 256)
            << QVideoSurfaceFormat::Format_YUV420P
            << -13288
            << 32;
}

void tst_QVideoFrame::createInvalid()
{
    QFETCH(QSize, size);
    QFETCH(QVideoSurfaceFormat::PixelFormat, pixelFormat);
    QFETCH(int, bytes);
    QFETCH(int, bytesPerLine);

    QVideoFrame frame(bytes, bytesPerLine, QVideoSurfaceFormat(size, pixelFormat));

    QVERIFY(!frame.isValid());
    QCOMPARE(frame.handleType(), QVideoFrame::NoHandle);
    QCOMPARE(frame.textureHandle(0), 0);
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
    QTest::addColumn<QVideoSurfaceFormat::PixelFormat>("pixelFormat");

    QTest::newRow("64x64 ARGB32 no handle")
            << QVideoFrame::NoHandle
            << QSize(64, 64)
            << QVideoSurfaceFormat::Format_ARGB32;
    QTest::newRow("64x64 ARGB32 gl handle")
            << QVideoFrame::RhiTextureHandle
            << QSize(64, 64)
            << QVideoSurfaceFormat::Format_ARGB32;
}

void tst_QVideoFrame::createFromBuffer()
{
    QFETCH(QVideoFrame::HandleType, handleType);
    QFETCH(QSize, size);
    QFETCH(QVideoSurfaceFormat::PixelFormat, pixelFormat);

    QVideoFrame frame(new QtTestDummyVideoBuffer(handleType), QVideoSurfaceFormat(size, pixelFormat));

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
    QTest::addColumn<QVideoSurfaceFormat::PixelFormat>("pixelFormat");

    QTest::newRow("64x64 RGB32")
            << QSize(64, 64)
            << QImage::Format_RGB32
            << QVideoSurfaceFormat::Format_RGB32;
    QTest::newRow("12x45 RGB16")
            << QSize(12, 45)
            << QImage::Format_RGB16
            << QVideoSurfaceFormat::Format_RGB565;
    QTest::newRow("19x46 ARGB32_Premultiplied")
            << QSize(19, 46)
            << QImage::Format_ARGB32_Premultiplied
            << QVideoSurfaceFormat::Format_ARGB32_Premultiplied;
}

void tst_QVideoFrame::createFromImage()
{
    QFETCH(QSize, size);
    QFETCH(QImage::Format, imageFormat);
    QFETCH(QVideoSurfaceFormat::PixelFormat, pixelFormat);

    const QImage image(size.width(), size.height(), imageFormat);

    QVideoFrame frame(image);

    QVERIFY(frame.isValid());
    QCOMPARE(frame.handleType(), QVideoFrame::NoHandle);
    QCOMPARE(frame.pixelFormat(), pixelFormat);
    QCOMPARE(frame.size(), size);
    QCOMPARE(frame.width(), size.width());
    QCOMPARE(frame.height(), size.height());
    QCOMPARE(frame.startTime(), qint64(-1));
    QCOMPARE(frame.endTime(), qint64(-1));
}

void tst_QVideoFrame::createFromIncompatibleImage()
{
    const QImage image(64, 64, QImage::Format_Mono);

    QVideoFrame frame(image);

    QVERIFY(!frame.isValid());
    QCOMPARE(frame.handleType(), QVideoFrame::NoHandle);
    QCOMPARE(frame.pixelFormat(), QVideoSurfaceFormat::Format_Invalid);
    QCOMPARE(frame.size(), QSize(64, 64));
    QCOMPARE(frame.width(), 64);
    QCOMPARE(frame.height(), 64);
    QCOMPARE(frame.startTime(), qint64(-1));
    QCOMPARE(frame.endTime(), qint64(-1));
}

void tst_QVideoFrame::createNull()
{
    // Default ctor
    {
        QVideoFrame frame;

        QVERIFY(!frame.isValid());
        QCOMPARE(frame.handleType(), QVideoFrame::NoHandle);
        QCOMPARE(frame.pixelFormat(), QVideoSurfaceFormat::Format_Invalid);
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
        QVideoFrame frame(nullptr, QVideoSurfaceFormat(QSize(1024,768), QVideoSurfaceFormat::Format_ARGB32));
        QVERIFY(!frame.isValid());
        QCOMPARE(frame.handleType(), QVideoFrame::NoHandle);
        QCOMPARE(frame.pixelFormat(), QVideoSurfaceFormat::Format_ARGB32);
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
        QVideoFrame frame(buffer, QVideoSurfaceFormat(QSize(4, 1), QVideoSurfaceFormat::Format_ARGB32));
    }

    QVERIFY(buffer.isNull());
}

void tst_QVideoFrame::copy_data()
{
    QTest::addColumn<QVideoFrame::HandleType>("handleType");
    QTest::addColumn<QSize>("size");
    QTest::addColumn<QVideoSurfaceFormat::PixelFormat>("pixelFormat");
    QTest::addColumn<qint64>("startTime");
    QTest::addColumn<qint64>("endTime");

    QTest::newRow("64x64 ARGB32")
            << QVideoFrame::RhiTextureHandle
            << QSize(64, 64)
            << QVideoSurfaceFormat::Format_ARGB32
            << qint64(63641740)
            << qint64(63641954);
    QTest::newRow("64x64 ARGB32")
            << QVideoFrame::RhiTextureHandle
            << QSize(64, 64)
            << QVideoSurfaceFormat::Format_ARGB32
            << qint64(63641740)
            << qint64(63641954);
    QTest::newRow("32x256 YUV420P")
            << QVideoFrame::NoHandle
            << QSize(32, 256)
            << QVideoSurfaceFormat::Format_YUV420P
            << qint64(12345)
            << qint64(12389);
    QTest::newRow("1052x756 ARGB32")
            << QVideoFrame::NoHandle
            << QSize(1052, 756)
            << QVideoSurfaceFormat::Format_ARGB32
            << qint64(12345)
            << qint64(12389);
    QTest::newRow("32x256 YUV420P")
            << QVideoFrame::NoHandle
            << QSize(32, 256)
            << QVideoSurfaceFormat::Format_YUV420P
            << qint64(12345)
            << qint64(12389);
}

void tst_QVideoFrame::copy()
{
    QFETCH(QVideoFrame::HandleType, handleType);
    QFETCH(QSize, size);
    QFETCH(QVideoSurfaceFormat::PixelFormat, pixelFormat);
    QFETCH(qint64, startTime);
    QFETCH(qint64, endTime);

    QPointer<QtTestDummyVideoBuffer> buffer = new QtTestDummyVideoBuffer(handleType);

    {
        QVideoFrame frame(buffer, QVideoSurfaceFormat(size, pixelFormat));
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
    QTest::addColumn<QVideoSurfaceFormat::PixelFormat>("pixelFormat");
    QTest::addColumn<qint64>("startTime");
    QTest::addColumn<qint64>("endTime");

    QTest::newRow("64x64 ARGB32")
            << QVideoFrame::RhiTextureHandle
            << QSize(64, 64)
            << QVideoSurfaceFormat::Format_ARGB32
            << qint64(63641740)
            << qint64(63641954);
    QTest::newRow("32x256 YUV420P")
            << QVideoFrame::NoHandle
            << QSize(32, 256)
            << QVideoSurfaceFormat::Format_YUV420P
            << qint64(12345)
            << qint64(12389);
}

void tst_QVideoFrame::assign()
{
    QFETCH(QVideoFrame::HandleType, handleType);
    QFETCH(QSize, size);
    QFETCH(QVideoSurfaceFormat::PixelFormat, pixelFormat);
    QFETCH(qint64, startTime);
    QFETCH(qint64, endTime);

    QPointer<QtTestDummyVideoBuffer> buffer = new QtTestDummyVideoBuffer(handleType);

    QVideoFrame frame;
    {
        QVideoFrame otherFrame(buffer, QVideoSurfaceFormat(size, pixelFormat));
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
    QCOMPARE(frame.pixelFormat(), QVideoSurfaceFormat::Format_Invalid);
    QCOMPARE(frame.size(), QSize());
    QCOMPARE(frame.width(), -1);
    QCOMPARE(frame.height(), -1);
    QCOMPARE(frame.startTime(), qint64(-1));
    QCOMPARE(frame.endTime(), qint64(-1));
}

void tst_QVideoFrame::map_data()
{
    QTest::addColumn<QSize>("size");
    QTest::addColumn<int>("mappedBytes");
    QTest::addColumn<int>("bytesPerLine");
    QTest::addColumn<QVideoSurfaceFormat::PixelFormat>("pixelFormat");
    QTest::addColumn<QVideoFrame::MapMode>("mode");

    QTest::newRow("read-only")
            << QSize(64, 64)
            << 16384
            << 256
            << QVideoSurfaceFormat::Format_ARGB32
            << QVideoFrame::ReadOnly;

    QTest::newRow("write-only")
            << QSize(64, 64)
            << 16384
            << 256
            << QVideoSurfaceFormat::Format_ARGB32
            << QVideoFrame::WriteOnly;

    QTest::newRow("read-write")
            << QSize(64, 64)
            << 16384
            << 256
            << QVideoSurfaceFormat::Format_ARGB32
            << QVideoFrame::ReadWrite;
}

void tst_QVideoFrame::map()
{
    QFETCH(QSize, size);
    QFETCH(int, mappedBytes);
    QFETCH(int, bytesPerLine);
    QFETCH(QVideoSurfaceFormat::PixelFormat, pixelFormat);
    QFETCH(QVideoFrame::MapMode, mode);

    QVideoFrame frame(mappedBytes, bytesPerLine, QVideoSurfaceFormat(size, pixelFormat));

    QVERIFY(!frame.bits());
    QCOMPARE(frame.mappedBytes(), 0);
    QCOMPARE(frame.bytesPerLine(), 0);
    QCOMPARE(frame.mapMode(), QVideoFrame::NotMapped);

    QVERIFY(frame.map(mode));

    // Mapping multiple times is allowed in ReadOnly mode
    if (mode == QVideoFrame::ReadOnly) {
        const uchar *bits = frame.bits();

        QVERIFY(frame.map(QVideoFrame::ReadOnly));
        QVERIFY(frame.isMapped());
        QCOMPARE(frame.bits(), bits);

        frame.unmap();
        //frame should still be mapped after the first nested unmap
        QVERIFY(frame.isMapped());
        QCOMPARE(frame.bits(), bits);

        //re-mapping in Write or ReadWrite modes should fail
        QVERIFY(!frame.map(QVideoFrame::WriteOnly));
        QVERIFY(!frame.map(QVideoFrame::ReadWrite));
    } else {
        // Mapping twice in ReadWrite or WriteOnly modes should fail, but leave it mapped (and the mode is ignored)
        QVERIFY(!frame.map(mode));
        QVERIFY(!frame.map(QVideoFrame::ReadOnly));
    }

    QVERIFY(frame.bits());
    QCOMPARE(frame.mappedBytes(), mappedBytes);
    QCOMPARE(frame.bytesPerLine(), bytesPerLine);
    QCOMPARE(frame.mapMode(), mode);

    frame.unmap();

    QVERIFY(!frame.bits());
    QCOMPARE(frame.mappedBytes(), 0);
    QCOMPARE(frame.bytesPerLine(), 0);
    QCOMPARE(frame.mapMode(), QVideoFrame::NotMapped);
}

void tst_QVideoFrame::mapImage_data()
{
    QTest::addColumn<QSize>("size");
    QTest::addColumn<QImage::Format>("format");
    QTest::addColumn<QVideoFrame::MapMode>("mode");

    QTest::newRow("read-only")
            << QSize(64, 64)
            << QImage::Format_ARGB32
            << QVideoFrame::ReadOnly;

    QTest::newRow("write-only")
            << QSize(15, 106)
            << QImage::Format_RGB32
            << QVideoFrame::WriteOnly;

    QTest::newRow("read-write")
            << QSize(23, 111)
            << QImage::Format_RGB16
            << QVideoFrame::ReadWrite;
}

void tst_QVideoFrame::mapImage()
{
    QFETCH(QSize, size);
    QFETCH(QImage::Format, format);
    QFETCH(QVideoFrame::MapMode, mode);

    QImage image(size.width(), size.height(), format);

    QVideoFrame frame(image);

    QVERIFY(!frame.bits());
    QCOMPARE(frame.mappedBytes(), 0);
    QCOMPARE(frame.bytesPerLine(), 0);
    QCOMPARE(frame.mapMode(), QVideoFrame::NotMapped);

    QVERIFY(frame.map(mode));

    QVERIFY(frame.bits());
    QCOMPARE(qsizetype(frame.mappedBytes()), image.sizeInBytes());
    QCOMPARE(frame.bytesPerLine(), image.bytesPerLine());
    QCOMPARE(frame.mapMode(), mode);

    frame.unmap();

    QVERIFY(!frame.bits());
    QCOMPARE(frame.mappedBytes(), 0);
    QCOMPARE(frame.bytesPerLine(), 0);
    QCOMPARE(frame.mapMode(), QVideoFrame::NotMapped);
}

void tst_QVideoFrame::mapPlanes_data()
{
    QTest::addColumn<QVideoFrame>("frame");
    QTest::addColumn<QList<int> >("strides");
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
        << QVideoFrame(planarBuffer, QVideoSurfaceFormat(QSize(64, 64), QVideoSurfaceFormat::Format_YUV420P))
        << (QList<int>() << 64 << 36 << 36)
        << (QList<int>() << 512 << 765);
    QTest::newRow("Format_YUV420P")
        << QVideoFrame(8096, 64, QVideoSurfaceFormat(QSize(60, 64), QVideoSurfaceFormat::Format_YUV420P))
        << (QList<int>() << 64 << 62 << 62)
        << (QList<int>() << 4096 << 6080);
    QTest::newRow("Format_YV12")
        << QVideoFrame(8096, 64, QVideoSurfaceFormat(QSize(60, 64), QVideoSurfaceFormat::Format_YV12))
        << (QList<int>() << 64 << 62 << 62)
        << (QList<int>() << 4096 << 6080);
    QTest::newRow("Format_NV12")
        << QVideoFrame(8096, 64, QVideoSurfaceFormat(QSize(60, 64), QVideoSurfaceFormat::Format_NV12))
        << (QList<int>() << 64 << 64)
        << (QList<int>() << 4096);
    QTest::newRow("Format_NV21")
        << QVideoFrame(8096, 64, QVideoSurfaceFormat(QSize(60, 64), QVideoSurfaceFormat::Format_NV21))
        << (QList<int>() << 64 << 64)
        << (QList<int>() << 4096);
    QTest::newRow("Format_IMC2")
        << QVideoFrame(8096, 64, QVideoSurfaceFormat(QSize(60, 64), QVideoSurfaceFormat::Format_IMC2))
        << (QList<int>() << 64 << 64)
        << (QList<int>() << 4096);
    QTest::newRow("Format_IMC4")
        << QVideoFrame(8096, 64, QVideoSurfaceFormat(QSize(60, 64), QVideoSurfaceFormat::Format_IMC4))
        << (QList<int>() << 64 << 64)
        << (QList<int>() << 4096);
    QTest::newRow("Format_IMC1")
        << QVideoFrame(8096, 64, QVideoSurfaceFormat(QSize(60, 64), QVideoSurfaceFormat::Format_IMC1))
        << (QList<int>() << 64 << 64 << 64)
        << (QList<int>() << 4096 << 6144);
    QTest::newRow("Format_IMC3")
        << QVideoFrame(8096, 64, QVideoSurfaceFormat(QSize(60, 64), QVideoSurfaceFormat::Format_IMC3))
        << (QList<int>() << 64 << 64 << 64)
        << (QList<int>() << 4096 << 6144);
    QTest::newRow("Format_ARGB32")
        << QVideoFrame(8096, 256, QVideoSurfaceFormat(QSize(60, 64), QVideoSurfaceFormat::Format_ARGB32))
        << (QList<int>() << 256)
        << (QList<int>());
}

void tst_QVideoFrame::mapPlanes()
{
    QFETCH(QVideoFrame, frame);
    QFETCH(QList<int>, strides);
    QFETCH(QList<int>, offsets);

    QCOMPARE(strides.count(), offsets.count() + 1);

    QCOMPARE(frame.map(QVideoFrame::ReadOnly), true);
    QCOMPARE(frame.planeCount(), strides.count());

    QVERIFY(strides.count() > 0);
    QCOMPARE(frame.bytesPerLine(0), strides.at(0));
    QVERIFY(frame.bits(0));

    if (strides.count() > 1) {
        QCOMPARE(frame.bytesPerLine(1), strides.at(1));
        QCOMPARE(int(frame.bits(1) - frame.bits(0)), offsets.at(0));
    }
    if (strides.count() > 2) {
        QCOMPARE(frame.bytesPerLine(2), strides.at(2));
        QCOMPARE(int(frame.bits(2) - frame.bits(0)), offsets.at(1));
    }
    if (strides.count() > 3) {
        QCOMPARE(frame.bytesPerLine(3), strides.at(3));
        QCOMPARE(int(frame.bits(3) - frame.bits(0)), offsets.at(0));
    }

    frame.unmap();
}

void tst_QVideoFrame::imageDetach()
{
    const uint red = qRgb(255, 0, 0);
    const uint blue = qRgb(0, 0, 255);

    QImage image(8, 8, QImage::Format_RGB32);

    image.fill(red);
    QCOMPARE(image.pixel(4, 4), red);

    QVideoFrame frame(image);

    QVERIFY(frame.map(QVideoFrame::ReadWrite));

    QImage frameImage(frame.bits(), 8, 8, frame.bytesPerLine(), QImage::Format_RGB32);

    QCOMPARE(frameImage.pixel(4, 4), red);

    frameImage.fill(blue);
    QCOMPARE(frameImage.pixel(4, 4), blue);

    // Original image has detached and is therefore unchanged.
    QCOMPARE(image.pixel(4, 4), red);
}

void tst_QVideoFrame::formatConversion_data()
{
    QTest::addColumn<QImage::Format>("imageFormat");
    QTest::addColumn<QVideoSurfaceFormat::PixelFormat>("pixelFormat");

    QTest::newRow("QImage::Format_RGB32 | QVideoSurfaceFormat::Format_RGB32")
            << QImage::Format_RGB32
            << QVideoSurfaceFormat::Format_RGB32;
    QTest::newRow("QImage::Format_ARGB32 | QVideoSurfaceFormat::Format_ARGB32")
            << QImage::Format_ARGB32
            << QVideoSurfaceFormat::Format_ARGB32;
    QTest::newRow("QImage::Format_ARGB32_Premultiplied | QVideoSurfaceFormat::Format_ARGB32_Premultiplied")
            << QImage::Format_ARGB32_Premultiplied
            << QVideoSurfaceFormat::Format_ARGB32_Premultiplied;
    QTest::newRow("QImage::Format_RGB16 | QVideoSurfaceFormat::Format_RGB565")
            << QImage::Format_RGB16
            << QVideoSurfaceFormat::Format_RGB565;
    QTest::newRow("QImage::Format_ARGB8565_Premultiplied | QVideoSurfaceFormat::Format_ARGB8565_Premultiplied")
            << QImage::Format_ARGB8565_Premultiplied
            << QVideoSurfaceFormat::Format_ARGB8565_Premultiplied;
    QTest::newRow("QImage::Format_RGB555 | QVideoSurfaceFormat::Format_RGB555")
            << QImage::Format_RGB555
            << QVideoSurfaceFormat::Format_RGB555;
    QTest::newRow("QImage::Format_RGB888 | QVideoSurfaceFormat::Format_RGB24")
            << QImage::Format_RGB888
            << QVideoSurfaceFormat::Format_RGB24;

    QTest::newRow("QImage::Format_MonoLSB")
            << QImage::Format_MonoLSB
            << QVideoSurfaceFormat::Format_Invalid;
    QTest::newRow("QImage::Format_Indexed8")
            << QImage::Format_Indexed8
            << QVideoSurfaceFormat::Format_Invalid;
    QTest::newRow("QImage::Format_ARGB6666_Premultiplied")
            << QImage::Format_ARGB6666_Premultiplied
            << QVideoSurfaceFormat::Format_Invalid;
    QTest::newRow("QImage::Format_ARGB8555_Premultiplied")
            << QImage::Format_ARGB8555_Premultiplied
            << QVideoSurfaceFormat::Format_Invalid;
    QTest::newRow("QImage::Format_RGB666")
            << QImage::Format_RGB666
            << QVideoSurfaceFormat::Format_Invalid;
    QTest::newRow("QImage::Format_RGB444")
            << QImage::Format_RGB444
            << QVideoSurfaceFormat::Format_Invalid;
    QTest::newRow("QImage::Format_ARGB4444_Premultiplied")
            << QImage::Format_ARGB4444_Premultiplied
            << QVideoSurfaceFormat::Format_Invalid;

    QTest::newRow("QVideoSurfaceFormat::Format_BGRA32")
            << QImage::Format_Invalid
            << QVideoSurfaceFormat::Format_BGRA32;
    QTest::newRow("QVideoSurfaceFormat::Format_BGRA32_Premultiplied")
            << QImage::Format_Invalid
            << QVideoSurfaceFormat::Format_BGRA32_Premultiplied;
    QTest::newRow("QVideoSurfaceFormat::Format_BGR32")
            << QImage::Format_Invalid
            << QVideoSurfaceFormat::Format_BGR32;
    QTest::newRow("QVideoSurfaceFormat::Format_BGR24")
            << QImage::Format_Invalid
            << QVideoSurfaceFormat::Format_BGR24;
    QTest::newRow("QVideoSurfaceFormat::Format_BGR565")
            << QImage::Format_Invalid
            << QVideoSurfaceFormat::Format_BGR565;
    QTest::newRow("QVideoSurfaceFormat::Format_BGR555")
            << QImage::Format_Invalid
            << QVideoSurfaceFormat::Format_BGR555;
    QTest::newRow("QVideoSurfaceFormat::Format_BGRA5658_Premultiplied")
            << QImage::Format_Invalid
            << QVideoSurfaceFormat::Format_BGRA5658_Premultiplied;
    QTest::newRow("QVideoSurfaceFormat::Format_AYUV444")
            << QImage::Format_Invalid
            << QVideoSurfaceFormat::Format_AYUV444;
    QTest::newRow("QVideoSurfaceFormat::Format_AYUV444_Premultiplied")
            << QImage::Format_Invalid
            << QVideoSurfaceFormat::Format_AYUV444_Premultiplied;
    QTest::newRow("QVideoSurfaceFormat::Format_YUV444")
            << QImage::Format_Invalid
            << QVideoSurfaceFormat::Format_YUV444;
    QTest::newRow("QVideoSurfaceFormat::Format_YUV420P")
            << QImage::Format_Invalid
            << QVideoSurfaceFormat::Format_YUV420P;
    QTest::newRow("QVideoSurfaceFormat::Format_YV12")
            << QImage::Format_Invalid
            << QVideoSurfaceFormat::Format_YV12;
    QTest::newRow("QVideoSurfaceFormat::Format_UYVY")
            << QImage::Format_Invalid
            << QVideoSurfaceFormat::Format_UYVY;
    QTest::newRow("QVideoSurfaceFormat::Format_YUYV")
            << QImage::Format_Invalid
            << QVideoSurfaceFormat::Format_YUYV;
    QTest::newRow("QVideoSurfaceFormat::Format_NV12")
            << QImage::Format_Invalid
            << QVideoSurfaceFormat::Format_NV12;
    QTest::newRow("QVideoSurfaceFormat::Format_NV21")
            << QImage::Format_Invalid
            << QVideoSurfaceFormat::Format_NV21;
    QTest::newRow("QVideoSurfaceFormat::Format_IMC1")
            << QImage::Format_Invalid
            << QVideoSurfaceFormat::Format_IMC1;
    QTest::newRow("QVideoSurfaceFormat::Format_IMC2")
            << QImage::Format_Invalid
            << QVideoSurfaceFormat::Format_IMC2;
    QTest::newRow("QVideoSurfaceFormat::Format_IMC3")
            << QImage::Format_Invalid
            << QVideoSurfaceFormat::Format_IMC3;
    QTest::newRow("QVideoSurfaceFormat::Format_IMC4")
            << QImage::Format_Invalid
            << QVideoSurfaceFormat::Format_IMC4;
    QTest::newRow("QVideoSurfaceFormat::Format_Y8")
            << QImage::Format_Invalid
            << QVideoSurfaceFormat::Format_Y8;
    QTest::newRow("QVideoSurfaceFormat::Format_Y16")
            << QImage::Format_Invalid
            << QVideoSurfaceFormat::Format_Y16;
    QTest::newRow("QVideoSurfaceFormat::Format_Jpeg")
            << QImage::Format_Invalid
            << QVideoSurfaceFormat::Format_Jpeg;
}

void tst_QVideoFrame::formatConversion()
{
    QFETCH(QImage::Format, imageFormat);
    QFETCH(QVideoSurfaceFormat::PixelFormat, pixelFormat);

    QCOMPARE(QVideoSurfaceFormat::pixelFormatFromImageFormat(imageFormat) == pixelFormat,
             imageFormat != QImage::Format_Invalid);

    QCOMPARE(QVideoSurfaceFormat::imageFormatFromPixelFormat(pixelFormat) == imageFormat,
             pixelFormat != QVideoSurfaceFormat::Format_Invalid);
}

#define TEST_MAPPED(frame, mode) \
do { \
    QVERIFY(frame.bits()); \
    QVERIFY(frame.isMapped()); \
    QCOMPARE(frame.mappedBytes(), 16384); \
    QCOMPARE(frame.bytesPerLine(), 256); \
    QCOMPARE(frame.mapMode(), mode); \
} while (0)

#define TEST_UNMAPPED(frame) \
do { \
    QVERIFY(!frame.bits()); \
    QVERIFY(!frame.isMapped()); \
    QCOMPARE(frame.mappedBytes(), 0); \
    QCOMPARE(frame.bytesPerLine(), 0); \
    QCOMPARE(frame.mapMode(), QVideoFrame::NotMapped); \
} while (0)

void tst_QVideoFrame::isMapped()
{
    QVideoFrame frame(16384, 256, QVideoSurfaceFormat(QSize(64, 64), QVideoSurfaceFormat::Format_ARGB32));
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
    QVideoFrame frame(16384, 256, QVideoSurfaceFormat(QSize(64, 64), QVideoSurfaceFormat::Format_ARGB32));

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
    QVideoFrame frame(16384, 256, QVideoSurfaceFormat(QSize(64, 64), QVideoSurfaceFormat::Format_ARGB32));

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
    QTest::addColumn<QVideoSurfaceFormat::PixelFormat>("pixelFormat");
    QTest::addColumn<int>("bytes");
    QTest::addColumn<int>("bytesPerLine");
    QTest::addColumn<QImage::Format>("imageFormat");

    QTest::newRow("64x64 ARGB32")
            << QSize(64, 64)
            << QVideoSurfaceFormat::Format_ARGB32
            << 16384
            << 256
            << QImage::Format_ARGB32;

    QTest::newRow("64x64 ARGB32_Premultiplied")
            << QSize(64, 64)
            << QVideoSurfaceFormat::Format_ARGB32_Premultiplied
            << 16384
            << 256
            << QImage::Format_ARGB32_Premultiplied;

    QTest::newRow("64x64 RGB32")
            << QSize(64, 64)
            << QVideoSurfaceFormat::Format_RGB32
            << 16384
            << 256
            << QImage::Format_RGB32;

    QTest::newRow("64x64 RGB24")
            << QSize(64, 64)
            << QVideoSurfaceFormat::Format_RGB24
            << 16384
            << 192
            << QImage::Format_RGB888;

    QTest::newRow("64x64 RGB565")
            << QSize(64, 64)
            << QVideoSurfaceFormat::Format_RGB565
            << 16384
            << 128
            << QImage::Format_RGB16;

    QTest::newRow("64x64 RGB555")
            << QSize(64, 64)
            << QVideoSurfaceFormat::Format_RGB555
            << 16384
            << 128
            << QImage::Format_RGB555;

    QTest::newRow("64x64 BGRA32")
            << QSize(64, 64)
            << QVideoSurfaceFormat::Format_BGRA32
            << 16384
            << 256
            << QImage::Format_ARGB32;

    QTest::newRow("64x64 BGRA32_Premultiplied")
            << QSize(64, 64)
            << QVideoSurfaceFormat::Format_BGRA32_Premultiplied
            << 16384
            << 256
            << QImage::Format_ARGB32;

    QTest::newRow("64x64 BGR32")
            << QSize(64, 64)
            << QVideoSurfaceFormat::Format_BGR32
            << 16384
            << 256
            << QImage::Format_ARGB32;

    QTest::newRow("64x64 BGR24")
            << QSize(64, 64)
            << QVideoSurfaceFormat::Format_BGR24
            << 16384
            << 256
            << QImage::Format_ARGB32;

    QTest::newRow("64x64 BGR565")
            << QSize(64, 64)
            << QVideoSurfaceFormat::Format_BGR565
            << 16384
            << 256
            << QImage::Format_ARGB32;

    QTest::newRow("64x64 BGR555")
            << QSize(64, 64)
            << QVideoSurfaceFormat::Format_BGR555
            << 16384
            << 256
            << QImage::Format_ARGB32;
    QTest::newRow("64x64 AYUV444")
            << QSize(64, 64)
            << QVideoSurfaceFormat::Format_AYUV444
            << 16384
            << 256
            << QImage::Format_ARGB32;

    QTest::newRow("64x64 YUV444")
            << QSize(64, 64)
            << QVideoSurfaceFormat::Format_YUV444
            << 16384
            << 256
            << QImage::Format_ARGB32;

    QTest::newRow("64x64 YUV420P")
            << QSize(64, 64)
            << QVideoSurfaceFormat::Format_YUV420P
            << 13288
            << 256
            << QImage::Format_ARGB32;

    QTest::newRow("64x64 YV12")
            << QSize(64, 64)
            << QVideoSurfaceFormat::Format_YV12
            << 16384
            << 256
            << QImage::Format_ARGB32;

    QTest::newRow("64x64 UYVY")
            << QSize(64, 64)
            << QVideoSurfaceFormat::Format_UYVY
            << 16384
            << 256
            << QImage::Format_ARGB32;

    QTest::newRow("64x64 YUYV")
            << QSize(64, 64)
            << QVideoSurfaceFormat::Format_YUYV
            << 16384
            << 256
            << QImage::Format_ARGB32;

    QTest::newRow("64x64 NV12")
            << QSize(64, 64)
            << QVideoSurfaceFormat::Format_NV12
            << 16384
            << 256
            << QImage::Format_ARGB32;

    QTest::newRow("64x64 NV21")
            << QSize(64, 64)
            << QVideoSurfaceFormat::Format_NV21
            << 16384
            << 256
            << QImage::Format_ARGB32;
}

void tst_QVideoFrame::image()
{
    QFETCH(QSize, size);
    QFETCH(QVideoSurfaceFormat::PixelFormat, pixelFormat);
    QFETCH(int, bytes);
    QFETCH(int, bytesPerLine);
    QFETCH(QImage::Format, imageFormat);

    QVideoFrame frame(bytes, bytesPerLine, QVideoSurfaceFormat(size, pixelFormat));
    QImage img = frame.toImage();

    QVERIFY(!img.isNull());
    QCOMPARE(img.format(), imageFormat);
    QCOMPARE(img.size(), size);
    QCOMPARE(img.bytesPerLine(), bytesPerLine);
}

void tst_QVideoFrame::emptyData()
{
    QByteArray data(nullptr, 0);
    QVideoFrame f(new QMemoryVideoBuffer(data, 600),
                  QVideoSurfaceFormat(QSize(800, 600), QVideoSurfaceFormat::Format_ARGB32));
    QVERIFY(!f.map(QVideoFrame::ReadOnly));
}

QTEST_MAIN(tst_QVideoFrame)

#include "tst_qvideoframe.moc"
