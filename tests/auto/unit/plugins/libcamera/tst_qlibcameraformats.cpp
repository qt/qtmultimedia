// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>

// This test compiles the libcamera plugin's format-mapping source directly (see CMakeLists.txt)
// rather than linking the plugin itself, so it can exercise the mapping functions with no
// hardware and no running libcamera::CameraManager -- StreamFormats and ControlInfoMap can both
// be hand-built directly from their public constructors.
#include "qlibcamera_format_support_p.h"

#include <QtMultimedia/private/qmultimedia_ranges_p.h>

#include <libcamera/color_space.h>
#include <libcamera/control_ids.h>
#include <libcamera/controls.h>
#include <libcamera/formats.h>
#include <libcamera/geometry.h>
#include <libcamera/pixel_format.h>
#include <libcamera/stream.h>

#include <map>
#include <vector>

using namespace QtMultimediaPrivate;
using namespace libcamera;
using namespace QLibCamera;

class tst_QLibCameraFormats : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void pixelFormatFromLibcamera_data();
    void pixelFormatFromLibcamera();

    void colorSpaceFromLibcamera();

    void frameRateRangeFromControls_returnsComputedRange_whenFrameDurationLimitsPresent();
    void frameRateRangeFromControls_returnsFallbackRange_whenFrameDurationLimitsMissing();
    void frameRateRangeFromControls_returnsFallbackRange_whenFrameDurationLimitsIsNotAnArray();

    void sizesForPixelFormat_returnsFullDiscreteList_whenAllSizeRangesAreDiscrete();
    void sizesForPixelFormat_dedupesDuplicateDiscreteEntries();
    void sizesForPixelFormat_unionsNativeExtremes_withContinuousRangeTable();
    void sizesForPixelFormat_fallsBackToRangeExtremes_whenNoTableSizeFitsInRange();
    void sizesForPixelFormat_fallsBackToBoundingBox_forAmbiguousMultiRange();
    void sizesForPixelFormat_returnsEmpty_whenPixelFormatNotPresent();
};

void tst_QLibCameraFormats::pixelFormatFromLibcamera_data()
{
    QTest::addColumn<PixelFormat>("format");
    QTest::addColumn<bool>("expectedValid");
    QTest::addColumn<int>("expectedQtFormat"); // only meaningful when expectedValid is true

    // clang-format off
    QTest::newRow("NV12")   << formats::NV12   << true  << int(QVideoFrameFormat::Format_NV12);
    QTest::newRow("NV21")   << formats::NV21   << true  << int(QVideoFrameFormat::Format_NV21);
    QTest::newRow("YUV420") << formats::YUV420 << true  << int(QVideoFrameFormat::Format_YUV420P);
    QTest::newRow("YVU420") << formats::YVU420 << true  << int(QVideoFrameFormat::Format_YV12);
    QTest::newRow("YUV422") << formats::YUV422 << true  << int(QVideoFrameFormat::Format_YUV422P);
    QTest::newRow("YUYV")   << formats::YUYV   << true  << int(QVideoFrameFormat::Format_YUYV);
    QTest::newRow("UYVY")   << formats::UYVY   << true  << int(QVideoFrameFormat::Format_UYVY);
    QTest::newRow("XRGB8888") << formats::XRGB8888 << true << int(QVideoFrameFormat::Format_XRGB8888);
    QTest::newRow("XBGR8888") << formats::XBGR8888 << true << int(QVideoFrameFormat::Format_XBGR8888);
    QTest::newRow("ARGB8888") << formats::ARGB8888 << true << int(QVideoFrameFormat::Format_ARGB8888);
    QTest::newRow("ABGR8888") << formats::ABGR8888 << true << int(QVideoFrameFormat::Format_ABGR8888);
    QTest::newRow("R8")     << formats::R8     << true  << int(QVideoFrameFormat::Format_Y8);
    QTest::newRow("R16")    << formats::R16    << true  << int(QVideoFrameFormat::Format_Y16);
    QTest::newRow("MJPEG")  << formats::MJPEG  << true  << int(QVideoFrameFormat::Format_Jpeg);

    // No lossless QVideoFrameFormat equivalent -- must be rejected, not silently mapped to
    // something misleading.
    QTest::newRow("RGB888-no-24bpp-equivalent") << formats::RGB888 << false << 0;
    QTest::newRow("BGR888-no-24bpp-equivalent") << formats::BGR888 << false << 0;
    QTest::newRow("Bayer-SRGGB8")  << formats::SRGGB8  << false << 0;
    QTest::newRow("Bayer-SRGGB10") << formats::SRGGB10 << false << 0;
    // clang-format on
}

