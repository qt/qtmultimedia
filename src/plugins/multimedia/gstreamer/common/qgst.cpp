// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <common/qgst_p.h>

#include <common/qgst_debug_p.h>
#include <common/qgstpipeline_p.h>
#include <common/qgstreamermessage_p.h>
#include <common/qgstutils_p.h>
#include <common/qgstvideobuffer_p.h>

#include <QtCore/qdebug.h>
#include <QtMultimedia/private/qvideoframe_p.h>
#include <QtMultimedia/qcameradevice.h>

#if QT_CONFIG(gstreamer_gl)
#  include <gst/gl/gstglmemory.h>
#endif

#include <array>
#include <thread>

// DMA support
#if QT_CONFIG(gstreamer_gl_egl) && QT_CONFIG(linux_dmabuf)
#  include <gst/allocators/gstdmabuf.h>
#endif

QT_BEGIN_NAMESPACE

namespace {

struct VideoFormat
{
    QVideoFrameFormat::PixelFormat pixelFormat;
    GstVideoFormat gstFormat;
};

constexpr std::array<VideoFormat, 19> qt_videoFormatLookup{ {
        { QVideoFrameFormat::Format_YUV420P, GST_VIDEO_FORMAT_I420 },
        { QVideoFrameFormat::Format_YUV422P, GST_VIDEO_FORMAT_Y42B },
        { QVideoFrameFormat::Format_YV12, GST_VIDEO_FORMAT_YV12 },
        { QVideoFrameFormat::Format_UYVY, GST_VIDEO_FORMAT_UYVY },
        { QVideoFrameFormat::Format_YUYV, GST_VIDEO_FORMAT_YUY2 },
        { QVideoFrameFormat::Format_NV12, GST_VIDEO_FORMAT_NV12 },
        { QVideoFrameFormat::Format_NV21, GST_VIDEO_FORMAT_NV21 },
        { QVideoFrameFormat::Format_AYUV, GST_VIDEO_FORMAT_AYUV },
        { QVideoFrameFormat::Format_Y8, GST_VIDEO_FORMAT_GRAY8 },
        { QVideoFrameFormat::Format_XRGB8888, GST_VIDEO_FORMAT_xRGB },
        { QVideoFrameFormat::Format_XBGR8888, GST_VIDEO_FORMAT_xBGR },
        { QVideoFrameFormat::Format_RGBX8888, GST_VIDEO_FORMAT_RGBx },
        { QVideoFrameFormat::Format_BGRX8888, GST_VIDEO_FORMAT_BGRx },
        { QVideoFrameFormat::Format_ARGB8888, GST_VIDEO_FORMAT_ARGB },
        { QVideoFrameFormat::Format_ABGR8888, GST_VIDEO_FORMAT_ABGR },
        { QVideoFrameFormat::Format_RGBA8888, GST_VIDEO_FORMAT_RGBA },
        { QVideoFrameFormat::Format_BGRA8888, GST_VIDEO_FORMAT_BGRA },
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
        { QVideoFrameFormat::Format_Y16, GST_VIDEO_FORMAT_GRAY16_LE },
        { QVideoFrameFormat::Format_P010, GST_VIDEO_FORMAT_P010_10LE },
#else
        { QVideoFrameFormat::Format_Y16, GST_VIDEO_FORMAT_GRAY16_BE },
        { QVideoFrameFormat::Format_P010, GST_VIDEO_FORMAT_P010_10BE },
#endif
} };

#if QT_GSTREAMER_SUPPORTS_GST_VIDEO_FORMAT_DMA_DRM
void appendDmaDrmPixelFormats(QGstCaps &caps, const QList<QVideoFrameFormat::PixelFormat> &formats)
{
    GValue drmFormatList = {};
    g_value_init(&drmFormatList, GST_TYPE_LIST);

    for (QVideoFrameFormat::PixelFormat format : formats) {
        const GstVideoFormat gstFormat = qGstVideoFormatFromPixelFormat(format);
        if (gstFormat == GST_VIDEO_FORMAT_UNKNOWN)
            continue;

        const guint32 fourcc =
                gst_video_dma_drm_fourcc_from_format(gstFormat);
        if (!fourcc)
            continue;

        GValue drmFormat = {};
        g_value_init(&drmFormat, G_TYPE_STRING);
        g_value_take_string(&drmFormat, gst_video_dma_drm_fourcc_to_string(fourcc, 0));
        gst_value_list_append_value(&drmFormatList, &drmFormat);
        g_value_unset(&drmFormat);
    }

    QGstStructure dmaDrmStructure{ "video/x-raw" };
    dmaDrmStructure.setString("format", gst_video_format_to_string(GST_VIDEO_FORMAT_DMA_DRM));
    dmaDrmStructure.setFractionRange("framerate", Fraction{ 0, 1 }, Fraction{ INT_MAX, 1 });
    dmaDrmStructure.setIntRange("width", 1, INT_MAX);
    dmaDrmStructure.setIntRange("height", 1, INT_MAX);
    dmaDrmStructure.setValue("drm-format", &drmFormatList);

    gst_caps_append_structure(caps.get(), dmaDrmStructure.release());
    gst_caps_set_features(caps.get(), caps.size() - 1,
                          gst_caps_features_from_string(GST_CAPS_FEATURE_MEMORY_DMABUF));
    g_value_unset(&drmFormatList);
}
#endif

} // namespace

GstVideoFormat qGstVideoFormatFromPixelFormat(QVideoFrameFormat::PixelFormat format)
{
    for (const VideoFormat &formatPair : qt_videoFormatLookup)
        if (formatPair.pixelFormat == format)
            return formatPair.gstFormat;

    return GST_VIDEO_FORMAT_UNKNOWN;
}

QVideoFrameFormat::PixelFormat qPixelFormatFromGstVideoFormat(GstVideoFormat format)
{
    for (const VideoFormat &formatPair : qt_videoFormatLookup)
        if (formatPair.gstFormat == format)
            return formatPair.pixelFormat;

    return QVideoFrameFormat::Format_Invalid;
}

// QGValue

QGValue::QGValue(const GValue *v) : value(v) { }

bool QGValue::isNull() const
{
    return !value;
}

std::optional<bool> QGValue::toBool() const
{
    if (!G_VALUE_HOLDS_BOOLEAN(value))
        return std::nullopt;
    return g_value_get_boolean(value);
}

