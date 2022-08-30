// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QMEDIAPLAYLIST_P_H
#define QMEDIAPLAYLIST_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qmediaplaylist.h"
#include "qplaylistfileparser_p.h"

#include <QtCore/qdebug.h>

#ifdef Q_MOC_RUN
# pragma Q_MOC_EXPAND_MACROS
#endif

QT_BEGIN_NAMESPACE


class QMediaPlaylistControl;

class QMediaPlaylistPrivate
{
    Q_DECLARE_PUBLIC(QMediaPlaylist)
public:
    QMediaPlaylistPrivate()
        : error(QMediaPlaylist::NoError)
    {
    }

    virtual ~QMediaPlaylistPrivate()
    {
        if (parser)
            delete parser;
    }

    void loadFailed(QMediaPlaylist::Error error, const QString &errorString)
    {
        this->error = error;
        this->errorString = errorString;

        emit q_ptr->loadFailed();
    }

    void loadFinished()
    {
        q_ptr->addMedia(parser->playlist);

        emit q_ptr->loaded();
    }

    bool checkFormat(const char *format) const
    {
        QLatin1String f(format);
        QPlaylistFileParser::FileType type = format ? QPlaylistFileParser::UNKNOWN : QPlaylistFileParser::M3U8;
        if (format) {
            if (f == QLatin1String("m3u") || f == QLatin1String("text/uri-list") ||
                f == QLatin1String("audio/x-mpegurl") || f == QLatin1String("audio/mpegurl"))
                type = QPlaylistFileParser::M3U;
            else if (f == QLatin1String("m3u8") || f == QLatin1String("application/x-mpegURL") ||
                     f == QLatin1String("application/vnd.apple.mpegurl"))
                type = QPlaylistFileParser::M3U8;
        }

        if (type == QPlaylistFileParser::UNKNOWN || type == QPlaylistFileParser::PLS) {
            error = QMediaPlaylist::FormatNotSupportedError;
            errorString = QMediaPlaylist::tr("This file format is not supported.");
            return false;
        }
        return true;
    }

    void ensureParser()
    {
        if (parser)
            return;

        parser = new QPlaylistFileParser(q_ptr);
        QObject::connect(parser, &QPlaylistFileParser::finished, [this]() { loadFinished(); });
        QObject::connect(parser, &QPlaylistFileParser::error,
                [this](QMediaPlaylist::Error err, const QString& errorMsg) { loadFailed(err, errorMsg); });
    }

    int nextPosition(int steps) const;
    int prevPosition(int steps) const;

    QList<QUrl> playlist;

    int currentPos = -1;
    QMediaPlaylist::PlaybackMode playbackMode = QMediaPlaylist::Sequential;

    QPlaylistFileParser *parser = nullptr;
    mutable QMediaPlaylist::Error error;
    mutable QString errorString;

    QMediaPlaylist *q_ptr;
};

QT_END_NAMESPACE


#endif // QMEDIAPLAYLIST_P_H
