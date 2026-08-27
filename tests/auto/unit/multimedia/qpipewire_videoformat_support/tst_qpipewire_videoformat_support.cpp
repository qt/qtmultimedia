// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>

#include <private/qpipewire_videoformat_support_p.h>

using namespace QtPipeWire;

class tst_QPipeWireVideoFormatSupport : public QObject
{
    Q_OBJECT

private slots:
    void toQtPixelFormat_roundTrips_whenSpaFormatIsMappedFromQt_data();
    void toQtPixelFormat_roundTrips_whenSpaFormatIsMappedFromQt();

    void toQtPixelFormat_returnsInvalid_whenSpaFormatIsUnmapped();
    void toSpaVideoFormat_returnsUnknown_whenPixelFormatIsUnmapped();

    void rateFromFps_snapsToNtscFraction_whenFpsIsNearNtscRate_data();
    void rateFromFps_snapsToNtscFraction_whenFpsIsNearNtscRate();

    void rateFromFps_roundsDownToWholeNumber_whenFpsIsNotNtsc_data();
    void rateFromFps_roundsDownToWholeNumber_whenFpsIsNotNtsc();

    void rateFromFps_clampsToOne_whenFpsIsBelowOne();
};

void tst_QPipeWireVideoFormatSupport::toQtPixelFormat_roundTrips_whenSpaFormatIsMappedFromQt_data()
{
    QTest::addColumn<QVideoFrameFormat::PixelFormat>("pixelFormat");

    QTest::newRow("YUV420P") << QVideoFrameFormat::Format_YUV420P;
    QTest::newRow("YUV422P") << QVideoFrameFormat::Format_YUV422P;
    QTest::newRow("YV12") << QVideoFrameFormat::Format_YV12;
    QTest::newRow("UYVY") << QVideoFrameFormat::Format_UYVY;
    QTest::newRow("YUYV") << QVideoFrameFormat::Format_YUYV;
    QTest::newRow("NV12") << QVideoFrameFormat::Format_NV12;
    QTest::newRow("NV21") << QVideoFrameFormat::Format_NV21;
    QTest::newRow("AYUV") << QVideoFrameFormat::Format_AYUV;
    QTest::newRow("Y8") << QVideoFrameFormat::Format_Y8;
    QTest::newRow("XRGB8888") << QVideoFrameFormat::Format_XRGB8888;
    QTest::newRow("XBGR8888") << QVideoFrameFormat::Format_XBGR8888;
    QTest::newRow("RGBX8888") << QVideoFrameFormat::Format_RGBX8888;
    QTest::newRow("BGRX8888") << QVideoFrameFormat::Format_BGRX8888;
    QTest::newRow("ARGB8888") << QVideoFrameFormat::Format_ARGB8888;
    QTest::newRow("ABGR8888") << QVideoFrameFormat::Format_ABGR8888;
    QTest::newRow("RGBA8888") << QVideoFrameFormat::Format_RGBA8888;
    QTest::newRow("BGRA8888") << QVideoFrameFormat::Format_BGRA8888;
    QTest::newRow("Y16") << QVideoFrameFormat::Format_Y16;
    QTest::newRow("P010") << QVideoFrameFormat::Format_P010;
}

void tst_QPipeWireVideoFormatSupport::toQtPixelFormat_roundTrips_whenSpaFormatIsMappedFromQt()
{
    QFETCH(const QVideoFrameFormat::PixelFormat, pixelFormat);

    const spa_video_format spaFormat = toSpaVideoFormat(pixelFormat);
    QVERIFY(spaFormat != SPA_VIDEO_FORMAT_UNKNOWN);

    QCOMPARE(toQtPixelFormat(spaFormat), pixelFormat);
}

void tst_QPipeWireVideoFormatSupport::toQtPixelFormat_returnsInvalid_whenSpaFormatIsUnmapped()
{
    // SPA_VIDEO_FORMAT_ENCODED has no QVideoFrameFormat equivalent
    QCOMPARE(toQtPixelFormat(SPA_VIDEO_FORMAT_ENCODED), QVideoFrameFormat::Format_Invalid);
}

void tst_QPipeWireVideoFormatSupport::toSpaVideoFormat_returnsUnknown_whenPixelFormatIsUnmapped()
{
    // Format_Jpeg is handled separately from the raw pixel format mapping
    QCOMPARE(toSpaVideoFormat(QVideoFrameFormat::Format_Jpeg), SPA_VIDEO_FORMAT_UNKNOWN);
}

void tst_QPipeWireVideoFormatSupport::rateFromFps_snapsToNtscFraction_whenFpsIsNearNtscRate_data()
{
    QTest::addColumn<qreal>("fps");
    QTest::addColumn<int>("numerator");
    QTest::addColumn<int>("denominator");

    QTest::newRow("23.976") << 23.976 << 24'000 << 1'001;
    QTest::newRow("23.976, within precision") << 23.98 << 24'000 << 1'001;
    QTest::newRow("29.97") << 29.97 << 30'000 << 1'001;
    QTest::newRow("59.94") << 59.94 << 60'000 << 1'001;
}

void tst_QPipeWireVideoFormatSupport::rateFromFps_snapsToNtscFraction_whenFpsIsNearNtscRate()
{
    QFETCH(const qreal, fps);
    QFETCH(const int, numerator);
    QFETCH(const int, denominator);

    const FrameRate rate = rateFromFps(fps);

    QCOMPARE(rate.frac.num, uint32_t(numerator));
    QCOMPARE(rate.frac.denom, uint32_t(denominator));
}

void tst_QPipeWireVideoFormatSupport::rateFromFps_roundsDownToWholeNumber_whenFpsIsNotNtsc_data()
{
    QTest::addColumn<qreal>("fps");
    QTest::addColumn<uint32_t>("expectedRoundedFps");

    QTest::newRow("exact 30") << qreal(30) << uint32_t(30);
    QTest::newRow("25.9 rounds down to 25") << qreal(25.9) << uint32_t(25);
    QTest::newRow("60 stays 60, not mistaken for NTSC 59.94") << qreal(60) << uint32_t(60);
}

void tst_QPipeWireVideoFormatSupport::rateFromFps_roundsDownToWholeNumber_whenFpsIsNotNtsc()
{
    QFETCH(const qreal, fps);
    QFETCH(const uint32_t, expectedRoundedFps);

    const FrameRate rate = rateFromFps(fps);

    QCOMPARE(rate.fps, qreal(expectedRoundedFps));
    QCOMPARE(rate.frac.num, expectedRoundedFps);
    QCOMPARE(rate.frac.denom, uint32_t(1));
}

void tst_QPipeWireVideoFormatSupport::rateFromFps_clampsToOne_whenFpsIsBelowOne()
{
    // 0/1 is not accepted by PipeWire as a maximum rate
    const FrameRate rate = rateFromFps(0.4);

    QCOMPARE(rate.fps, qreal(1));
    QCOMPARE(rate.frac.num, uint32_t(1));
    QCOMPARE(rate.frac.denom, uint32_t(1));
}

QTEST_MAIN(tst_QPipeWireVideoFormatSupport)
#include "tst_qpipewire_videoformat_support.moc"
