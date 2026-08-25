// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>

#include <QtFFmpegMediaPluginImpl/private/qwindowscamera_p.h>
#include <QtMultimedia/private/qcameradevice_p.h>
#include <QtCore/qobject.h>

using namespace QFFmpeg;

namespace {

constexpr QSize hd{ 1280, 720 };

QCameraFormat makeCameraFormat(QVideoFrameFormat::PixelFormat pixelFormat, QSize resolution,
                               float minFrameRate, float maxFrameRate)
{
    return (new QCameraFormatPrivate{ QSharedData(), pixelFormat, resolution, minFrameRate,
                                      maxFrameRate })
            ->create();
}

QCameraFormat fixedRate(QVideoFrameFormat::PixelFormat pixelFormat, QSize resolution,
                        float frameRate)
{
    return makeCameraFormat(pixelFormat, resolution, frameRate, frameRate);
}

QList<QCameraFormat> logitechBrio500Formats()
{
    return { fixedRate(QVideoFrameFormat::Format_NV12, QSize{ 1920, 1080 }, 30.f),
             fixedRate(QVideoFrameFormat::Format_YUYV, hd, 10.f),
             fixedRate(QVideoFrameFormat::Format_YUYV, hd, 7.5f),
             fixedRate(QVideoFrameFormat::Format_YUYV, hd, 5.f),
             fixedRate(QVideoFrameFormat::Format_NV12, hd, 30.f),
             fixedRate(QVideoFrameFormat::Format_YUYV, hd, 60.f),
             fixedRate(QVideoFrameFormat::Format_YUYV, hd, 30.f),
             fixedRate(QVideoFrameFormat::Format_YUYV, hd, 24.f),
             fixedRate(QVideoFrameFormat::Format_YUYV, hd, 20.f),
             fixedRate(QVideoFrameFormat::Format_YUYV, hd, 15.f) };
}

} // namespace

class tst_QWindowsCamera : public QObject
{
    Q_OBJECT

private slots:
    void indexOfClosestCameraFormat_returnsFormatWithRequestedFrameRate_data();
    void indexOfClosestCameraFormat_returnsFormatWithRequestedFrameRate();
    void indexOfClosestCameraFormat_returnsNearestRate_whenRateIsNotOffered();
    void indexOfClosestCameraFormat_prefersRangeType_whenMaxRatesAreEqual();
    void indexOfClosestCameraFormat_returnsNoMatch_whenResolutionIsNotOffered();
};

void tst_QWindowsCamera::indexOfClosestCameraFormat_returnsFormatWithRequestedFrameRate_data()
{
    QTest::addColumn<float>("requestedFrameRate");
    QTest::addColumn<qsizetype>("expectedIndex");

    QTest::newRow("60 fps, enumerated after the slow group") << 60.f << qsizetype(5);
    QTest::newRow("30 fps, also offered by another pixel format") << 30.f << qsizetype(6);
    QTest::newRow("24 fps") << 24.f << qsizetype(7);
    QTest::newRow("20 fps") << 20.f << qsizetype(8);
    QTest::newRow("15 fps") << 15.f << qsizetype(9);
    QTest::newRow("10 fps, enumerated first") << 10.f << qsizetype(1);
    QTest::newRow("7.5 fps, a non-integer rate") << 7.5f << qsizetype(2);
    QTest::newRow("5 fps, the slowest offered") << 5.f << qsizetype(3);
}

void tst_QWindowsCamera::indexOfClosestCameraFormat_returnsFormatWithRequestedFrameRate()
{
    QFETCH(const float, requestedFrameRate);
    QFETCH(const qsizetype, expectedIndex);

    const QList<QCameraFormat> candidates = logitechBrio500Formats();
    const QCameraFormat requested =
            fixedRate(QVideoFrameFormat::Format_YUYV, hd, requestedFrameRate);

    QCOMPARE(indexOfClosestCameraFormat(candidates, requested), expectedIndex);
}

void tst_QWindowsCamera::indexOfClosestCameraFormat_returnsNearestRate_whenRateIsNotOffered()
{
    const QList<QCameraFormat> candidates = logitechBrio500Formats();
    const QCameraFormat requested = fixedRate(QVideoFrameFormat::Format_YUYV, hd, 40.f);

    QCOMPARE(indexOfClosestCameraFormat(candidates, requested), qsizetype(6));
}

void tst_QWindowsCamera::indexOfClosestCameraFormat_prefersRangeType_whenMaxRatesAreEqual()
{
    const QList<QCameraFormat> candidates = {
        fixedRate(QVideoFrameFormat::Format_YUYV, hd, 30.f),
        makeCameraFormat(QVideoFrameFormat::Format_YUYV, hd, 5.f, 30.f)
    };
    const QCameraFormat requested = makeCameraFormat(QVideoFrameFormat::Format_YUYV, hd, 5.f, 30.f);

    QCOMPARE(indexOfClosestCameraFormat(candidates, requested), qsizetype(1));
}

void tst_QWindowsCamera::indexOfClosestCameraFormat_returnsNoMatch_whenResolutionIsNotOffered()
{
    const QList<QCameraFormat> candidates = logitechBrio500Formats();
    const QCameraFormat requested =
            fixedRate(QVideoFrameFormat::Format_YUYV, QSize{ 640, 480 }, 30.f);

    QCOMPARE(indexOfClosestCameraFormat(candidates, requested), qsizetype(-1));
}

QTEST_MAIN(tst_QWindowsCamera)

#include "tst_qwindowscamera.moc"
