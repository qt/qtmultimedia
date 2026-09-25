// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>

#include <private/qpipewire_spa_video_format_support_p.h>

#include <spa/param/audio/raw.h>
#include <spa/param/format.h>
#include <spa/param/video/format-utils.h>
#include <spa/pod/builder.h>

#include <array>

using namespace QtPipeWire;

namespace {

// Enough for every pod these tests build.
constexpr size_t podBufferSize = 4096;

struct PodBuilder
{
    PodBuilder() { builder = SPA_POD_BUILDER_INIT(buffer.data(), podBufferSize); }

    std::array<uint8_t, podBufferSize> buffer{};
    spa_pod_builder builder{};
};

// A raw video format with one fixed pixel format, one fixed size and one
// fixed frame rate -- the shape a fully fixated format takes.
const spa_pod *buildFixedRawFormat(PodBuilder &pb, spa_video_format format, uint32_t width,
                                   uint32_t height, uint32_t fpsNum, uint32_t fpsDenom)
{
    const spa_rectangle size = SPA_RECTANGLE(width, height);
    const spa_fraction rate = SPA_FRACTION(fpsNum, fpsDenom);

    return static_cast<const spa_pod *>(spa_pod_builder_add_object(
            &pb.builder, SPA_TYPE_OBJECT_Format, SPA_PARAM_EnumFormat, SPA_FORMAT_mediaType,
            SPA_POD_Id(SPA_MEDIA_TYPE_video), SPA_FORMAT_mediaSubtype,
            SPA_POD_Id(SPA_MEDIA_SUBTYPE_raw), SPA_FORMAT_VIDEO_format, SPA_POD_Id(format),
            SPA_FORMAT_VIDEO_size, SPA_POD_Rectangle(&size), SPA_FORMAT_VIDEO_framerate,
            SPA_POD_Fraction(&rate)));
}

} // namespace

class tst_QPipeWireSpaVideoFormat : public QObject
{
    Q_OBJECT

private slots:
    void parse_returnsSingleFormat_whenPodIsFullyFixated();
    void parse_expandsPixelFormats_whenFormatIsAnEnumChoice();
    void parse_reportsBounds_whenSizeIsARangeChoice();
    void parse_expandsResolutions_whenSizeIsAnEnumChoice();
    void parse_reportsFrameRateBounds_whenFrameRateIsARangeChoice();
    void parse_fallsBackToMaxFramerate_whenFrameRateIsZero();
    void parse_reportsJpeg_whenSubtypeIsMjpg();
    void parse_fails_whenMediaTypeIsAudio();
    void parse_fails_whenSubtypeIsUnsupported();
    void parse_fails_whenNoPixelFormatIsMappable();
    void parse_fails_whenPodIsNull();

    void toCameraFormats_buildsCrossProduct_andRemovesDuplicates();
    void toCameraFormats_expandsSizeRangeToPreferredAndBounds();
};

void tst_QPipeWireSpaVideoFormat::parse_returnsSingleFormat_whenPodIsFullyFixated()
{
    PodBuilder pb;
    const auto parsed = SpaObjectVideoFormat::parse(
            buildFixedRawFormat(pb, SPA_VIDEO_FORMAT_NV12, 1280, 720, 30, 1));

    QVERIFY(parsed);
    QCOMPARE(parsed->mediaSubtype, SPA_MEDIA_SUBTYPE_raw);
    QCOMPARE(parsed->pixelFormats, std::vector{ QVideoFrameFormat::Format_NV12 });
    QVERIFY(std::holds_alternative<std::vector<QSize>>(parsed->resolutions));
    QCOMPARE(std::get<std::vector<QSize>>(parsed->resolutions), std::vector{ QSize(1280, 720) });
    QCOMPARE(parsed->minFrameRate, 30.);
    QCOMPARE(parsed->maxFrameRate, 30.);
}

