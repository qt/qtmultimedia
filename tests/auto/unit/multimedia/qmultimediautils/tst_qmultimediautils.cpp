// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QtTest>
#include <QDebug>
#include <private/qmultimediautils_p.h>
#include <qvideoframeformat.h>

class tst_QMultimediaUtils : public QObject
{
    Q_OBJECT

private slots:
    void fraction_of_0();
    void fraction_of_negative_1_5();
    void fraction_of_1_5();
    void fraction_of_30();
    void fraction_of_29_97();
    void fraction_of_lower_boundary();
    void fraction_of_upper_boundary();

    void qRotatedFrameSize_returnsSizeAccordinglyToRotation();

    void qMediaFromUserInput_addsFilePrefix_whenCalledWithLocalFile();

    void qGetRequiredSwapChainFormat_returnsSdr_whenMaxLuminanceIsBelowSdrThreshold_data();
    void qGetRequiredSwapChainFormat_returnsSdr_whenMaxLuminanceIsBelowSdrThreshold();
    void qGetRequiredSwapChainFormat_returnsHdr_whenMaxLuminanceIsBelowHdrThreshold_data();
    void qGetRequiredSwapChainFormat_returnsHdr_whenMaxLuminanceIsBelowHdrThreshold();

    void qShouldUpdateSwapChainFormat_returnsFalse_whenSwapChainIsNullPointer();
};

void tst_QMultimediaUtils::fraction_of_0()
{
    auto [n, d] = qRealToFraction(0.);
    QCOMPARE(n, 0);
    QCOMPARE(d, 1);
}

void tst_QMultimediaUtils::fraction_of_negative_1_5()
{
    auto [n, d] = qRealToFraction(-1.5);
    QCOMPARE(double(n) / double(d), -1.5);
    QCOMPARE(n, -3);
    QCOMPARE(d, 2);
}

void tst_QMultimediaUtils::fraction_of_1_5()
{
    auto [n, d] = qRealToFraction(1.5);
    QCOMPARE(double(n) / double(d), 1.5);
    QCOMPARE(n, 3);
    QCOMPARE(d, 2);
}

void tst_QMultimediaUtils::fraction_of_30()
{
    auto [n, d] = qRealToFraction(30.);
    QCOMPARE(double(n) / double(d), 30.);
    QCOMPARE(d, 1);
}

void tst_QMultimediaUtils::fraction_of_29_97()
{
    auto [n, d] = qRealToFraction(29.97);
    QCOMPARE(double(n) / double(d), 29.97);
}

void tst_QMultimediaUtils::fraction_of_lower_boundary()
{
    double f = 0.000001;
    auto [n, d] = qRealToFraction(f);
    QVERIFY(double(n) / double(d) < f);
    QVERIFY(double(n) / double(d) >= 0.);
}

void tst_QMultimediaUtils::fraction_of_upper_boundary()
{
    double f = 0.999999;
    auto [n, d] = qRealToFraction(f);
    QVERIFY(double(n) / double(d) <= 1.);
    QVERIFY(double(n) / double(d) > f);
}

void tst_QMultimediaUtils::qRotatedFrameSize_returnsSizeAccordinglyToRotation()
{
    QCOMPARE(qRotatedFrameSize({ 10, 22 }, 0), QSize(10, 22));
    QCOMPARE(qRotatedFrameSize({ 10, 23 }, -180), QSize(10, 23));
    QCOMPARE(qRotatedFrameSize({ 10, 24 }, 180), QSize(10, 24));
    QCOMPARE(qRotatedFrameSize({ 10, 25 }, 360), QSize(10, 25));
    QCOMPARE(qRotatedFrameSize({ 11, 26 }, 540), QSize(11, 26));

    QCOMPARE(qRotatedFrameSize({ 10, 22 }, -90), QSize(22, 10));
    QCOMPARE(qRotatedFrameSize({ 10, 23 }, 90), QSize(23, 10));
    QCOMPARE(qRotatedFrameSize({ 10, 24 }, 270), QSize(24, 10));
    QCOMPARE(qRotatedFrameSize({ 10, 25 }, 450), QSize(25, 10));

    QCOMPARE(qRotatedFrameSize({ 10, 22 }, QtVideo::Rotation::None), QSize(10, 22));
    QCOMPARE(qRotatedFrameSize({ 10, 22 }, QtVideo::Rotation::Clockwise180), QSize(10, 22));

    QCOMPARE(qRotatedFrameSize({ 11, 22 }, QtVideo::Rotation::Clockwise90), QSize(22, 11));
    QCOMPARE(qRotatedFrameSize({ 11, 22 }, QtVideo::Rotation::Clockwise270), QSize(22, 11));
}

