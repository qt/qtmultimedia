/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
****************************************************************************/

#ifndef ANDROIDMEDIAMETADATARETRIEVER_H
#define ANDROIDMEDIAMETADATARETRIEVER_H

#include <QtCore/private/qjni_p.h>

QT_BEGIN_NAMESPACE

class AndroidMediaMetadataRetriever
{
public:
    enum MetadataKey {
        Album = 1,
        AlbumArtist = 13,
        Artist = 2,
        Author = 3,
        Bitrate = 20,
        CDTrackNumber = 0,
        Compilation = 15,
        Composer = 4,
        Date = 5,
        DiscNumber = 14,
        Duration = 9,
        Genre = 6,
        HasAudio = 16,
        HasVideo = 17,
        Location = 23,
        MimeType = 12,
        NumTracks = 10,
        Title = 7,
        VideoHeight = 19,
        VideoWidth = 18,
        VideoRotation = 24,
        Writer = 11,
        Year = 8
    };

    AndroidMediaMetadataRetriever();
    ~AndroidMediaMetadataRetriever();

    QString extractMetadata(MetadataKey key);
    bool setDataSource(const QUrl &url);

private:
    void release();
    QJNIObjectPrivate m_metadataRetriever;
};

QT_END_NAMESPACE

#endif // ANDROIDMEDIAMETADATARETRIEVER_H