std::optional<int> QGValue::toInt() const
{
    if (!G_VALUE_HOLDS_INT(value))
        return std::nullopt;
    return g_value_get_int(value);
}

std::optional<int> QGValue::toInt64() const
{
    if (!G_VALUE_HOLDS_INT64(value))
        return std::nullopt;
    return g_value_get_int64(value);
}

const char *QGValue::toString() const
{
    return value ? g_value_get_string(value) : nullptr;
}

std::optional<float> QGValue::getFraction() const
{
    if (!GST_VALUE_HOLDS_FRACTION(value))
        return std::nullopt;
    return (float)gst_value_get_fraction_numerator(value)
            / (float)gst_value_get_fraction_denominator(value);
}

std::optional<QGRange<float>> QGValue::getFractionRange() const
{
    if (!GST_VALUE_HOLDS_FRACTION_RANGE(value))
        return std::nullopt;
    QGValue min = QGValue{ gst_value_get_fraction_range_min(value) };
    QGValue max = QGValue{ gst_value_get_fraction_range_max(value) };
    return QGRange<float>{ *min.getFraction(), *max.getFraction() };
}

std::optional<QGRange<int>> QGValue::toIntRange() const
{
    if (!GST_VALUE_HOLDS_INT_RANGE(value))
        return std::nullopt;
    return QGRange<int>{ gst_value_get_int_range_min(value), gst_value_get_int_range_max(value) };
}

QGstStructureView QGValue::toStructure() const
{
    if (!value || !GST_VALUE_HOLDS_STRUCTURE(value))
        return QGstStructureView(nullptr);
    return QGstStructureView(gst_value_get_structure(value));
}

QGstCaps QGValue::toCaps() const
{
    if (!value || !GST_VALUE_HOLDS_CAPS(value))
        return {};
    return QGstCaps(gst_caps_copy(gst_value_get_caps(value)), QGstCaps::HasRef);
}

bool QGValue::isList() const
{
    return value && GST_VALUE_HOLDS_LIST(value);
}

int QGValue::listSize() const
{
    return gst_value_list_get_size(value);
}

QGValue QGValue::at(int index) const
{
    return QGValue{ gst_value_list_get_value(value, index) };
}

// QGstStructureView

QGstStructureView::QGstStructureView(const GstStructure *s) : structure(s) { }

QGstStructureView::QGstStructureView(const QUniqueGstStructureHandle &handle)
    : QGstStructureView{ handle.get() }
{
}

QUniqueGstStructureHandle QGstStructureView::clone() const
{
    return QUniqueGstStructureHandle{ gst_structure_copy(structure) };
}

bool QGstStructureView::isNull() const
{
    return !structure;
}

QByteArrayView QGstStructureView::name() const
{
    return gst_structure_get_name(structure);
}

QGValue QGstStructureView::operator[](const char *fieldname) const
{
    return QGValue{ gst_structure_get_value(structure, fieldname) };
}

QGstCaps QGstStructureView::caps() const
{
    return operator[]("caps").toCaps();
}

QGstTagListHandle QGstStructureView::tags() const
{
    QGValue tags = operator[]("tags");
    if (tags.isNull())
        return {};

    QGstTagListHandle tagList;
    gst_structure_get(structure, "tags", GST_TYPE_TAG_LIST, &tagList, nullptr);
    return tagList;
}

QSize QGstStructureView::resolution() const
{
    QSize size;

    int w, h;
    if (structure && gst_structure_get_int(structure, "width", &w)
        && gst_structure_get_int(structure, "height", &h)) {
        size.rwidth() = w;
        size.rheight() = h;
    }

    return size;
}

QList<QVideoFrameFormat::PixelFormat> QGstStructureView::pixelFormats() const
{
    QList<QVideoFrameFormat::PixelFormat> pixelFormats;

    if (!structure)
        return pixelFormats;

    auto appendFromGstVideoFormat = [&](GstVideoFormat format) {
        const QVideoFrameFormat::PixelFormat pixelFormat = qPixelFormatFromGstVideoFormat(format);
        if (pixelFormat == QVideoFrameFormat::Format_Invalid)
            return;

        if (!pixelFormats.contains(pixelFormat))
            pixelFormats.append(pixelFormat);
    };

    if (gst_structure_has_name(structure, "video/x-raw")) {
#if QT_GSTREAMER_SUPPORTS_GST_VIDEO_FORMAT_DMA_DRM
        if (const GValue *drmFormatValue = gst_structure_get_value(structure, "drm-format")) {
            auto parseDrmFormat = [&](const GValue *value) {
                if (!value || !G_VALUE_HOLDS_STRING(value))
                    return;

                const char *drmFormatString = g_value_get_string(value);
                if (!drmFormatString)
                    return;

                guint64 modifier = 0;
                const guint32 fourcc = gst_video_dma_drm_fourcc_from_string(drmFormatString,
                                                                            &modifier);
                if (fourcc) {
                    if (modifier != 0)
                        qWarning() << "Ignoring drm-format modifier when mapping to Qt PixelFormat:"
                                   << drmFormatString;
                    appendFromGstVideoFormat(gst_video_dma_drm_fourcc_to_format(fourcc));
                }
            };

            if (GST_VALUE_HOLDS_LIST(drmFormatValue)) {
                const guint listSize = gst_value_list_get_size(drmFormatValue);
                for (guint i = 0; i < listSize; ++i)
                    parseDrmFormat(gst_value_list_get_value(drmFormatValue, i));
            } else {
                parseDrmFormat(drmFormatValue);
            }

            if (!pixelFormats.isEmpty())
                return pixelFormats; // Skip checking "format" field when "drm-format" succeeds
        }
#endif
        if (const GValue *formatValue = gst_structure_get_value(structure, "format")) {
            auto parseFormat = [&](const GValue *value) {
                if (!value || !G_VALUE_HOLDS_STRING(value))
                    return;
                const char *formatString = g_value_get_string(value);
                if (!formatString)
                    return;
                appendFromGstVideoFormat(gst_video_format_from_string(formatString));
            };

            if (GST_VALUE_HOLDS_LIST(formatValue)) {
                const guint listSize = gst_value_list_get_size(formatValue);
                for (guint i = 0; i < listSize; ++i)
                    parseFormat(gst_value_list_get_value(formatValue, i));
            } else {
                parseFormat(formatValue);
            }
        }
    } else if (gst_structure_has_name(structure, "image/jpeg")) {
        pixelFormats.append(QVideoFrameFormat::Format_Jpeg);
    }

    return pixelFormats;
}

