/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
#include <QtMultimedia/qvideosurfaceformat.h>
#include <private/qmultimediautils_p.h>

#include <gst/audio/audio.h>
#include <gst/video/video.h>

template<typename T, int N> static int lengthOf(const T (&)[N]) { return N; }


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
  Returns audio format for a sample.
  If the buffer doesn't have a valid audio format, an empty QAudioFormat is returned.
*/
QAudioFormat QGstUtils::audioFormatForSample(GstSample *sample)
{
    GstCaps* caps = gst_sample_get_caps(sample);
    if (!caps)
        return QAudioFormat();

    QAudioFormat format;
    QGstStructure s = QGstCaps(caps).at(0);
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

/*!
  Builds GstCaps for an audio format.
  Returns 0 if the audio format is not valid.
  Caller must unref GstCaps.
*/

GstCaps *QGstUtils::capsForAudioFormat(const QAudioFormat &format)
{
    if (!format.isValid())
        return nullptr;

    auto sampleFormat = format.sampleFormat();
    return gst_caps_new_simple(
                "audio/x-raw",
                "format"  , G_TYPE_STRING, audioSampleFormatNames[sampleFormat],
                "rate"    , G_TYPE_INT   , format.sampleRate(),
                "channels", G_TYPE_INT   , format.channelCount(),
                "layout"  , G_TYPE_STRING, "interleaved",
                nullptr);
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

struct ColorFormat { QImage::Format imageFormat; GstVideoFormat gstFormat; };
static const ColorFormat qt_colorLookup[] =
{
    { QImage::Format_RGBX8888, GST_VIDEO_FORMAT_RGBx  },
    { QImage::Format_RGBA8888, GST_VIDEO_FORMAT_RGBA  },
    { QImage::Format_RGB888  , GST_VIDEO_FORMAT_RGB   },
    { QImage::Format_RGB16   , GST_VIDEO_FORMAT_RGB16 }
};

}

QImage QGstUtils::bufferToImage(GstBuffer *buffer, const GstVideoInfo &videoInfo)
{
    QImage img;

    GstVideoInfo info = videoInfo;
    GstVideoFrame frame;
    if (!gst_video_frame_map(&frame, &info, buffer, GST_MAP_READ))
        return img;

    if (videoInfo.finfo->format == GST_VIDEO_FORMAT_I420) {
        const int width = videoInfo.width;
        const int height = videoInfo.height;

        const int stride[] = { frame.info.stride[0], frame.info.stride[1], frame.info.stride[2] };
        const uchar *data[] = {
            static_cast<const uchar *>(frame.data[0]),
            static_cast<const uchar *>(frame.data[1]),
            static_cast<const uchar *>(frame.data[2])
        };
        img = QImage(width/2, height/2, QImage::Format_RGB32);

        for (int y=0; y<height; y+=2) {
            const uchar *yLine = data[0] + (y * stride[0]);
            const uchar *uLine = data[1] + (y * stride[1] / 2);
            const uchar *vLine = data[2] + (y * stride[2] / 2);

            for (int x=0; x<width; x+=2) {
                const qreal Y = 1.164*(yLine[x]-16);
                const int U = uLine[x/2]-128;
                const int V = vLine[x/2]-128;

                int b = qBound(0, int(Y + 2.018*U), 255);
                int g = qBound(0, int(Y - 0.813*V - 0.391*U), 255);
                int r = qBound(0, int(Y + 1.596*V), 255);

                img.setPixel(x/2,y/2,qRgb(r,g,b));
            }
        }
    } else for (int i = 0; i < lengthOf(qt_colorLookup); ++i) {
        if (qt_colorLookup[i].gstFormat != videoInfo.finfo->format)
            continue;

        const QImage image(
                    static_cast<const uchar *>(frame.data[0]),
                    videoInfo.width,
                    videoInfo.height,
                    frame.info.stride[0],
                    qt_colorLookup[i].imageFormat);
        img = image;
        img.detach();

        break;
    }

    gst_video_frame_unmap(&frame);

    return img;
}


namespace {

struct VideoFormat
{
    QVideoFrame::PixelFormat pixelFormat;
    GstVideoFormat gstFormat;
};

static const VideoFormat qt_videoFormatLookup[] =
{
    { QVideoFrame::Format_YUV420P, GST_VIDEO_FORMAT_I420 },
    { QVideoFrame::Format_YUV422P, GST_VIDEO_FORMAT_Y42B },
    { QVideoFrame::Format_YV12   , GST_VIDEO_FORMAT_YV12 },
    { QVideoFrame::Format_UYVY   , GST_VIDEO_FORMAT_UYVY },
    { QVideoFrame::Format_YUYV   , GST_VIDEO_FORMAT_YUY2 },
    { QVideoFrame::Format_NV12   , GST_VIDEO_FORMAT_NV12 },
    { QVideoFrame::Format_NV21   , GST_VIDEO_FORMAT_NV21 },
    { QVideoFrame::Format_AYUV444, GST_VIDEO_FORMAT_AYUV },
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    { QVideoFrame::Format_RGB32 ,  GST_VIDEO_FORMAT_BGRx },
    { QVideoFrame::Format_BGR32 ,  GST_VIDEO_FORMAT_RGBx },
    { QVideoFrame::Format_ARGB32,  GST_VIDEO_FORMAT_BGRA },
    { QVideoFrame::Format_ABGR32,  GST_VIDEO_FORMAT_RGBA },
    { QVideoFrame::Format_BGRA32,  GST_VIDEO_FORMAT_ARGB },
#else
    { QVideoFrame::Format_RGB32 ,  GST_VIDEO_FORMAT_xRGB },
    { QVideoFrame::Format_BGR32 ,  GST_VIDEO_FORMAT_xBGR },
    { QVideoFrame::Format_ARGB32,  GST_VIDEO_FORMAT_ARGB },
    { QVideoFrame::Format_ABGR32,  GST_VIDEO_FORMAT_ABGR },
    { QVideoFrame::Format_BGRA32,  GST_VIDEO_FORMAT_BGRA },
#endif
    { QVideoFrame::Format_RGB24 ,  GST_VIDEO_FORMAT_RGB },
    { QVideoFrame::Format_BGR24 ,  GST_VIDEO_FORMAT_BGR },
    { QVideoFrame::Format_RGB565,  GST_VIDEO_FORMAT_RGB16 }
};

static int indexOfVideoFormat(QVideoFrame::PixelFormat format)
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

QVideoSurfaceFormat QGstUtils::formatForCaps(
        GstCaps *caps, GstVideoInfo *info, QVideoFrame::HandleType handleType)
{
    GstVideoInfo vidInfo;
    GstVideoInfo *infoPtr = info ? info : &vidInfo;

    if (gst_video_info_from_caps(infoPtr, caps)) {
        int index = indexOfVideoFormat(infoPtr->finfo->format);

        if (index != -1) {
            QVideoSurfaceFormat format(
                        QSize(infoPtr->width, infoPtr->height),
                        qt_videoFormatLookup[index].pixelFormat,
                        handleType);

            if (infoPtr->fps_d > 0)
                format.setFrameRate(qreal(infoPtr->fps_n) / infoPtr->fps_d);

            return format;
        }
    }
    return QVideoSurfaceFormat();
}

GstCaps *QGstUtils::capsForFormats(const QList<QVideoFrame::PixelFormat> &formats)
{
    GstCaps *caps = gst_caps_new_empty();

    for (QVideoFrame::PixelFormat format : formats) {
        int index = indexOfVideoFormat(format);

        if (index != -1) {
            gst_caps_append_structure(caps, gst_structure_new(
                    "video/x-raw",
                    "format"   , G_TYPE_STRING, gst_video_format_to_string(qt_videoFormatLookup[index].gstFormat),
                    nullptr));
        }
    }

    gst_caps_set_simple(
                caps,
                "framerate", GST_TYPE_FRACTION_RANGE, 0, 1, INT_MAX, 1,
                "width"    , GST_TYPE_INT_RANGE, 1, INT_MAX,
                "height"   , GST_TYPE_INT_RANGE, 1, INT_MAX,
                nullptr);

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

QVideoFrame::PixelFormat QGstStructure::pixelFormat() const
{
    QVideoFrame::PixelFormat pixelFormat = QVideoFrame::Format_Invalid;

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

bool QGstUtils::useOpenGL()
{
    static bool result = qEnvironmentVariableIntValue("QT_GSTREAMER_USE_OPENGL_PLUGIN");
    return result;
}

GList *qt_gst_video_sinks()
{
    GList *list = nullptr;

    list = gst_element_factory_list_get_elements(GST_ELEMENT_FACTORY_TYPE_SINK | GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO,
                                                 GST_RANK_MARGINAL);

    return list;
}

QT_END_NAMESPACE
