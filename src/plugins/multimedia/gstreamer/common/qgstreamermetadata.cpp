// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qgstreamermetadata_p.h"
#include <QDebug>
#include <QtMultimedia/qmediametadata.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qtimezone.h>

#include <gst/gstversion.h>
#include <qgstutils_p.h>
#include <qlocale.h>

QT_BEGIN_NAMESPACE

struct {
    const char *tag;
    QMediaMetaData::Key key;
} gstTagToMetaDataKey[] = {
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

    { nullptr, QMediaMetaData::Title }
};

static QMediaMetaData::Key tagToKey(const char *tag)
{
    auto *map = gstTagToMetaDataKey;
    while (map->tag) {
        if (!strcmp(map->tag, tag))
            return map->key;
        ++map;
    }
    return QMediaMetaData::Key(-1);
}

static const char *keyToTag(QMediaMetaData::Key key)
{
    auto *map = gstTagToMetaDataKey;
    while (map->tag) {
        if (map->key == key)
            return map->tag;
        ++map;
    }
    return nullptr;
}

//internal
static void addTagToMap(const GstTagList *list,
                        const gchar *tag,
                        gpointer user_data)
{
    QMediaMetaData::Key key = tagToKey(tag);
    if (key == QMediaMetaData::Key(-1))
        return;

    auto *map = reinterpret_cast<QHash<QMediaMetaData::Key, QVariant>* >(user_data);

    GValue val;
    val.g_type = 0;
    gst_tag_list_copy_value(&val, list, tag);


    switch( G_VALUE_TYPE(&val) ) {
        case G_TYPE_STRING:
        {
            const gchar *str_value = g_value_get_string(&val);
            if (key == QMediaMetaData::Language) {
                map->insert(key, QVariant::fromValue(QLocale::codeToLanguage(QString::fromUtf8(str_value), QLocale::ISO639Part2)));
                break;
            }
            map->insert(key, QString::fromUtf8(str_value));
            break;
        }
        case G_TYPE_INT:
            map->insert(key, g_value_get_int(&val));
            break;
        case G_TYPE_UINT:
            map->insert(key, g_value_get_uint(&val));
            break;
        case G_TYPE_LONG:
            map->insert(key, qint64(g_value_get_long(&val)));
            break;
        case G_TYPE_BOOLEAN:
            map->insert(key, g_value_get_boolean(&val));
            break;
        case G_TYPE_CHAR:
            map->insert(key, g_value_get_schar(&val));
            break;
        case G_TYPE_DOUBLE:
            map->insert(key, g_value_get_double(&val));
            break;
        default:
            // GST_TYPE_DATE is a function, not a constant, so pull it out of the switch
            if (G_VALUE_TYPE(&val) == G_TYPE_DATE) {
                const GDate *date = (const GDate *)g_value_get_boxed(&val);
                if (g_date_valid(date)) {
                    int year = g_date_get_year(date);
                    int month = g_date_get_month(date);
                    int day = g_date_get_day(date);
                    // don't insert if we already have a datetime.
                    if (!map->contains(key))
                        map->insert(key, QDateTime(QDate(year, month, day), QTime()));
                }
            } else if (G_VALUE_TYPE(&val) == GST_TYPE_DATE_TIME) {
                const GstDateTime *dateTime = (const GstDateTime *)g_value_get_boxed(&val);
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
                QDateTime qDateTime(QDate(year, month, day), QTime(hour, minute, second),
                                    QTimeZone(tz * 60 * 60));
                map->insert(key, qDateTime);
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
                            map->insert(key, QImage::fromData(info.data, info.size, name));
                            gst_buffer_unmap(buffer, &info);
                        }
                    }
                }
            } else if (G_VALUE_TYPE(&val) == GST_TYPE_FRACTION) {
                int nom = gst_value_get_fraction_numerator(&val);
                int denom = gst_value_get_fraction_denominator(&val);

                if (denom > 0) {
                    map->insert(key, double(nom)/denom);
                }
            }
            break;
    }

    g_value_unset(&val);
}


QGstreamerMetaData QGstreamerMetaData::fromGstTagList(const GstTagList *tags)
{
    QGstreamerMetaData m;
    gst_tag_list_foreach(tags, addTagToMap, &m.data);
    return m;
}


void QGstreamerMetaData::setMetaData(GstElement *element) const
{
    if (!GST_IS_TAG_SETTER(element))
        return;

    gst_tag_setter_reset_tags(GST_TAG_SETTER(element));

    for (auto it = data.cbegin(), end = data.cend(); it != end; ++it) {
        const char *tagName = keyToTag(it.key());
        if (!tagName)
            continue;
        const QVariant &tagValue = it.value();

        switch (tagValue.typeId()) {
            case QMetaType::QString:
                gst_tag_setter_add_tags(GST_TAG_SETTER(element),
                    GST_TAG_MERGE_REPLACE,
                    tagName,
                    tagValue.toString().toUtf8().constData(),
                    nullptr);
                break;
            case QMetaType::Int:
            case QMetaType::LongLong:
                gst_tag_setter_add_tags(GST_TAG_SETTER(element),
                    GST_TAG_MERGE_REPLACE,
                    tagName,
                    tagValue.toInt(),
                    nullptr);
                break;
            case QMetaType::Double:
                gst_tag_setter_add_tags(GST_TAG_SETTER(element),
                    GST_TAG_MERGE_REPLACE,
                    tagName,
                    tagValue.toDouble(),
                    nullptr);
                break;
            case QMetaType::QDate:
            case QMetaType::QDateTime: {
                QDateTime date = tagValue.toDateTime();
                gst_tag_setter_add_tags(GST_TAG_SETTER(element),
                    GST_TAG_MERGE_REPLACE,
                    tagName,
                    gst_date_time_new(date.offsetFromUtc() / 60. / 60.,
                                date.date().year(), date.date().month(), date.date().day(),
                                date.time().hour(), date.time().minute(), date.time().second()),
                    nullptr);
                break;
            }
            default: {
                if (tagValue.typeId() == qMetaTypeId<QLocale::Language>()) {
                    QByteArray language = QLocale::languageToCode(tagValue.value<QLocale::Language>(), QLocale::ISO639Part2).toUtf8();
                    gst_tag_setter_add_tags(GST_TAG_SETTER(element),
                                            GST_TAG_MERGE_REPLACE,
                                            tagName,
                                            language.constData(),
                                            nullptr);
                }

                break;
            }
        }
    }
}

void QGstreamerMetaData::setMetaData(GstBin *bin) const
{
    GstIterator *elements = gst_bin_iterate_all_by_interface(bin, GST_TYPE_TAG_SETTER);
    GValue item = G_VALUE_INIT;
    while (gst_iterator_next(elements, &item) == GST_ITERATOR_OK) {
        GstElement * const element = GST_ELEMENT(g_value_get_object(&item));
        setMetaData(element);
    }
    gst_iterator_free(elements);
}


QT_END_NAMESPACE
