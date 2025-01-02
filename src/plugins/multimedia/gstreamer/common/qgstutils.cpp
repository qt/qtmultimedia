// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtMultimedia/private/qtmultimediaglobal_p.h>
#include "qgstutils_p.h"

#include <QtCore/qdatetime.h>
#include <QtCore/qdir.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qvariant.h>
#include <QtCore/qregularexpression.h>
#include <QtCore/qsize.h>
#include <QtCore/qset.h>
#include <QtCore/qstringlist.h>
#include <QtGui/qimage.h>
#include <qaudioformat.h>
#include <QtCore/qelapsedtimer.h>
#include <QtMultimedia/qvideoframeformat.h>
#include <private/qmultimediautils_p.h>

#include <gst/audio/audio.h>
#include <gst/video/video.h>

template<typename T, int N> constexpr int lengthOf(const T (&)[N]) { return N; }

QT_BEGIN_NAMESPACE


namespace {

static const char *audioSampleFormatNames[QAudioFormat::NSampleFormats] = {
    nullptr,
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    "U8",
    "S16LE",
    "S32LE",
    "F32LE"
#else
    "U8",
    "S16BE",
    "S32BE",
    "F32BE"
#endif
};

static QAudioFormat::SampleFormat gstSampleFormatToSampleFormat(const char *fmt)
{
    if (fmt) {
        for (int i = 1; i < QAudioFormat::NSampleFormats; ++i) {
            if (strcmp(fmt, audioSampleFormatNames[i]))
                continue;
            return QAudioFormat::SampleFormat(i);
        }
    }
    return QAudioFormat::Unknown;
}

}

/*
  Returns audio format for a sample \a sample.
  If the buffer doesn't have a valid audio format, an empty QAudioFormat is returned.
*/
QAudioFormat QGstUtils::audioFormatForSample(GstSample *sample)
{
    auto caps = QGstCaps(gst_sample_get_caps(sample), QGstCaps::NeedsRef);
    if (caps.isNull())
        return {};
    return audioFormatForCaps(caps);
}

QAudioFormat QGstUtils::audioFormatForCaps(const QGstCaps &caps)
{
    QAudioFormat format;
    QGstStructure s = caps.at(0);
    if (s.name() != "audio/x-raw")
        return format;

    auto rate = s["rate"].toInt();
    auto channels = s["channels"].toInt();
    QAudioFormat::SampleFormat fmt = gstSampleFormatToSampleFormat(s["format"].toString());
    if (!rate || !channels || fmt == QAudioFormat::Unknown)
        return format;

    format.setSampleRate(*rate);
    format.setChannelCount(*channels);
    format.setSampleFormat(fmt);

    return format;
}

/*
  Builds GstCaps for an audio format \a format.
  Returns 0 if the audio format is not valid.

  \note Caller must unreference GstCaps.
*/

QGstCaps QGstUtils::capsForAudioFormat(const QAudioFormat &format)
{
    if (!format.isValid())
        return {};

    auto sampleFormat = format.sampleFormat();
    auto caps = gst_caps_new_simple(
                "audio/x-raw",
                "format"  , G_TYPE_STRING, audioSampleFormatNames[sampleFormat],
                "rate"    , G_TYPE_INT   , format.sampleRate(),
                "channels", G_TYPE_INT   , format.channelCount(),
                "layout"  , G_TYPE_STRING, "interleaved",
                nullptr);

    return QGstCaps(caps, QGstCaps::HasRef);
}

QList<QAudioFormat::SampleFormat> QGValue::getSampleFormats() const
{
    if (!GST_VALUE_HOLDS_LIST(value))
        return {};

    QList<QAudioFormat::SampleFormat> formats;
    guint nFormats = gst_value_list_get_size(value);
    for (guint f = 0; f < nFormats; ++f) {
        QGValue v = gst_value_list_get_value(value, f);
        auto *name = v.toString();
        QAudioFormat::SampleFormat fmt = gstSampleFormatToSampleFormat(name);
        if (fmt == QAudioFormat::Unknown)
            continue;;
        formats.append(fmt);
    }
    return formats;
}

