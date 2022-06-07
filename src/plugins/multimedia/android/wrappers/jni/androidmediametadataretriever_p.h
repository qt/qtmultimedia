// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef ANDROIDMEDIAMETADATARETRIEVER_H
#define ANDROIDMEDIAMETADATARETRIEVER_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/private/qglobal_p.h>
#include <QtCore/qurl.h>
#include <QtCore/qjniobject.h>

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
    QJniObject m_metadataRetriever;
};

QT_END_NAMESPACE

#endif // ANDROIDMEDIAMETADATARETRIEVER_H