void tst_QPipeWireSpaVideoFormat::parse_expandsPixelFormats_whenFormatIsAnEnumChoice()
{
    PodBuilder pb;
    const spa_rectangle size = SPA_RECTANGLE(640, 480);
    const spa_fraction rate = SPA_FRACTION(30, 1);

    const auto *pod = static_cast<const spa_pod *>(spa_pod_builder_add_object(
            &pb.builder, SPA_TYPE_OBJECT_Format, SPA_PARAM_EnumFormat, SPA_FORMAT_mediaType,
            SPA_POD_Id(SPA_MEDIA_TYPE_video), SPA_FORMAT_mediaSubtype,
            SPA_POD_Id(SPA_MEDIA_SUBTYPE_raw), SPA_FORMAT_VIDEO_format,
            SPA_POD_CHOICE_ENUM_Id(4, SPA_VIDEO_FORMAT_NV12, SPA_VIDEO_FORMAT_NV12,
                                   SPA_VIDEO_FORMAT_YUY2, SPA_VIDEO_FORMAT_I420),
            SPA_FORMAT_VIDEO_size, SPA_POD_Rectangle(&size), SPA_FORMAT_VIDEO_framerate,
            SPA_POD_Fraction(&rate)));

    const auto parsed = SpaObjectVideoFormat::parse(pod);
    QVERIFY(parsed);

    // The default (first) value repeats one of the alternatives; it must not
    // show up twice.
    const std::vector expected{ QVideoFrameFormat::Format_NV12, QVideoFrameFormat::Format_YUYV,
                                QVideoFrameFormat::Format_YUV420P };
    QCOMPARE(parsed->pixelFormats, expected);
}

void tst_QPipeWireSpaVideoFormat::parse_reportsBounds_whenSizeIsARangeChoice()
{
    PodBuilder pb;
    const spa_rectangle defaultSize = SPA_RECTANGLE(640, 480);
    const spa_rectangle minSize = SPA_RECTANGLE(320, 240);
    const spa_rectangle maxSize = SPA_RECTANGLE(1920, 1080);
    const spa_fraction rate = SPA_FRACTION(30, 1);

    const auto *pod = static_cast<const spa_pod *>(spa_pod_builder_add_object(
            &pb.builder, SPA_TYPE_OBJECT_Format, SPA_PARAM_EnumFormat, SPA_FORMAT_mediaType,
            SPA_POD_Id(SPA_MEDIA_TYPE_video), SPA_FORMAT_mediaSubtype,
            SPA_POD_Id(SPA_MEDIA_SUBTYPE_raw), SPA_FORMAT_VIDEO_format,
            SPA_POD_Id(SPA_VIDEO_FORMAT_NV12), SPA_FORMAT_VIDEO_size,
            SPA_POD_CHOICE_RANGE_Rectangle(&defaultSize, &minSize, &maxSize),
            SPA_FORMAT_VIDEO_framerate, SPA_POD_Fraction(&rate)));

    const auto parsed = SpaObjectVideoFormat::parse(pod);
    QVERIFY(parsed);

    QVERIFY(std::holds_alternative<SizeRange>(parsed->resolutions));
    const SizeRange &range = std::get<SizeRange>(parsed->resolutions);
    QCOMPARE(range.preferred, QSize(640, 480));
    QCOMPARE(range.min, QSize(320, 240));
    QCOMPARE(range.max, QSize(1920, 1080));
}

void tst_QPipeWireSpaVideoFormat::parse_expandsResolutions_whenSizeIsAnEnumChoice()
{
    PodBuilder pb;
    const spa_rectangle a = SPA_RECTANGLE(1920, 1080);
    const spa_rectangle b = SPA_RECTANGLE(1280, 720);
    const spa_fraction rate = SPA_FRACTION(30, 1);

    const auto *pod = static_cast<const spa_pod *>(spa_pod_builder_add_object(
            &pb.builder, SPA_TYPE_OBJECT_Format, SPA_PARAM_EnumFormat, SPA_FORMAT_mediaType,
            SPA_POD_Id(SPA_MEDIA_TYPE_video), SPA_FORMAT_mediaSubtype,
            SPA_POD_Id(SPA_MEDIA_SUBTYPE_raw), SPA_FORMAT_VIDEO_format,
            SPA_POD_Id(SPA_VIDEO_FORMAT_NV12), SPA_FORMAT_VIDEO_size,
            SPA_POD_CHOICE_ENUM_Rectangle(3, &a, &a, &b), SPA_FORMAT_VIDEO_framerate,
            SPA_POD_Fraction(&rate)));

    const auto parsed = SpaObjectVideoFormat::parse(pod);
    QVERIFY(parsed);

    QVERIFY(std::holds_alternative<std::vector<QSize>>(parsed->resolutions));
    const std::vector expected{ QSize(1920, 1080), QSize(1280, 720) };
    QCOMPARE(std::get<std::vector<QSize>>(parsed->resolutions), expected);
}

