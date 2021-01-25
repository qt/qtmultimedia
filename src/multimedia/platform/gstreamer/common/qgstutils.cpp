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

//internal
static void addTagToMap(const GstTagList *list,
                        const gchar *tag,
                        gpointer user_data)
{
    QMap<QByteArray, QVariant> *map = reinterpret_cast<QMap<QByteArray, QVariant>* >(user_data);

    GValue val;
    val.g_type = 0;
    gst_tag_list_copy_value(&val,list,tag);

    switch( G_VALUE_TYPE(&val) ) {
        case G_TYPE_STRING:
        {
            const gchar *str_value = g_value_get_string(&val);
            map->insert(QByteArray(tag), QString::fromUtf8(str_value));
            break;
        }
        case G_TYPE_INT:
            map->insert(QByteArray(tag), g_value_get_int(&val));
            break;
        case G_TYPE_UINT:
            map->insert(QByteArray(tag), g_value_get_uint(&val));
            break;
        case G_TYPE_LONG:
            map->insert(QByteArray(tag), qint64(g_value_get_long(&val)));
            break;
        case G_TYPE_BOOLEAN:
            map->insert(QByteArray(tag), g_value_get_boolean(&val));
            break;
        case G_TYPE_CHAR:
#if GLIB_CHECK_VERSION(2,32,0)
            map->insert(QByteArray(tag), g_value_get_schar(&val));
#else
            map->insert(QByteArray(tag), g_value_get_char(&val));
#endif
            break;
        case G_TYPE_DOUBLE:
            map->insert(QByteArray(tag), g_value_get_double(&val));
            break;
        default:
            // GST_TYPE_DATE is a function, not a constant, so pull it out of the switch
            if (G_VALUE_TYPE(&val) == G_TYPE_DATE) {
                const GDate *date = (const GDate *)g_value_get_boxed(&val);
                if (g_date_valid(date)) {
                    int year = g_date_get_year(date);
                    int month = g_date_get_month(date);
                    int day = g_date_get_day(date);
                    map->insert(QByteArray(tag), QDate(year,month,day));
                    if (!map->contains("year"))
                        map->insert("year", year);
                }
            } else if (G_VALUE_TYPE(&val) == GST_TYPE_DATE_TIME) {
                const GstDateTime *dateTime = (const GstDateTime *)g_value_get_boxed(&val);
                int year = gst_date_time_has_year(dateTime) ? gst_date_time_get_year(dateTime) : 0;
                int month = gst_date_time_has_month(dateTime) ? gst_date_time_get_month(dateTime) : 0;
                int day = gst_date_time_has_day(dateTime) ? gst_date_time_get_day(dateTime) : 0;
                if (gst_date_time_has_time(dateTime)) {
                    int hour = gst_date_time_get_hour(dateTime);
                    int minute = gst_date_time_get_minute(dateTime);
                    int second = gst_date_time_get_second(dateTime);
                    float tz = gst_date_time_get_time_zone_offset(dateTime);
                    QDateTime dateTime(QDate(year, month, day), QTime(hour, minute, second),
                                       Qt::OffsetFromUTC, tz * 60 * 60);
                    map->insert(QByteArray(tag), dateTime);
                } else if (year > 0 && month > 0 && day > 0) {
                    map->insert(QByteArray(tag), QDate(year,month,day));
                }
                if (!map->contains("year") && year > 0)
                    map->insert("year", year);
            } else if (G_VALUE_TYPE(&val) == GST_TYPE_SAMPLE) {
                GstSample *sample = (GstSample *)g_value_get_boxed(&val);
                GstCaps* caps = gst_sample_get_caps(sample);
                if (caps && !gst_caps_is_empty(caps)) {
                    GstStructure *structure = gst_caps_get_structure(caps, 0);
                    const gchar *name = gst_structure_get_name(structure);
                    if (QByteArray(name).startsWith("image/")) {
                        GstBuffer *buffer = gst_sample_get_buffer(sample);
                        if (buffer) {
                            GstMapInfo info;
                            gst_buffer_map(buffer, &info, GST_MAP_READ);
                            map->insert(QByteArray(tag), QImage::fromData(info.data, info.size, name));
                            gst_buffer_unmap(buffer, &info);
                        }
                    }
                }
            } else if (G_VALUE_TYPE(&val) == GST_TYPE_FRACTION) {
                int nom = gst_value_get_fraction_numerator(&val);
                int denom = gst_value_get_fraction_denominator(&val);

                if (denom > 0) {
                    map->insert(QByteArray(tag), double(nom)/denom);
                }
            }
            break;
    }

    g_value_unset(&val);
}

