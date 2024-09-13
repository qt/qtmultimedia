// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QtTest>
#include <QDebug>
#include <private/qmultimediautils_p.h>
#include <qvideoframeformat.h>
#include <qvideoframe.h>

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

    void qNormalizedFrameTransformation_normilizesInputTransformation_data();
    void qNormalizedFrameTransformation_normilizesInputTransformation();

    void qVideoRotationFromDegrees_basicValues_data();
    void qVideoRotationFromDegrees_basicValues();
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

void tst_QMultimediaUtils::qNormalizedFrameTransformation_normilizesInputTransformation_data()
{
    QTest::addColumn<QtVideo::Rotation>("surfaceRotation");
    QTest::addColumn<bool>("surfaceMirrored");
    QTest::addColumn<QVideoFrameFormat::Direction>("scanLineDirection");

    QTest::addColumn<QtVideo::Rotation>("frameRotation");
    QTest::addColumn<bool>("frameMirrored");
    QTest::addColumn<int>("additionalFrameRotation");

    const auto rotations = { QtVideo::Rotation::None, QtVideo::Rotation::Clockwise90,
                             QtVideo::Rotation::Clockwise180, QtVideo::Rotation::Clockwise270 };
    const auto scanLineDirections = { QVideoFrameFormat::TopToBottom,
                                      QVideoFrameFormat::BottomToTop };

    auto newRow = [](QtVideo::Rotation surfaceRotation, bool surfaceMirrored,
                     QVideoFrameFormat::Direction scanLineDirection,
                     QtVideo::Rotation frameRotation, bool frameMirrored, int additionalRotation) {
        QString tag;
        QDebug stream(&tag);
        stream << "Surface transform:" << surfaceRotation;
        if (surfaceMirrored)
            stream << "mirrored";
        if (scanLineDirection == QVideoFrameFormat::BottomToTop)
            stream << "opposite_scanline";

        stream << "Frame transform:" << frameRotation;
        if (frameMirrored)
            stream << "mirrored";
        if (additionalRotation)
            stream << "additionalRotation:" << additionalRotation;

        QTest::newRow(tag.toLatin1().data())
                << surfaceRotation << surfaceMirrored << scanLineDirection << frameRotation
                << frameMirrored << additionalRotation;
    };

    for (QtVideo::Rotation surfaceRotation : rotations)
        for (bool surfaceMirrored : { false, true })
            for (QVideoFrameFormat::Direction scanLineDirection : scanLineDirections)
                for (QtVideo::Rotation frameRotation : rotations)
                    for (bool frameMirrored : { false, true })
                        newRow(surfaceRotation, surfaceMirrored, scanLineDirection, frameRotation,
                               frameMirrored, 0);

    for (int additionalRotation : { -3690, -3600, -90, 90, 1800, 1980 })
        newRow(QtVideo::Rotation::Clockwise90, true, QVideoFrameFormat::TopToBottom,
               QtVideo::Rotation::None, false, additionalRotation);
}

// eliminates float numbers inaccuracy
static void fixTransform(QTransform &transform)
{
    auto &matrix = transform.asAffineMatrix().m_matrix;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            matrix[i][j] = std::round(matrix[i][j]);
}

static void rotate(QTransform &transform, int rotation)
{
    if (rotation != 0) {
        transform.rotate(-qreal(rotation));
        fixTransform(transform);
    }
}

static void rotate(QTransform &transform, QtVideo::Rotation rotation)
{
    rotate(transform, qToUnderlying(rotation));
}

static void xMirror(QTransform &transform, bool mirror)
{
    if (mirror) {
        transform.scale(-1., 1.);
        fixTransform(transform);
    }
}

static void yMirror(QTransform &transform, bool mirror)
{
    if (mirror) {
        transform.scale(1., -1.);
        fixTransform(transform);
    }
}

static QTransform makeTransformMatrix(const NormalizedVideoTransformation &transform)
{
    QTransform result;

    rotate(result, transform.rotation);
    xMirror(result, transform.xMirrorredAfterRotation);

    return result;
}

