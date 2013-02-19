/****************************************************************************
**
** Copyright (C) 2012 Research In Motion
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
#ifndef BBMETADATA_H
#define BBMETADATA_H

#include <QtCore/qglobal.h>
#include <QtCore/QSize>
#include <QtCore/QString>

QT_BEGIN_NAMESPACE

class BbMetaData
{
public:
    BbMetaData();
    bool parse(const QString &contextName);
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
