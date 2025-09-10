// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qgstreamermetadata_p.h"
#include <QtMultimedia/qmediametadata.h>
#include <QtMultimedia/qtvideo.h>
#include <QtCore/qdebug.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qlocale.h>
#include <QtCore/qtimezone.h>
#include <QtGui/qimage.h>

#include <gst/gstversion.h>
#include <common/qgst_handle_types_p.h>
#include <common/qgstutils_p.h>
#include <qgstreamerformatinfo_p.h>

QT_BEGIN_NAMESPACE

RotationResult parseRotationTag(std::string_view tag)
{
    using namespace std::string_view_literals;
    Q_ASSERT(!tag.empty());

    if (tag[0] == 'r') {
        if (tag == "rotate-90"sv)
            return { QtVideo::Rotation::Clockwise90, false };
        if (tag == "rotate-180"sv)
            return { QtVideo::Rotation::Clockwise180, false };
        if (tag == "rotate-270"sv)
            return { QtVideo::Rotation::Clockwise270, false };
        if (tag == "rotate-0"sv)
            return { QtVideo::Rotation::None, false };
    }
    if (tag[0] == 'f') {
        // To flip by horizontal axis is the same as to mirror by vertical axis
        // and rotate by 180 degrees.

        if (tag == "flip-rotate-90"sv)
            return { QtVideo::Rotation::Clockwise270, true };
        if (tag == "flip-rotate-180"sv)
            return { QtVideo::Rotation::None, true };
        if (tag == "flip-rotate-270"sv)
            return { QtVideo::Rotation::Clockwise90, true };
        if (tag == "flip-rotate-0"sv)
            return { QtVideo::Rotation::Clockwise180, true };
    }

    qCritical() << "cannot parse orientation: {}" << tag;
    return { QtVideo::Rotation::None, false };
}