namespace {

struct VideoFormat
{
    QVideoFrameFormat::PixelFormat pixelFormat;
    GstVideoFormat gstFormat;
};

static const VideoFormat qt_videoFormatLookup[] =
{
    { QVideoFrameFormat::Format_YUV420P, GST_VIDEO_FORMAT_I420 },
    { QVideoFrameFormat::Format_YUV422P, GST_VIDEO_FORMAT_Y42B },
    { QVideoFrameFormat::Format_YV12   , GST_VIDEO_FORMAT_YV12 },
    { QVideoFrameFormat::Format_UYVY   , GST_VIDEO_FORMAT_UYVY },
    { QVideoFrameFormat::Format_YUYV   , GST_VIDEO_FORMAT_YUY2 },
    { QVideoFrameFormat::Format_NV12   , GST_VIDEO_FORMAT_NV12 },
    { QVideoFrameFormat::Format_NV21   , GST_VIDEO_FORMAT_NV21 },
    { QVideoFrameFormat::Format_AYUV   , GST_VIDEO_FORMAT_AYUV },
    { QVideoFrameFormat::Format_Y8     , GST_VIDEO_FORMAT_GRAY8 },
    { QVideoFrameFormat::Format_XRGB8888 ,  GST_VIDEO_FORMAT_xRGB },
    { QVideoFrameFormat::Format_XBGR8888 ,  GST_VIDEO_FORMAT_xBGR },
    { QVideoFrameFormat::Format_RGBX8888 ,  GST_VIDEO_FORMAT_RGBx },
    { QVideoFrameFormat::Format_BGRX8888 ,  GST_VIDEO_FORMAT_BGRx },
    { QVideoFrameFormat::Format_ARGB8888,  GST_VIDEO_FORMAT_ARGB },
    { QVideoFrameFormat::Format_ABGR8888,  GST_VIDEO_FORMAT_ABGR },
    { QVideoFrameFormat::Format_RGBA8888,  GST_VIDEO_FORMAT_RGBA },
    { QVideoFrameFormat::Format_BGRA8888,  GST_VIDEO_FORMAT_BGRA },
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    { QVideoFrameFormat::Format_Y16 , GST_VIDEO_FORMAT_GRAY16_LE },
    { QVideoFrameFormat::Format_P010 , GST_VIDEO_FORMAT_P010_10LE },
#else
    { QVideoFrameFormat::Format_Y16 , GST_VIDEO_FORMAT_GRAY16_BE },
    { QVideoFrameFormat::Format_P010 , GST_VIDEO_FORMAT_P010_10BE },
#endif
};

static int indexOfVideoFormat(QVideoFrameFormat::PixelFormat format)
{
    for (int i = 0; i < lengthOf(qt_videoFormatLookup); ++i)
        if (qt_videoFormatLookup[i].pixelFormat == format)
            return i;

    return -1;
}

static int indexOfVideoFormat(GstVideoFormat format)
{
    for (int i = 0; i < lengthOf(qt_videoFormatLookup); ++i)
        if (qt_videoFormatLookup[i].gstFormat == format)
            return i;

    return -1;
}

}

