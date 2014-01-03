/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

//TESTED_COMPONENT=src/multimedia

#include <QtTest/QtTest>

#include <qvideoframe.h>
#include <QtGui/QImage>
#include <QtCore/QPointer>

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
    ~tst_QVideoFrame();

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
    void imageDetach();
    void formatConversion_data();
    void formatConversion();

    void metadata();

    void debugType_data();
    void debugType();

    void debug_data();
    void debug();

    void debugFormat_data();
    void debugFormat();

    void isMapped();
    void isReadable();
    void isWritable();
};

Q_DECLARE_METATYPE(QImage::Format)

class QtTestVideoBuffer : public QObject, public QAbstractVideoBuffer
{
    Q_OBJECT
public:
    QtTestVideoBuffer()
        : QAbstractVideoBuffer(NoHandle) {}
    explicit QtTestVideoBuffer(QAbstractVideoBuffer::HandleType type)
        : QAbstractVideoBuffer(type) {}

    MapMode mapMode() const { return NotMapped; }

    uchar *map(MapMode, int *, int *) { return 0; }
    void unmap() {}
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
    QTest::addColumn<QVideoFrame::PixelFormat>("pixelFormat");
    QTest::addColumn<int>("bytes");
    QTest::addColumn<int>("bytesPerLine");

    QTest::newRow("64x64 ARGB32")
            << QSize(64, 64)
            << QVideoFrame::Format_ARGB32
            << 16384
            << 256;
    QTest::newRow("32x256 YUV420P")
            << QSize(32, 256)
            << QVideoFrame::Format_YUV420P
            << 13288
            << 32;
}

void tst_QVideoFrame::create()
{
    QFETCH(QSize, size);
    QFETCH(QVideoFrame::PixelFormat, pixelFormat);
    QFETCH(int, bytes);
    QFETCH(int, bytesPerLine);

    QVideoFrame frame(bytes, size, bytesPerLine, pixelFormat);

    QVERIFY(frame.isValid());
    QCOMPARE(frame.handleType(), QAbstractVideoBuffer::NoHandle);
    QCOMPARE(frame.handle(), QVariant());
    QCOMPARE(frame.pixelFormat(), pixelFormat);
    QCOMPARE(frame.size(), size);
    QCOMPARE(frame.width(), size.width());
    QCOMPARE(frame.height(), size.height());
    QCOMPARE(frame.fieldType(), QVideoFrame::ProgressiveFrame);
    QCOMPARE(frame.startTime(), qint64(-1));
    QCOMPARE(frame.endTime(), qint64(-1));
}

void tst_QVideoFrame::createInvalid_data()
{
    QTest::addColumn<QSize>("size");
    QTest::addColumn<QVideoFrame::PixelFormat>("pixelFormat");
    QTest::addColumn<int>("bytes");
    QTest::addColumn<int>("bytesPerLine");

    QTest::newRow("64x64 ARGB32 0 size")
            << QSize(64, 64)
            << QVideoFrame::Format_ARGB32
            << 0
            << 45;
    QTest::newRow("32x256 YUV420P negative size")
            << QSize(32, 256)
            << QVideoFrame::Format_YUV420P
            << -13288
            << 32;
}

void tst_QVideoFrame::createInvalid()
{
    QFETCH(QSize, size);
    QFETCH(QVideoFrame::PixelFormat, pixelFormat);
    QFETCH(int, bytes);
    QFETCH(int, bytesPerLine);

    QVideoFrame frame(bytes, size, bytesPerLine, pixelFormat);

    QVERIFY(!frame.isValid());
    QCOMPARE(frame.handleType(), QAbstractVideoBuffer::NoHandle);
    QCOMPARE(frame.handle(), QVariant());
    QCOMPARE(frame.pixelFormat(), pixelFormat);
    QCOMPARE(frame.size(), size);
    QCOMPARE(frame.width(), size.width());
    QCOMPARE(frame.height(), size.height());
    QCOMPARE(frame.fieldType(), QVideoFrame::ProgressiveFrame);
    QCOMPARE(frame.startTime(), qint64(-1));
    QCOMPARE(frame.endTime(), qint64(-1));
}

void tst_QVideoFrame::createFromBuffer_data()
{
    QTest::addColumn<QAbstractVideoBuffer::HandleType>("handleType");
    QTest::addColumn<QSize>("size");
    QTest::addColumn<QVideoFrame::PixelFormat>("pixelFormat");

    QTest::newRow("64x64 ARGB32 no handle")
            << QAbstractVideoBuffer::NoHandle
            << QSize(64, 64)
            << QVideoFrame::Format_ARGB32;
    QTest::newRow("64x64 ARGB32 gl handle")
            << QAbstractVideoBuffer::GLTextureHandle
            << QSize(64, 64)
            << QVideoFrame::Format_ARGB32;
    QTest::newRow("64x64 ARGB32 user handle")
            << QAbstractVideoBuffer::UserHandle
            << QSize(64, 64)
            << QVideoFrame::Format_ARGB32;
}