/*!
    \class QGstUtils
    \internal
*/

/*!
  Convert GstTagList structure to QMap<QByteArray, QVariant>.

  Mapping to int, bool, char, string, fractions and date are supported.
  Fraction values are converted to doubles.
*/
QMap<QByteArray, QVariant> QGstUtils::gstTagListToMap(const GstTagList *tags)
{
    QMap<QByteArray, QVariant> res;
    gst_tag_list_foreach(tags, addTagToMap, &res);

    return res;
}

/*!
  Returns resolution of \a caps.
  If caps doesn't have a valid size, an empty QSize is returned.
*/
QSize QGstUtils::capsResolution(const GstCaps *caps)
{
    if (gst_caps_get_size(caps) == 0)
        return QSize();

    return structureResolution(gst_caps_get_structure(caps, 0));
}

/*!
  Returns aspect ratio corrected resolution of \a caps.
  If caps doesn't have a valid size, an empty QSize is returned.
*/
QSize QGstUtils::capsCorrectedResolution(const GstCaps *caps)
{
    QSize size;

    if (caps) {
        size = capsResolution(caps);

        gint aspectNum = 0;
        gint aspectDenum = 0;
        if (!size.isEmpty() && gst_structure_get_fraction(
                    gst_caps_get_structure(caps, 0), "pixel-aspect-ratio", &aspectNum, &aspectDenum)) {
            if (aspectDenum > 0)
                size.setWidth(size.width()*aspectNum/aspectDenum);
        }
    }

    return size;
}


namespace {

struct AudioFormat
{
    GstAudioFormat format;
    QAudioFormat::SampleType sampleType;
    QAudioFormat::Endian byteOrder;
    int sampleSize;
};
static const AudioFormat qt_audioLookup[] =
{
    { GST_AUDIO_FORMAT_S8   , QAudioFormat::SignedInt  , QAudioFormat::LittleEndian, 8  },
    { GST_AUDIO_FORMAT_U8   , QAudioFormat::UnSignedInt, QAudioFormat::LittleEndian, 8  },
    { GST_AUDIO_FORMAT_S16LE, QAudioFormat::SignedInt  , QAudioFormat::LittleEndian, 16 },
    { GST_AUDIO_FORMAT_S16BE, QAudioFormat::SignedInt  , QAudioFormat::BigEndian   , 16 },
    { GST_AUDIO_FORMAT_U16LE, QAudioFormat::UnSignedInt, QAudioFormat::LittleEndian, 16 },
    { GST_AUDIO_FORMAT_U16BE, QAudioFormat::UnSignedInt, QAudioFormat::BigEndian   , 16 },
    { GST_AUDIO_FORMAT_S32LE, QAudioFormat::SignedInt  , QAudioFormat::LittleEndian, 32 },
    { GST_AUDIO_FORMAT_S32BE, QAudioFormat::SignedInt  , QAudioFormat::BigEndian   , 32 },
    { GST_AUDIO_FORMAT_U32LE, QAudioFormat::UnSignedInt, QAudioFormat::LittleEndian, 32 },
    { GST_AUDIO_FORMAT_U32BE, QAudioFormat::UnSignedInt, QAudioFormat::BigEndian   , 32 },
    { GST_AUDIO_FORMAT_S24LE, QAudioFormat::SignedInt  , QAudioFormat::LittleEndian, 24 },
    { GST_AUDIO_FORMAT_S24BE, QAudioFormat::SignedInt  , QAudioFormat::BigEndian   , 24 },
    { GST_AUDIO_FORMAT_U24LE, QAudioFormat::UnSignedInt, QAudioFormat::LittleEndian, 24 },
    { GST_AUDIO_FORMAT_U24BE, QAudioFormat::UnSignedInt, QAudioFormat::BigEndian   , 24 },
    { GST_AUDIO_FORMAT_F32LE, QAudioFormat::Float      , QAudioFormat::LittleEndian, 32 },
    { GST_AUDIO_FORMAT_F32BE, QAudioFormat::Float      , QAudioFormat::BigEndian   , 32 },
    { GST_AUDIO_FORMAT_F64LE, QAudioFormat::Float      , QAudioFormat::LittleEndian, 64 },
    { GST_AUDIO_FORMAT_F64BE, QAudioFormat::Float      , QAudioFormat::BigEndian   , 64 }
};

}