void tst_QLibCameraFormats::pixelFormatFromLibcamera()
{
    QFETCH(PixelFormat, format);
    QFETCH(bool, expectedValid);
    QFETCH(int, expectedQtFormat);

    const std::optional<QVideoFrameFormat::PixelFormat> result =
            QLibCameraFormatSupport::pixelFormatFromLibcamera(format);

    QCOMPARE(result.has_value(), expectedValid);
    if (expectedValid)
        QCOMPARE(int(*result), expectedQtFormat);
}

void tst_QLibCameraFormats::colorSpaceFromLibcamera()
{
    // Not a data-driven test: libcamera::ColorSpace has no default constructor, which QTest's
    // data-table machinery (QMetaType-backed columns) cannot store, so each case is verified
    // directly instead.
    using Primaries = ColorSpace::Primaries;
    using Transfer = ColorSpace::TransferFunction;
    using Encoding = ColorSpace::YcbcrEncoding;
    using Range = ColorSpace::Range;

    {
        const auto [space, range] = QLibCameraFormatSupport::colorSpaceFromLibcamera(std::nullopt);
        QCOMPARE(space, QVideoFrameFormat::ColorSpace_Undefined);
        QCOMPARE(range, QVideoFrameFormat::ColorRange_Unknown);
    }
    {
        const ColorSpace cs(Primaries::Smpte170m, Transfer::Rec709, Encoding::Rec601,
                            Range::Limited);
        const auto [space, range] = QLibCameraFormatSupport::colorSpaceFromLibcamera(cs);
        QCOMPARE(space, QVideoFrameFormat::ColorSpace_BT601);
        QCOMPARE(range, QVideoFrameFormat::ColorRange_Video);
    }
    {
        const ColorSpace cs(Primaries::Rec709, Transfer::Rec709, Encoding::Rec709, Range::Full);
        const auto [space, range] = QLibCameraFormatSupport::colorSpaceFromLibcamera(cs);
        QCOMPARE(space, QVideoFrameFormat::ColorSpace_BT709);
        QCOMPARE(range, QVideoFrameFormat::ColorRange_Full);
    }
    {
        const ColorSpace cs(Primaries::Rec2020, Transfer::Linear, Encoding::Rec2020,
                            Range::Limited);
        const auto [space, range] = QLibCameraFormatSupport::colorSpaceFromLibcamera(cs);
        QCOMPARE(space, QVideoFrameFormat::ColorSpace_BT2020);
        QCOMPARE(range, QVideoFrameFormat::ColorRange_Video);
    }
    {
        // RGB colorspace (no YCbCr encoding) with wide-gamut primaries still maps to BT2020.
        const ColorSpace cs(Primaries::Rec2020, Transfer::Linear, Encoding::None, Range::Full);
        const auto [space, range] = QLibCameraFormatSupport::colorSpaceFromLibcamera(cs);
        QCOMPARE(space, QVideoFrameFormat::ColorSpace_BT2020);
        QCOMPARE(range, QVideoFrameFormat::ColorRange_Full);
    }
    {
        // sRGB: no YCbCr encoding and no wide-gamut primaries -- falls back to Undefined rather
        // than misreporting a specific YCbCr colorspace for an RGB signal.
        const ColorSpace cs(Primaries::Rec709, Transfer::Srgb, Encoding::None, Range::Full);
        const auto [space, range] = QLibCameraFormatSupport::colorSpaceFromLibcamera(cs);
        QCOMPARE(space, QVideoFrameFormat::ColorSpace_Undefined);
        QCOMPARE(range, QVideoFrameFormat::ColorRange_Full);
    }
}

