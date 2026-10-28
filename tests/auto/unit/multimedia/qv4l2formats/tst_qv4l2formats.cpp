// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QtMultimedia/private/qv4l2formats_p.h>

#include <linux/videodev2.h>

// NOLINTBEGIN(readability-convert-member-functions-to-static)

class tst_QV4L2Formats : public QObject
{
    Q_OBJECT

private slots:
    void formatMap_entriesMapBackToPixelFormat();
    void formatForV4L2Format_returnsInvalidForUnsupportedFormat();
    void v4l2FormatForPixelFormat_returnsNulloptForUnsupportedPixelFormat();
    void v4l2FormatForPixelFormat_prefersMjpegForJpeg();

    void colorSpaceForV4L2Colorspace_data();
    void colorSpaceForV4L2Colorspace();
    void v4l2ColorspaceForColorSpace_data();
    void v4l2ColorspaceForColorSpace();
    void v4l2ColorspaceForColorSpace_returnsNulloptForUndefined();
    void colorSpace_roundTripsThroughV4L2();
};

void tst_QV4L2Formats::formatMap_entriesMapBackToPixelFormat()
{
    for (const auto &entry : QtV4L2::formatMap) {
        const auto v4l2Format = QtV4L2::v4l2FormatForPixelFormat(entry.format);
        QVERIFY(v4l2Format.has_value());

        QCOMPARE(QtV4L2::formatForV4L2Format(entry.v4l2Format), entry.format);
    }
}

void tst_QV4L2Formats::formatForV4L2Format_returnsInvalidForUnsupportedFormat()
{
    QCOMPARE(QtV4L2::formatForV4L2Format(V4L2_PIX_FMT_H264), QVideoFrameFormat::Format_Invalid);
}

void tst_QV4L2Formats::v4l2FormatForPixelFormat_returnsNulloptForUnsupportedPixelFormat()
{
    QVERIFY(!QtV4L2::v4l2FormatForPixelFormat(QVideoFrameFormat::Format_Invalid).has_value());
}

void tst_QV4L2Formats::v4l2FormatForPixelFormat_prefersMjpegForJpeg()
{
    const auto v4l2Format = QtV4L2::v4l2FormatForPixelFormat(QVideoFrameFormat::Format_Jpeg);
    QVERIFY(v4l2Format.has_value());
    QCOMPARE(*v4l2Format, uint32_t(V4L2_PIX_FMT_MJPEG));
}

void tst_QV4L2Formats::colorSpaceForV4L2Colorspace_data()
{
    QTest::addColumn<v4l2_colorspace>("v4l2Colorspace");
    QTest::addColumn<QVideoFrameFormat::ColorSpace>("expected");

    QTest::newRow("REC709") << V4L2_COLORSPACE_REC709 << QVideoFrameFormat::ColorSpace_BT709;
    QTest::newRow("JPEG") << V4L2_COLORSPACE_JPEG << QVideoFrameFormat::ColorSpace_AdobeRgb;
    QTest::newRow("SRGB") << V4L2_COLORSPACE_SRGB << QVideoFrameFormat::ColorSpace_BT601;
    QTest::newRow("BT2020") << V4L2_COLORSPACE_BT2020 << QVideoFrameFormat::ColorSpace_BT2020;
    QTest::newRow("DCI_P3") << V4L2_COLORSPACE_DCI_P3 << QVideoFrameFormat::ColorSpace_Undefined;
    QTest::newRow("DEFAULT") << V4L2_COLORSPACE_DEFAULT << QVideoFrameFormat::ColorSpace_Undefined;
    QTest::newRow("RAW") << V4L2_COLORSPACE_RAW << QVideoFrameFormat::ColorSpace_Undefined;
}

void tst_QV4L2Formats::colorSpaceForV4L2Colorspace()
{
    QFETCH(v4l2_colorspace, v4l2Colorspace);
    QFETCH(QVideoFrameFormat::ColorSpace, expected);

    QCOMPARE(QtV4L2::colorSpaceForV4L2Colorspace(v4l2Colorspace), expected);
}

void tst_QV4L2Formats::v4l2ColorspaceForColorSpace_data()
{
    QTest::addColumn<QVideoFrameFormat::ColorSpace>("colorSpace");
    QTest::addColumn<v4l2_colorspace>("expected");

    QTest::newRow("BT601") << QVideoFrameFormat::ColorSpace_BT601 << V4L2_COLORSPACE_SMPTE170M;
    QTest::newRow("BT709") << QVideoFrameFormat::ColorSpace_BT709 << V4L2_COLORSPACE_REC709;
    QTest::newRow("AdobeRgb") << QVideoFrameFormat::ColorSpace_AdobeRgb << V4L2_COLORSPACE_OPRGB;
    QTest::newRow("BT2020") << QVideoFrameFormat::ColorSpace_BT2020 << V4L2_COLORSPACE_BT2020;
}

void tst_QV4L2Formats::v4l2ColorspaceForColorSpace()
{
    QFETCH(QVideoFrameFormat::ColorSpace, colorSpace);
    QFETCH(v4l2_colorspace, expected);

    const auto v4l2Colorspace = QtV4L2::v4l2ColorspaceForColorSpace(colorSpace);
    QVERIFY(v4l2Colorspace.has_value());
    QCOMPARE(*v4l2Colorspace, expected);
}

void tst_QV4L2Formats::v4l2ColorspaceForColorSpace_returnsNulloptForUndefined()
{
    QVERIFY(!QtV4L2::v4l2ColorspaceForColorSpace(QVideoFrameFormat::ColorSpace_Undefined)
                     .has_value());
}

void tst_QV4L2Formats::colorSpace_roundTripsThroughV4L2()
{
    for (const auto colorSpace :
         { QVideoFrameFormat::ColorSpace_BT709, QVideoFrameFormat::ColorSpace_BT2020 }) {
        const auto v4l2Colorspace = QtV4L2::v4l2ColorspaceForColorSpace(colorSpace);
        QVERIFY(v4l2Colorspace.has_value());
        QCOMPARE(QtV4L2::colorSpaceForV4L2Colorspace(*v4l2Colorspace), colorSpace);
    }
}

QTEST_GUILESS_MAIN(tst_QV4L2Formats)

#include "tst_qv4l2formats.moc"