/*!
  Returns audio format for caps.
  If caps doesn't have a valid audio format, an empty QAudioFormat is returned.
*/

QAudioFormat QGstUtils::audioFormatForCaps(const GstCaps *caps)
{
    QAudioFormat format;
    GstAudioInfo info;
    if (gst_audio_info_from_caps(&info, caps)) {
        for (int i = 0; i < lengthOf(qt_audioLookup); ++i) {
            if (qt_audioLookup[i].format != info.finfo->format)
                continue;

            format.setSampleType(qt_audioLookup[i].sampleType);
            format.setByteOrder(qt_audioLookup[i].byteOrder);
            format.setSampleSize(qt_audioLookup[i].sampleSize);
            format.setSampleRate(info.rate);
            format.setChannelCount(info.channels);
            format.setCodec(QStringLiteral("audio/x-raw"));

            return format;
        }
    }

    return format;
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

    return QGstUtils::audioFormatForCaps(caps);
}

/*!
  Builds GstCaps for an audio format.
  Returns 0 if the audio format is not valid.
  Caller must unref GstCaps.
*/

GstCaps *QGstUtils::capsForAudioFormat(const QAudioFormat &format)
{
    if (!format.isValid())
        return 0;

    const QAudioFormat::SampleType sampleType = format.sampleType();
    const QAudioFormat::Endian byteOrder = format.byteOrder();
    const int sampleSize = format.sampleSize();

    for (int i = 0; i < lengthOf(qt_audioLookup); ++i) {
        if (qt_audioLookup[i].sampleType != sampleType
                || qt_audioLookup[i].byteOrder != byteOrder
                || qt_audioLookup[i].sampleSize != sampleSize) {
            continue;
        }

        return gst_caps_new_simple(
                    "audio/x-raw",
                    "format"  , G_TYPE_STRING, gst_audio_format_to_string(qt_audioLookup[i].format),
                    "rate"    , G_TYPE_INT   , format.sampleRate(),
                    "channels", G_TYPE_INT   , format.channelCount(),
                    "layout"  , G_TYPE_STRING, "interleaved",
                    nullptr);
    }
    return 0;
}

static QSet<GstDevice *> m_videoSources;
static QSet<GstDevice *> m_audioSources;
static QSet<GstDevice *> m_audioSinks;

static void addDevice(GstDevice *device)
{
    gchar *type = gst_device_get_device_class(device);
//    qDebug() << "adding device:" << device << type << gst_device_get_display_name(device) << gst_structure_to_string(gst_device_get_properties(device));
    gst_object_ref(device);
    if (!strcmp(type, "Video/Source"))
        m_videoSources.insert(device);
    else if (!strcmp(type, "Audio/Source"))
        m_audioSources.insert(device);
    else if (!strcmp(type, "Audio/Sink"))
        m_audioSinks.insert(device);
    else
        gst_object_unref(device);
    g_free(type);
}

static void removeDevice(GstDevice *device)
{
//    qDebug() << "removing device:" << device << gst_device_get_display_name(device);
    if (m_videoSources.remove(device) ||
        m_audioSources.remove(device) ||
        m_audioSinks.remove(device))
        gst_object_unref(device);
}

static gboolean deviceMonitor(GstBus *, GstMessage *message, gpointer)
{
    GstDevice *device = nullptr;

    switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_DEVICE_ADDED:
        gst_message_parse_device_added(message, &device);
        addDevice(device);
        break;
    case GST_MESSAGE_DEVICE_REMOVED:
        gst_message_parse_device_removed(message, &device);
        removeDevice(device);
        break;
    default:
        break;
    }
    if (device)
        gst_object_unref (device);

    return G_SOURCE_CONTINUE;
}