namespace {

namespace MetadataLookupImpl {

#ifdef __cpp_lib_constexpr_algorithms
#  define constexpr_lookup constexpr
#else
#  define constexpr_lookup /*constexpr*/
#endif

struct MetadataKeyValuePair
{
    const char *tag;
    QMediaMetaData::Key key;
};

constexpr const char *toTag(const char *t)
{
    return t;
}
constexpr const char *toTag(const MetadataKeyValuePair &kv)
{
    return kv.tag;
}

constexpr QMediaMetaData::Key toKey(QMediaMetaData::Key k)
{
    return k;
}
constexpr QMediaMetaData::Key toKey(const MetadataKeyValuePair &kv)
{
    return kv.key;
}

constexpr auto compareByKey = [](const auto &lhs, const auto &rhs) {
    return toKey(lhs) < toKey(rhs);
};

constexpr auto compareByTag = [](const auto &lhs, const auto &rhs) {
    return std::strcmp(toTag(lhs), toTag(rhs)) < 0;
};

constexpr_lookup auto makeLookupTable()
{
    std::array<MetadataKeyValuePair, 22> lookupTable{ {
            { GST_TAG_TITLE, QMediaMetaData::Title },
            { GST_TAG_COMMENT, QMediaMetaData::Comment },
            { GST_TAG_DESCRIPTION, QMediaMetaData::Description },
            { GST_TAG_GENRE, QMediaMetaData::Genre },
            { GST_TAG_DATE_TIME, QMediaMetaData::Date },
            { GST_TAG_DATE, QMediaMetaData::Date },

            { GST_TAG_LANGUAGE_CODE, QMediaMetaData::Language },

            { GST_TAG_ORGANIZATION, QMediaMetaData::Publisher },
            { GST_TAG_COPYRIGHT, QMediaMetaData::Copyright },

            // Media
            { GST_TAG_DURATION, QMediaMetaData::Duration },

            // Audio
            { GST_TAG_BITRATE, QMediaMetaData::AudioBitRate },
            { GST_TAG_AUDIO_CODEC, QMediaMetaData::AudioCodec },

            // Music
            { GST_TAG_ALBUM, QMediaMetaData::AlbumTitle },
            { GST_TAG_ALBUM_ARTIST, QMediaMetaData::AlbumArtist },
            { GST_TAG_ARTIST, QMediaMetaData::ContributingArtist },
            { GST_TAG_TRACK_NUMBER, QMediaMetaData::TrackNumber },

            { GST_TAG_PREVIEW_IMAGE, QMediaMetaData::ThumbnailImage },
            { GST_TAG_IMAGE, QMediaMetaData::CoverArtImage },

            // Image/Video
            { "resolution", QMediaMetaData::Resolution },
            { GST_TAG_IMAGE_ORIENTATION, QMediaMetaData::Orientation },

            // Video
            { GST_TAG_VIDEO_CODEC, QMediaMetaData::VideoCodec },

            // Movie
            { GST_TAG_PERFORMER, QMediaMetaData::LeadPerformer },
    } };

    std::sort(lookupTable.begin(), lookupTable.end(),
              [](const MetadataKeyValuePair &lhs, const MetadataKeyValuePair &rhs) {
                  return std::string_view(lhs.tag) < std::string_view(rhs.tag);
              });
    return lookupTable;
}

constexpr_lookup auto gstTagToMetaDataKey = makeLookupTable();
constexpr_lookup auto metaDataKeyToGstTag = [] {
    auto array = gstTagToMetaDataKey;
    std::sort(array.begin(), array.end(), compareByKey);
    return array;
}();

} // namespace MetadataLookupImpl

QMediaMetaData::Key tagToKey(const char *tag)
{
    if (tag == nullptr)
        return QMediaMetaData::Key(-1);

    using namespace MetadataLookupImpl;
    auto foundIterator = std::lower_bound(gstTagToMetaDataKey.begin(), gstTagToMetaDataKey.end(),
                                          tag, compareByTag);
    if (std::strcmp(foundIterator->tag, tag) == 0)
        return foundIterator->key;

    return QMediaMetaData::Key(-1);
}

const char *keyToTag(QMediaMetaData::Key key)
{
    using namespace MetadataLookupImpl;
    auto foundIterator = std::lower_bound(metaDataKeyToGstTag.begin(), metaDataKeyToGstTag.end(),
                                          key, compareByKey);
    if (foundIterator->key == key)
        return foundIterator->tag;

    return nullptr;
}

#undef constexpr_lookup

QDateTime parseDate(const GDate *date)
{
    if (!g_date_valid(date))
        return {};

    int year = g_date_get_year(date);
    int month = g_date_get_month(date);
    int day = g_date_get_day(date);
    return QDateTime(QDate(year, month, day), QTime());
}

QDateTime parseDate(const GValue &val)
{
    Q_ASSERT(G_VALUE_TYPE(&val) == G_TYPE_DATE);
    const GDate *date = (const GDate *)g_value_get_boxed(&val);
    return parseDate(date);
}

QDateTime parseDateTime(const GstDateTime *dateTime)
{
    int year = gst_date_time_has_year(dateTime) ? gst_date_time_get_year(dateTime) : 0;
    int month = gst_date_time_has_month(dateTime) ? gst_date_time_get_month(dateTime) : 0;
    int day = gst_date_time_has_day(dateTime) ? gst_date_time_get_day(dateTime) : 0;
    int hour = 0;
    int minute = 0;
    int second = 0;
    float tz = 0;
    if (gst_date_time_has_time(dateTime)) {
        hour = gst_date_time_get_hour(dateTime);
        minute = gst_date_time_get_minute(dateTime);
        second = gst_date_time_get_second(dateTime);
        tz = gst_date_time_get_time_zone_offset(dateTime);
    }
    return QDateTime{
        QDate(year, month, day),
        QTime(hour, minute, second),
        QTimeZone(tz * 60 * 60),
    };
}

QDateTime parseDateTime(const GValue &val)
{
    Q_ASSERT(G_VALUE_TYPE(&val) == GST_TYPE_DATE_TIME);
    const GstDateTime *dateTime = (const GstDateTime *)g_value_get_boxed(&val);
    return parseDateTime(dateTime);
}

QImage parseImage(const GValue &val)
{
    Q_ASSERT(G_VALUE_TYPE(&val) == GST_TYPE_SAMPLE);

    GstSample *sample = (GstSample *)g_value_get_boxed(&val);
    GstCaps *caps = gst_sample_get_caps(sample);
    if (caps && !gst_caps_is_empty(caps)) {
        GstStructure *structure = gst_caps_get_structure(caps, 0);
        const gchar *name = gst_structure_get_name(structure);
        if (QByteArray(name).startsWith("image/")) {
            GstBuffer *buffer = gst_sample_get_buffer(sample);
            if (buffer) {
                GstMapInfo info;
                gst_buffer_map(buffer, &info, GST_MAP_READ);
                QImage image = QImage::fromData(info.data, info.size, name);
                gst_buffer_unmap(buffer, &info);
                return image;
            }
        }
    }

    return {};
}

std::optional<double> parseFractionAsDouble(const GValue &val)
{
    Q_ASSERT(G_VALUE_TYPE(&val) == GST_TYPE_FRACTION);

    int nom = gst_value_get_fraction_numerator(&val);
    int denom = gst_value_get_fraction_denominator(&val);
    if (denom == 0)
        return std::nullopt;
    return double(nom) / double(denom);
}

constexpr std::string_view extendedComment{ GST_TAG_EXTENDED_COMMENT };

void addTagsFromExtendedComment(const GstTagList *list, const gchar *tag, QMediaMetaData &metadata)
{
    using namespace Qt::Literals;
    assert(tag == extendedComment);

    int entryCount = gst_tag_list_get_tag_size(list, tag);
    for (int i = 0; i != entryCount; ++i) {
        const GValue *value = gst_tag_list_get_value_index(list, tag, i);

        const QLatin1StringView strValue{ g_value_get_string(value) };

        auto equalIndex = strValue.indexOf(QLatin1StringView("="));
        if (equalIndex == -1) {
            qDebug() << "Cannot parse GST_TAG_EXTENDED_COMMENT entry: " << value;
            continue;
        }

        const QLatin1StringView key = strValue.first(equalIndex);
        const QLatin1StringView valueString = strValue.last(strValue.size() - equalIndex - 1);

        if (key == "DURATION"_L1) {
            QGstDateTimeHandle duration{
                gst_date_time_new_from_iso8601_string(valueString.data()),
                QGstDateTimeHandle::HasRef,
            };

            if (duration) {
                using namespace std::chrono;

                auto chronoDuration = hours(gst_date_time_get_hour(duration.get()))
                        + minutes(gst_date_time_get_minute(duration.get()))
                        + seconds(gst_date_time_get_second(duration.get()))
                        + microseconds(gst_date_time_get_microsecond(duration.get()));

                metadata.insert(QMediaMetaData::Duration,
                                QVariant::fromValue(round<milliseconds>(chronoDuration).count()));
            }
        }
    }
}

void addTagToMetaData(const GstTagList *list, const gchar *tag, void *userdata)
{
    QMediaMetaData &metadata = *reinterpret_cast<QMediaMetaData *>(userdata);

    QMediaMetaData::Key key = tagToKey(tag);
    if (key == QMediaMetaData::Key::Date)
        return; // date/datetime are handled on a higher layer

    if (key == QMediaMetaData::Key(-1)) {
        if (tag == extendedComment)
            addTagsFromExtendedComment(list, tag, metadata);

        return;
    }

    GValue val{};
    gst_tag_list_copy_value(&val, list, tag);

    GType type = G_VALUE_TYPE(&val);

    if (auto entryCount = gst_tag_list_get_tag_size(list, tag) != 0; entryCount != 1)
        qWarning() << "addTagToMetaData: invaled entry count for" << tag << "-" << entryCount;

    if (type == G_TYPE_STRING) {
        const gchar *str_value = g_value_get_string(&val);

        switch (key) {
        case QMediaMetaData::Language: {
            metadata.insert(key, QVariant::fromValue(QGstUtils::codeToLanguage(str_value)));
            break;
        }
        case QMediaMetaData::Orientation: {
            RotationResult result = parseRotationTag(str_value);
            metadata.insert(key, QVariant::fromValue(result.rotation));
            break;
        }
        default:
            metadata.insert(key, QString::fromUtf8(str_value));
            break;
        };
    } else if (type == G_TYPE_INT) {
        metadata.insert(key, g_value_get_int(&val));
    } else if (type == G_TYPE_UINT) {
        metadata.insert(key, g_value_get_uint(&val));
    } else if (type == G_TYPE_LONG) {
        metadata.insert(key, qint64(g_value_get_long(&val)));
    } else if (type == G_TYPE_BOOLEAN) {
        metadata.insert(key, g_value_get_boolean(&val));
    } else if (type == G_TYPE_CHAR) {
        metadata.insert(key, g_value_get_schar(&val));
    } else if (type == G_TYPE_DOUBLE) {
        metadata.insert(key, g_value_get_double(&val));
    } else if (type == G_TYPE_DATE) {
        if (!metadata.keys().contains(key)) {
            QDateTime date = parseDate(val);
            if (date.isValid())
                metadata.insert(key, date);
        }
    } else if (type == GST_TYPE_DATE_TIME) {
        QDateTime date = parseDateTime(val);
        if (date.isValid())
            metadata.insert(key, parseDateTime(val));
    } else if (type == GST_TYPE_SAMPLE) {
        QImage image = parseImage(val);
        if (!image.isNull())
            metadata.insert(key, image);
    } else if (type == GST_TYPE_FRACTION) {
        std::optional<double> fraction = parseFractionAsDouble(val);

        if (fraction)
            metadata.insert(key, *fraction);
    }

    g_value_unset(&val);
}

} // namespace

