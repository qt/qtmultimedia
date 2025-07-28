// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>

#include <qvideoframeformat.h>

// Adds an enum, and the stringized version
#define ADD_ENUM_TEST(x) \
    QTest::newRow(#x) \
        << QVideoFrameFormat::x \
    << QString(QLatin1String(#x));

class tst_QVideoFrameFormat : public QObject
{
    Q_OBJECT
public:
    tst_QVideoFrameFormat();
    ~tst_QVideoFrameFormat() override;

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
    void planeCount_returnsNumberOfColorPlanesDictatedByPixelFormat_data() const;
    void planeCount_returnsNumberOfColorPlanesDictatedByPixelFormat() const;
    void viewport_data();
    void viewport();
    void scanLineDirection_data();
    void scanLineDirection();
    void frameRate_data();
    void frameRate();
    void colorSpaceEnum_data();
    void colorSpaceEnum ();
    void compare();
    void copy();
    void assign();

    void isValid();
    void copyAllParameters ();
    void assignAllParameters ();
};

tst_QVideoFrameFormat::tst_QVideoFrameFormat()
{
}

tst_QVideoFrameFormat::~tst_QVideoFrameFormat()
{
}

void tst_QVideoFrameFormat::initTestCase()
{
}

void tst_QVideoFrameFormat::cleanupTestCase()
{
}

void tst_QVideoFrameFormat::init()
{
}

void tst_QVideoFrameFormat::cleanup()
{
}

void tst_QVideoFrameFormat::constructNull()
{
    QVideoFrameFormat format;

    QVERIFY(!format.isValid());
    QCOMPARE(format.pixelFormat(), QVideoFrameFormat::Format_Invalid);
    QCOMPARE(format.frameSize(), QSize());
    QCOMPARE(format.frameWidth(), -1);
    QCOMPARE(format.frameHeight(), -1);
    QCOMPARE(format.viewport(), QRect());
    QCOMPARE(format.scanLineDirection(), QVideoFrameFormat::TopToBottom);
    QCOMPARE(format.streamFrameRate(), 0.0);
    QCOMPARE(format.colorSpace(), QVideoFrameFormat::ColorSpace_Undefined);
}

void tst_QVideoFrameFormat::construct_data()
{
    QTest::addColumn<QSize>("frameSize");
    QTest::addColumn<QVideoFrameFormat::PixelFormat>("pixelFormat");
    QTest::addColumn<bool>("valid");

    QTest::newRow("32x32 rgb32 no handle")
            << QSize(32, 32)
            << QVideoFrameFormat::Format_XRGB8888
            << true;

    QTest::newRow("32x32 invalid no handle")
            << QSize(32, 32)
            << QVideoFrameFormat::Format_Invalid
            << false;

    QTest::newRow("invalid size, rgb32 no handle")
            << QSize()
            << QVideoFrameFormat::Format_XRGB8888
            << false;

    QTest::newRow("0x0 rgb32 no handle")
            << QSize(0,0)
            << QVideoFrameFormat::Format_XRGB8888
            << true;
}

void tst_QVideoFrameFormat::construct()
{
    QFETCH(QSize, frameSize);
    QFETCH(QVideoFrameFormat::PixelFormat, pixelFormat);
    QFETCH(bool, valid);

    QRect viewport(QPoint(0, 0), frameSize);

    QVideoFrameFormat format(frameSize, pixelFormat);

    QCOMPARE(format.pixelFormat(), pixelFormat);
    QCOMPARE(format.frameSize(), frameSize);
    QCOMPARE(format.frameWidth(), frameSize.width());
    QCOMPARE(format.frameHeight(), frameSize.height());
    QCOMPARE(format.isValid(), valid);
    QCOMPARE(format.viewport(), viewport);
    QCOMPARE(format.scanLineDirection(), QVideoFrameFormat::TopToBottom);
    QCOMPARE(format.streamFrameRate(), 0.0);
    QCOMPARE(format.colorSpace(), QVideoFrameFormat::ColorSpace_Undefined);
}

void tst_QVideoFrameFormat::frameSize_data()
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

void tst_QVideoFrameFormat::frameSize()
{
    QFETCH(QSize, initialSize);
    QFETCH(QSize, newSize);

    QVideoFrameFormat format(initialSize, QVideoFrameFormat::Format_XRGB8888);

    format.setFrameSize(newSize);

    QCOMPARE(format.frameSize(), newSize);
    QCOMPARE(format.frameWidth(), newSize.width());
    QCOMPARE(format.frameHeight(), newSize.height());
}

void tst_QVideoFrameFormat::planeCount_returnsNumberOfColorPlanesDictatedByPixelFormat_data() const {
    QTest::addColumn<QVideoFrameFormat::PixelFormat>("pixelFormat");
    QTest::addColumn<int>("colorPlanes"); // Number of planes as specified by QVideoFrameFormat::PixelFormat documentation

    QTest::newRow("ARGB8888") << QVideoFrameFormat::Format_ARGB8888 << 1;
    QTest::newRow("ARGB8888_Premultiplied") << QVideoFrameFormat::Format_ARGB8888_Premultiplied << 1;
    QTest::newRow("XRGB8888") << QVideoFrameFormat::Format_XRGB8888 << 1;
    QTest::newRow("BGRA8888") << QVideoFrameFormat::Format_BGRA8888 << 1;
    QTest::newRow("BGRA8888_Premultiplied") << QVideoFrameFormat::Format_BGRA8888_Premultiplied << 1;
    QTest::newRow("BGRX8888") << QVideoFrameFormat::Format_BGRX8888 << 1;
    QTest::newRow("ABGR8888") << QVideoFrameFormat::Format_ABGR8888 << 1;
    QTest::newRow("XBGR8888") << QVideoFrameFormat::Format_XBGR8888 << 1;
    QTest::newRow("RGBA8888") << QVideoFrameFormat::Format_RGBA8888 << 1;
    QTest::newRow("RGBX8888") << QVideoFrameFormat::Format_RGBX8888 << 1;

    QTest::newRow("AUYVY") << QVideoFrameFormat::Format_AYUV << 1;
    QTest::newRow("AYUV_Premultiplied") << QVideoFrameFormat::Format_AYUV_Premultiplied << 1;
    QTest::newRow("YUV420P") << QVideoFrameFormat::Format_YUV420P << 3;
    QTest::newRow("YUV422P") << QVideoFrameFormat::Format_YUV422P << 3;
    QTest::newRow("YV12") << QVideoFrameFormat::Format_YV12 << 3;

    QTest::newRow("UYVY") << QVideoFrameFormat::Format_UYVY << 1;
    QTest::newRow("YUYV") << QVideoFrameFormat::Format_YUYV << 1;
    QTest::newRow("NV12") << QVideoFrameFormat::Format_NV12 << 2;
    QTest::newRow("NV21") << QVideoFrameFormat::Format_NV21 << 2;

    QTest::newRow("IMC1") << QVideoFrameFormat::Format_IMC1 << 3;
    QTest::newRow("IMC2") << QVideoFrameFormat::Format_IMC2 << 2;
    QTest::newRow("IMC3") << QVideoFrameFormat::Format_IMC3 << 3;
    QTest::newRow("IMC4") << QVideoFrameFormat::Format_IMC4 << 2;

    QTest::newRow("Y8") << QVideoFrameFormat::Format_Y8 << 1;
    QTest::newRow("Y16") << QVideoFrameFormat::Format_Y16 << 1;

    QTest::newRow("P010") << QVideoFrameFormat::Format_P010 << 2;
    QTest::newRow("P016") << QVideoFrameFormat::Format_P016 << 2;

    QTest::newRow("SamplerExternalOES") << QVideoFrameFormat::Format_SamplerExternalOES << 1;
    QTest::newRow("Jpeg") << QVideoFrameFormat::Format_Jpeg << 1;
    QTest::newRow("SamplerRect") << QVideoFrameFormat::Format_SamplerRect << 1;

    QTest::newRow("YUV420P10") << QVideoFrameFormat::Format_YUV420P10 << 3;
}

void tst_QVideoFrameFormat::planeCount_returnsNumberOfColorPlanesDictatedByPixelFormat() const {
    QFETCH(QVideoFrameFormat::PixelFormat, pixelFormat);
    QFETCH(int, colorPlanes);

    const QVideoFrameFormat frameFormat = QVideoFrameFormat({}, pixelFormat);

    QCOMPARE_EQ(frameFormat.planeCount(), colorPlanes);
}

void tst_QVideoFrameFormat::viewport_data()
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

void tst_QVideoFrameFormat::viewport()
{
    QFETCH(QSize, initialSize);
    QFETCH(QRect, viewport);
    QFETCH(QSize, newSize);
    QFETCH(QRect, expectedViewport);

    QRect initialViewport(QPoint(0, 0), initialSize);

    QVideoFrameFormat format(initialSize, QVideoFrameFormat::Format_XRGB8888);

    format.setViewport(viewport);

    QCOMPARE(format.viewport(), viewport);

    format.setFrameSize(newSize);

    QCOMPARE(format.viewport(), expectedViewport);
}

void tst_QVideoFrameFormat::scanLineDirection_data()
{
    QTest::addColumn<QVideoFrameFormat::Direction>("direction");
    QTest::addColumn<QString>("stringized");

    ADD_ENUM_TEST(TopToBottom);
    ADD_ENUM_TEST(BottomToTop);
}

void tst_QVideoFrameFormat::scanLineDirection()
{
    QFETCH(QVideoFrameFormat::Direction, direction);
    QFETCH(QString, stringized);

    QVideoFrameFormat format(QSize(16, 16), QVideoFrameFormat::Format_XRGB8888);

    format.setScanLineDirection(direction);

    QCOMPARE(format.scanLineDirection(), direction);

    QTest::ignoreMessage(QtDebugMsg, stringized.toLatin1().constData());
    qDebug() << direction;
}

void tst_QVideoFrameFormat::colorSpaceEnum_data()
{
    QTest::addColumn<QVideoFrameFormat::ColorSpace>("colorspace");
    QTest::addColumn<QString>("stringized");

    ADD_ENUM_TEST(ColorSpace_BT601);
    ADD_ENUM_TEST(ColorSpace_BT709);
    ADD_ENUM_TEST(ColorSpace_BT2020);
    ADD_ENUM_TEST(ColorSpace_AdobeRgb);
    ADD_ENUM_TEST(ColorSpace_Undefined);
}

/* Test case for Enum ColorSpace */
void tst_QVideoFrameFormat::colorSpaceEnum()
{
    QFETCH(QVideoFrameFormat::ColorSpace, colorspace);
    QFETCH(QString, stringized);

    QVideoFrameFormat format(QSize(64, 64), QVideoFrameFormat::Format_XRGB8888);
    format.setColorSpace(colorspace);

    QCOMPARE(format.colorSpace(), colorspace);

    QTest::ignoreMessage(QtDebugMsg, stringized.toLatin1().constData());
    qDebug() << colorspace;
}


void tst_QVideoFrameFormat::frameRate_data()
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

void tst_QVideoFrameFormat::frameRate()
{
    QFETCH(qreal, frameRate);

    QVideoFrameFormat format(QSize(64, 64), QVideoFrameFormat::Format_XRGB8888);

    format.setStreamFrameRate(frameRate);

    QCOMPARE(format.streamFrameRate(), frameRate);
}

void tst_QVideoFrameFormat::compare()
{
    QVideoFrameFormat format1(
            QSize(16, 16), QVideoFrameFormat::Format_XRGB8888);
    QVideoFrameFormat format2(
            QSize(16, 16), QVideoFrameFormat::Format_XRGB8888);
    QVideoFrameFormat format3(
            QSize(32, 32), QVideoFrameFormat::Format_AYUV);
    QVideoFrameFormat format4(
            QSize(16, 16), QVideoFrameFormat::Format_XBGR8888);

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

    format2.setScanLineDirection(QVideoFrameFormat::BottomToTop);

    // Not equal scan line direction differs.
    QCOMPARE(format1 == format2, false);
    QCOMPARE(format1 != format2, true);

    format1.setScanLineDirection(QVideoFrameFormat::BottomToTop);

    // Equal.
    QCOMPARE(format1 == format2, true);
    QCOMPARE(format1 != format2, false);

    format1.setStreamFrameRate(7.5);

    // Not equal frame rate differs.
    QCOMPARE(format1 == format2, false);
    QCOMPARE(format1 != format2, true);

    format2.setStreamFrameRate(qreal(7.50001));

    // Equal.
    QCOMPARE(format1 == format2, true);
    QCOMPARE(format1 != format2, false);

    format2.setColorSpace(QVideoFrameFormat::ColorSpace_BT601);

    // Not equal yuv color space differs.
    QCOMPARE(format1 == format2, false);
    QCOMPARE(format1 != format2, true);

    format1.setColorSpace(QVideoFrameFormat::ColorSpace_BT601);

    // Equal.
    QCOMPARE(format1 == format2, true);
    QCOMPARE(format1 != format2, false);
}


void tst_QVideoFrameFormat::copy()
{
    QVideoFrameFormat original(
            QSize(1024, 768), QVideoFrameFormat::Format_ARGB8888);
    original.setScanLineDirection(QVideoFrameFormat::BottomToTop);

    QVideoFrameFormat copy(original);

    QCOMPARE(copy.pixelFormat(), QVideoFrameFormat::Format_ARGB8888);
    QCOMPARE(copy.frameSize(), QSize(1024, 768));
    QCOMPARE(copy.scanLineDirection(), QVideoFrameFormat::BottomToTop);

    QCOMPARE(original == copy, true);
    QCOMPARE(original != copy, false);

    copy.setScanLineDirection(QVideoFrameFormat::TopToBottom);

    QCOMPARE(copy.scanLineDirection(), QVideoFrameFormat::TopToBottom);

    QCOMPARE(original.scanLineDirection(), QVideoFrameFormat::BottomToTop);

    QCOMPARE(original == copy, false);
    QCOMPARE(original != copy, true);
}

void tst_QVideoFrameFormat::assign()
{
    QVideoFrameFormat copy(
            QSize(64, 64), QVideoFrameFormat::Format_AYUV);

    QVideoFrameFormat original(
            QSize(1024, 768), QVideoFrameFormat::Format_ARGB8888);
    original.setScanLineDirection(QVideoFrameFormat::BottomToTop);

    copy = original;

    QCOMPARE(copy.pixelFormat(), QVideoFrameFormat::Format_ARGB8888);
    QCOMPARE(copy.frameSize(), QSize(1024, 768));
    QCOMPARE(copy.scanLineDirection(), QVideoFrameFormat::BottomToTop);

    QCOMPARE(original == copy, true);
    QCOMPARE(original != copy, false);

    copy.setScanLineDirection(QVideoFrameFormat::TopToBottom);

    QCOMPARE(copy.scanLineDirection(), QVideoFrameFormat::TopToBottom);

    QCOMPARE(original.scanLineDirection(), QVideoFrameFormat::BottomToTop);

    QCOMPARE(original == copy, false);
    QCOMPARE(original != copy, true);
}

/* Test case for api isValid */
void tst_QVideoFrameFormat::isValid()
{
    /* When both pixel format and framesize is not valid */
    QVideoFrameFormat format;
    QVERIFY(!format.isValid());

    /* When framesize is valid and pixel format is not valid */
    format.setFrameSize(64,64);
    QVERIFY(format.frameSize() == QSize(64,64));
    QVERIFY(!format.pixelFormat());
    QVERIFY(!format.isValid());

    /* When both the pixel format and framesize is valid. */
    QVideoFrameFormat format1(QSize(32, 32), QVideoFrameFormat::Format_AYUV);
    QVERIFY(format1.isValid());

    /* When pixel format is valid and frame size is not valid */
    format1.setFrameSize(-1,-1);
    QVERIFY(!format1.frameSize().isValid());
    QVERIFY(!format1.isValid());
}

/* Test case for copy constructor with all the parameters. */
void tst_QVideoFrameFormat::copyAllParameters()
{
    /* Create the instance and set all the parameters. */
    QVideoFrameFormat original(
            QSize(1024, 768), QVideoFrameFormat::Format_ARGB8888);

    original.setScanLineDirection(QVideoFrameFormat::BottomToTop);
    original.setViewport(QRect(0, 0, 1024, 1024));
    original.setStreamFrameRate(qreal(15.0));
    original.setColorSpace(QVideoFrameFormat::ColorSpace_BT709);

    /* Copy the original instance to copy and verify if both the instances
      have the same parameters. */
    QVideoFrameFormat copy(original);

    QCOMPARE(copy.pixelFormat(), QVideoFrameFormat::Format_ARGB8888);
    QCOMPARE(copy.frameSize(), QSize(1024, 768));
    QCOMPARE(copy.scanLineDirection(), QVideoFrameFormat::BottomToTop);
    QCOMPARE(copy.viewport(), QRect(0, 0, 1024, 1024));
    QCOMPARE(copy.streamFrameRate(), qreal(15.0));
    QCOMPARE(copy.colorSpace(), QVideoFrameFormat::ColorSpace_BT709);

    /* Verify if both the instances are eqaul */
    QCOMPARE(original == copy, true);
    QCOMPARE(original != copy, false);
}

/* Test case for copy constructor with all the parameters. */
void tst_QVideoFrameFormat::assignAllParameters()
{
    /* Create the instance and set all the parameters. */
    QVideoFrameFormat copy(
            QSize(64, 64), QVideoFrameFormat::Format_AYUV);
    copy.setScanLineDirection(QVideoFrameFormat::TopToBottom);
    copy.setViewport(QRect(0, 0, 640, 320));
    copy.setStreamFrameRate(qreal(7.5));
    copy.setColorSpace(QVideoFrameFormat::ColorSpace_BT601);

    /* Create the instance and set all the parameters. */
    QVideoFrameFormat original(
            QSize(1024, 768), QVideoFrameFormat::Format_ARGB8888);
    original.setScanLineDirection(QVideoFrameFormat::BottomToTop);
    original.setViewport(QRect(0, 0, 1024, 1024));
    original.setStreamFrameRate(qreal(15.0));
    original.setColorSpace(QVideoFrameFormat::ColorSpace_BT709);

    /* Assign the original instance to copy and verify if both the instancess
      have the same parameters. */
    copy = original;

    QCOMPARE(copy.pixelFormat(), QVideoFrameFormat::Format_ARGB8888);
    QCOMPARE(copy.frameSize(), QSize(1024, 768));
    QCOMPARE(copy.scanLineDirection(), QVideoFrameFormat::BottomToTop);
    QCOMPARE(copy.viewport(), QRect(0, 0, 1024, 1024));
    QCOMPARE(copy.streamFrameRate(), qreal(15.0));
    QCOMPARE(copy.colorSpace(), QVideoFrameFormat::ColorSpace_BT709);

     /* Verify if both the instances are eqaul */
    QCOMPARE(original == copy, true);
    QCOMPARE(original != copy, false);
}

QTEST_MAIN(tst_QVideoFrameFormat)



#include "tst_qvideoframeformat.moc"