void tst_QMultimediaUtils::qMediaFromUserInput_addsFilePrefix_whenCalledWithLocalFile()
{
    using namespace Qt::Literals;

    QCOMPARE(qMediaFromUserInput(QUrl(u"/foo/bar/baz"_s)), QUrl(u"file:///foo/bar/baz"_s));
    QCOMPARE(qMediaFromUserInput(QUrl(u"file:///foo/bar/baz"_s)), QUrl(u"file:///foo/bar/baz"_s));
    QCOMPARE(qMediaFromUserInput(QUrl(u"http://foo/bar/baz"_s)), QUrl(u"http://foo/bar/baz"_s));

    QCOMPARE(qMediaFromUserInput(QUrl(u"foo/bar/baz"_s)),
             QUrl::fromLocalFile(QDir::currentPath() + u"/foo/bar/baz"_s));

#ifdef Q_OS_WIN
    QCOMPARE(qMediaFromUserInput(QUrl(u"C:/foo/bar/baz"_s)), QUrl(u"file:///c:/foo/bar/baz"_s));
#else
    QCOMPARE(qMediaFromUserInput(QUrl(u"C:/foo/bar/baz"_s)), QUrl(u"c:/foo/bar/baz"_s));
#endif
}

void tst_QMultimediaUtils::
        qGetRequiredSwapChainFormat_returnsSdr_whenMaxLuminanceIsBelowSdrThreshold_data()
{
    QTest::addColumn<float>("maxLuminance");

    QTest::newRow("0") << 0.0f;
    QTest::newRow("80") << 80.0f;
    QTest::newRow("100") << 100.0f;
}

void tst_QMultimediaUtils::
        qGetRequiredSwapChainFormat_returnsSdr_whenMaxLuminanceIsBelowSdrThreshold()
{
    // Arrange
    QFETCH(float, maxLuminance);

    QVideoFrameFormat format;
    format.setMaxLuminance(maxLuminance);

    // Act
    QRhiSwapChain::Format requiredSwapChainFormat = qGetRequiredSwapChainFormat(format);

    // Assert
    QCOMPARE(requiredSwapChainFormat, QRhiSwapChain::Format::SDR);
}

void tst_QMultimediaUtils::
        qGetRequiredSwapChainFormat_returnsHdr_whenMaxLuminanceIsBelowHdrThreshold_data()
{
    QTest::addColumn<float>("maxLuminance");

    QTest::newRow("101") << 101.0f;
    QTest::newRow("300") << 300.0f;
    QTest::newRow("1600") << 1600.0f;
}

void tst_QMultimediaUtils::
        qGetRequiredSwapChainFormat_returnsHdr_whenMaxLuminanceIsBelowHdrThreshold()
{
    // Arrange
    QVideoFrameFormat format;
    format.setMaxLuminance(300.0f);

    // Act
    QRhiSwapChain::Format requiredSwapChainFormat = qGetRequiredSwapChainFormat(format);

    // Assert
    QCOMPARE(requiredSwapChainFormat, QRhiSwapChain::Format::HDRExtendedSrgbLinear);
}

void tst_QMultimediaUtils::qShouldUpdateSwapChainFormat_returnsFalse_whenSwapChainIsNullPointer()
{
    // Arrange
    QRhiSwapChain *swapChain = nullptr;
    QRhiSwapChain::Format requiredSwapChainFormat = QRhiSwapChain::Format::SDR;

    // Act
    bool shouldUpdate = qShouldUpdateSwapChainFormat(swapChain, requiredSwapChainFormat);

    // Assert
    QCOMPARE(shouldUpdate, false);
}

QTEST_MAIN(tst_QMultimediaUtils)
#include "tst_qmultimediautils.moc"