QMediaMetaData taglistToMetaData(const QGstTagListHandle &handle)
{
    QMediaMetaData m;
    extendMetaDataFromTagList(m, handle);
    return m;
}

void extendMetaDataFromTagList(QMediaMetaData &metadata, const QGstTagListHandle &handle)
{
    if (handle) {
        // gstreamer has both GST_TAG_DATE_TIME and GST_TAG_DATE tags.
        // if both are present, we use GST_TAG_DATE_TIME, else we fall back to GST_TAG_DATE

        auto readDateTime = [&]() -> std::optional<QDateTime> {
            GstDateTime *dateTimeHandle{};
            gst_tag_list_get_date_time(handle.get(), GST_TAG_DATE_TIME, &dateTimeHandle);
            if (dateTimeHandle) {
                QDateTime ret = parseDateTime(dateTimeHandle);
                gst_date_time_unref(dateTimeHandle);
                if (ret.isValid())
                    return ret;
            }
            return std::nullopt;
        };

        auto readDate = [&]() -> std::optional<QDateTime> {
            GDate *dateHandle{};
            gst_tag_list_get_date(handle.get(), GST_TAG_DATE, &dateHandle);
            if (dateHandle) {
                QDateTime ret = parseDate(dateHandle);
                g_date_free(dateHandle);
                if (ret.isValid())
                    return ret;
            }
            return std::nullopt;
        };

        std::optional<QDateTime> date = readDateTime();
        if (!date)
            date = readDate();

        if (date)
            metadata.insert(QMediaMetaData::Key::Date, *date);

        gst_tag_list_foreach(handle.get(), reinterpret_cast<GstTagForeachFunc>(&addTagToMetaData),
                             &metadata);
    }
}

