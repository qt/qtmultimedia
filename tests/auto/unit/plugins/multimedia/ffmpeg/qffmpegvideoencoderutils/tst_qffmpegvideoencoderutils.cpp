// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>

#include <qobject.h>

#include <QtFFmpegMediaPluginImpl/private/qffmpegvideoencoderutils_p.h>

QT_USE_NAMESPACE

class tst_QFFmpegVideoEncoderUtils : public QObject
{
    Q_OBJECT

private slots:
    void getScaleConversionType_returnsCorrectConversionType_basedOnScaling();
    void getScaleConversionType_returnsCorrectConversionType_basedOnScaling_data();
};

void tst_QFFmpegVideoEncoderUtils::getScaleConversionType_returnsCorrectConversionType_basedOnScaling_data()
{
    QTest::addColumn<QSize>("sourceSize");
    QTest::addColumn<QSize>("targetSize");
    QTest::addColumn<int>("expectedConversionType");

#ifdef Q_OS_ANDROID
    int expectedConversionTypeForUpscaling = SWS_BICUBIC;
#else
    int expectedConversionTypeForUpscaling = SWS_FAST_BILINEAR;
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
    QFETCH(int, expectedConversionType);

    // Act
    const int actualConversionType = QFFmpeg::getScaleConversionType(sourceSize, targetSize);

    // Assert
    QCOMPARE(actualConversionType, expectedConversionType);
}

QTEST_MAIN(tst_QFFmpegVideoEncoderUtils)

#include "tst_qffmpegvideoencoderutils.moc"
