/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

#ifndef QMEDIAMETADATA_H
#define QMEDIAMETADATA_H

#include <QtCore/qpair.h>
#include <QtCore/qvariant.h>
#include <QtCore/qstring.h>
#include <QtCore/qhash.h>
#include <QtMultimedia/qtmultimediaglobal.h>

QT_BEGIN_NAMESPACE

// Class forward declaration required for QDoc bug
class QString;

class Q_MULTIMEDIA_EXPORT QMediaMetaData
{
    Q_GADGET
public:
    enum Key {
        Title,
        Author,
        Comment,
        Description,
        Genre,
        Date,

        Language,
        Publisher,
        Copyright,
        Url,

        Duration,
        MediaType,
        FileFormat,

        AudioBitRate,
        AudioCodec,
        VideoBitRate,
        VideoCodec,
        VideoFrameRate,

        AlbumTitle,
        AlbumArtist,
        ContributingArtist,
        TrackNumber,
        Composer,
        LeadPerformer,

        ThumbnailImage,
        CoverArtImage,

        Orientation,
        Resolution
    };
    Q_ENUM(Key)

    static constexpr int NumMetaData = Resolution + 1;

//    QMetaType typeForKey(Key k);
    Q_INVOKABLE QVariant value(Key k) const { return data.value(k); }
    Q_INVOKABLE void insert(Key k, const QVariant &value) { data.insert(k, value); }
    Q_INVOKABLE void remove(Key k) { data.remove(k); }
    Q_INVOKABLE QList<Key> keys() const { return data.keys(); }

    QVariant &operator[](Key k) { return data[k]; }
    Q_INVOKABLE void clear() { data.clear(); }

    Q_INVOKABLE bool isEmpty() const { return data.isEmpty(); }
    Q_INVOKABLE QString stringValue(Key k) const;

    Q_INVOKABLE static QString metaDataKeyToString(Key k);

protected:
    friend bool operator==(const QMediaMetaData &a, const QMediaMetaData &b)
    { return a.data == b.data; }
    friend bool operator!=(const QMediaMetaData &a, const QMediaMetaData &b)
    { return a.data != b.data; }

    QHash<Key, QVariant> data;
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QMediaMetaData)

#endif // QMEDIAMETADATA_H