static void applyMetaDataToTagSetter(const QMediaMetaData &metadata, GstTagSetter *element)
{
    gst_tag_setter_reset_tags(element);

    for (QMediaMetaData::Key key : metadata.keys()) {
        const char *tagName = keyToTag(key);
        if (!tagName)
            continue;
        const QVariant &tagValue = metadata.value(key);

        auto setTag = [&](const auto &value) {
            gst_tag_setter_add_tags(element, GST_TAG_MERGE_REPLACE, tagName, value, nullptr);
        };

        switch (tagValue.typeId()) {
        case QMetaType::QString:
            setTag(tagValue.toString().toUtf8().constData());
            break;
        case QMetaType::Int:
        case QMetaType::LongLong:
            setTag(tagValue.toInt());
            break;
        case QMetaType::Double:
            setTag(tagValue.toDouble());
            break;

        case QMetaType::QDateTime: {
            // tagName does not properly disambiguate between GST_TAG_DATE_TIME and
            // GST_TAG_DATE, as both map to QMediaMetaData::Key::Date. so we set it accordingly to
            // the QVariant.

            QDateTime date = tagValue.toDateTime();

            QGstGstDateTimeHandle dateTime{
                gst_date_time_new(date.offsetFromUtc() / 60. / 60., date.date().year(),
                                  date.date().month(), date.date().day(), date.time().hour(),
                                  date.time().minute(), date.time().second()),
                QGstGstDateTimeHandle::HasRef,
            };

            gst_tag_setter_add_tags(element, GST_TAG_MERGE_REPLACE, GST_TAG_DATE_TIME,
                                    dateTime.get(), nullptr);
            break;
        }
        case QMetaType::QDate: {
            QDate date = tagValue.toDate();

            QUniqueGDateHandle dateHandle{
                g_date_new_dmy(date.day(), GDateMonth(date.month()), date.year()),
            };

            gst_tag_setter_add_tags(element, GST_TAG_MERGE_REPLACE, GST_TAG_DATE, dateHandle.get(),
                                    nullptr);
            break;
        }
        default: {
            if (tagValue.typeId() == qMetaTypeId<QLocale::Language>()) {
                QByteArray language = QLocale::languageToCode(tagValue.value<QLocale::Language>(),
                                                              QLocale::ISO639Part2)
                                              .toUtf8();
                setTag(language.constData());
            }

            break;
        }
        }
    }
}