void setupDeviceMonitor()
{
    GstDeviceMonitor *monitor;
    GstBus *bus;

    monitor = gst_device_monitor_new();

    gst_device_monitor_add_filter (monitor, "Video/Source", NULL);
    gst_device_monitor_add_filter (monitor, "Audio/Source", NULL);
    gst_device_monitor_add_filter (monitor, "Audio/Sink", NULL);

    bus = gst_device_monitor_get_bus(monitor);
    gst_bus_add_watch(bus, deviceMonitor, NULL);
    gst_object_unref(bus);

    gst_device_monitor_start(monitor);

        auto devices = gst_device_monitor_get_devices(monitor);

        while (devices) {
            GstDevice *device = static_cast<GstDevice *>(devices->data);
            addDevice(device);
            gst_object_unref(device);
            devices = g_list_delete_link(devices, devices);
        }
}


void QGstUtils::initializeGst()
{
    static bool initialized = false;
    if (!initialized) {
        initialized = true;
        gst_init(nullptr, nullptr);
        setupDeviceMonitor();
    }
}

namespace {
    const char* getCodecAlias(const QString &codec)
    {
        if (codec.startsWith(QLatin1String("avc1.")))
            return "video/x-h264";

        if (codec.startsWith(QLatin1String("mp4a.")))
            return "audio/mpeg4";

        if (codec.startsWith(QLatin1String("mp4v.20.")))
            return "video/mpeg4";

        if (codec == QLatin1String("samr"))
            return "audio/amr";

        return 0;
    }

    const char* getMimeTypeAlias(const QString &mimeType)
    {
        if (mimeType == QLatin1String("video/mp4"))
            return "video/mpeg4";

        if (mimeType == QLatin1String("audio/mp4"))
            return "audio/mpeg4";

        if (mimeType == QLatin1String("video/ogg")
            || mimeType == QLatin1String("audio/ogg"))
            return "application/ogg";

        return 0;
    }
}

QMultimedia::SupportEstimate QGstUtils::hasSupport(const QString &mimeType,
                                                    const QStringList &codecs,
                                                    const QSet<QString> &supportedMimeTypeSet)
{
    if (supportedMimeTypeSet.isEmpty())
        return QMultimedia::NotSupported;

    QString mimeTypeLowcase = mimeType.toLower();
    bool containsMimeType = supportedMimeTypeSet.contains(mimeTypeLowcase);
    if (!containsMimeType) {
        const char* mimeTypeAlias = getMimeTypeAlias(mimeTypeLowcase);
        containsMimeType = supportedMimeTypeSet.contains(QLatin1String(mimeTypeAlias));
        if (!containsMimeType) {
            containsMimeType = supportedMimeTypeSet.contains(QLatin1String("video/") + mimeTypeLowcase)
                               || supportedMimeTypeSet.contains(QLatin1String("video/x-") + mimeTypeLowcase)
                               || supportedMimeTypeSet.contains(QLatin1String("audio/") + mimeTypeLowcase)
                               || supportedMimeTypeSet.contains(QLatin1String("audio/x-") + mimeTypeLowcase);
        }
    }

    int supportedCodecCount = 0;
    for (const QString &codec : codecs) {
        QString codecLowcase = codec.toLower();
        const char* codecAlias = getCodecAlias(codecLowcase);
        if (codecAlias) {
            if (supportedMimeTypeSet.contains(QLatin1String(codecAlias)))
                supportedCodecCount++;
        } else if (supportedMimeTypeSet.contains(QLatin1String("video/") + codecLowcase)
                   || supportedMimeTypeSet.contains(QLatin1String("video/x-") + codecLowcase)
                   || supportedMimeTypeSet.contains(QLatin1String("audio/") + codecLowcase)
                   || supportedMimeTypeSet.contains(QLatin1String("audio/x-") + codecLowcase)) {
            supportedCodecCount++;
        }
    }
    if (supportedCodecCount > 0 && supportedCodecCount == codecs.size())
        return QMultimedia::ProbablySupported;

    if (supportedCodecCount == 0 && !containsMimeType)
        return QMultimedia::NotSupported;

    return QMultimedia::MaybeSupported;
}

const QSet<GstDevice *> &QGstUtils::audioSources()
{
    return m_audioSources;
}

const QSet<GstDevice *> &QGstUtils::audioSinks()
{
    return m_audioSinks;
}

