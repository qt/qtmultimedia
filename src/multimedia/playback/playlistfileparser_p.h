/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef PLAYLISTFILEPARSER_P_H
#define PLAYLISTFILEPARSER_P_H

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

#include <QtNetwork>
#include "qtmultimediadefs.h"

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

class QPlaylistFileParserPrivate;

class Q_MULTIMEDIA_EXPORT QPlaylistFileParser : public QObject
{
    Q_OBJECT
public:
    QPlaylistFileParser(QObject *parent = 0);

    enum FileType
    {
        UNKNOWN,
        M3U,
        M3U8, // UTF-8 version of M3U
        PLS
    };

    enum ParserError
    {
        NoError,
        FormatError,
        FormatNotSupportedError,
        NetworkError
    };

    static FileType findPlaylistType(const QString& uri, const QString& mime, const void *data, quint32 size);

    void start(const QNetworkRequest &request, bool utf8 = false);
    void stop();

Q_SIGNALS:
    void newItem(const QVariant& content);
    void finished();
    void error(QPlaylistFileParser::ParserError err, const QString& errorMsg);

protected:
    QPlaylistFileParserPrivate *d_ptr;

private:
    Q_DISABLE_COPY(QPlaylistFileParser)
    Q_DECLARE_PRIVATE(QPlaylistFileParser)
    Q_PRIVATE_SLOT(d_func(), void _q_handleData())
    Q_PRIVATE_SLOT(d_func(), void _q_handleError())
    Q_PRIVATE_SLOT(d_func(), void _q_handleParserError(QPlaylistFileParser::ParserError err, const QString& errorMsg))
    Q_PRIVATE_SLOT(d_func(), void _q_handleParserFinished())
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // PLAYLISTFILEPARSER_P_H
