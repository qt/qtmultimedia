// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>

#include <qobject.h>

#include <QtCore/qlist.h>

#include <QtFFmpegMediaPluginImpl/private/qffmpegvideoencoderutils_p.h>


using namespace QFFmpeg;

class tst_QFFmpegVideoEncoderUtils : public QObject
{
    Q_OBJECT

private slots:
    void getScaleConversionType_returnsCorrectConversionType_basedOnScaling();
    void getScaleConversionType_returnsCorrectConversionType_basedOnScaling_data();
    void adjustFrameRate_returnsExpectedRate_basedOnSettingsAndSource_data();
    void adjustFrameRate_returnsExpectedRate_basedOnSettingsAndSource();
    void adjustFrameTimeBase_returnsExpectedTimeBase_data();
    void adjustFrameTimeBase_returnsExpectedTimeBase();
};

void tst_QFFmpegVideoEncoderUtils::getScaleConversionType_returnsCorrectConversionType_basedOnScaling_data()
{
    QTest::addColumn<QSize>("sourceSize");
    QTest::addColumn<QSize>("targetSize");
    QTest::addColumn<SwsFlags>("expectedConversionType");

#ifdef Q_OS_ANDROID
    SwsFlags expectedConversionTypeForUpscaling = SWS_BICUBIC;
#else
    SwsFlags expectedConversionTypeForUpscaling = SWS_FAST_BILINEAR;
#endif

    QTest::newRow("Sizes are equal")
            << QSize{ 800, 600 } << QSize{ 800, 600 } << SWS_FAST_BILINEAR;
    QTest::newRow("Uniform downscaling")
            << QSize{ 800, 600 } << QSize{ 400, 300 } << SWS_FAST_BILINEAR;
    QTest::newRow("Uniform upscaling")
            << QSize{ 400, 300 } << QSize{ 800, 600 } << expectedConversionTypeForUpscaling;
    QTest::newRow("Anisotropic downscaling by width")
            << QSize{ 800, 600 } << QSize{ 400, 600 } << SWS_FAST_BILINEAR;
    QTest::newRow("Anisotropic downscaling by height")
            << QSize{ 800, 600 } << QSize{ 800, 300 } << SWS_FAST_BILINEAR;
    QTest::newRow("Anisotropic upscaling by width")
            << QSize{ 400, 300 } << QSize{ 800, 300 } << expectedConversionTypeForUpscaling;
    QTest::newRow("Anisotropic upscaling by height")
            << QSize{ 400, 300 } << QSize{ 400, 600 } << expectedConversionTypeForUpscaling;
    QTest::newRow("Anisotropic mixed scaling (width up, height down)")
            << QSize{ 400, 600 } << QSize{ 800, 300 } << expectedConversionTypeForUpscaling;
    QTest::newRow("Anisotropic mixed scaling (width down, height up)")
            << QSize{ 800, 300 } << QSize{ 400, 600 } << expectedConversionTypeForUpscaling;
}

void tst_QFFmpegVideoEncoderUtils::getScaleConversionType_returnsCorrectConversionType_basedOnScaling()
{
    // Arrange
    QFETCH(QSize, sourceSize);
    QFETCH(QSize, targetSize);
    QFETCH(SwsFlags, expectedConversionType);

    // Act
    const SwsFlags actualConversionType = QFFmpeg::getScaleConversionType(sourceSize, targetSize);

    // Assert
    QCOMPARE(actualConversionType, expectedConversionType);
}

