// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <common/qgst_p.h>
#include <common/qgst_debug_p.h>
#include <common/qgstpipeline_p.h>
#include <common/qgstreamermessage_p.h>

#include <QtCore/qdebug.h>
#include <QtMultimedia/qcameradevice.h>

#include <array>
#include <thread>

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

int indexOfVideoFormat(QVideoFrameFormat::PixelFormat format)
{
    for (size_t i = 0; i < qt_videoFormatLookup.size(); ++i)
        if (qt_videoFormatLookup[i].pixelFormat == format)
            return int(i);

    return -1;
}

int indexOfVideoFormat(GstVideoFormat format)
{
    for (size_t i = 0; i < qt_videoFormatLookup.size(); ++i)
        if (qt_videoFormatLookup[i].gstFormat == format)
            return int(i);

    return -1;
}

} // namespace

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

QVideoFrameFormat::PixelFormat QGstStructureView::pixelFormat() const
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

// QTBUG-125249: gstreamer tries "to keep the input height (because of interlacing)". Can we align
// the behavior between gstreamer and ffmpeg?
static QSize qCalculateFrameSizeGStreamer(QSize resolution, Fraction par)
{
    if (par.numerator == par.denominator || par.numerator < 1 || par.denominator < 1)
        return resolution;

    return QSize{
        resolution.width() * par.numerator / par.denominator,
        resolution.height(),
    };
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
        size = qCalculateFrameSizeGStreamer(size, *par);
    return size;
}

// QGstCaps

std::optional<std::pair<QVideoFrameFormat, GstVideoInfo>> QGstCaps::formatAndVideoInfo() const
{
    GstVideoInfo vidInfo;

    bool success = gst_video_info_from_caps(&vidInfo, get());
    if (!success)
        return std::nullopt;

    int index = indexOfVideoFormat(vidInfo.finfo->format);
    if (index == -1)
        return std::nullopt;

    QVideoFrameFormat format(QSize(vidInfo.width, vidInfo.height),
                             qt_videoFormatLookup[index].pixelFormat);

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

    return std::pair{
        std::move(format),
        vidInfo,
    };
}

void QGstCaps::addPixelFormats(const QList<QVideoFrameFormat::PixelFormat> &formats,
                               const char *modifier)
{
    if (!gst_caps_is_writable(get()))
        *this = QGstCaps(gst_caps_make_writable(release()), QGstCaps::RefMode::HasRef);

    GValue list = {};
    g_value_init(&list, GST_TYPE_LIST);

    for (QVideoFrameFormat::PixelFormat format : formats) {
        int index = indexOfVideoFormat(format);
        if (index == -1)
            continue;
        GValue item = {};

        g_value_init(&item, G_TYPE_STRING);
        g_value_set_string(&item,
                           gst_video_format_to_string(qt_videoFormatLookup[index].gstFormat));
        gst_value_list_append_value(&list, &item);
        g_value_unset(&item);
    }

    auto *structure = gst_structure_new("video/x-raw", "framerate", GST_TYPE_FRACTION_RANGE, 0, 1,
                                        INT_MAX, 1, "width", GST_TYPE_INT_RANGE, 1, INT_MAX,
                                        "height", GST_TYPE_INT_RANGE, 1, INT_MAX, nullptr);
    gst_structure_set_value(structure, "format", &list);
    gst_caps_append_structure(get(), structure);
    g_value_unset(&list);

    if (modifier)
        gst_caps_set_features(get(), size() - 1, gst_caps_features_from_string(modifier));
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
    GstStructure *structure = nullptr;
    if (format.pixelFormat() == QVideoFrameFormat::Format_Jpeg) {
        structure = gst_structure_new("image/jpeg", "width", G_TYPE_INT, size.width(), "height",
                                      G_TYPE_INT, size.height(), nullptr);
    } else {
        int index = indexOfVideoFormat(format.pixelFormat());
        if (index < 0)
            return {};
        auto gstFormat = qt_videoFormatLookup[index].gstFormat;
        structure = gst_structure_new("video/x-raw", "format", G_TYPE_STRING,
                                      gst_video_format_to_string(gstFormat), "width", G_TYPE_INT,
                                      size.width(), "height", G_TYPE_INT, size.height(), nullptr);
    }
    auto caps = QGstCaps::create();
    gst_caps_append_structure(caps.get(), structure);
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

QT_END_NAMESPACE
