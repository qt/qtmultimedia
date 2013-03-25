/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef JMEDIAMETADATARETRIEVER_H
#define JMEDIAMETADATARETRIEVER_H

#include <QtPlatformSupport/private/qjniobject_p.h>
#include <qurl.h>

QT_BEGIN_NAMESPACE

class JMediaMetadataRetriever : public QJNIObject
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

    JMediaMetadataRetriever();
    ~JMediaMetadataRetriever();

    QString extractMetadata(MetadataKey key);
    void release();
    bool setDataSource(const QUrl &url);
    bool setDataSource(const QString &path);

};

QT_END_NAMESPACE

#endif // JMEDIAMETADATARETRIEVER_H