void tst_QFFmpegVideoEncoderUtils::
        adjustFrameRate_returnsExpectedRate_basedOnSettingsAndSource_data()
{
    QTest::addColumn<QList<AVRational>>("supportedRates");
    QTest::addColumn<qreal>("settingsRate");
    QTest::addColumn<qreal>("sourceRate");
    QTest::addColumn<AVRational>("expectedRate");

    const QList<AVRational> fixedOnlyRates = { { 15, 1 }, { 25, 1 }, { 30, 1 }, { 60, 1 } };

    QTest::newRow("fixed-only codec, unset settings, variable source -> closest to 30")
            << fixedOnlyRates << qreal(-1.) << qreal(0.) << AVRational{ 30, 1 };

    QTest::newRow("fixed-only codec, settings set, variable source -> closest to settings")
            << fixedOnlyRates << qreal(24.) << qreal(0.) << AVRational{ 25, 1 };

    QTest::newRow("fixed-only codec, settings set, fixed source (different) -> settings wins")
            << fixedOnlyRates << qreal(60.) << qreal(15.) << AVRational{ 60, 1 };

    QTest::newRow("fixed-only codec, unset settings, fixed source -> closest to source")
            << fixedOnlyRates << qreal(-1.) << qreal(26.) << AVRational{ 25, 1 };

    QTest::newRow("variable-capable codec, unset settings -> stays variable")
            << QList<AVRational>{ } << qreal(-1.) << qreal(0.) << AVRational{ 0, 1 };

    QTest::newRow("variable-capable codec, settings set -> fixed at settings rate")
            << QList<AVRational>{ } << qreal(24.) << qreal(0.) << AVRational{ 24, 1 };

    QTest::newRow("variable-capable codec, unset settings, fixed source -> fixed at source rate")
            << QList<AVRational>{} << qreal(-1.) << qreal(30.) << AVRational{ 30, 1 };
}

void tst_QFFmpegVideoEncoderUtils::adjustFrameRate_returnsExpectedRate_basedOnSettingsAndSource()
{
    // Arrange
    QFETCH(QList<AVRational>, supportedRates);
    QFETCH(qreal, settingsRate);
    QFETCH(qreal, sourceRate);
    QFETCH(AVRational, expectedRate);

    // Act
    const AVRational actualRate =
            QFFmpeg::adjustFrameRate(supportedRates, settingsRate, sourceRate);

    // Assert
    QCOMPARE(actualRate.num, expectedRate.num);
    QCOMPARE(actualRate.den, expectedRate.den);
}

void tst_QFFmpegVideoEncoderUtils::adjustFrameTimeBase_returnsExpectedTimeBase_data()
{
    QTest::addColumn<QList<AVRational>>("supportedRates");
    QTest::addColumn<AVRational>("frameRate");
    QTest::addColumn<bool>("isFixedRate");
    QTest::addColumn<AVRational>("expectedTimeBase");

    const QList<AVRational> fixedOnlyRates = { { 15, 1 }, { 25, 1 }, { 30, 1 }, { 60, 1 } };

    QTest::newRow("fixed-only codec -> timebase is 1/framerate")
            << fixedOnlyRates << AVRational{ 30, 1 } << true << AVRational{ 1, 30 };

    QTest::newRow("variable-capable codec, placeholder default rate, not fixed "
                  "-> timebase scaled up for pts precision")
            << QList<AVRational>{ } << AVRational{ 30, 1 } << false << AVRational{ 1, 30000 };

    QTest::newRow("variable-capable codec, genuine fixed rate -> timebase is 1/framerate, "
                  "no scaling")
            << QList<AVRational>{ } << AVRational{ 125, 3 } << true << AVRational{ 3, 125 };
}

void tst_QFFmpegVideoEncoderUtils::adjustFrameTimeBase_returnsExpectedTimeBase()
{
    // Arrange
    QFETCH(QList<AVRational>, supportedRates);
    QFETCH(AVRational, frameRate);
    QFETCH(bool, isFixedRate);
    QFETCH(AVRational, expectedTimeBase);

    // Act
    const AVRational actualTimeBase =
            QFFmpeg::adjustFrameTimeBase(supportedRates, frameRate, isFixedRate);

    // Assert
    QCOMPARE(actualTimeBase.num, expectedTimeBase.num);
    QCOMPARE(actualTimeBase.den, expectedTimeBase.den);
}

QTEST_MAIN(tst_QFFmpegVideoEncoderUtils)

#include "tst_qffmpegvideoencoderutils.moc"