void tst_QVideoFrame::createFromBuffer()
{
    QFETCH(QAbstractVideoBuffer::HandleType, handleType);
    QFETCH(QSize, size);
    QFETCH(QVideoFrame::PixelFormat, pixelFormat);

    QVideoFrame frame(new QtTestVideoBuffer(handleType), size, pixelFormat);

    QVERIFY(frame.isValid());
    QCOMPARE(frame.handleType(), handleType);
    QCOMPARE(frame.pixelFormat(), pixelFormat);
    QCOMPARE(frame.size(), size);
    QCOMPARE(frame.width(), size.width());
    QCOMPARE(frame.height(), size.height());
    QCOMPARE(frame.fieldType(), QVideoFrame::ProgressiveFrame);
    QCOMPARE(frame.startTime(), qint64(-1));
    QCOMPARE(frame.endTime(), qint64(-1));
}

void tst_QVideoFrame::createFromImage_data()
{
    QTest::addColumn<QSize>("size");
    QTest::addColumn<QImage::Format>("imageFormat");
    QTest::addColumn<QVideoFrame::PixelFormat>("pixelFormat");

    QTest::newRow("64x64 RGB32")
            << QSize(64, 64)
            << QImage::Format_RGB32
            << QVideoFrame::Format_RGB32;
    QTest::newRow("12x45 RGB16")
            << QSize(12, 45)
            << QImage::Format_RGB16
            << QVideoFrame::Format_RGB565;
    QTest::newRow("19x46 ARGB32_Premultiplied")
            << QSize(19, 46)
            << QImage::Format_ARGB32_Premultiplied
            << QVideoFrame::Format_ARGB32_Premultiplied;
}

void tst_QVideoFrame::createFromImage()
{
    QFETCH(QSize, size);
    QFETCH(QImage::Format, imageFormat);
    QFETCH(QVideoFrame::PixelFormat, pixelFormat);

    const QImage image(size.width(), size.height(), imageFormat);

    QVideoFrame frame(image);

    QVERIFY(frame.isValid());
    QCOMPARE(frame.handleType(), QAbstractVideoBuffer::NoHandle);
    QCOMPARE(frame.pixelFormat(), pixelFormat);
    QCOMPARE(frame.size(), size);
    QCOMPARE(frame.width(), size.width());
    QCOMPARE(frame.height(), size.height());
    QCOMPARE(frame.fieldType(), QVideoFrame::ProgressiveFrame);
    QCOMPARE(frame.startTime(), qint64(-1));
    QCOMPARE(frame.endTime(), qint64(-1));
}

void tst_QVideoFrame::createFromIncompatibleImage()
{
    const QImage image(64, 64, QImage::Format_Mono);

    QVideoFrame frame(image);

    QVERIFY(!frame.isValid());
    QCOMPARE(frame.handleType(), QAbstractVideoBuffer::NoHandle);
    QCOMPARE(frame.pixelFormat(), QVideoFrame::Format_Invalid);
    QCOMPARE(frame.size(), QSize(64, 64));
    QCOMPARE(frame.width(), 64);
    QCOMPARE(frame.height(), 64);
    QCOMPARE(frame.fieldType(), QVideoFrame::ProgressiveFrame);
    QCOMPARE(frame.startTime(), qint64(-1));
    QCOMPARE(frame.endTime(), qint64(-1));
}

void tst_QVideoFrame::createNull()
{
    // Default ctor
    {
        QVideoFrame frame;

        QVERIFY(!frame.isValid());
        QCOMPARE(frame.handleType(), QAbstractVideoBuffer::NoHandle);
        QCOMPARE(frame.pixelFormat(), QVideoFrame::Format_Invalid);
        QCOMPARE(frame.size(), QSize());
        QCOMPARE(frame.width(), -1);
        QCOMPARE(frame.height(), -1);
        QCOMPARE(frame.fieldType(), QVideoFrame::ProgressiveFrame);
        QCOMPARE(frame.startTime(), qint64(-1));
        QCOMPARE(frame.endTime(), qint64(-1));
        QCOMPARE(frame.mapMode(), QAbstractVideoBuffer::NotMapped);
        QVERIFY(!frame.map(QAbstractVideoBuffer::ReadOnly));
        QVERIFY(!frame.map(QAbstractVideoBuffer::ReadWrite));
        QVERIFY(!frame.map(QAbstractVideoBuffer::WriteOnly));
        QCOMPARE(frame.isMapped(), false);
        frame.unmap(); // Shouldn't crash
        QCOMPARE(frame.isReadable(), false);
        QCOMPARE(frame.isWritable(), false);
    }

    // Null buffer (shouldn't crash)
    {
        QVideoFrame frame(0, QSize(1024,768), QVideoFrame::Format_ARGB32);
        QVERIFY(!frame.isValid());
        QCOMPARE(frame.handleType(), QAbstractVideoBuffer::NoHandle);
        QCOMPARE(frame.pixelFormat(), QVideoFrame::Format_ARGB32);
        QCOMPARE(frame.size(), QSize(1024, 768));
        QCOMPARE(frame.width(), 1024);
        QCOMPARE(frame.height(), 768);
        QCOMPARE(frame.fieldType(), QVideoFrame::ProgressiveFrame);
        QCOMPARE(frame.startTime(), qint64(-1));
        QCOMPARE(frame.endTime(), qint64(-1));
        QCOMPARE(frame.mapMode(), QAbstractVideoBuffer::NotMapped);
        QVERIFY(!frame.map(QAbstractVideoBuffer::ReadOnly));
        QVERIFY(!frame.map(QAbstractVideoBuffer::ReadWrite));
        QVERIFY(!frame.map(QAbstractVideoBuffer::WriteOnly));
        QCOMPARE(frame.isMapped(), false);
        frame.unmap(); // Shouldn't crash
        QCOMPARE(frame.isReadable(), false);
        QCOMPARE(frame.isWritable(), false);
    }
}