void applyMetaDataToTagSetter(const QMediaMetaData &metadata, const QGstElement &element)
{
    GstTagSetter *tagSetter = qGstSafeCast<GstTagSetter>(element.element());
    if (tagSetter)
        applyMetaDataToTagSetter(metadata, tagSetter);
    else
        qWarning() << "applyMetaDataToTagSetter failed: element not a GstTagSetter"
                   << element.name();
}

void applyMetaDataToTagSetter(const QMediaMetaData &metadata, const QGstBin &bin)
{
    GstIterator *elements = gst_bin_iterate_all_by_interface(bin.bin(), GST_TYPE_TAG_SETTER);
    GValue item = {};

    while (gst_iterator_next(elements, &item) == GST_ITERATOR_OK) {
        GstElement *element = static_cast<GstElement *>(g_value_get_object(&item));
        if (!element)
            continue;

        GstTagSetter *tagSetter = qGstSafeCast<GstTagSetter>(element);

        if (tagSetter)
            applyMetaDataToTagSetter(metadata, tagSetter);
    }

    gst_iterator_free(elements);
}

void extendMetaDataFromCaps(QMediaMetaData &metadata, const QGstCaps &caps)
{
    QGstStructureView structure = caps.at(0);

    QMediaFormat::FileFormat fileFormat = QGstreamerFormatInfo::fileFormatForCaps(structure);
    if (fileFormat != QMediaFormat::FileFormat::UnspecifiedFormat) {
        // Container caps
        metadata.insert(QMediaMetaData::FileFormat, fileFormat);
        return;
    }

    QMediaFormat::AudioCodec audioCodec = QGstreamerFormatInfo::audioCodecForCaps(structure);
    if (audioCodec != QMediaFormat::AudioCodec::Unspecified) {
        // Audio stream caps
        metadata.insert(QMediaMetaData::AudioCodec, QVariant::fromValue(audioCodec));
        return;
    }

    QMediaFormat::VideoCodec videoCodec = QGstreamerFormatInfo::videoCodecForCaps(structure);
    if (videoCodec != QMediaFormat::VideoCodec::Unspecified) {
        // Video stream caps
        metadata.insert(QMediaMetaData::VideoCodec, QVariant::fromValue(videoCodec));
        std::optional<float> framerate = structure["framerate"].getFraction();
        if (framerate)
            metadata.insert(QMediaMetaData::VideoFrameRate, *framerate);

        QSize resolution = structure.resolution();
        if (resolution.isValid())
            metadata.insert(QMediaMetaData::Resolution, resolution);
    }
}

QMediaMetaData capsToMetaData(const QGstCaps &caps)
{
    QMediaMetaData metadata;
    extendMetaDataFromCaps(metadata, caps);
    return metadata;
}

QT_END_NAMESPACE
