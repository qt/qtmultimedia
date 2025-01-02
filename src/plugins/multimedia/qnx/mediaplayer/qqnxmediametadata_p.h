// Copyright (C) 2016 Research In Motion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QQnxMediaMetaData_H
#define QQnxMediaMetaData_H

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

#include <QtCore/qglobal.h>
#include <QtCore/QSize>
#include <QtCore/QString>

typedef struct strm_dict strm_dict_t;

QT_BEGIN_NAMESPACE

class QQnxMediaMetaData
{
public:
    QQnxMediaMetaData();
    bool update(const strm_dict_t *dict);
    void clear();

    // Duration in milliseconds
    qlonglong duration() const;

    int height() const;
    int width() const;
    bool hasVideo() const;
    bool hasAudio() const;
    bool isSeekable() const;

    QString title() const;
    QString artist() const;
    QString comment() const;
    QString genre() const;
    int year() const;
    QString mediaType() const;
    int audioBitRate() const;
    int sampleRate() const;
    QString album() const;
    int track() const;
    QSize resolution() const;

private:
    qlonglong m_duration;
    int m_height;
    int m_width;
    int m_mediaType;
    float m_pixelWidth;
    float m_pixelHeight;
    bool m_seekable;
    QString m_title;
    QString m_artist;
    QString m_comment;
    QString m_genre;
    int m_year;
    int m_audioBitRate;
    int m_sampleRate;
    QString m_album;
    int m_track;
};

QT_END_NAMESPACE

#endif