static QTransform makeTransformMatrix(QtVideo::Rotation surfaceRotation, bool surfaceMirrored,
                                      QVideoFrameFormat::Direction scanLineDirection,
                                      QtVideo::Rotation frameRotation, bool frameMirrored,
                                      int additionalRotation)
{
    QTransform result;

    yMirror(result, scanLineDirection == QVideoFrameFormat::BottomToTop);
    rotate(result, surfaceRotation);
    xMirror(result, surfaceMirrored);
    rotate(result, frameRotation);
    xMirror(result, frameMirrored);
    rotate(result, additionalRotation);

    return result;
}

void tst_QMultimediaUtils::qNormalizedFrameTransformation_normilizesInputTransformation()
{
    // Arrange
    QFETCH(const QtVideo::Rotation, surfaceRotation);
    QFETCH(const bool, surfaceMirrored);
    QFETCH(const QVideoFrameFormat::Direction, scanLineDirection);

    QFETCH(const QtVideo::Rotation, frameRotation);
    QFETCH(const bool, frameMirrored);
    QFETCH(const int, additionalFrameRotation);

    QVideoFrameFormat format(QSize(4, 4), QVideoFrameFormat::Format_ARGB8888);
    format.setRotation(surfaceRotation);
    format.setMirrored(surfaceMirrored);
    format.setScanLineDirection(scanLineDirection);

    QVideoFrame frame(format);

    frame.setRotation(frameRotation);
    frame.setMirrored(frameMirrored);

    // Act
    const NormalizedVideoTransformation actual =
            qNormalizedFrameTransformation(frame, additionalFrameRotation);

    const QTransform actualTransform = makeTransformMatrix(actual);
    const QTransform expectedTransform =
            makeTransformMatrix(surfaceRotation, surfaceMirrored, scanLineDirection, frameRotation,
                                frameMirrored, additionalFrameRotation);

    if (actualTransform != expectedTransform) {
        qWarning() << "actualRotation:" << actual.rotation
                   << "actualMirrored:" << actual.xMirrorredAfterRotation;
        for (bool mirrored : { true, false })
            for (int rotationIndex = 0; rotationIndex < 4; ++rotationIndex) {
                const NormalizedVideoTransformation transform{
                    QtVideo::Rotation(rotationIndex * 90), rotationIndex, mirrored
                };
                const auto matrix = makeTransformMatrix(transform);
                if (matrix == expectedTransform)
                    qWarning() << "expectedRotation:" << transform.rotation
                               << "expectedMirrored:" << transform.xMirrorredAfterRotation;
            }
    }

    // Assert
    QCOMPARE(actualTransform, expectedTransform);
    QCOMPARE(qToUnderlying(actual.rotation), actual.rotationIndex * 90);
}

void tst_QMultimediaUtils::qVideoRotationFromDegrees_basicValues_data()
{
    QTest::addColumn<int>("inputDegrees");
    QTest::addColumn<QtVideo::Rotation>("expectedOutput");

    QTest::newRow("0") << 0 << QtVideo::Rotation::None;
    QTest::newRow("90") << 90 << QtVideo::Rotation::Clockwise90;
    QTest::newRow("180") << 180 << QtVideo::Rotation::Clockwise180;
    QTest::newRow("270") << 270 << QtVideo::Rotation::Clockwise270;
    QTest::newRow("360") << 360 << QtVideo::Rotation::None;
    QTest::newRow("450") << 450 << QtVideo::Rotation::Clockwise90;
    QTest::newRow("630") << 630 << QtVideo::Rotation::Clockwise270;
    QTest::newRow("-90") << -90 << QtVideo::Rotation::Clockwise270;
    QTest::newRow("-270") << -270 << QtVideo::Rotation::Clockwise90;
    QTest::newRow("-450") << -450 << QtVideo::Rotation::Clockwise270;
    QTest::newRow("1") << 1 << QtVideo::Rotation::None;
}

void tst_QMultimediaUtils::qVideoRotationFromDegrees_basicValues()
{
    QFETCH(int, inputDegrees);
    QFETCH(QtVideo::Rotation, expectedOutput);

    auto actual = qVideoRotationFromDegrees(inputDegrees);

    QCOMPARE(actual, expectedOutput);
}

QTEST_MAIN(tst_QMultimediaUtils)
#include "tst_qmultimediautils.moc"