void tst_QPipeWireSpaVideoFormat::parse_reportsFrameRateBounds_whenFrameRateIsARangeChoice()
{
    PodBuilder pb;
    const spa_rectangle size = SPA_RECTANGLE(640, 480);
    const spa_fraction defaultRate = SPA_FRACTION(30, 1);
    const spa_fraction minRate = SPA_FRACTION(15, 2); // 7.5 fps
    const spa_fraction maxRate = SPA_FRACTION(60, 1);

    const auto *pod = static_cast<const spa_pod *>(spa_pod_builder_add_object(
            &pb.builder, SPA_TYPE_OBJECT_Format, SPA_PARAM_EnumFormat, SPA_FORMAT_mediaType,
            SPA_POD_Id(SPA_MEDIA_TYPE_video), SPA_FORMAT_mediaSubtype,
            SPA_POD_Id(SPA_MEDIA_SUBTYPE_raw), SPA_FORMAT_VIDEO_format,
            SPA_POD_Id(SPA_VIDEO_FORMAT_NV12), SPA_FORMAT_VIDEO_size, SPA_POD_Rectangle(&size),
            SPA_FORMAT_VIDEO_framerate,
            SPA_POD_CHOICE_RANGE_Fraction(&defaultRate, &minRate, &maxRate)));

    const auto parsed = SpaObjectVideoFormat::parse(pod);
    QVERIFY(parsed);
    QCOMPARE(parsed->minFrameRate, 7.5);
    QCOMPARE(parsed->maxFrameRate, 60.);
}

void tst_QPipeWireSpaVideoFormat::parse_fallsBackToMaxFramerate_whenFrameRateIsZero()
{
    // Variable frame rate nodes advertise framerate 0/1 and carry the real
    // upper bound in maxFramerate.
    PodBuilder pb;
    const spa_rectangle size = SPA_RECTANGLE(640, 480);
    const spa_fraction rate = SPA_FRACTION(0, 1);
    const spa_fraction maxRate = SPA_FRACTION(25, 1);

    const auto *pod = static_cast<const spa_pod *>(spa_pod_builder_add_object(
            &pb.builder, SPA_TYPE_OBJECT_Format, SPA_PARAM_EnumFormat, SPA_FORMAT_mediaType,
            SPA_POD_Id(SPA_MEDIA_TYPE_video), SPA_FORMAT_mediaSubtype,
            SPA_POD_Id(SPA_MEDIA_SUBTYPE_raw), SPA_FORMAT_VIDEO_format,
            SPA_POD_Id(SPA_VIDEO_FORMAT_NV12), SPA_FORMAT_VIDEO_size, SPA_POD_Rectangle(&size),
            SPA_FORMAT_VIDEO_framerate, SPA_POD_Fraction(&rate), SPA_FORMAT_VIDEO_maxFramerate,
            SPA_POD_Fraction(&maxRate)));

    const auto parsed = SpaObjectVideoFormat::parse(pod);
    QVERIFY(parsed);
    QCOMPARE(parsed->maxFrameRate, 25.);
}

void tst_QPipeWireSpaVideoFormat::parse_reportsJpeg_whenSubtypeIsMjpg()
{
    PodBuilder pb;
    const spa_rectangle size = SPA_RECTANGLE(1920, 1080);
    const spa_fraction rate = SPA_FRACTION(30, 1);

    // No SPA_FORMAT_VIDEO_format property: the pixel format follows from the
    // subtype.
    const auto *pod = static_cast<const spa_pod *>(spa_pod_builder_add_object(
            &pb.builder, SPA_TYPE_OBJECT_Format, SPA_PARAM_EnumFormat, SPA_FORMAT_mediaType,
            SPA_POD_Id(SPA_MEDIA_TYPE_video), SPA_FORMAT_mediaSubtype,
            SPA_POD_Id(SPA_MEDIA_SUBTYPE_mjpg), SPA_FORMAT_VIDEO_size, SPA_POD_Rectangle(&size),
            SPA_FORMAT_VIDEO_framerate, SPA_POD_Fraction(&rate)));

    const auto parsed = SpaObjectVideoFormat::parse(pod);
    QVERIFY(parsed);
    QCOMPARE(parsed->mediaSubtype, SPA_MEDIA_SUBTYPE_mjpg);
    QCOMPARE(parsed->pixelFormats, std::vector{ QVideoFrameFormat::Format_Jpeg });
}

void tst_QPipeWireSpaVideoFormat::parse_fails_whenMediaTypeIsAudio()
{
    PodBuilder pb;
    const auto *pod = static_cast<const spa_pod *>(spa_pod_builder_add_object(
            &pb.builder, SPA_TYPE_OBJECT_Format, SPA_PARAM_EnumFormat, SPA_FORMAT_mediaType,
            SPA_POD_Id(SPA_MEDIA_TYPE_audio), SPA_FORMAT_mediaSubtype,
            SPA_POD_Id(SPA_MEDIA_SUBTYPE_raw), SPA_FORMAT_AUDIO_format,
            SPA_POD_Id(SPA_AUDIO_FORMAT_S16), SPA_FORMAT_AUDIO_rate, SPA_POD_Int(48000)));

    QVERIFY(!SpaObjectVideoFormat::parse(pod));
}