QVideoFrameFormat QGstCaps::formatForCaps(GstVideoInfo *info) const
{
    GstVideoInfo vidInfo;
    GstVideoInfo *infoPtr = info ? info : &vidInfo;

    if (gst_video_info_from_caps(infoPtr, caps)) {
        int index = indexOfVideoFormat(infoPtr->finfo->format);

        if (index != -1) {
            QVideoFrameFormat format(
                        QSize(infoPtr->width, infoPtr->height),
                        qt_videoFormatLookup[index].pixelFormat);

            if (infoPtr->fps_d > 0)
                format.setFrameRate(qreal(infoPtr->fps_n) / infoPtr->fps_d);

            QVideoFrameFormat::ColorRange range = QVideoFrameFormat::ColorRange_Unknown;
            switch (infoPtr->colorimetry.range) {
            case GST_VIDEO_COLOR_RANGE_UNKNOWN:
                break;
            case GST_VIDEO_COLOR_RANGE_0_255:
                range = QVideoFrameFormat::ColorRange_Full;
                break;
            case GST_VIDEO_COLOR_RANGE_16_235:
                range = QVideoFrameFormat::ColorRange_Video;
                break;
            }
            format.setColorRange(range);

            QVideoFrameFormat::ColorSpace colorSpace = QVideoFrameFormat::ColorSpace_Undefined;
            switch (infoPtr->colorimetry.matrix) {
            case GST_VIDEO_COLOR_MATRIX_UNKNOWN:
            case GST_VIDEO_COLOR_MATRIX_RGB:
            case GST_VIDEO_COLOR_MATRIX_FCC:
                break;
            case GST_VIDEO_COLOR_MATRIX_BT709:
                colorSpace = QVideoFrameFormat::ColorSpace_BT709;
                break;
            case GST_VIDEO_COLOR_MATRIX_BT601:
                colorSpace = QVideoFrameFormat::ColorSpace_BT601;
                break;
            case GST_VIDEO_COLOR_MATRIX_SMPTE240M:
                colorSpace = QVideoFrameFormat::ColorSpace_AdobeRgb;
                break;
            case GST_VIDEO_COLOR_MATRIX_BT2020:
                colorSpace = QVideoFrameFormat::ColorSpace_BT2020;
                break;
            }
            format.setColorSpace(colorSpace);

            QVideoFrameFormat::ColorTransfer transfer = QVideoFrameFormat::ColorTransfer_Unknown;
            switch (infoPtr->colorimetry.transfer) {
            case GST_VIDEO_TRANSFER_UNKNOWN:
                break;
            case GST_VIDEO_TRANSFER_GAMMA10:
                transfer = QVideoFrameFormat::ColorTransfer_Linear;
                break;
            case GST_VIDEO_TRANSFER_GAMMA22:
            case GST_VIDEO_TRANSFER_SMPTE240M:
            case GST_VIDEO_TRANSFER_SRGB:
            case GST_VIDEO_TRANSFER_ADOBERGB:
                transfer = QVideoFrameFormat::ColorTransfer_Gamma22;
                break;
            case GST_VIDEO_TRANSFER_GAMMA18:
            case GST_VIDEO_TRANSFER_GAMMA20:
                // not quite, but best fit
            case GST_VIDEO_TRANSFER_BT709:
            case GST_VIDEO_TRANSFER_BT2020_12:
                transfer = QVideoFrameFormat::ColorTransfer_BT709;
                break;
            case GST_VIDEO_TRANSFER_GAMMA28:
                transfer = QVideoFrameFormat::ColorTransfer_Gamma28;
                break;
            case GST_VIDEO_TRANSFER_LOG100:
            case GST_VIDEO_TRANSFER_LOG316:
                break;
#if GST_CHECK_VERSION(1, 18, 0)
            case GST_VIDEO_TRANSFER_SMPTE2084:
                transfer = QVideoFrameFormat::ColorTransfer_ST2084;
                break;
            case GST_VIDEO_TRANSFER_ARIB_STD_B67:
                transfer = QVideoFrameFormat::ColorTransfer_STD_B67;
                break;
            case GST_VIDEO_TRANSFER_BT2020_10:
                transfer = QVideoFrameFormat::ColorTransfer_BT709;
                break;
            case GST_VIDEO_TRANSFER_BT601:
                transfer = QVideoFrameFormat::ColorTransfer_BT601;
                break;
#endif
            }
            format.setColorTransfer(transfer);

            return format;
        }
    }
    return QVideoFrameFormat();
}

void QGstCaps::addPixelFormats(const QList<QVideoFrameFormat::PixelFormat> &formats, const char *modifier)
{
    if (!gst_caps_is_writable(caps))
        caps = gst_caps_make_writable(caps);

    GValue list = {};
    g_value_init(&list, GST_TYPE_LIST);

    for (QVideoFrameFormat::PixelFormat format : formats) {
        int index = indexOfVideoFormat(format);
        if (index == -1)
            continue;
        GValue item = {};

        g_value_init(&item, G_TYPE_STRING);
        g_value_set_string(&item, gst_video_format_to_string(qt_videoFormatLookup[index].gstFormat));
        gst_value_list_append_value(&list, &item);
        g_value_unset(&item);
    }

    auto *structure = gst_structure_new("video/x-raw",
                                        "framerate", GST_TYPE_FRACTION_RANGE, 0, 1, INT_MAX, 1,
                                        "width"    , GST_TYPE_INT_RANGE, 1, INT_MAX,
                                        "height"   , GST_TYPE_INT_RANGE, 1, INT_MAX,
                                        nullptr);
    gst_structure_set_value(structure, "format", &list);
    gst_caps_append_structure(caps, structure);
    g_value_unset(&list);

    if (modifier)
        gst_caps_set_features(caps, size() - 1, gst_caps_features_from_string(modifier));
}

