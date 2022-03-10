/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "qffmpegmediametadata_p.h"
#include <QDebug>
#include <QtCore/qdatetime.h>
#include <qstringlist.h>
#include <qurl.h>
#include <private/qiso639_2_p.h>

QT_BEGIN_NAMESPACE

namespace  {

struct {
    const char *tag;
    QMediaMetaData::Key key;
} ffmpegTagToMetaDataKey[] = {
    { "title", QMediaMetaData::Title },
    { "comment", QMediaMetaData::Comment },
    { "description", QMediaMetaData::Description },
    { "genre", QMediaMetaData::Genre },
    { "date", QMediaMetaData::Date },
    { "year", QMediaMetaData::Date },
    { "creation_time", QMediaMetaData::Date },

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

}

static QMediaMetaData::Key tagToKey(const char *tag)
{
    auto *map = ffmpegTagToMetaDataKey;
    while (map->tag) {
        if (!strcmp(map->tag, tag))
            return map->key;
        ++map;
    }
    return QMediaMetaData::Key(-1);
}

#if 0
static const char *keyToTag(QMediaMetaData::Key key)
{
    auto *map = ffmpegTagToMetaDataKey;
    while (map->tag) {
        if (map->key == key)
            return map->tag;
        ++map;
    }
    return nullptr;
}
#endif

//internal
void QFFmpegMetaData::addEntry(QMediaMetaData &metaData, AVDictionaryEntry *entry)
{
//    qDebug() << "   checking:" << entry->key << entry->value;
    QByteArray tag(entry->key);
    QMediaMetaData::Key key = tagToKey(tag.toLower());
    if (key == QMediaMetaData::Key(-1))
        return;
//    qDebug() << "       adding" << key;

    auto *map = &metaData;

    int metaTypeId = keyType(key).id();
    switch (metaTypeId) {
    case qMetaTypeId<QString>():
        map->insert(key, QString::fromUtf8(entry->value));
        return;
    case qMetaTypeId<QStringList>():
        map->insert(key, QString::fromUtf8(entry->value).split(QLatin1Char(',')));
        return;
    case qMetaTypeId<QDateTime>(): {
        QDateTime date;
        if (!qstrcmp(entry->key, "year")) {
            if (map->keys().contains(QMediaMetaData::Date))
                return;
            date = QDateTime(QDate(QByteArray(entry->value).toInt(), 1, 1), QTime(0, 0, 0));
        } else {
            date = QDateTime::fromString(QString::fromUtf8(entry->value), Qt::ISODate);
        }
        map->insert(key, date);
        return;
    }
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
        map->insert(key, QVariant::fromValue(QtMultimediaPrivate::fromIso639(entry->value)));
    }
}


QMediaMetaData QFFmpegMetaData::fromAVMetaData(const AVDictionary *tags)
{
    QMediaMetaData metaData;
    AVDictionaryEntry *entry = nullptr;
    while ((entry = av_dict_get(tags, "", entry, AV_DICT_IGNORE_SUFFIX)))
        addEntry(metaData, entry);

    return metaData;
}



QT_END_NAMESPACE