QSet<QString> QGstUtils::supportedMimeTypes(bool (*isValidFactory)(GstElementFactory *factory))
{
    QSet<QString> supportedMimeTypes;

    //enumerate supported mime types
    gst_init(nullptr, nullptr);

    GstRegistry *registry = gst_registry_get();
    GList *orig_plugins = gst_registry_get_plugin_list(registry);
    for (GList *plugins = orig_plugins; plugins; plugins = g_list_next(plugins)) {
        GstPlugin *plugin = (GstPlugin *) (plugins->data);
        if (GST_OBJECT_FLAG_IS_SET(GST_OBJECT(plugin), GST_PLUGIN_FLAG_BLACKLISTED))
            continue;

        GList *orig_features = gst_registry_get_feature_list_by_plugin(
                    registry, gst_plugin_get_name(plugin));
        for (GList *features = orig_features; features; features = g_list_next(features)) {
            if (G_UNLIKELY(features->data == nullptr))
                continue;

            GstPluginFeature *feature = GST_PLUGIN_FEATURE(features->data);
            GstElementFactory *factory;

            if (GST_IS_TYPE_FIND_FACTORY(feature)) {
                QString name(QLatin1String(gst_plugin_feature_get_name(feature)));
                if (name.contains(QLatin1Char('/'))) //filter out any string without '/' which is obviously not a mime type
                    supportedMimeTypes.insert(name.toLower());
                continue;
            } else if (!GST_IS_ELEMENT_FACTORY (feature)
                        || !(factory = GST_ELEMENT_FACTORY(gst_plugin_feature_load(feature)))) {
                continue;
            } else if (!isValidFactory(factory)) {
                // Do nothing
            } else for (const GList *pads = gst_element_factory_get_static_pad_templates(factory);
                        pads;
                        pads = g_list_next(pads)) {
                GstStaticPadTemplate *padtemplate = static_cast<GstStaticPadTemplate *>(pads->data);

                if (padtemplate->direction == GST_PAD_SINK && padtemplate->static_caps.string) {
                    GstCaps *caps = gst_static_caps_get(&padtemplate->static_caps);
                    if (gst_caps_is_any(caps) || gst_caps_is_empty(caps)) {
                    } else for (guint i = 0; i < gst_caps_get_size(caps); i++) {
                        GstStructure *structure = gst_caps_get_structure(caps, i);
                        QString nameLowcase = QString::fromLatin1(gst_structure_get_name(structure)).toLower();

                        supportedMimeTypes.insert(nameLowcase);
                        if (nameLowcase.contains(QLatin1String("mpeg"))) {
                            //Because mpeg version number is only included in the detail
                            //description,  it is necessary to manually extract this information
                            //in order to match the mime type of mpeg4.
                            const GValue *value = gst_structure_get_value(structure, "mpegversion");
                            if (value) {
                                gchar *str = gst_value_serialize(value);
                                QString versions = QLatin1String(str);
                                const QStringList elements = versions.split(QRegularExpression(QLatin1String("\\D+")), Qt::SkipEmptyParts);
                                for (const QString &e : elements)
                                    supportedMimeTypes.insert(nameLowcase + e);
                                g_free(str);
                            }
                        }
                    }
                }
            }
            gst_object_unref(factory);
        }
        gst_plugin_feature_list_free(orig_features);
    }
    gst_plugin_list_free (orig_plugins);

#if defined QT_SUPPORTEDMIMETYPES_DEBUG
    QStringList list = supportedMimeTypes.toList();
    list.sort();
    if (qgetenv("QT_DEBUG_PLUGINS").toInt() > 0) {
        for (const QString &type : qAsConst(list))
            qDebug() << type;
    }
#endif
    return supportedMimeTypes;
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
        GstCaps *caps, GstVideoInfo *info, QAbstractVideoBuffer::HandleType handleType)
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

            if (infoPtr->par_d > 0)
                format.setPixelAspectRatio(infoPtr->par_n, infoPtr->par_d);

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