void tst_QVideoFrame::destructor()
{
    QPointer<QtTestVideoBuffer> buffer = new QtTestVideoBuffer;

    {
        QVideoFrame frame(buffer, QSize(4, 1), QVideoFrame::Format_ARGB32);
    }

    QVERIFY(buffer.isNull());
}

void tst_QVideoFrame::copy_data()
{
    QTest::addColumn<QAbstractVideoBuffer::HandleType>("handleType");
    QTest::addColumn<QSize>("size");
    QTest::addColumn<QVideoFrame::PixelFormat>("pixelFormat");
    QTest::addColumn<QVideoFrame::FieldType>("fieldType");
    QTest::addColumn<qint64>("startTime");
    QTest::addColumn<qint64>("endTime");

    QTest::newRow("64x64 ARGB32")
            << QAbstractVideoBuffer::GLTextureHandle
            << QSize(64, 64)
            << QVideoFrame::Format_ARGB32
            << QVideoFrame::TopField
            << qint64(63641740)
            << qint64(63641954);
    QTest::newRow("64x64 ARGB32")
            << QAbstractVideoBuffer::GLTextureHandle
            << QSize(64, 64)
            << QVideoFrame::Format_ARGB32
            << QVideoFrame::BottomField
            << qint64(63641740)
            << qint64(63641954);
    QTest::newRow("32x256 YUV420P")
            << QAbstractVideoBuffer::UserHandle
            << QSize(32, 256)
            << QVideoFrame::Format_YUV420P
            << QVideoFrame::InterlacedFrame
            << qint64(12345)
            << qint64(12389);
    QTest::newRow("1052x756 ARGB32")
            << QAbstractVideoBuffer::NoHandle
            << QSize(1052, 756)
            << QVideoFrame::Format_ARGB32
            << QVideoFrame::ProgressiveFrame
            << qint64(12345)
            << qint64(12389);
    QTest::newRow("32x256 YUV420P")
            << QAbstractVideoBuffer::UserHandle
            << QSize(32, 256)
            << QVideoFrame::Format_YUV420P
            << QVideoFrame::InterlacedFrame
            << qint64(12345)
            << qint64(12389);
}

void tst_QVideoFrame::copy()
{
    QFETCH(QAbstractVideoBuffer::HandleType, handleType);
    QFETCH(QSize, size);
    QFETCH(QVideoFrame::PixelFormat, pixelFormat);
    QFETCH(QVideoFrame::FieldType, fieldType);
    QFETCH(qint64, startTime);
    QFETCH(qint64, endTime);

    QPointer<QtTestVideoBuffer> buffer = new QtTestVideoBuffer(handleType);

    {
        QVideoFrame frame(buffer, size, pixelFormat);
        frame.setFieldType(QVideoFrame::FieldType(fieldType));
        frame.setStartTime(startTime);
        frame.setEndTime(endTime);

        QVERIFY(frame.isValid());
        QCOMPARE(frame.handleType(), handleType);
        QCOMPARE(frame.pixelFormat(), pixelFormat);
        QCOMPARE(frame.size(), size);
        QCOMPARE(frame.width(), size.width());
        QCOMPARE(frame.height(), size.height());
        QCOMPARE(frame.fieldType(), fieldType);
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
            QCOMPARE(otherFrame.fieldType(), fieldType);
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
            QCOMPARE(otherFrame.fieldType(), fieldType);
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
        QCOMPARE(frame.fieldType(), fieldType);
        QCOMPARE(frame.startTime(), startTime);
        QCOMPARE(frame.endTime(), qint64(-1));  // Explicitly shared.
    }

    QVERIFY(buffer.isNull());
}

void tst_QVideoFrame::assign_data()
{
    QTest::addColumn<QAbstractVideoBuffer::HandleType>("handleType");
    QTest::addColumn<QSize>("size");
    QTest::addColumn<QVideoFrame::PixelFormat>("pixelFormat");
    QTest::addColumn<QVideoFrame::FieldType>("fieldType");
    QTest::addColumn<qint64>("startTime");
    QTest::addColumn<qint64>("endTime");

    QTest::newRow("64x64 ARGB32")
            << QAbstractVideoBuffer::GLTextureHandle
            << QSize(64, 64)
            << QVideoFrame::Format_ARGB32
            << QVideoFrame::TopField
            << qint64(63641740)
            << qint64(63641954);
    QTest::newRow("32x256 YUV420P")
            << QAbstractVideoBuffer::UserHandle
            << QSize(32, 256)
            << QVideoFrame::Format_YUV420P
            << QVideoFrame::InterlacedFrame
            << qint64(12345)
            << qint64(12389);
}

