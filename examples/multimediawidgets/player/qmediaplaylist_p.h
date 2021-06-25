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