void QGstUtils::setMetaData(GstElement *element, const QMap<QByteArray, QVariant> &data)
{
    if (!GST_IS_TAG_SETTER(element))
        return;

    gst_tag_setter_reset_tags(GST_TAG_SETTER(element));

    for (auto it = data.cbegin(), end = data.cend(); it != end; ++it) {
        const QString tagName = QString::fromLatin1(it.key());
        const QVariant &tagValue = it.value();

        switch (tagValue.typeId()) {
            case QMetaType::QString:
                gst_tag_setter_add_tags(GST_TAG_SETTER(element),
                    GST_TAG_MERGE_REPLACE,
                    tagName.toUtf8().constData(),
                    tagValue.toString().toUtf8().constData(),
                    nullptr);
                break;
            case QMetaType::Int:
            case QMetaType::LongLong:
                gst_tag_setter_add_tags(GST_TAG_SETTER(element),
                    GST_TAG_MERGE_REPLACE,
                    tagName.toUtf8().constData(),
                    tagValue.toInt(),
                    nullptr);
                break;
            case QMetaType::Double:
                gst_tag_setter_add_tags(GST_TAG_SETTER(element),
                    GST_TAG_MERGE_REPLACE,
                    tagName.toUtf8().constData(),
                    tagValue.toDouble(),
                    nullptr);
                break;
            case QMetaType::QDateTime: {
                QDateTime date = tagValue.toDateTime().toLocalTime();
                gst_tag_setter_add_tags(GST_TAG_SETTER(element),
                    GST_TAG_MERGE_REPLACE,
                    tagName.toUtf8().constData(),
                    gst_date_time_new_local_time(
                                date.date().year(), date.date().month(), date.date().day(),
                                date.time().hour(), date.time().minute(), date.time().second()),
                    nullptr);
                break;
            }
            default:
                break;
        }
    }
}

void QGstUtils::setMetaData(GstBin *bin, const QMap<QByteArray, QVariant> &data)
{
    GstIterator *elements = gst_bin_iterate_all_by_interface(bin, GST_TYPE_TAG_SETTER);
    GValue item = G_VALUE_INIT;
    while (gst_iterator_next(elements, &item) == GST_ITERATOR_OK) {
        GstElement * const element = GST_ELEMENT(g_value_get_object(&item));
        setMetaData(element, data);
    }
    gst_iterator_free(elements);
}


GstCaps *QGstUtils::videoFilterCaps()
{
    const char *caps =
        "video/x-raw(ANY);"
        "image/jpeg;"
        "video/x-h264";
    static GstStaticCaps staticCaps = GST_STATIC_CAPS(caps);

    return gst_caps_make_writable(gst_static_caps_get(&staticCaps));
}

QSize QGstUtils::structureResolution(const GstStructure *s)
{
    QSize size;

    int w, h;
    if (s && gst_structure_get_int(s, "width", &w) && gst_structure_get_int(s, "height", &h)) {
        size.rwidth() = w;
        size.rheight() = h;
    }

    return size;
}

QVideoFrame::PixelFormat QGstUtils::structurePixelFormat(const GstStructure *structure)
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

QSize QGstUtils::structurePixelAspectRatio(const GstStructure *s)
{
    QSize ratio(1, 1);

    gint aspectNum = 0;
    gint aspectDenum = 0;
    if (s && gst_structure_get_fraction(s, "pixel-aspect-ratio", &aspectNum, &aspectDenum)) {
        if (aspectDenum > 0) {
            ratio.rwidth() = aspectNum;
            ratio.rheight() = aspectDenum;
        }
    }

    return ratio;
}

QPair<float, float> QGstUtils::structureFrameRateRange(const GstStructure *s)
{
    float minRate = 0.;
    float maxRate = 0.;

    if (!s)
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

    const GValue *gstFrameRates = gst_structure_get_value(s, "framerate");
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
        const GValue *min = gst_structure_get_value(s, "min-framerate");
        const GValue *max = gst_structure_get_value(s, "max-framerate");
        if (min && max) {
            minRate = extractFraction(min);
            maxRate = extractFraction(max);
        }
    }

    return {minRate, maxRate};
}

typedef QMap<QString, QString> FileExtensionMap;
Q_GLOBAL_STATIC(FileExtensionMap, fileExtensionMap)