void tst_QPipeWireSpaVideoFormat::parse_fails_whenSubtypeIsUnsupported()
{
    PodBuilder pb;
    const spa_rectangle size = SPA_RECTANGLE(1920, 1080);

    const auto *pod = static_cast<const spa_pod *>(spa_pod_builder_add_object(
            &pb.builder, SPA_TYPE_OBJECT_Format, SPA_PARAM_EnumFormat, SPA_FORMAT_mediaType,
            SPA_POD_Id(SPA_MEDIA_TYPE_video), SPA_FORMAT_mediaSubtype,
            SPA_POD_Id(SPA_MEDIA_SUBTYPE_h264), SPA_FORMAT_VIDEO_size, SPA_POD_Rectangle(&size)));

    QVERIFY(!SpaObjectVideoFormat::parse(pod));
}

void tst_QPipeWireSpaVideoFormat::parse_fails_whenNoPixelFormatIsMappable()
{
    PodBuilder pb;
    // SPA_VIDEO_FORMAT_ENCODED has no QVideoFrameFormat equivalent.
    QVERIFY(!SpaObjectVideoFormat::parse(
            buildFixedRawFormat(pb, SPA_VIDEO_FORMAT_ENCODED, 640, 480, 30, 1)));
}

void tst_QPipeWireSpaVideoFormat::parse_fails_whenPodIsNull()
{
    QVERIFY(!SpaObjectVideoFormat::parse(nullptr));
}

void tst_QPipeWireSpaVideoFormat::toCameraFormats_buildsCrossProduct_andRemovesDuplicates()
{
    SpaObjectVideoFormat first;
    first.pixelFormats = { QVideoFrameFormat::Format_NV12, QVideoFrameFormat::Format_YUYV };
    first.resolutions = std::vector{ QSize(1280, 720), QSize(640, 480) };
    first.minFrameRate = 15.;
    first.maxFrameRate = 30.;

    // Overlaps with 'first' in NV12/640x480 at the same frame rates.
    SpaObjectVideoFormat second;
    second.pixelFormats = { QVideoFrameFormat::Format_NV12 };
    second.resolutions = std::vector{ QSize(640, 480) };
    second.minFrameRate = 15.;
    second.maxFrameRate = 30.;

    const std::array formats{ first, second };
    const QList<QCameraFormat> cameraFormats = toCameraFormats(formats);

    QCOMPARE(cameraFormats.size(), 4);
    QCOMPARE(cameraFormats[0].pixelFormat(), QVideoFrameFormat::Format_NV12);
    QCOMPARE(cameraFormats[0].resolution(), QSize(1280, 720));
    QCOMPARE(cameraFormats[0].minFrameRate(), 15.);
    QCOMPARE(cameraFormats[0].maxFrameRate(), 30.);
    QCOMPARE(cameraFormats[1].resolution(), QSize(640, 480));
    QCOMPARE(cameraFormats[2].pixelFormat(), QVideoFrameFormat::Format_YUYV);
    QCOMPARE(cameraFormats[3].pixelFormat(), QVideoFrameFormat::Format_YUYV);
}

void tst_QPipeWireSpaVideoFormat::toCameraFormats_expandsSizeRangeToPreferredAndBounds()
{
    SpaObjectVideoFormat format;
    format.pixelFormats = { QVideoFrameFormat::Format_NV12 };
    format.resolutions = SizeRange{
        .preferred = QSize(640, 480),
        .min = QSize(320, 240),
        .max = QSize(1920, 1080),
    };
    format.minFrameRate = 15.;
    format.maxFrameRate = 30.;

    const std::array formats{ format };
    const QList<QCameraFormat> cameraFormats = toCameraFormats(formats);

    QCOMPARE(cameraFormats.size(), 3);
    QCOMPARE(cameraFormats[0].resolution(), QSize(640, 480));
    QCOMPARE(cameraFormats[1].resolution(), QSize(320, 240));
    QCOMPARE(cameraFormats[2].resolution(), QSize(1920, 1080));
}

QTEST_GUILESS_MAIN(tst_QPipeWireSpaVideoFormat)

#include "tst_qpipewire_spa_video_format.moc"
