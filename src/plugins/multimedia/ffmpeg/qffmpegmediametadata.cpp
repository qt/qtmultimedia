// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpegmediametadata_p.h"
#include <QDebug>
#include <QtCore/qdatetime.h>
#include <qstringlist.h>
#include <qurl.h>
#include <qlocale.h>

#include <qloggingcategory.h>

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(qLcMetaData, "qt.multimedia.ffmpeg.metadata")

namespace  {

struct ffmpegTagToMetaDataKey
{
    const char *tag;
    QMediaMetaData::Key key;
};

constexpr ffmpegTagToMetaDataKey ffmpegTagToMetaDataKey[] = {
    { "title", QMediaMetaData::Title },
    { "comment", QMediaMetaData::Comment },
    { "description", QMediaMetaData::Description },
    { "genre", QMediaMetaData::Genre },
    { "date", QMediaMetaData::Date },

    { "language", QMediaMetaData::Language },

    { "copyright", QMediaMetaData::Copyright },

    // Music
    { "album", QMediaMetaData::AlbumTitle },
    { "album_artist", QMediaMetaData::AlbumArtist },
    { "artist", QMediaMetaData::ContributingArtist },
    { "track", QMediaMetaData::TrackNumber },

    // Movie
    { "performer", QMediaMetaData::LeadPerformer },

    { nullptr, QMediaMetaData::Title }
};

} // namespace

static QMediaMetaData::Key tagToKey(const char *tag)
{
    const auto *map = ffmpegTagToMetaDataKey;
    while (map->tag) {
        if (!strcmp(map->tag, tag))
            return map->key;
        ++map;
    }
    return QMediaMetaData::Key(-1);
}

static const char *keyToTag(QMediaMetaData::Key key)
{
    const auto *map = ffmpegTagToMetaDataKey;
    while (map->tag) {
        if (map->key == key)
            return map->tag;
        ++map;
    }
    return nullptr;
}

static QDateTime getRecordingTime(const AVDictionary *tags)
{
    constexpr std::array prioritizedDateTags = {
        "date",                             // Time of work creation provided by FFmpeg
        "com.apple.quicktime.creationdate", // QuickTime creation time
        "year",                             // Year of work, prioritized after more specific times
        "creation_time",                    // Time of file creation or encoding, prioritized last
    };

    AVDictionaryEntry *entry = nullptr;

    for (const char *dateTag : prioritizedDateTags) {
        using namespace std::string_view_literals;
        entry = av_dict_get(tags, dateTag, nullptr, 0);
        if (!entry)
            continue;
        else if (entry->key == "year"sv)
            return QDateTime(QDate(QByteArray(entry->value).toInt(), 1, 1), QTime(0, 0, 0));
        else
            return QDateTime::fromString(QString::fromUtf8(entry->value), Qt::ISODate);
    }

    return QDateTime();
}

//internal
void QFFmpegMetaData::addEntry(QMediaMetaData &metaData, AVDictionaryEntry *entry)
{
    qCDebug(qLcMetaData) << "   checking:" << entry->key << entry->value;
    QByteArray tag(entry->key);
    QMediaMetaData::Key key = tagToKey(tag.toLower().constData());
    if (key == QMediaMetaData::Key(-1))
        return;
    qCDebug(qLcMetaData) << "       adding" << key;

    auto *map = &metaData;

    int metaTypeId = keyType(key).id();
    switch (metaTypeId) {
    case qMetaTypeId<QString>():
        map->insert(key, QString::fromUtf8(entry->value));
        return;
    case qMetaTypeId<QStringList>():
        map->insert(key, QString::fromUtf8(entry->value).split(QLatin1Char(',')));
        return;
    case qMetaTypeId<QDateTime>():
        // Dates have their own handling
        return;
    case qMetaTypeId<QUrl>():
        map->insert(key, QUrl::fromEncoded(entry->value));
        return;
    case qMetaTypeId<qint64>():
        map->insert(key, (qint64)QByteArray(entry->value).toLongLong());
        return;
    case qMetaTypeId<int>():
        map->insert(key, QByteArray(entry->value).toInt());
        return;
    case qMetaTypeId<qreal>():
        map->insert(key, (qreal)QByteArray(entry->value).toDouble());
        return;
    default:
        break;
    }
    if (metaTypeId == qMetaTypeId<QLocale::Language>()) {
        map->insert(key, QVariant::fromValue(QLocale::codeToLanguage(QString::fromUtf8(entry->value), QLocale::ISO639Part2)));
    }
}


QMediaMetaData QFFmpegMetaData::fromAVMetaData(const AVDictionary *tags)
{
    QMediaMetaData metaData;
    AVDictionaryEntry *entry = nullptr;
    while ((entry = av_dict_get(tags, "", entry, AV_DICT_IGNORE_SUFFIX)))
        addEntry(metaData, entry);

    // Get date from provided tags in order of priority
    QDateTime date = getRecordingTime(tags);
    if (!date.isNull())
        metaData.insert(QMediaMetaData::Date, date);

    return metaData;
}

QByteArray QFFmpegMetaData::value(const QMediaMetaData &metaData, QMediaMetaData::Key key)
{
    const int metaTypeId = keyType(key).id();
    const QVariant val = metaData.value(key);
    switch (metaTypeId) {
    case qMetaTypeId<QString>():
        return val.toString().toUtf8();
    case qMetaTypeId<QStringList>():
        return val.toStringList().join(u",").toUtf8();
    case qMetaTypeId<QDateTime>():
        return val.toDateTime().toString(Qt::ISODate).toUtf8();
    case qMetaTypeId<QUrl>():
        return val.toUrl().toEncoded();
    case qMetaTypeId<qint64>():
    case qMetaTypeId<int>():
        return QByteArray::number(val.toLongLong());
    case qMetaTypeId<qreal>():
        return QByteArray::number(val.toDouble());
    default:
        break;
    }
    if (metaTypeId == qMetaTypeId<QLocale::Language>())
        return QLocale::languageToCode(val.value<QLocale::Language>(), QLocale::ISO639Part2).toUtf8();
    return {};
}


AVDictionary *QFFmpegMetaData::toAVMetaData(const QMediaMetaData &metaData)
{
    AVDictionary *dict = nullptr;
    for (const auto &&[k, v] : metaData.asKeyValueRange()) {
        const char *key = ::keyToTag(k);
        if (!key)
            continue;
        QByteArray val = value(metaData, k);
        if (val.isEmpty())
            continue;
        av_dict_set(&dict, key, val.constData(), 0);
    }
    return dict;
}



QT_END_NAMESPACE