QString QGstUtils::fileExtensionForMimeType(const QString &mimeType)
{
    if (fileExtensionMap->isEmpty()) {
        //extension for containers hard to guess from mimetype
        fileExtensionMap->insert(QStringLiteral("video/x-matroska"), QLatin1String("mkv"));
        fileExtensionMap->insert(QStringLiteral("video/quicktime"), QLatin1String("mov"));
        fileExtensionMap->insert(QStringLiteral("video/x-msvideo"), QLatin1String("avi"));
        fileExtensionMap->insert(QStringLiteral("video/msvideo"), QLatin1String("avi"));
        fileExtensionMap->insert(QStringLiteral("audio/mpeg"), QLatin1String("mp3"));
        fileExtensionMap->insert(QStringLiteral("application/x-shockwave-flash"), QLatin1String("swf"));
        fileExtensionMap->insert(QStringLiteral("application/x-pn-realmedia"), QLatin1String("rm"));
    }

    //for container names like avi instead of video/x-msvideo, use it as extension
    if (!mimeType.contains(QLatin1Char('/')))
        return mimeType;

    QString format = mimeType.left(mimeType.indexOf(QLatin1Char(',')));
    QString extension = fileExtensionMap->value(format);

    if (!extension.isEmpty() || format.isEmpty())
        return extension;

    QRegularExpression rx(QStringLiteral("[-/]([\\w]+)$"));
    QRegularExpressionMatch match = rx.match(format);

    if (match.hasMatch())
        extension = match.captured(1);

    return extension;
}

QVariant QGstUtils::fromGStreamerOrientation(const QVariant &value)
{
    // Note gstreamer tokens either describe the counter clockwise rotation of the
    // image or the clockwise transform to apply to correct the image.  The orientation
    // value returned is the clockwise rotation of the image.
    const QString token = value.toString();
    if (token == QStringLiteral("rotate-90"))
        return 270;
    if (token == QStringLiteral("rotate-180"))
        return 180;
    if (token == QStringLiteral("rotate-270"))
        return 90;
    return 0;
}

QVariant QGstUtils::toGStreamerOrientation(const QVariant &value)
{
    switch (value.toInt()) {
    case 90:
        return QStringLiteral("rotate-270");
    case 180:
        return QStringLiteral("rotate-180");
    case 270:
        return QStringLiteral("rotate-90");
    default:
        return QStringLiteral("rotate-0");
    }
}

bool QGstUtils::useOpenGL()
{
    static bool result = qEnvironmentVariableIntValue("QT_GSTREAMER_USE_OPENGL_PLUGIN");
    return result;
}

void qt_gst_object_ref_sink(gpointer object)
{
    gst_object_ref_sink(object);
}

GstCaps *qt_gst_pad_get_current_caps(GstPad *pad)
{
    return gst_pad_get_current_caps(pad);
}

GstCaps *qt_gst_pad_get_caps(GstPad *pad)
{
    return gst_pad_query_caps(pad, nullptr);
}

GstStructure *qt_gst_structure_new_empty(const char *name)
{
    return gst_structure_new_empty(name);
}

gboolean qt_gst_element_query_position(GstElement *element, GstFormat format, gint64 *cur)
{
    return gst_element_query_position(element, format, cur);
}

gboolean qt_gst_element_query_duration(GstElement *element, GstFormat format, gint64 *cur)
{
    return gst_element_query_duration(element, format, cur);
}

GstCaps *qt_gst_caps_normalize(GstCaps *caps)
{
    // gst_caps_normalize() takes ownership of the argument in 1.0
    return gst_caps_normalize(caps);
}

const gchar *qt_gst_element_get_factory_name(GstElement *element)
{
    const gchar *name = 0;
    const GstElementFactory *factory = 0;

    if (element && (factory = gst_element_get_factory(element)))
        name = gst_plugin_feature_get_name(GST_PLUGIN_FEATURE(factory));

    return name;
}

gboolean qt_gst_caps_can_intersect(const GstCaps * caps1, const GstCaps * caps2)
{
    return gst_caps_can_intersect(caps1, caps2);
}

GList *qt_gst_video_sinks()
{
    GList *list = nullptr;

    list = gst_element_factory_list_get_elements(GST_ELEMENT_FACTORY_TYPE_SINK | GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO,
                                                 GST_RANK_MARGINAL);

    return list;
}

void qt_gst_util_double_to_fraction(gdouble src, gint *dest_n, gint *dest_d)
{
    gst_util_double_to_fraction(src, dest_n, dest_d);
}

QDebug operator <<(QDebug debug, GstCaps *caps)
{
    if (caps) {
        gchar *string = gst_caps_to_string(caps);
        debug = debug << string;
        g_free(string);
    }
    return debug;
}

QT_END_NAMESPACE