void tst_QLibCameraFormats::
        frameRateRangeFromControls_returnsComputedRange_whenFrameDurationLimitsPresent()
{
    // 33333us -> ~30fps, 100000us -> 10fps. The *minimum* duration corresponds to the *maximum*
    // frame rate and vice versa.
    const std::array<int64_t, 2> minDuration{ 33333, 33333 };
    const std::array<int64_t, 2> maxDuration{ 100000, 100000 };
    const ControlValue minValue{ Span<const int64_t, 2>(minDuration) };
    const ControlValue maxValue{ Span<const int64_t, 2>(maxDuration) };

    const ControlInfoMap infoMap(ControlInfoMap::Map{ { &controls::FrameDurationLimits,
                                                        ControlInfo(minValue, maxValue) } },
                                 controls::controls);

    const auto [minFrameRate, maxFrameRate] =
            QLibCameraFormatSupport::frameRateRangeFromControls(infoMap);

    QVERIFY(qFuzzyCompare(minFrameRate, 10.f));
    QVERIFY(qAbs(maxFrameRate - 30.0003f) < 0.01f);
}

void tst_QLibCameraFormats::
        frameRateRangeFromControls_returnsFallbackRange_whenFrameDurationLimitsMissing()
{
    const ControlInfoMap emptyInfoMap;

    const auto [minFrameRate, maxFrameRate] =
            QLibCameraFormatSupport::frameRateRangeFromControls(emptyInfoMap);

    QCOMPARE(minFrameRate, 1.f);
    QCOMPARE(maxFrameRate, 30.f);
}

void tst_QLibCameraFormats::
        frameRateRangeFromControls_returnsFallbackRange_whenFrameDurationLimitsIsNotAnArray()
{
    // Regression test: libcamera's own "virtual" test pipeline handler (as of libcamera 0.7.0)
    // reports FrameDurationLimits with scalar (non-array) min/max ControlValues, even though the
    // control is declared as Control<Span<const int64_t, 2>>. Calling
    // ControlValue::get<Span<...>>() on such a value asserts isArray_ and aborts the process --
    // confirmed by running the plugin against the real virtual pipeline handler. This must be
    // handled as a graceful fallback, not a crash.
    const ControlValue scalarMin{ int64_t(33333) };
    const ControlValue scalarMax{ int64_t(100000) };
    QVERIFY(!scalarMin.isArray());
    QVERIFY(!scalarMax.isArray());

    const ControlInfoMap infoMap(ControlInfoMap::Map{ { &controls::FrameDurationLimits,
                                                        ControlInfo(scalarMin, scalarMax) } },
                                 controls::controls);

    const auto [minFrameRate, maxFrameRate] =
            QLibCameraFormatSupport::frameRateRangeFromControls(infoMap);

    QCOMPARE(minFrameRate, 1.f);
    QCOMPARE(maxFrameRate, 30.f);
}

void tst_QLibCameraFormats::
        sizesForPixelFormat_returnsFullDiscreteList_whenAllSizeRangesAreDiscrete()
{
    // Direct regression test for the reported bug: a pixel format backed by several genuinely
    // discrete sizes (as libcamera's "simple" pipeline handler reports V4L2's discrete
    // VIDIOC_ENUM_FRAMESIZES result) must surface every one of them, not just the overall min/max.
    const std::map<PixelFormat, std::vector<SizeRange>> map{
        { formats::NV12,
          { SizeRange(Size(1920, 1080)), SizeRange(Size(640, 480)), SizeRange(Size(1280, 720)),
            SizeRange(Size(800, 600)) } },
    };
    const StreamFormats streamFormats(map);

    const std::vector<QSize> sizes =
            QLibCameraFormatSupport::sizesForPixelFormat(streamFormats, formats::NV12);

    QCOMPARE(sizes,
             std::vector<QSize>(
                     { QSize(640, 480), QSize(800, 600), QSize(1280, 720), QSize(1920, 1080) }));
}