QGRange<float> QGstStructureView::frameRateRange() const
{
    if (!structure)
        return { 0.f, 0.f };

    std::optional<float> minRate;
    std::optional<float> maxRate;

    auto extractFraction = [](const GValue *v) -> float {
        return (float)gst_value_get_fraction_numerator(v)
                / (float)gst_value_get_fraction_denominator(v);
    };
    auto extractFrameRate = [&](const GValue *v) {
        auto insert = [&](float min, float max) {
            if (!maxRate || max > maxRate)
                maxRate = max;
            if (!minRate || min < minRate)
                minRate = min;
        };

        if (GST_VALUE_HOLDS_FRACTION(v)) {
            float rate = extractFraction(v);
            insert(rate, rate);
        } else if (GST_VALUE_HOLDS_FRACTION_RANGE(v)) {
            const GValue *min = gst_value_get_fraction_range_min(v);
            const GValue *max = gst_value_get_fraction_range_max(v);
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

    if (!minRate || !maxRate)
        return { 0.f, 0.f };

    return {
        minRate.value_or(*maxRate),
        maxRate.value_or(*minRate),
    };
}

std::optional<QGRange<QSize>> QGstStructureView::resolutionRange() const
{
    if (!structure)
        return std::nullopt;

    const GValue *width = gst_structure_get_value(structure, "width");
    const GValue *height = gst_structure_get_value(structure, "height");

    if (!width || !height)
        return std::nullopt;

    for (const GValue *v : { width, height })
        if (!GST_VALUE_HOLDS_INT_RANGE(v))
            return std::nullopt;

    int minWidth = gst_value_get_int_range_min(width);
    int maxWidth = gst_value_get_int_range_max(width);
    int minHeight = gst_value_get_int_range_min(height);
    int maxHeight = gst_value_get_int_range_max(height);

    return QGRange<QSize>{
        QSize(minWidth, minHeight),
        QSize(maxWidth, maxHeight),
    };
}

QGstreamerMessage QGstStructureView::getMessage()
{
    GstMessage *message = nullptr;
    gst_structure_get(structure, "message", GST_TYPE_MESSAGE, &message, nullptr);
    return QGstreamerMessage(message, QGstreamerMessage::HasRef);
}

std::optional<Fraction> QGstStructureView::pixelAspectRatio() const
{
    gint numerator;
    gint denominator;
    if (gst_structure_get_fraction(structure, "pixel-aspect-ratio", &numerator, &denominator)) {
        return Fraction{
            numerator,
            denominator,
        };
    }

    return std::nullopt;
}

QSize QGstStructureView::nativeSize() const
{
    QSize size = resolution();
    if (!size.isValid()) {
        qWarning() << Q_FUNC_INFO << "invalid resolution when querying nativeSize";
        return size;
    }

    std::optional<Fraction> par = pixelAspectRatio();
    if (par)
        size = QGstUtils::qCalculateFrameSizeGStreamer(size, *par);
    return size;
}

// QGstCaps

std::optional<QGstVideoInfo> QGstCaps::videoInfo() const
{
    GstVideoInfo vidInfo;
    std::optional<guint64> dmaDrmModifier;

#if QT_GSTREAMER_SUPPORTS_GST_VIDEO_FORMAT_DMA_DRM
    GstVideoInfoDmaDrm videoInfoDmaDrm;
    if (gst_video_info_dma_drm_from_caps(&videoInfoDmaDrm, get())) {
        dmaDrmModifier = videoInfoDmaDrm.drm_modifier;
        // Sets vidInfo‘s format based on drm_fourcc from videoInfoDmaDrm:
        if (!gst_video_info_dma_drm_to_video_info(&videoInfoDmaDrm, &vidInfo))
            return std::nullopt;
    } else
#endif
    {
        if (!gst_video_info_from_caps(&vidInfo, get()))
            return std::nullopt;
    }

    return QGstVideoInfo{vidInfo, dmaDrmModifier};
}

void QGstCaps::addPixelFormats(const QList<QVideoFrameFormat::PixelFormat> &formats,
                               const char *capsFeatures)
{
    if (!gst_caps_is_writable(get()))
        *this = QGstCaps(gst_caps_make_writable(release()), QGstCaps::RefMode::HasRef);

#if QT_GSTREAMER_SUPPORTS_GST_VIDEO_FORMAT_DMA_DRM
    if (qstrcmp(capsFeatures, GST_CAPS_FEATURE_MEMORY_DMABUF) == 0)
        appendDmaDrmPixelFormats(*this, formats);
#endif

    GValue list = {};
    g_value_init(&list, GST_TYPE_LIST);

    for (QVideoFrameFormat::PixelFormat format : formats) {
        const GstVideoFormat gstFormat = qGstVideoFormatFromPixelFormat(format);
        if (gstFormat == GST_VIDEO_FORMAT_UNKNOWN)
            continue;
        GValue item = {};

        g_value_init(&item, G_TYPE_STRING);
        g_value_set_string(&item, gst_video_format_to_string(gstFormat));
        gst_value_list_append_value(&list, &item);
        g_value_unset(&item);
    }

    QGstStructure structure{ "video/x-raw" };
    structure.setFractionRange("framerate", Fraction{ 0, 1 }, Fraction{ INT_MAX, 1 });
    structure.setIntRange("width", 1, INT_MAX);
    structure.setIntRange("height", 1, INT_MAX);
    structure.setValue("format", &list);

    gst_caps_append_structure(get(), structure.release());
    g_value_unset(&list);

    if (capsFeatures)
        gst_caps_set_features(get(), size() - 1,
                              gst_caps_features_from_string(capsFeatures));
}

void QGstCaps::setResolution(QSize resolution)
{
    Q_ASSERT(resolution.isValid());
    GValue width{};
    g_value_init(&width, G_TYPE_INT);
    g_value_set_int(&width, resolution.width());
    GValue height{};
    g_value_init(&height, G_TYPE_INT);
    g_value_set_int(&height, resolution.height());

    gst_caps_set_value(caps(), "width", &width);
    gst_caps_set_value(caps(), "height", &height);
}

QGstCaps QGstCaps::fromCameraFormat(const QCameraFormat &format)
{
    QSize size = format.resolution();
    auto caps = QGstCaps::create();

    if (format.pixelFormat() == QVideoFrameFormat::Format_Jpeg) {
        QGstStructure jpegStructure("image/jpeg");
        jpegStructure.setInt("width", size.width());
        jpegStructure.setInt("height", size.height());

        gst_caps_append_structure(caps.get(), jpegStructure.release());
        return caps;
    }

    const GstVideoFormat gstFormat = qGstVideoFormatFromPixelFormat(format.pixelFormat());
    if (gstFormat == GST_VIDEO_FORMAT_UNKNOWN)
        return {};

    QGstStructure rawStructure("video/x-raw");
    rawStructure.setString("format", gst_video_format_to_string(gstFormat));
    rawStructure.setInt("width", size.width());
    rawStructure.setInt("height", size.height());

    gst_caps_append_structure(caps.get(), rawStructure.release());

#if QT_GSTREAMER_SUPPORTS_GST_VIDEO_FORMAT_DMA_DRM
    if (const guint32 fourcc = gst_video_dma_drm_fourcc_from_format(gstFormat)) {
        if (QGString drmFormat{gst_video_dma_drm_fourcc_to_string(fourcc, 0)}) {
            QGstStructure drmFormatDmabufRawStructure("video/x-raw");
            drmFormatDmabufRawStructure.setString(
                    "format", gst_video_format_to_string(GST_VIDEO_FORMAT_DMA_DRM));
            drmFormatDmabufRawStructure.setString("drm-format", drmFormat.get());
            drmFormatDmabufRawStructure.setInt("width", size.width());
            drmFormatDmabufRawStructure.setInt("height", size.height());

            gst_caps_append_structure(caps.get(), drmFormatDmabufRawStructure.release());
            gst_caps_set_features(
                    caps.get(), caps.size() - 1,
                    gst_caps_features_from_string(GST_CAPS_FEATURE_MEMORY_DMABUF));
        }
    }
#endif

#if QT_CONFIG(gstreamer_gl_egl) && QT_CONFIG(linux_dmabuf)
    QGstStructure dmabufRawStructure("video/x-raw");
    dmabufRawStructure.setString("format", gst_video_format_to_string(gstFormat));
    dmabufRawStructure.setInt("width", size.width());
    dmabufRawStructure.setInt("height", size.height());

    gst_caps_append_structure(caps.get(), dmabufRawStructure.release());
    gst_caps_set_features(caps.get(), caps.size() - 1,
                          gst_caps_features_from_string(GST_CAPS_FEATURE_MEMORY_DMABUF));
#endif

    return caps;
}

QGstCaps QGstCaps::copy() const
{
    return QGstCaps{
        gst_caps_copy(caps()),
        QGstCaps::HasRef,
    };
}

QGstCaps::MemoryFormat QGstCaps::memoryFormat() const
{
    auto *features = gst_caps_get_features(get(), 0);
    if (gst_caps_features_contains(features, "memory:GLMemory"))
        return GLTexture;
    if (gst_caps_features_contains(features, "memory:DMABuf"))
        return DMABuf;
    return CpuMemory;
}

int QGstCaps::size() const
{
    return int(gst_caps_get_size(get()));
}

QGstStructureView QGstCaps::at(int index) const
{
    return QGstStructureView{
        gst_caps_get_structure(get(), index),
    };
}

GstCaps *QGstCaps::caps() const
{
    return get();
}

QGstCaps QGstCaps::create()
{
    return QGstCaps(gst_caps_new_empty(), HasRef);
}

// QGstObject

void QGstObject::set(const char *property, const char *str)
{
    g_object_set(get(), property, str, nullptr);
}

void QGstObject::set(const char *property, bool b)
{
    g_object_set(get(), property, gboolean(b), nullptr);
}

void QGstObject::set(const char *property, uint32_t i)
{
    g_object_set(get(), property, guint(i), nullptr);
}

void QGstObject::set(const char *property, int32_t i)
{
    g_object_set(get(), property, gint(i), nullptr);
}

void QGstObject::set(const char *property, int64_t i)
{
    g_object_set(get(), property, gint64(i), nullptr);
}

void QGstObject::set(const char *property, uint64_t i)
{
    g_object_set(get(), property, guint64(i), nullptr);
}

void QGstObject::set(const char *property, double d)
{
    g_object_set(get(), property, gdouble(d), nullptr);
}

void QGstObject::set(const char *property, const QGstObject &o)
{
    g_object_set(get(), property, o.object(), nullptr);
}

void QGstObject::set(const char *property, const QGstCaps &c)
{
    g_object_set(get(), property, c.caps(), nullptr);
}

void QGstObject::set(const char *property, void *object, GDestroyNotify destroyFunction)
{
    g_object_set_data_full(qGstCheckedCast<GObject>(get()), property, object, destroyFunction);
}

QGString QGstObject::getString(const char *property) const
{
    char *s = nullptr;
    g_object_get(get(), property, &s, nullptr);
    return QGString(s);
}

QGstStructureView QGstObject::getStructure(const char *property) const
{
    GstStructure *s = nullptr;
    g_object_get(get(), property, &s, nullptr);
    return QGstStructureView(s);
}

bool QGstObject::getBool(const char *property) const
{
    gboolean b = false;
    g_object_get(get(), property, &b, nullptr);
    return b;
}

uint QGstObject::getUInt(const char *property) const
{
    guint i = 0;
    g_object_get(get(), property, &i, nullptr);
    return i;
}

int QGstObject::getInt(const char *property) const
{
    gint i = 0;
    g_object_get(get(), property, &i, nullptr);
    return i;
}

quint64 QGstObject::getUInt64(const char *property) const
{
    guint64 i = 0;
    g_object_get(get(), property, &i, nullptr);
    return i;
}

qint64 QGstObject::getInt64(const char *property) const
{
    gint64 i = 0;
    g_object_get(get(), property, &i, nullptr);
    return i;
}

float QGstObject::getFloat(const char *property) const
{
    gfloat d = 0;
    g_object_get(get(), property, &d, nullptr);
    return d;
}

double QGstObject::getDouble(const char *property) const
{
    gdouble d = 0;
    g_object_get(get(), property, &d, nullptr);
    return d;
}

QGstObject QGstObject::getGstObject(const char *property) const
{
    GstObject *o = nullptr;
    g_object_get(get(), property, &o, nullptr);
    return QGstObject(o, HasRef);
}

void *QGstObject::getObject(const char *property) const
{
    return g_object_get_data(qGstCheckedCast<GObject>(get()), property);
}

QGObjectHandlerConnection QGstObject::connect(const char *name, GCallback callback,
                                              gpointer userData)
{
    return QGObjectHandlerConnection{
        *this,
        g_signal_connect(get(), name, callback, userData),
    };
}

void QGstObject::disconnect(gulong handlerId)
{
    g_signal_handler_disconnect(get(), handlerId);
}

GType QGstObject::type() const
{
    return G_OBJECT_TYPE(get());
}

QLatin1StringView QGstObject::typeName() const
{
    return QLatin1StringView{
        g_type_name(type()),
    };
}

GstObject *QGstObject::object() const
{
    return get();
}

QLatin1StringView QGstObject::name() const
{
    using namespace Qt::StringLiterals;

    return get() ? QLatin1StringView{ GST_OBJECT_NAME(get()) } : "(null)"_L1;
}

// QGObjectHandlerConnection

QGObjectHandlerConnection::QGObjectHandlerConnection(QGstObject object, gulong handlerId)
    : object{ std::move(object) }, handlerId{ handlerId }
{
}

void QGObjectHandlerConnection::disconnect()
{
    if (!object)
        return;

    object.disconnect(handlerId);
    object = {};
    handlerId = invalidHandlerId;
}

// QGObjectHandlerScopedConnection

QGObjectHandlerScopedConnection::QGObjectHandlerScopedConnection(
        QGObjectHandlerConnection connection)
    : connection{
          std::move(connection),
      }
{
}

QGObjectHandlerScopedConnection::~QGObjectHandlerScopedConnection()
{
    connection.disconnect();
}

void QGObjectHandlerScopedConnection::disconnect()
{
    connection.disconnect();
}

// QGstPad

QGstPad::QGstPad(const QGstObject &o)
    : QGstPad{
          qGstSafeCast<GstPad>(o.object()),
          QGstElement::NeedsRef,
      }
{
}

QGstPad::QGstPad(GstPad *pad, RefMode mode)
    : QGstObject{
          qGstCheckedCast<GstObject>(pad),
          mode,
      }
{
}

QGstCaps QGstPad::currentCaps() const
{
    return QGstCaps(gst_pad_get_current_caps(pad()), QGstCaps::HasRef);
}

QGstCaps QGstPad::queryCaps() const
{
    return QGstCaps(gst_pad_query_caps(pad(), nullptr), QGstCaps::HasRef);
}

QGstTagListHandle QGstPad::tags() const
{
    QGstTagListHandle tagList;
    g_object_get(object(), "tags", &tagList, nullptr);
    return tagList;
}

QGString QGstPad::streamId() const
{
    return QGString{
        gst_pad_get_stream_id(pad()),
    };
}

std::optional<QPlatformMediaPlayer::TrackType> QGstPad::inferTrackTypeFromName() const
{
    using namespace Qt::Literals;
    QLatin1StringView padName = name();

    if (padName.startsWith("video_"_L1))
        return QPlatformMediaPlayer::TrackType::VideoStream;
    if (padName.startsWith("audio_"_L1))
        return QPlatformMediaPlayer::TrackType::AudioStream;
    if (padName.startsWith("text_"_L1))
        return QPlatformMediaPlayer::TrackType::SubtitleStream;

    return std::nullopt;
}

bool QGstPad::isLinked() const
{
    return gst_pad_is_linked(pad());
}

bool QGstPad::link(const QGstPad &sink) const
{
    return gst_pad_link(pad(), sink.pad()) == GST_PAD_LINK_OK;
}

bool QGstPad::unlink(const QGstPad &sink) const
{
    return gst_pad_unlink(pad(), sink.pad());
}

bool QGstPad::unlinkPeer() const
{
    QGstPad peerPad = peer();
    if (peerPad)
        return GST_PAD_IS_SRC(pad()) ? unlink(peerPad) : peerPad.unlink(*this);

    return true;
}

QGstPad QGstPad::peer() const
{
    return QGstPad(gst_pad_get_peer(pad()), HasRef);
}

QGstElement QGstPad::parent() const
{
    return QGstElement(gst_pad_get_parent_element(pad()), HasRef);
}

GstPad *QGstPad::pad() const
{
    return qGstCheckedCast<GstPad>(object());
}

GstEvent *QGstPad::stickyEvent(GstEventType type)
{
    return gst_pad_get_sticky_event(pad(), type, 0);
}

bool QGstPad::sendEvent(GstEvent *event)
{
    return gst_pad_send_event(pad(), event);
}

void QGstPad::sendFlushStartStop(bool resetTime)
{
    GstEvent *flushStart = gst_event_new_flush_start();
    gboolean ret = sendEvent(flushStart);
    if (!ret) {
        qWarning("failed to send flush-start event");
        return;
    }

    GstEvent *flushStop = gst_event_new_flush_stop(resetTime);
    ret = sendEvent(flushStop);
    if (!ret)
        qWarning("failed to send flush-stop event");
}

void QGstPad::sendFlushIfPaused()
{
    using namespace std::chrono_literals;

    GstState state = parent().state(1s);

    if (state != GST_STATE_PAUSED)
        return;

    sendFlushStartStop(/*resetTime=*/true);
}

// QGstClock

QGstClock::QGstClock(const QGstObject &o)
    : QGstClock{
          qGstSafeCast<GstClock>(o.object()),
          QGstElement::NeedsRef,
      }
{
}

QGstClock::QGstClock(GstClock *clock, RefMode mode)
    : QGstObject{
          qGstCheckedCast<GstObject>(clock),
          mode,
      }
{
}

GstClock *QGstClock::clock() const
{
    return qGstCheckedCast<GstClock>(object());
}

GstClockTime QGstClock::time() const
{
    return gst_clock_get_time(clock());
}

// QGstElement

QGstElement::QGstElement(GstElement *element, RefMode mode)
    : QGstObject{
          qGstCheckedCast<GstObject>(element),
          mode,
      }
{
}

QGstElement QGstElement::createFromFactory(const char *factory, const char *name)
{
    GstElement *element = gst_element_factory_make(factory, name);

#ifndef QT_NO_DEBUG
    if (!element) {
        qWarning() << "Failed to make element" << name << "from factory" << factory;
        return QGstElement{};
    }
#endif

    return QGstElement{
        element,
        NeedsRef,
    };
}

QGstElement QGstElement::createFromFactory(GstElementFactory *factory, const char *name)
{
    return QGstElement{
        gst_element_factory_create(factory, name),
        NeedsRef,
    };
}

QGstElement QGstElement::createFromFactory(const QGstElementFactoryHandle &factory,
                                           const char *name)
{
    return createFromFactory(factory.get(), name);
}

QGstElement QGstElement::createFromDevice(const QGstDeviceHandle &device, const char *name)
{
    return createFromDevice(device.get(), name);
}

QGstElement QGstElement::createFromDevice(GstDevice *device, const char *name)
{
    return QGstElement{
        gst_device_create_element(device, name),
        QGstElement::NeedsRef,
    };
}

QGstElement QGstElement::createFromPipelineDescription(const char *str)
{
    QUniqueGErrorHandle error;
    QGstElement element{
        gst_parse_launch(str, &error),
        QGstElement::NeedsRef,
    };

    if (error) // error does not mean that the element could not be constructed
        qWarning() << "gst_parse_launch error:" << error;

    return element;
}

QGstElement QGstElement::createFromPipelineDescription(const QByteArray &str)
{
    return createFromPipelineDescription(str.constData());
}

QGstElementFactoryHandle QGstElement::findFactory(const char *name)
{
    return QGstElementFactoryHandle{
        gst_element_factory_find(name),
        QGstElementFactoryHandle::HasRef,
    };
}

QGstElementFactoryHandle QGstElement::findFactory(const QByteArray &name)
{
    return findFactory(name.constData());
}

QByteArrayView QGstElement::factoryName() const
{
    GstElementFactory *factory = gst_element_get_factory(element());
    if (!factory)
        return {};

    const char *name = gst_plugin_feature_get_name(GST_PLUGIN_FEATURE(factory));
    return name ? QByteArrayView{name} : QByteArrayView{};
}

QGstPad QGstElement::staticPad(const char *name) const
{
    return QGstPad(gst_element_get_static_pad(element(), name), HasRef);
}

QGstPad QGstElement::src() const
{
    return staticPad("src");
}

QGstPad QGstElement::sink() const
{
    return staticPad("sink");
}

QGstPad QGstElement::getRequestPad(const char *name) const
{
    return QGstPad(gst_element_request_pad_simple(element(), name), HasRef);
}

void QGstElement::releaseRequestPad(const QGstPad &pad) const
{
    return gst_element_release_request_pad(element(), pad.pad());
}

GstState QGstElement::state(std::chrono::nanoseconds timeout) const
{
    using namespace std::chrono_literals;

    GstState state;
    GstStateChangeReturn change =
            gst_element_get_state(element(), &state, nullptr, timeout.count());

    if (Q_UNLIKELY(change == GST_STATE_CHANGE_ASYNC))
        qWarning() << "QGstElement::state detected an asynchronous state change. Return value not "
                      "reliable";

    return state;
}

GstStateChangeReturn QGstElement::setState(GstState state)
{
    return gst_element_set_state(element(), state);
}

bool QGstElement::setStateSync(GstState state, std::chrono::nanoseconds timeout)
{
    GstStateChangeReturn change = gst_element_set_state(element(), state);
    if (change == GST_STATE_CHANGE_ASYNC)
        change = gst_element_get_state(element(), nullptr, &state, timeout.count());

    if (change != GST_STATE_CHANGE_SUCCESS && change != GST_STATE_CHANGE_NO_PREROLL) {
        qWarning() << "Could not change state of" << name() << "to" << state << change;
        dumpPipelineGraph("setStateSyncFailure");
    }
    return change == GST_STATE_CHANGE_SUCCESS || change == GST_STATE_CHANGE_NO_PREROLL;
}

bool QGstElement::syncStateWithParent()
{
    Q_ASSERT(element());
    return gst_element_sync_state_with_parent(element()) == TRUE;
}

bool QGstElement::finishStateChange(std::chrono::nanoseconds timeout)
{
    GstState state, pending;
    GstStateChangeReturn change =
            gst_element_get_state(element(), &state, &pending, timeout.count());

    if (change != GST_STATE_CHANGE_SUCCESS && change != GST_STATE_CHANGE_NO_PREROLL) {
        qWarning() << "Could not finish change state of" << name() << change << state << pending;
        dumpPipelineGraph("finishStateChangeFailure");
    }
    return change == GST_STATE_CHANGE_SUCCESS;
}

bool QGstElement::hasAsyncStateChange(std::chrono::nanoseconds timeout) const
{
    GstState state;
    GstStateChangeReturn change =
            gst_element_get_state(element(), &state, nullptr, timeout.count());
    return change == GST_STATE_CHANGE_ASYNC;
}

bool QGstElement::waitForAsyncStateChangeComplete(std::chrono::nanoseconds timeout) const
{
    using namespace std::chrono_literals;
    for (;;) {
        if (!hasAsyncStateChange())
            return true;
        timeout -= 10ms;
        if (timeout < 0ms)
            return false;
        std::this_thread::sleep_for(10ms);
    }
}

void QGstElement::lockState(bool locked)
{
    gst_element_set_locked_state(element(), locked);
}

bool QGstElement::isStateLocked() const
{
    return gst_element_is_locked_state(element());
}

void QGstElement::sendEvent(GstEvent *event) const
{
    gst_element_send_event(element(), event);
}

void QGstElement::sendEos() const
{
    sendEvent(gst_event_new_eos());
}

std::optional<std::chrono::nanoseconds> QGstElement::duration() const
{
    gint64 d;
    if (!gst_element_query_duration(element(), GST_FORMAT_TIME, &d)) {
        qDebug() << "QGstElement: failed to query duration";
        return std::nullopt;
    }
    return std::chrono::nanoseconds{ d };
}

std::optional<std::chrono::milliseconds> QGstElement::durationInMs() const
{
    using namespace std::chrono;
    auto dur = duration();
    if (dur)
        return round<milliseconds>(*dur);
    return std::nullopt;
}

std::optional<std::chrono::nanoseconds> QGstElement::position() const
{
    QGstQueryHandle &query = positionQuery();

    gint64 pos;
    if (gst_element_query(element(), query.get())) {
        gst_query_parse_position(query.get(), nullptr, &pos);
        return std::chrono::nanoseconds{ pos };
    }

    qDebug() << "QGstElement: failed to query position";
    return std::nullopt;
}

std::optional<std::chrono::milliseconds> QGstElement::positionInMs() const
{
    using namespace std::chrono;
    auto pos = position();
    if (pos)
        return round<milliseconds>(*pos);
    return std::nullopt;
}

std::optional<bool> QGstElement::canSeek() const
{
    QGstQueryHandle query{
        gst_query_new_seeking(GST_FORMAT_TIME),
        QGstQueryHandle::HasRef,
    };
    gboolean canSeek = false;
    gst_query_parse_seeking(query.get(), nullptr, &canSeek, nullptr, nullptr);

    if (gst_element_query(element(), query.get())) {
        gst_query_parse_seeking(query.get(), nullptr, &canSeek, nullptr, nullptr);
        return canSeek;
    }
    return std::nullopt;
}

GstClockTime QGstElement::baseTime() const
{
    return gst_element_get_base_time(element());
}

void QGstElement::setBaseTime(GstClockTime time) const
{
    gst_element_set_base_time(element(), time);
}

GstElement *QGstElement::element() const
{
    return GST_ELEMENT_CAST(get());
}

QGstElement QGstElement::getParent() const
{
    return QGstElement{
        qGstCheckedCast<GstElement>(gst_element_get_parent(object())),
        QGstElement::HasRef,
    };
}

QGstBin QGstElement::getParentBin() const
{
    return QGstBin{
        qGstCheckedCast<GstBin>(gst_element_get_parent(object())),
        QGstElement::HasRef,
    };
}

QGstBin QGstElement::getRootBin() const
{
    QGstElement ancestor = *this;
    for (;;) {
        QGstElement greatAncestor = ancestor.getParent();
        if (greatAncestor) {
            ancestor = std::move(greatAncestor);
            continue;
        }
        if (GST_IS_BIN(ancestor.element())) {
            return QGstBin{
                qGstSafeCast<GstBin>(ancestor.element()),
                QGstBin::NeedsRef,
            };
        } else {
            return QGstBin{};
        }
    }
}

QGstPipeline QGstElement::getPipeline() const
{
    QGstBin rootBin = getRootBin();
    if (GST_IS_PIPELINE(rootBin.get())) {
        return QGstPipeline{
            qGstSafeCast<GstPipeline>(rootBin.element()),
            QGstPipeline::NeedsRef,
        };
    } else {
        qWarning() << "QGstElement::getPipeline failed for element:" << *this;
        return QGstPipeline{};
    }
}

void QGstElement::removeFromParent()
{
    if (QGstBin parent = getParentBin())
        parent.remove(*this);
}

void QGstElement::dumpPipelineGraph(const char *filename) const
{
    static const bool dumpEnabled = qEnvironmentVariableIsSet("GST_DEBUG_DUMP_DOT_DIR");
    if (dumpEnabled) {
        getRootBin().dumpGraph(filename);
    }
}

QGstQueryHandle &QGstElement::positionQuery() const
{
    if (Q_UNLIKELY(!m_positionQuery))
        m_positionQuery = QGstQueryHandle{
            gst_query_new_position(GST_FORMAT_TIME),
            QGstQueryHandle::HasRef,
        };

    return m_positionQuery;
}

// QGstBin

QGstBin QGstBin::create(const char *name)
{
    return QGstBin(gst_bin_new(name), NeedsRef);
}

QGstBin QGstBin::createFromFactory(const char *factory, const char *name)
{
    QGstElement element = QGstElement::createFromFactory(factory, name);
    Q_ASSERT(GST_IS_BIN(element.element()));
    return QGstBin{
        GST_BIN(element.release()),
        RefMode::HasRef,
    };
}

QGstBin QGstBin::createFromPipelineDescription(const QByteArray &pipelineDescription,
                                               const char *name, bool ghostUnlinkedPads)
{
    return createFromPipelineDescription(pipelineDescription.constData(), name, ghostUnlinkedPads);
}

QGstBin QGstBin::createFromPipelineDescription(const char *pipelineDescription, const char *name,
                                               bool ghostUnlinkedPads)
{
    QUniqueGErrorHandle error;

    GstElement *element =
            gst_parse_bin_from_description_full(pipelineDescription, ghostUnlinkedPads,
                                                /*context=*/nullptr, GST_PARSE_FLAG_NONE, &error);

    if (!element) {
        qWarning() << "Failed to make element from pipeline description" << pipelineDescription
                   << error;
        return QGstBin{};
    }

    if (name)
        gst_element_set_name(element, name);

    return QGstBin{
        element,
        NeedsRef,
    };
}

QGstBin::QGstBin(GstBin *bin, RefMode mode)
    : QGstElement{
          qGstCheckedCast<GstElement>(bin),
          mode,
      }
{
}

GstBin *QGstBin::bin() const
{
    return qGstCheckedCast<GstBin>(object());
}

void QGstBin::addGhostPad(const QGstElement &child, const char *name)
{
    addGhostPad(name, child.staticPad(name));
}

void QGstBin::addGhostPad(const char *name, const QGstPad &pad)
{
    gst_element_add_pad(element(), gst_ghost_pad_new(name, pad.pad()));
}

void QGstBin::addUnlinkedGhostPads(GstPadDirection direction)
{
    Q_ASSERT(direction != GstPadDirection::GST_PAD_UNKNOWN);

    for (;;) {
        QGstPad unlinkedPad{
            gst_bin_find_unlinked_pad(bin(), direction),
            QGstPad::HasRef,
        };

        if (!unlinkedPad)
            return;

        addGhostPad(unlinkedPad.name().constData(), unlinkedPad);
    }
}

bool QGstBin::syncChildrenState()
{
    return gst_bin_sync_children_states(bin());
}

void QGstBin::dumpGraph(const char *fileNamePrefix, bool includeTimestamp) const
{
    if (!get())
        return;

    if (includeTimestamp)
        GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS(bin(), GST_DEBUG_GRAPH_SHOW_VERBOSE, fileNamePrefix);
    else
        GST_DEBUG_BIN_TO_DOT_FILE(bin(), GST_DEBUG_GRAPH_SHOW_VERBOSE, fileNamePrefix);
}

QGstElement QGstBin::findByName(const char *name)
{
    return QGstElement{
        gst_bin_get_by_name(bin(), name),
        QGstElement::NeedsRef,
    };
}

void QGstBin::recalculateLatency()
{
    gst_bin_recalculate_latency(bin());
}

// QGstBaseSink

QGstBaseSink::QGstBaseSink(GstBaseSink *element, RefMode mode)
    : QGstElement{
          qGstCheckedCast<GstElement>(element),
          mode,
      }
{
}

void QGstBaseSink::setSync(bool arg)
{
    gst_base_sink_set_sync(baseSink(), arg ? TRUE : FALSE);
}

GstBaseSink *QGstBaseSink::baseSink() const
{
    return qGstCheckedCast<GstBaseSink>(element());
}

// QGstBaseSrc

QGstBaseSrc::QGstBaseSrc(GstBaseSrc *element, RefMode mode)
    : QGstElement{
          qGstCheckedCast<GstElement>(element),
          mode,
      }
{
}

GstBaseSrc *QGstBaseSrc::baseSrc() const
{
    return qGstCheckedCast<GstBaseSrc>(element());
}

// QGstAppSink

QGstAppSink::QGstAppSink(GstAppSink *element, RefMode mode)
    : QGstBaseSink{
          qGstCheckedCast<GstBaseSink>(element),
          mode,
      }
{
}

QGstAppSink QGstAppSink::create(const char *name)
{
    QGstElement created = QGstElement::createFromFactory("appsink", name);
    return QGstAppSink{
        qGstCheckedCast<GstAppSink>(created.element()),
        QGstAppSink::NeedsRef,
    };
}

GstAppSink *QGstAppSink::appSink() const
{
    return qGstCheckedCast<GstAppSink>(element());
}

#  if GST_CHECK_VERSION(1, 24, 0)
void QGstAppSink::setMaxBufferTime(std::chrono::nanoseconds ns)
{
    gst_app_sink_set_max_time(appSink(), qGstClockTimeFromChrono(ns));
}
#  endif

void QGstAppSink::setMaxBuffers(int n)
{
    gst_app_sink_set_max_buffers(appSink(), n);
}

void QGstAppSink::setCaps(const QGstCaps &caps)
{
    gst_app_sink_set_caps(appSink(), caps.caps());
}

void QGstAppSink::setCallbacks(GstAppSinkCallbacks &callbacks, gpointer user_data,
                               GDestroyNotify notify)
{
    gst_app_sink_set_callbacks(appSink(), &callbacks, user_data, notify);
}

QGstSampleHandle QGstAppSink::pullSample()
{
    return QGstSampleHandle{
        gst_app_sink_pull_sample(appSink()),
        QGstSampleHandle::HasRef,
    };
}

// QGstAppSrc

QGstAppSrc::QGstAppSrc(GstAppSrc *element, RefMode mode)
    : QGstBaseSrc{
          qGstCheckedCast<GstBaseSrc>(element),
          mode,
      }
{
}

QGstAppSrc QGstAppSrc::create(const char *name)
{
    QGstElement created = QGstElement::createFromFactory("appsrc", name);
    return QGstAppSrc{
        qGstCheckedCast<GstAppSrc>(created.element()),
        QGstAppSrc::NeedsRef,
    };
}

GstAppSrc *QGstAppSrc::appSrc() const
{
    return qGstCheckedCast<GstAppSrc>(element());
}

void QGstAppSrc::setCallbacks(GstAppSrcCallbacks &callbacks, gpointer user_data,
                              GDestroyNotify notify)
{
    gst_app_src_set_callbacks(appSrc(), &callbacks, user_data, notify);
}

GstFlowReturn QGstAppSrc::pushBuffer(GstBuffer *buffer)
{
    return gst_app_src_push_buffer(appSrc(), buffer);
}

QString qGstErrorMessageCannotFindElement(std::string_view element)
{
    return QStringLiteral("Could not find the %1 GStreamer element")
            .arg(QLatin1StringView(element));
}

QVideoFrameFormat qVideoFrameFormatFromGstVideoInfo(const QGstVideoInfo &qtVideoInfo)
{
    auto &vidInfo = qtVideoInfo.gstVideoInfo;
    GstVideoFormat gstFormat = GST_VIDEO_INFO_FORMAT(&vidInfo);

    auto pixelFormat = qPixelFormatFromGstVideoFormat(gstFormat);
    if (pixelFormat == QVideoFrameFormat::Format_Invalid)
        return QVideoFrameFormat();

    QVideoFrameFormat format(QSize(vidInfo.width, vidInfo.height), pixelFormat);

    if (vidInfo.fps_d > 0)
        format.setStreamFrameRate(qreal(vidInfo.fps_n) / vidInfo.fps_d);

    QVideoFrameFormat::ColorRange range = QVideoFrameFormat::ColorRange_Unknown;
    switch (vidInfo.colorimetry.range) {
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
    switch (vidInfo.colorimetry.matrix) {
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
    switch (vidInfo.colorimetry.transfer) {
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
    }
    format.setColorTransfer(transfer);

    return format;
}

QGstCaps::MemoryFormat qMemoryFormatFromGstBuffer(GstBuffer *buffer)
{
    Q_ASSERT(buffer);

    QGstCaps::MemoryFormat memoryFormat = QGstCaps::CpuMemory;

    [[maybe_unused]] GstMemory *mem = gst_buffer_peek_memory(buffer, 0);

#if QT_CONFIG(gstreamer_gl_egl) && QT_CONFIG(linux_dmabuf)
    if (gst_is_dmabuf_memory(mem))
        memoryFormat = QGstCaps::DMABuf;
#endif
#if QT_CONFIG(gstreamer_gl)
    if (gst_is_gl_memory(mem))
        memoryFormat = QGstCaps::GLTexture;
#endif
    return memoryFormat;
}

QVideoFrame qCreateFrameFromGstBuffer(QGstBufferHandle buffer, const QGstVideoInfo &videoInfo)
{
    auto format = qVideoFrameFormatFromGstVideoInfo(videoInfo);
    auto videoBuffer = std::make_unique<QGstVideoBuffer>(buffer, videoInfo, format);
    return QVideoFramePrivate::createFrame(std::move(videoBuffer), format);
}

QT_END_NAMESPACE