void tst_QVideoFrame::assign()
{
    QFETCH(QAbstractVideoBuffer::HandleType, handleType);
    QFETCH(QSize, size);
    QFETCH(QVideoFrame::PixelFormat, pixelFormat);
    QFETCH(QVideoFrame::FieldType, fieldType);
    QFETCH(qint64, startTime);
    QFETCH(qint64, endTime);

    QPointer<QtTestVideoBuffer> buffer = new QtTestVideoBuffer(handleType);

    QVideoFrame frame;
    {
        QVideoFrame otherFrame(buffer, size, pixelFormat);
        otherFrame.setFieldType(fieldType);
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
        QCOMPARE(otherFrame.fieldType(), fieldType);
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
        QCOMPARE(otherFrame.fieldType(), fieldType);
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
    QCOMPARE(frame.fieldType(), fieldType);
    QCOMPARE(frame.startTime(), qint64(-1));
    QCOMPARE(frame.endTime(), endTime);

    frame = QVideoFrame();

    QVERIFY(buffer.isNull());

    QVERIFY(!frame.isValid());
    QCOMPARE(frame.handleType(), QAbstractVideoBuffer::NoHandle);
    QCOMPARE(frame.pixelFormat(), QVideoFrame::Format_Invalid);
    QCOMPARE(frame.size(), QSize());
    QCOMPARE(frame.width(), -1);
    QCOMPARE(frame.height(), -1);
    QCOMPARE(frame.fieldType(), QVideoFrame::ProgressiveFrame);
    QCOMPARE(frame.startTime(), qint64(-1));
    QCOMPARE(frame.endTime(), qint64(-1));
}

void tst_QVideoFrame::map_data()
{
    QTest::addColumn<QSize>("size");
    QTest::addColumn<int>("mappedBytes");
    QTest::addColumn<int>("bytesPerLine");
    QTest::addColumn<QVideoFrame::PixelFormat>("pixelFormat");
    QTest::addColumn<QAbstractVideoBuffer::MapMode>("mode");

    QTest::newRow("read-only")
            << QSize(64, 64)
            << 16384
            << 256
            << QVideoFrame::Format_ARGB32
            << QAbstractVideoBuffer::ReadOnly;

    QTest::newRow("write-only")
            << QSize(64, 64)
            << 16384
            << 256
            << QVideoFrame::Format_ARGB32
            << QAbstractVideoBuffer::WriteOnly;

    QTest::newRow("read-write")
            << QSize(64, 64)
            << 16384
            << 256
            << QVideoFrame::Format_ARGB32
            << QAbstractVideoBuffer::ReadWrite;
}

void tst_QVideoFrame::map()
{
    QFETCH(QSize, size);
    QFETCH(int, mappedBytes);
    QFETCH(int, bytesPerLine);
    QFETCH(QVideoFrame::PixelFormat, pixelFormat);
    QFETCH(QAbstractVideoBuffer::MapMode, mode);

    QVideoFrame frame(mappedBytes, size, bytesPerLine, pixelFormat);

    QVERIFY(!frame.bits());
    QCOMPARE(frame.mappedBytes(), 0);
    QCOMPARE(frame.bytesPerLine(), 0);
    QCOMPARE(frame.mapMode(), QAbstractVideoBuffer::NotMapped);

    QVERIFY(frame.map(mode));

    // Mapping multiple times is allowed in ReadOnly mode
    if (mode == QAbstractVideoBuffer::ReadOnly) {
        const uchar *bits = frame.bits();

        QVERIFY(frame.map(QAbstractVideoBuffer::ReadOnly));
        QVERIFY(frame.isMapped());
        QCOMPARE(frame.bits(), bits);

        frame.unmap();
        //frame should still be mapped after the first nested unmap
        QVERIFY(frame.isMapped());
        QCOMPARE(frame.bits(), bits);

        //re-mapping in Write or ReadWrite modes should fail
        QVERIFY(!frame.map(QAbstractVideoBuffer::WriteOnly));
        QVERIFY(!frame.map(QAbstractVideoBuffer::ReadWrite));
    } else {
        // Mapping twice in ReadWrite or WriteOnly modes should fail, but leave it mapped (and the mode is ignored)
        QVERIFY(!frame.map(mode));
        QVERIFY(!frame.map(QAbstractVideoBuffer::ReadOnly));
    }

    QVERIFY(frame.bits());
    QCOMPARE(frame.mappedBytes(), mappedBytes);
    QCOMPARE(frame.bytesPerLine(), bytesPerLine);
    QCOMPARE(frame.mapMode(), mode);

    frame.unmap();

    QVERIFY(!frame.bits());
    QCOMPARE(frame.mappedBytes(), 0);
    QCOMPARE(frame.bytesPerLine(), 0);
    QCOMPARE(frame.mapMode(), QAbstractVideoBuffer::NotMapped);
}

void tst_QVideoFrame::mapImage_data()
{
    QTest::addColumn<QSize>("size");
    QTest::addColumn<QImage::Format>("format");
    QTest::addColumn<QAbstractVideoBuffer::MapMode>("mode");

    QTest::newRow("read-only")
            << QSize(64, 64)
            << QImage::Format_ARGB32
            << QAbstractVideoBuffer::ReadOnly;

    QTest::newRow("write-only")
            << QSize(15, 106)
            << QImage::Format_RGB32
            << QAbstractVideoBuffer::WriteOnly;

    QTest::newRow("read-write")
            << QSize(23, 111)
            << QImage::Format_RGB16
            << QAbstractVideoBuffer::ReadWrite;
}

void tst_QVideoFrame::mapImage()
{
    QFETCH(QSize, size);
    QFETCH(QImage::Format, format);
    QFETCH(QAbstractVideoBuffer::MapMode, mode);

    QImage image(size.width(), size.height(), format);

    QVideoFrame frame(image);

    QVERIFY(!frame.bits());
    QCOMPARE(frame.mappedBytes(), 0);
    QCOMPARE(frame.bytesPerLine(), 0);
    QCOMPARE(frame.mapMode(), QAbstractVideoBuffer::NotMapped);

    QVERIFY(frame.map(mode));

    QVERIFY(frame.bits());
    QCOMPARE(frame.mappedBytes(), image.byteCount());
    QCOMPARE(frame.bytesPerLine(), image.bytesPerLine());
    QCOMPARE(frame.mapMode(), mode);

    frame.unmap();

    QVERIFY(!frame.bits());
    QCOMPARE(frame.mappedBytes(), 0);
    QCOMPARE(frame.bytesPerLine(), 0);
    QCOMPARE(frame.mapMode(), QAbstractVideoBuffer::NotMapped);
}

void tst_QVideoFrame::imageDetach()
{
    const uint red = qRgb(255, 0, 0);
    const uint blue = qRgb(0, 0, 255);

    QImage image(8, 8, QImage::Format_RGB32);

    image.fill(red);
    QCOMPARE(image.pixel(4, 4), red);

    QVideoFrame frame(image);

    QVERIFY(frame.map(QAbstractVideoBuffer::ReadWrite));

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
    QTest::addColumn<QVideoFrame::PixelFormat>("pixelFormat");

    QTest::newRow("QImage::Format_RGB32 | QVideoFrame::Format_RGB32")
            << QImage::Format_RGB32
            << QVideoFrame::Format_RGB32;
    QTest::newRow("QImage::Format_ARGB32 | QVideoFrame::Format_ARGB32")
            << QImage::Format_ARGB32
            << QVideoFrame::Format_ARGB32;
    QTest::newRow("QImage::Format_ARGB32_Premultiplied | QVideoFrame::Format_ARGB32_Premultiplied")
            << QImage::Format_ARGB32_Premultiplied
            << QVideoFrame::Format_ARGB32_Premultiplied;
    QTest::newRow("QImage::Format_RGB16 | QVideoFrame::Format_RGB565")
            << QImage::Format_RGB16
            << QVideoFrame::Format_RGB565;
    QTest::newRow("QImage::Format_ARGB8565_Premultiplied | QVideoFrame::Format_ARGB8565_Premultiplied")
            << QImage::Format_ARGB8565_Premultiplied
            << QVideoFrame::Format_ARGB8565_Premultiplied;
    QTest::newRow("QImage::Format_RGB555 | QVideoFrame::Format_RGB555")
            << QImage::Format_RGB555
            << QVideoFrame::Format_RGB555;
    QTest::newRow("QImage::Format_RGB888 | QVideoFrame::Format_RGB24")
            << QImage::Format_RGB888
            << QVideoFrame::Format_RGB24;

    QTest::newRow("QImage::Format_MonoLSB")
            << QImage::Format_MonoLSB
            << QVideoFrame::Format_Invalid;
    QTest::newRow("QImage::Format_Indexed8")
            << QImage::Format_Indexed8
            << QVideoFrame::Format_Invalid;
    QTest::newRow("QImage::Format_ARGB6666_Premultiplied")
            << QImage::Format_ARGB6666_Premultiplied
            << QVideoFrame::Format_Invalid;
    QTest::newRow("QImage::Format_ARGB8555_Premultiplied")
            << QImage::Format_ARGB8555_Premultiplied
            << QVideoFrame::Format_Invalid;
    QTest::newRow("QImage::Format_RGB666")
            << QImage::Format_RGB666
            << QVideoFrame::Format_Invalid;
    QTest::newRow("QImage::Format_RGB444")
            << QImage::Format_RGB444
            << QVideoFrame::Format_Invalid;
    QTest::newRow("QImage::Format_ARGB4444_Premultiplied")
            << QImage::Format_ARGB4444_Premultiplied
            << QVideoFrame::Format_Invalid;

    QTest::newRow("QVideoFrame::Format_BGRA32")
            << QImage::Format_Invalid
            << QVideoFrame::Format_BGRA32;
    QTest::newRow("QVideoFrame::Format_BGRA32_Premultiplied")
            << QImage::Format_Invalid
            << QVideoFrame::Format_BGRA32_Premultiplied;
    QTest::newRow("QVideoFrame::Format_BGR32")
            << QImage::Format_Invalid
            << QVideoFrame::Format_BGR32;
    QTest::newRow("QVideoFrame::Format_BGR24")
            << QImage::Format_Invalid
            << QVideoFrame::Format_BGR24;
    QTest::newRow("QVideoFrame::Format_BGR565")
            << QImage::Format_Invalid
            << QVideoFrame::Format_BGR565;
    QTest::newRow("QVideoFrame::Format_BGR555")
            << QImage::Format_Invalid
            << QVideoFrame::Format_BGR555;
    QTest::newRow("QVideoFrame::Format_BGRA5658_Premultiplied")
            << QImage::Format_Invalid
            << QVideoFrame::Format_BGRA5658_Premultiplied;
    QTest::newRow("QVideoFrame::Format_AYUV444")
            << QImage::Format_Invalid
            << QVideoFrame::Format_AYUV444;
    QTest::newRow("QVideoFrame::Format_AYUV444_Premultiplied")
            << QImage::Format_Invalid
            << QVideoFrame::Format_AYUV444_Premultiplied;
    QTest::newRow("QVideoFrame::Format_YUV444")
            << QImage::Format_Invalid
            << QVideoFrame::Format_YUV444;
    QTest::newRow("QVideoFrame::Format_YUV420P")
            << QImage::Format_Invalid
            << QVideoFrame::Format_YUV420P;
    QTest::newRow("QVideoFrame::Format_YV12")
            << QImage::Format_Invalid
            << QVideoFrame::Format_YV12;
    QTest::newRow("QVideoFrame::Format_UYVY")
            << QImage::Format_Invalid
            << QVideoFrame::Format_UYVY;
    QTest::newRow("QVideoFrame::Format_YUYV")
            << QImage::Format_Invalid
            << QVideoFrame::Format_YUYV;
    QTest::newRow("QVideoFrame::Format_NV12")
            << QImage::Format_Invalid
            << QVideoFrame::Format_NV12;
    QTest::newRow("QVideoFrame::Format_NV21")
            << QImage::Format_Invalid
            << QVideoFrame::Format_NV21;
    QTest::newRow("QVideoFrame::Format_IMC1")
            << QImage::Format_Invalid
            << QVideoFrame::Format_IMC1;
    QTest::newRow("QVideoFrame::Format_IMC2")
            << QImage::Format_Invalid
            << QVideoFrame::Format_IMC2;
    QTest::newRow("QVideoFrame::Format_IMC3")
            << QImage::Format_Invalid
            << QVideoFrame::Format_IMC3;
    QTest::newRow("QVideoFrame::Format_IMC4")
            << QImage::Format_Invalid
            << QVideoFrame::Format_IMC4;
    QTest::newRow("QVideoFrame::Format_Y8")
            << QImage::Format_Invalid
            << QVideoFrame::Format_Y8;
    QTest::newRow("QVideoFrame::Format_Y16")
            << QImage::Format_Invalid
            << QVideoFrame::Format_Y16;
    QTest::newRow("QVideoFrame::Format_Jpeg")
            << QImage::Format_Invalid
            << QVideoFrame::Format_Jpeg;
    QTest::newRow("QVideoFrame::Format_CameraRaw")
            << QImage::Format_Invalid
            << QVideoFrame::Format_CameraRaw;
    QTest::newRow("QVideoFrame::Format_AdobeDng")
            << QImage::Format_Invalid
            << QVideoFrame::Format_AdobeDng;
    QTest::newRow("QVideoFrame::Format_User")
            << QImage::Format_Invalid
            << QVideoFrame::Format_User;
    QTest::newRow("QVideoFrame::Format_User + 1")
            << QImage::Format_Invalid
            << QVideoFrame::PixelFormat(QVideoFrame::Format_User + 1);
}

void tst_QVideoFrame::formatConversion()
{
    QFETCH(QImage::Format, imageFormat);
    QFETCH(QVideoFrame::PixelFormat, pixelFormat);

    QCOMPARE(QVideoFrame::pixelFormatFromImageFormat(imageFormat) == pixelFormat,
             imageFormat != QImage::Format_Invalid);

    QCOMPARE(QVideoFrame::imageFormatFromPixelFormat(pixelFormat) == imageFormat,
             pixelFormat != QVideoFrame::Format_Invalid);
}

void tst_QVideoFrame::metadata()
{
    // Simple metadata test
    QVideoFrame f;

    QCOMPARE(f.availableMetaData(), QVariantMap());
    f.setMetaData("frob", QVariant("string"));
    f.setMetaData("bar", QVariant(42));
    QCOMPARE(f.metaData("frob"), QVariant("string"));
    QCOMPARE(f.metaData("bar"), QVariant(42));

    QVariantMap map;
    map.insert("frob", QVariant("string"));
    map.insert("bar", QVariant(42));

    QCOMPARE(f.availableMetaData(), map);

    f.setMetaData("frob", QVariant(56));
    QCOMPARE(f.metaData("frob"), QVariant(56));

    f.setMetaData("frob", QVariant());
    QCOMPARE(f.metaData("frob"), QVariant());

    QCOMPARE(f.availableMetaData().count(), 1);

    f.setMetaData("frob", QVariant("")); // empty but not null
    QCOMPARE(f.availableMetaData().count(), 2);
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
    QCOMPARE(frame.mapMode(), QAbstractVideoBuffer::NotMapped); \
} while (0)

void tst_QVideoFrame::isMapped()
{
    QVideoFrame frame(16384, QSize(64, 64), 256,  QVideoFrame::Format_ARGB32);
    const QVideoFrame& constFrame(frame);

    TEST_UNMAPPED(frame);
    TEST_UNMAPPED(constFrame);

    QVERIFY(frame.map(QAbstractVideoBuffer::ReadOnly));
    TEST_MAPPED(frame, QAbstractVideoBuffer::ReadOnly);
    TEST_MAPPED(constFrame, QAbstractVideoBuffer::ReadOnly);
    frame.unmap();
    TEST_UNMAPPED(frame);
    TEST_UNMAPPED(constFrame);

    QVERIFY(frame.map(QAbstractVideoBuffer::WriteOnly));
    TEST_MAPPED(frame, QAbstractVideoBuffer::WriteOnly);
    TEST_MAPPED(constFrame, QAbstractVideoBuffer::WriteOnly);
    frame.unmap();
    TEST_UNMAPPED(frame);
    TEST_UNMAPPED(constFrame);

    QVERIFY(frame.map(QAbstractVideoBuffer::ReadWrite));
    TEST_MAPPED(frame, QAbstractVideoBuffer::ReadWrite);
    TEST_MAPPED(constFrame, QAbstractVideoBuffer::ReadWrite);
    frame.unmap();
    TEST_UNMAPPED(frame);
    TEST_UNMAPPED(constFrame);
}

void tst_QVideoFrame::isReadable()
{
    QVideoFrame frame(16384, QSize(64, 64), 256,  QVideoFrame::Format_ARGB32);

    QVERIFY(!frame.isMapped());
    QVERIFY(!frame.isReadable());

    QVERIFY(frame.map(QAbstractVideoBuffer::ReadOnly));
    QVERIFY(frame.isMapped());
    QVERIFY(frame.isReadable());
    frame.unmap();

    QVERIFY(frame.map(QAbstractVideoBuffer::WriteOnly));
    QVERIFY(frame.isMapped());
    QVERIFY(!frame.isReadable());
    frame.unmap();

    QVERIFY(frame.map(QAbstractVideoBuffer::ReadWrite));
    QVERIFY(frame.isMapped());
    QVERIFY(frame.isReadable());
    frame.unmap();
}

void tst_QVideoFrame::isWritable()
{
    QVideoFrame frame(16384, QSize(64, 64), 256,  QVideoFrame::Format_ARGB32);

    QVERIFY(!frame.isMapped());
    QVERIFY(!frame.isWritable());

    QVERIFY(frame.map(QAbstractVideoBuffer::ReadOnly));
    QVERIFY(frame.isMapped());
    QVERIFY(!frame.isWritable());
    frame.unmap();

    QVERIFY(frame.map(QAbstractVideoBuffer::WriteOnly));
    QVERIFY(frame.isMapped());
    QVERIFY(frame.isWritable());
    frame.unmap();

    QVERIFY(frame.map(QAbstractVideoBuffer::ReadWrite));
    QVERIFY(frame.isMapped());
    QVERIFY(frame.isWritable());
    frame.unmap();
}

void tst_QVideoFrame::debugType_data()
{
    QTest::addColumn<QVideoFrame::FieldType>("fieldType");
    QTest::addColumn<QString>("stringized");

    ADD_ENUM_TEST(ProgressiveFrame);
    ADD_ENUM_TEST(InterlacedFrame);
    ADD_ENUM_TEST(TopField);
    ADD_ENUM_TEST(BottomField);
}

void tst_QVideoFrame::debugType()
{
    QFETCH(QVideoFrame::FieldType, fieldType);
    QFETCH(QString, stringized);

    QTest::ignoreMessage(QtDebugMsg, stringized.toLatin1().constData());
    qDebug() << fieldType;
}

void tst_QVideoFrame::debug_data()
{
    QTest::addColumn<QVideoFrame>("frame");
    QTest::addColumn<QString>("stringized");

    QVideoFrame f;
    QTest::newRow("default") << f << QString::fromLatin1("QVideoFrame(QSize(-1, -1) , Format_Invalid, NoHandle, NotMapped, [no timestamp])");

    QVideoFrame f2;
    f2.setStartTime(12345);
    f2.setEndTime(8000000000LL);
    QTest::newRow("times") << f2 << QString::fromLatin1("QVideoFrame(QSize(-1, -1) , Format_Invalid, NoHandle, NotMapped, 0:00:00.12345 - 2:13:20.00)");

    QVideoFrame f3;
    f3.setFieldType(QVideoFrame::ProgressiveFrame);
    QTest::newRow("times prog") << f3 << QString::fromLatin1("QVideoFrame(QSize(-1, -1) , Format_Invalid, NoHandle, NotMapped, [no timestamp])");

    QVideoFrame f4;
    f4.setFieldType(QVideoFrame::TopField);
    QTest::newRow("times top") << f4 << QString::fromLatin1("QVideoFrame(QSize(-1, -1) , Format_Invalid, NoHandle, NotMapped, [no timestamp])");

    QVideoFrame f5;
    f5.setFieldType(QVideoFrame::TopField);
    f5.setEndTime(90000000000LL);
    QTest::newRow("end but no start") << f5 << QString::fromLatin1("QVideoFrame(QSize(-1, -1) , Format_Invalid, NoHandle, NotMapped, [no timestamp])");

    QVideoFrame f6;
    f6.setStartTime(12345000000LL);
    f6.setEndTime(80000000000LL);
    QTest::newRow("times big") << f6 << QString::fromLatin1("QVideoFrame(QSize(-1, -1) , Format_Invalid, NoHandle, NotMapped, 3:25:45.00 - 22:13:20.00)");

    QVideoFrame g(0, QSize(320,240), 640, QVideoFrame::Format_ARGB32);
    QTest::newRow("more valid") << g << QString::fromLatin1("QVideoFrame(QSize(320, 240) , Format_ARGB32, NoHandle, NotMapped, [no timestamp])");

    QVideoFrame g2(0, QSize(320,240), 640, QVideoFrame::Format_ARGB32);
    g2.setStartTime(9000000000LL);
    g2.setEndTime(9000000000LL);
    QTest::newRow("more valid") << g2 << QString::fromLatin1("QVideoFrame(QSize(320, 240) , Format_ARGB32, NoHandle, NotMapped, @2:30:00.00)");

    QVideoFrame g3(0, QSize(320,240), 640, QVideoFrame::Format_ARGB32);
    g3.setStartTime(900000LL);
    g3.setEndTime(900000LL);
    QTest::newRow("more valid single timestamp") << g3 << QString::fromLatin1("QVideoFrame(QSize(320, 240) , Format_ARGB32, NoHandle, NotMapped, @00:00.900000)");

    QVideoFrame g4(0, QSize(320,240), 640, QVideoFrame::Format_ARGB32);
    g4.setStartTime(200000000LL);
    g4.setEndTime(300000000LL);
    QTest::newRow("more valid") << g4 << QString::fromLatin1("QVideoFrame(QSize(320, 240) , Format_ARGB32, NoHandle, NotMapped, 03:20.00 - 05:00.00)");

    QVideoFrame g5(0, QSize(320,240), 640, QVideoFrame::Format_ARGB32);
    g5.setStartTime(200000000LL);
    QTest::newRow("more valid until forever") << g5 << QString::fromLatin1("QVideoFrame(QSize(320, 240) , Format_ARGB32, NoHandle, NotMapped, 03:20.00 - forever)");

    QVideoFrame g6(0, QSize(320,240), 640, QVideoFrame::Format_ARGB32);
    g6.setStartTime(9000000000LL);
    QTest::newRow("more valid for long forever") << g6 << QString::fromLatin1("QVideoFrame(QSize(320, 240) , Format_ARGB32, NoHandle, NotMapped, 2:30:00.00 - forever)");

    QVideoFrame g7(0, QSize(320,240), 640, QVideoFrame::Format_ARGB32);
    g7.setStartTime(9000000000LL);
    g7.setMetaData("bar", 42);
    QTest::newRow("more valid for long forever + metadata") << g7 << QString::fromLatin1("QVideoFrame(QSize(320, 240) , Format_ARGB32, NoHandle, NotMapped, 2:30:00.00 - forever, metaData: QMap((\"bar\", QVariant(int, 42) ) ) )");
}

void tst_QVideoFrame::debug()
{
    QFETCH(QVideoFrame, frame);
    QFETCH(QString, stringized);

    QTest::ignoreMessage(QtDebugMsg, stringized.toLatin1().constData());
    qDebug() << frame;
}

void tst_QVideoFrame::debugFormat_data()
{
    QTest::addColumn<QVideoFrame::PixelFormat>("format");
    QTest::addColumn<QString>("stringized");

    ADD_ENUM_TEST(Format_Invalid);
    ADD_ENUM_TEST(Format_ARGB32);
    ADD_ENUM_TEST(Format_ARGB32_Premultiplied);
    ADD_ENUM_TEST(Format_RGB32);
    ADD_ENUM_TEST(Format_RGB24);
    ADD_ENUM_TEST(Format_RGB565);
    ADD_ENUM_TEST(Format_RGB555);
    ADD_ENUM_TEST(Format_ARGB8565_Premultiplied);
    ADD_ENUM_TEST(Format_BGRA32);
    ADD_ENUM_TEST(Format_BGRA32_Premultiplied);
    ADD_ENUM_TEST(Format_BGR32);
    ADD_ENUM_TEST(Format_BGR24);
    ADD_ENUM_TEST(Format_BGR565);
    ADD_ENUM_TEST(Format_BGR555);
    ADD_ENUM_TEST(Format_BGRA5658_Premultiplied);

    ADD_ENUM_TEST(Format_AYUV444);
    ADD_ENUM_TEST(Format_AYUV444_Premultiplied);
    ADD_ENUM_TEST(Format_YUV444);
    ADD_ENUM_TEST(Format_YUV420P);
    ADD_ENUM_TEST(Format_YV12);
    ADD_ENUM_TEST(Format_UYVY);
    ADD_ENUM_TEST(Format_YUYV);
    ADD_ENUM_TEST(Format_NV12);
    ADD_ENUM_TEST(Format_NV21);
    ADD_ENUM_TEST(Format_IMC1);
    ADD_ENUM_TEST(Format_IMC2);
    ADD_ENUM_TEST(Format_IMC3);
    ADD_ENUM_TEST(Format_IMC4);
    ADD_ENUM_TEST(Format_Y8);
    ADD_ENUM_TEST(Format_Y16);

    ADD_ENUM_TEST(Format_Jpeg);

    ADD_ENUM_TEST(Format_CameraRaw);
    ADD_ENUM_TEST(Format_AdobeDng);

    // User enums are formatted differently
    QTest::newRow("user 1000") << QVideoFrame::Format_User << QString::fromLatin1("UserType(1000)");
    QTest::newRow("user 1005") << QVideoFrame::PixelFormat(QVideoFrame::Format_User + 5) << QString::fromLatin1("UserType(1005)");
}

void tst_QVideoFrame::debugFormat()
{
    QFETCH(QVideoFrame::PixelFormat, format);
    QFETCH(QString, stringized);

    QTest::ignoreMessage(QtDebugMsg, stringized.toLatin1().constData());
    qDebug() << format;
}

QTEST_MAIN(tst_QVideoFrame)

#include "tst_qvideoframe.moc"