void tst_QLibCameraFormats::sizesForPixelFormat_dedupesDuplicateDiscreteEntries()
{
    // StreamFormats::sizes() itself doesn't dedupe -- a pipeline handler reporting the same
    // discrete size twice must not produce a duplicate QCameraFormat.
    const std::map<PixelFormat, std::vector<SizeRange>> map{
        { formats::NV12, { SizeRange(Size(640, 480)), SizeRange(Size(640, 480)) } },
    };
    const StreamFormats streamFormats(map);

    const std::vector<QSize> sizes =
            QLibCameraFormatSupport::sizesForPixelFormat(streamFormats, formats::NV12);

    QCOMPARE(sizes, std::vector<QSize>({ QSize(640, 480) }));
}

void tst_QLibCameraFormats::sizesForPixelFormat_unionsNativeExtremes_withContinuousRangeTable()
{
    // A single continuous range (min != max): StreamFormats::sizes() only intersects a fixed
    // 53-entry table of common resolutions against it, which can miss the camera's true native
    // extremes -- those must still come through via range().
    const std::map<PixelFormat, std::vector<SizeRange>> map{
        { formats::NV12, { SizeRange(Size(176, 144), Size(2592, 1944)) } },
    };
    const StreamFormats streamFormats(map);

    const std::vector<QSize> sizes =
            QLibCameraFormatSupport::sizesForPixelFormat(streamFormats, formats::NV12);

    QVERIFY(ranges::contains(sizes, QSize(176, 144)));
    QVERIFY(ranges::contains(sizes, QSize(2592, 1944)));
    QVERIFY(ranges::contains(sizes, QSize(640, 480))); // a common resolution from the table
    QVERIFY(ranges::contains(sizes, QSize(1920, 1080)));

    for (const QSize &size : sizes) {
        QVERIFY(size.width() >= 176 && size.width() <= 2592);
        QVERIFY(size.height() >= 144 && size.height() <= 1944);
    }
    for (size_t i = 1; i < sizes.size(); ++i)
        QCOMPARE_NE(sizes[i], sizes[i - 1]); // sorted with no duplicates
}

void tst_QLibCameraFormats::
        sizesForPixelFormat_fallsBackToRangeExtremes_whenNoTableSizeFitsInRange()
{
    // None of the fixed common-resolution table entries fit inside such a small range, so
    // sizes() alone returns nothing for it; the true min/max must still be reported rather than
    // silently dropping the pixel format.
    const std::map<PixelFormat, std::vector<SizeRange>> map{
        { formats::NV12, { SizeRange(Size(2, 2), Size(100, 100)) } },
    };
    const StreamFormats streamFormats(map);

    const std::vector<QSize> sizes =
            QLibCameraFormatSupport::sizesForPixelFormat(streamFormats, formats::NV12);

    QCOMPARE(sizes, std::vector<QSize>({ QSize(2, 2), QSize(100, 100) }));
}

void tst_QLibCameraFormats::sizesForPixelFormat_fallsBackToBoundingBox_forAmbiguousMultiRange()
{
    // More than one SizeRange entry where at least one isn't a single discrete point: libcamera's
    // own StreamFormats::sizes() logs this as "ambiguous" (LOG(Stream, Error), not Qt-visible) and
    // returns an empty list. range() still computes a min/max bounding box across all entries in
    // this case, so that bounding box -- not an empty result -- is what should surface here.
    const std::map<PixelFormat, std::vector<SizeRange>> map{
        { formats::NV12, { SizeRange(Size(100, 100)), SizeRange(Size(200, 200), Size(400, 400)) } },
    };
    const StreamFormats streamFormats(map);

    const std::vector<QSize> sizes =
            QLibCameraFormatSupport::sizesForPixelFormat(streamFormats, formats::NV12);

    QCOMPARE(sizes, std::vector<QSize>({ QSize(100, 100), QSize(400, 400) }));
}

void tst_QLibCameraFormats::sizesForPixelFormat_returnsEmpty_whenPixelFormatNotPresent()
{
    const std::map<PixelFormat, std::vector<SizeRange>> map{
        { formats::NV12, { SizeRange(Size(640, 480)) } },
    };
    const StreamFormats streamFormats(map);

    const std::vector<QSize> sizes =
            QLibCameraFormatSupport::sizesForPixelFormat(streamFormats, formats::MJPEG);

    QVERIFY(sizes.empty());
}

QTEST_MAIN(tst_QLibCameraFormats)
#include "tst_qlibcameraformats.moc"
