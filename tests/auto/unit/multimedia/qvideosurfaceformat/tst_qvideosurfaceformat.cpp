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

#include <qvideosurfaceformat.h>

// Adds an enum, and the stringized version
#define ADD_ENUM_TEST(x) \
    QTest::newRow(#x) \
        << QVideoSurfaceFormat::x \
    << QString(QLatin1String(#x));

class tst_QVideoSurfaceFormat : public QObject
{
    Q_OBJECT
public:
    tst_QVideoSurfaceFormat();
    ~tst_QVideoSurfaceFormat() override;

public slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

private slots:
    void constructNull();
    void construct_data();
    void construct();
    void frameSize_data();
    void frameSize();
    void viewport_data();
    void viewport();
    void scanLineDirection_data();
    void scanLineDirection();
    void frameRate_data();
    void frameRate();
    void sizeHint_data();
    void sizeHint();
    void yCbCrColorSpaceEnum_data();
    void yCbCrColorSpaceEnum ();
    void compare();
    void copy();
    void assign();

    void isValid();
    void copyAllParameters ();
    void assignAllParameters ();
};

tst_QVideoSurfaceFormat::tst_QVideoSurfaceFormat()
{
}

tst_QVideoSurfaceFormat::~tst_QVideoSurfaceFormat()
{
}

void tst_QVideoSurfaceFormat::initTestCase()
{
}

void tst_QVideoSurfaceFormat::cleanupTestCase()
{
}

void tst_QVideoSurfaceFormat::init()
{
}

void tst_QVideoSurfaceFormat::cleanup()
{
}

void tst_QVideoSurfaceFormat::constructNull()
{
    QVideoSurfaceFormat format;

    QVERIFY(!format.isValid());
    QCOMPARE(format.pixelFormat(), QVideoFrame::Format_Invalid);
    QCOMPARE(format.frameSize(), QSize());
    QCOMPARE(format.frameWidth(), -1);
    QCOMPARE(format.frameHeight(), -1);
    QCOMPARE(format.viewport(), QRect());
    QCOMPARE(format.scanLineDirection(), QVideoSurfaceFormat::TopToBottom);
    QCOMPARE(format.frameRate(), 0.0);
    QCOMPARE(format.yCbCrColorSpace(), QVideoSurfaceFormat::YCbCr_Undefined);
}

void tst_QVideoSurfaceFormat::construct_data()
{
    QTest::addColumn<QSize>("frameSize");
    QTest::addColumn<QVideoFrame::PixelFormat>("pixelFormat");
    QTest::addColumn<bool>("valid");

    QTest::newRow("32x32 rgb32 no handle")
            << QSize(32, 32)
            << QVideoFrame::Format_RGB32
            << true;

    QTest::newRow("1024x768 YUV444 GL texture")
            << QSize(32, 32)
            << QVideoFrame::Format_YUV444
            << true;

    QTest::newRow("32x32 invalid no handle")
            << QSize(32, 32)
            << QVideoFrame::Format_Invalid
            << false;

    QTest::newRow("invalid size, rgb32 no handle")
            << QSize()
            << QVideoFrame::Format_RGB32
            << false;

    QTest::newRow("0x0 rgb32 no handle")
            << QSize(0,0)
            << QVideoFrame::Format_RGB32
            << true;
}

void tst_QVideoSurfaceFormat::construct()
{
    QFETCH(QSize, frameSize);
    QFETCH(QVideoFrame::PixelFormat, pixelFormat);
    QFETCH(bool, valid);

    QRect viewport(QPoint(0, 0), frameSize);

    QVideoSurfaceFormat format(frameSize, pixelFormat);

    QCOMPARE(format.pixelFormat(), pixelFormat);
    QCOMPARE(format.frameSize(), frameSize);
    QCOMPARE(format.frameWidth(), frameSize.width());
    QCOMPARE(format.frameHeight(), frameSize.height());
    QCOMPARE(format.isValid(), valid);
    QCOMPARE(format.viewport(), viewport);
    QCOMPARE(format.scanLineDirection(), QVideoSurfaceFormat::TopToBottom);
    QCOMPARE(format.frameRate(), 0.0);
    QCOMPARE(format.yCbCrColorSpace(), QVideoSurfaceFormat::YCbCr_Undefined);
}

void tst_QVideoSurfaceFormat::frameSize_data()
{
    QTest::addColumn<QSize>("initialSize");
    QTest::addColumn<QSize>("newSize");

    QTest::newRow("grow")
            << QSize(64, 64)
            << QSize(1024, 1024);
    QTest::newRow("shrink")
            << QSize(1024, 1024)
            << QSize(64, 64);
    QTest::newRow("unchanged")
            << QSize(512, 512)
            << QSize(512, 512);
}

void tst_QVideoSurfaceFormat::frameSize()
{
    QFETCH(QSize, initialSize);
    QFETCH(QSize, newSize);

    QVideoSurfaceFormat format(initialSize, QVideoFrame::Format_RGB32);

    format.setFrameSize(newSize);

    QCOMPARE(format.frameSize(), newSize);
    QCOMPARE(format.frameWidth(), newSize.width());
    QCOMPARE(format.frameHeight(), newSize.height());
}

void tst_QVideoSurfaceFormat::viewport_data()
{
    QTest::addColumn<QSize>("initialSize");
    QTest::addColumn<QRect>("viewport");
    QTest::addColumn<QSize>("newSize");
    QTest::addColumn<QRect>("expectedViewport");

    QTest::newRow("grow reset")
            << QSize(64, 64)
            << QRect(8, 8, 48, 48)
            << QSize(1024, 1024)
            << QRect(0, 0, 1024, 1024);
    QTest::newRow("shrink reset")
            << QSize(1024, 1024)
            << QRect(8, 8, 1008, 1008)
            << QSize(64, 64)
            << QRect(0, 0, 64, 64);
    QTest::newRow("unchanged reset")
            << QSize(512, 512)
            << QRect(8, 8, 496, 496)
            << QSize(512, 512)
            << QRect(0, 0, 512, 512);
}

void tst_QVideoSurfaceFormat::viewport()
{
    QFETCH(QSize, initialSize);
    QFETCH(QRect, viewport);
    QFETCH(QSize, newSize);
    QFETCH(QRect, expectedViewport);

    QRect initialViewport(QPoint(0, 0), initialSize);

    QVideoSurfaceFormat format(initialSize, QVideoFrame::Format_RGB32);

    format.setViewport(viewport);

    QCOMPARE(format.viewport(), viewport);

    format.setFrameSize(newSize);

    QCOMPARE(format.viewport(), expectedViewport);
}

void tst_QVideoSurfaceFormat::scanLineDirection_data()
{
    QTest::addColumn<QVideoSurfaceFormat::Direction>("direction");
    QTest::addColumn<QString>("stringized");

    ADD_ENUM_TEST(TopToBottom);
    ADD_ENUM_TEST(BottomToTop);
}

void tst_QVideoSurfaceFormat::scanLineDirection()
{
    QFETCH(QVideoSurfaceFormat::Direction, direction);
    QFETCH(QString, stringized);

    QVideoSurfaceFormat format(QSize(16, 16), QVideoFrame::Format_RGB32);

    format.setScanLineDirection(direction);

    QCOMPARE(format.scanLineDirection(), direction);

    QTest::ignoreMessage(QtDebugMsg, stringized.toLatin1().constData());
    qDebug() << direction;
}

void tst_QVideoSurfaceFormat::yCbCrColorSpaceEnum_data()
{
    QTest::addColumn<QVideoSurfaceFormat::YCbCrColorSpace>("colorspace");
    QTest::addColumn<QString>("stringized");

    ADD_ENUM_TEST(YCbCr_BT601);
    ADD_ENUM_TEST(YCbCr_BT709);
    ADD_ENUM_TEST(YCbCr_xvYCC601);
    ADD_ENUM_TEST(YCbCr_xvYCC709);
    ADD_ENUM_TEST(YCbCr_JPEG);
    ADD_ENUM_TEST(YCbCr_Undefined);
}

/* Test case for Enum YCbCr_BT601, YCbCr_xvYCC709 */
void tst_QVideoSurfaceFormat::yCbCrColorSpaceEnum()
{
    QFETCH(QVideoSurfaceFormat::YCbCrColorSpace, colorspace);
    QFETCH(QString, stringized);

    QVideoSurfaceFormat format(QSize(64, 64), QVideoFrame::Format_RGB32);
    format.setYCbCrColorSpace(colorspace);

    QCOMPARE(format.yCbCrColorSpace(), colorspace);

    QTest::ignoreMessage(QtDebugMsg, stringized.toLatin1().constData());
    qDebug() << colorspace;
}


void tst_QVideoSurfaceFormat::frameRate_data()
{
    QTest::addColumn<qreal>("frameRate");

    QTest::newRow("null")
            << qreal(0.0);
    QTest::newRow("1/1")
            << qreal(1.0);
    QTest::newRow("24/1")
            << qreal(24.0);
    QTest::newRow("15/2")
            << qreal(7.5);
}

void tst_QVideoSurfaceFormat::frameRate()
{
    QFETCH(qreal, frameRate);

    QVideoSurfaceFormat format(QSize(64, 64), QVideoFrame::Format_RGB32);

    format.setFrameRate(frameRate);

    QCOMPARE(format.frameRate(), frameRate);
}

void tst_QVideoSurfaceFormat::sizeHint_data()
{
    QTest::addColumn<QSize>("frameSize");
    QTest::addColumn<QRect>("viewport");
    QTest::addColumn<QSize>("sizeHint");

    QTest::newRow("(0, 0, 1024x768), 1:1")
            << QSize(1024, 768)
            << QRect(0, 0, 1024, 768)
            << QSize(1024, 768);
    QTest::newRow("(168, 84, 800x600), 1:1")
        << QSize(1024, 768)
        << QRect(168, 84, 800, 600)
        << QSize(800, 600);
}

void tst_QVideoSurfaceFormat::sizeHint()
{
    QFETCH(QSize, frameSize);
    QFETCH(QRect, viewport);
    QFETCH(QSize, sizeHint);

    QVideoSurfaceFormat format(frameSize, QVideoFrame::Format_RGB32);
    format.setViewport(viewport);

    QCOMPARE(format.sizeHint(), sizeHint);
}

void tst_QVideoSurfaceFormat::compare()
{
    QVideoSurfaceFormat format1(
            QSize(16, 16), QVideoFrame::Format_RGB32);
    QVideoSurfaceFormat format2(
            QSize(16, 16), QVideoFrame::Format_RGB32);
    QVideoSurfaceFormat format3(
            QSize(32, 32), QVideoFrame::Format_YUV444);
    QVideoSurfaceFormat format4(
            QSize(16, 16), QVideoFrame::Format_RGB32);

    QCOMPARE(format1 == format2, true);
    QCOMPARE(format1 != format2, false);
    QCOMPARE(format1 == format3, false);
    QCOMPARE(format1 != format3, true);
    QCOMPARE(format1 == format4, false);
    QCOMPARE(format1 != format4, true);

    format2.setFrameSize(1024, 768);

    // Not equal, frame size differs.
    QCOMPARE(format1 == format2, false);
    QCOMPARE(format1 != format2, true);

    format1.setFrameSize(1024, 768);

    // Equal.
    QCOMPARE(format1 == format2, true);
    QCOMPARE(format1 != format2, false);

    format1.setViewport(QRect(0, 0, 800, 600));
    format2.setViewport(QRect(112, 84, 800, 600));

    // Not equal, viewports differ.
    QCOMPARE(format1 == format2, false);
    QCOMPARE(format1 != format2, true);

    format1.setViewport(QRect(112, 84, 800, 600));

    // Equal.
    QCOMPARE(format1 == format2, true);
    QCOMPARE(format1 != format2, false);

    format2.setScanLineDirection(QVideoSurfaceFormat::BottomToTop);

    // Not equal scan line direction differs.
    QCOMPARE(format1 == format2, false);
    QCOMPARE(format1 != format2, true);

    format1.setScanLineDirection(QVideoSurfaceFormat::BottomToTop);

    // Equal.
    QCOMPARE(format1 == format2, true);
    QCOMPARE(format1 != format2, false);

    format1.setFrameRate(7.5);

    // Not equal frame rate differs.
    QCOMPARE(format1 == format2, false);
    QCOMPARE(format1 != format2, true);

    format2.setFrameRate(qreal(7.50001));

    // Equal.
    QCOMPARE(format1 == format2, true);
    QCOMPARE(format1 != format2, false);

    format2.setYCbCrColorSpace(QVideoSurfaceFormat::YCbCr_xvYCC601);

    // Not equal yuv color space differs.
    QCOMPARE(format1 == format2, false);
    QCOMPARE(format1 != format2, true);

    format1.setYCbCrColorSpace(QVideoSurfaceFormat::YCbCr_xvYCC601);

    // Equal.
    QCOMPARE(format1 == format2, true);
    QCOMPARE(format1 != format2, false);
}


void tst_QVideoSurfaceFormat::copy()
{
    QVideoSurfaceFormat original(
            QSize(1024, 768), QVideoFrame::Format_ARGB32);
    original.setScanLineDirection(QVideoSurfaceFormat::BottomToTop);

    QVideoSurfaceFormat copy(original);

    QCOMPARE(copy.pixelFormat(), QVideoFrame::Format_ARGB32);
    QCOMPARE(copy.frameSize(), QSize(1024, 768));
    QCOMPARE(copy.scanLineDirection(), QVideoSurfaceFormat::BottomToTop);

    QCOMPARE(original == copy, true);
    QCOMPARE(original != copy, false);

    copy.setScanLineDirection(QVideoSurfaceFormat::TopToBottom);

    QCOMPARE(copy.scanLineDirection(), QVideoSurfaceFormat::TopToBottom);

    QCOMPARE(original.scanLineDirection(), QVideoSurfaceFormat::BottomToTop);

    QCOMPARE(original == copy, false);
    QCOMPARE(original != copy, true);
}

void tst_QVideoSurfaceFormat::assign()
{
    QVideoSurfaceFormat copy(
            QSize(64, 64), QVideoFrame::Format_AYUV444);

    QVideoSurfaceFormat original(
            QSize(1024, 768), QVideoFrame::Format_ARGB32);
    original.setScanLineDirection(QVideoSurfaceFormat::BottomToTop);

    copy = original;

    QCOMPARE(copy.pixelFormat(), QVideoFrame::Format_ARGB32);
    QCOMPARE(copy.frameSize(), QSize(1024, 768));
    QCOMPARE(copy.scanLineDirection(), QVideoSurfaceFormat::BottomToTop);

    QCOMPARE(original == copy, true);
    QCOMPARE(original != copy, false);

    copy.setScanLineDirection(QVideoSurfaceFormat::TopToBottom);

    QCOMPARE(copy.scanLineDirection(), QVideoSurfaceFormat::TopToBottom);

    QCOMPARE(original.scanLineDirection(), QVideoSurfaceFormat::BottomToTop);

    QCOMPARE(original == copy, false);
    QCOMPARE(original != copy, true);
}

/* Test case for api isValid */
void tst_QVideoSurfaceFormat::isValid()
{
    /* When both pixel format and framesize is not valid */
    QVideoSurfaceFormat format;
    QVERIFY(!format.isValid());

    /* When framesize is valid and pixel format is not valid */
    format.setFrameSize(64,64);
    QVERIFY(format.frameSize() == QSize(64,64));
    QVERIFY(!format.pixelFormat());
    QVERIFY(!format.isValid());

    /* When both the pixel format and framesize is valid. */
    QVideoSurfaceFormat format1(QSize(32, 32), QVideoFrame::Format_AYUV444);
    QVERIFY(format1.isValid());

    /* When pixel format is valid and frame size is not valid */
    format1.setFrameSize(-1,-1);
    QVERIFY(!format1.frameSize().isValid());
    QVERIFY(!format1.isValid());
}

/* Test case for copy constructor with all the parameters. */
void tst_QVideoSurfaceFormat::copyAllParameters()
{
    /* Create the instance and set all the parameters. */
    QVideoSurfaceFormat original(
            QSize(1024, 768), QVideoFrame::Format_ARGB32);

    original.setScanLineDirection(QVideoSurfaceFormat::BottomToTop);
    original.setViewport(QRect(0, 0, 1024, 1024));
    original.setFrameRate(qreal(15.0));
    original.setYCbCrColorSpace(QVideoSurfaceFormat::YCbCr_BT709);

    /* Copy the original instance to copy and verify if both the instances
      have the same parameters. */
    QVideoSurfaceFormat copy(original);

    QCOMPARE(copy.pixelFormat(), QVideoFrame::Format_ARGB32);
    QCOMPARE(copy.frameSize(), QSize(1024, 768));
    QCOMPARE(copy.scanLineDirection(), QVideoSurfaceFormat::BottomToTop);
    QCOMPARE(copy.viewport(), QRect(0, 0, 1024, 1024));
    QCOMPARE(copy.frameRate(), qreal(15.0));
    QCOMPARE(copy.yCbCrColorSpace(), QVideoSurfaceFormat::YCbCr_BT709);

    /* Verify if both the instances are eqaul */
    QCOMPARE(original == copy, true);
    QCOMPARE(original != copy, false);
}

/* Test case for copy constructor with all the parameters. */
void tst_QVideoSurfaceFormat::assignAllParameters()
{
    /* Create the instance and set all the parameters. */
    QVideoSurfaceFormat copy(
            QSize(64, 64), QVideoFrame::Format_AYUV444);
    copy.setScanLineDirection(QVideoSurfaceFormat::TopToBottom);
    copy.setViewport(QRect(0, 0, 640, 320));
    copy.setFrameRate(qreal(7.5));
    copy.setYCbCrColorSpace(QVideoSurfaceFormat::YCbCr_BT601);

    /* Create the instance and set all the parameters. */
    QVideoSurfaceFormat original(
            QSize(1024, 768), QVideoFrame::Format_ARGB32);
    original.setScanLineDirection(QVideoSurfaceFormat::BottomToTop);
    original.setViewport(QRect(0, 0, 1024, 1024));
    original.setFrameRate(qreal(15.0));
    original.setYCbCrColorSpace(QVideoSurfaceFormat::YCbCr_BT709);

    /* Assign the original instance to copy and verify if both the instancess
      have the same parameters. */
    copy = original;

    QCOMPARE(copy.pixelFormat(), QVideoFrame::Format_ARGB32);
    QCOMPARE(copy.frameSize(), QSize(1024, 768));
    QCOMPARE(copy.scanLineDirection(), QVideoSurfaceFormat::BottomToTop);
    QCOMPARE(copy.viewport(), QRect(0, 0, 1024, 1024));
    QCOMPARE(copy.frameRate(), qreal(15.0));
    QCOMPARE(copy.yCbCrColorSpace(), QVideoSurfaceFormat::YCbCr_BT709);

     /* Verify if both the instances are eqaul */
    QCOMPARE(original == copy, true);
    QCOMPARE(original != copy, false);
}

QTEST_MAIN(tst_QVideoSurfaceFormat)



#include "tst_qvideosurfaceformat.moc"