QGstCaps QGstCaps::fromCameraFormat(const QCameraFormat &format)
{
    QSize size = format.resolution();
    GstStructure *structure = nullptr;
    if (format.pixelFormat() == QVideoFrameFormat::Format_Jpeg) {
        structure = gst_structure_new("image/jpeg",
                                      "width"    , G_TYPE_INT, size.width(),
                                      "height"   , G_TYPE_INT, size.height(),
                                      nullptr);
    } else {
        int index = indexOfVideoFormat(format.pixelFormat());
        if (index < 0)
            return {};
        auto gstFormat = qt_videoFormatLookup[index].gstFormat;
        structure = gst_structure_new("video/x-raw",
                                      "format"   , G_TYPE_STRING, gst_video_format_to_string(gstFormat),
                                      "width"    , G_TYPE_INT, size.width(),
                                      "height"   , G_TYPE_INT, size.height(),
                                      nullptr);
    }
    auto caps = QGstCaps::create();
    gst_caps_append_structure(caps.caps, structure);
    return caps;
}

void QGstUtils::setFrameTimeStamps(QVideoFrame *frame, GstBuffer *buffer)
{
    // GStreamer uses nanoseconds, Qt uses microseconds
    qint64 startTime = GST_BUFFER_TIMESTAMP(buffer);
    if (startTime >= 0) {
        frame->setStartTime(startTime/G_GINT64_CONSTANT (1000));

        qint64 duration = GST_BUFFER_DURATION(buffer);
        if (duration >= 0)
            frame->setEndTime((startTime + duration)/G_GINT64_CONSTANT (1000));
    }
}

QSize QGstStructure::resolution() const
{
    QSize size;

    int w, h;
    if (structure &&
        gst_structure_get_int(structure, "width", &w) &&
        gst_structure_get_int(structure, "height", &h)) {
        size.rwidth() = w;
        size.rheight() = h;
    }

    return size;
}

QVideoFrameFormat::PixelFormat QGstStructure::pixelFormat() const
{
    QVideoFrameFormat::PixelFormat pixelFormat = QVideoFrameFormat::Format_Invalid;

    if (!structure)
        return pixelFormat;

    if (gst_structure_has_name(structure, "video/x-raw")) {
        const gchar *s = gst_structure_get_string(structure, "format");
        if (s) {
            GstVideoFormat format = gst_video_format_from_string(s);
            int index = indexOfVideoFormat(format);

            if (index != -1)
                pixelFormat = qt_videoFormatLookup[index].pixelFormat;
        }
    } else if (gst_structure_has_name(structure, "image/jpeg")) {
        pixelFormat = QVideoFrameFormat::Format_Jpeg;
    }

    return pixelFormat;
}

QGRange<float> QGstStructure::frameRateRange() const
{
    float minRate = 0.;
    float maxRate = 0.;

    if (!structure)
        return {0.f, 0.f};

    auto extractFraction = [] (const GValue *v) -> float {
        return (float)gst_value_get_fraction_numerator(v)/(float)gst_value_get_fraction_denominator(v);
    };
    auto extractFrameRate = [&] (const GValue *v) {
        auto insert = [&] (float min, float max) {
            if (max > maxRate)
                maxRate = max;
            if (min < minRate)
                minRate = min;
        };

        if (GST_VALUE_HOLDS_FRACTION(v)) {
            float rate = extractFraction(v);
            insert(rate, rate);
        } else if (GST_VALUE_HOLDS_FRACTION_RANGE(v)) {
            auto *min = gst_value_get_fraction_range_max(v);
            auto *max = gst_value_get_fraction_range_max(v);
            insert(extractFraction(min), extractFraction(max));
        }
    };

    const GValue *gstFrameRates = gst_structure_get_value(structure, "framerate");
    if (gstFrameRates) {
        if (GST_VALUE_HOLDS_LIST(gstFrameRates)) {
            guint nFrameRates = gst_value_list_get_size(gstFrameRates);
            for (guint f = 0; f < nFrameRates; ++f) {
                extractFrameRate(gst_value_list_get_value(gstFrameRates, f));
            }
        } else {
            extractFrameRate(gstFrameRates);
        }
    } else {
        const GValue *min = gst_structure_get_value(structure, "min-framerate");
        const GValue *max = gst_structure_get_value(structure, "max-framerate");
        if (min && max) {
            minRate = extractFraction(min);
            maxRate = extractFraction(max);
        }
    }

    return {minRate, maxRate};
}

GList *qt_gst_video_sinks()
{
    return gst_element_factory_list_get_elements(GST_ELEMENT_FACTORY_TYPE_SINK
                                                         | GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO,
                                                 GST_RANK_MARGINAL);
}

QT_END_NAMESPACE
