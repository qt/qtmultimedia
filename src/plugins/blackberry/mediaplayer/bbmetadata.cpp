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
#include "bbmetadata.h"

#include <QtCore/qdebug.h>
#include <QtCore/qfile.h>
#include <QtCore/qstringlist.h>

QT_BEGIN_NAMESPACE

BbMetaData::BbMetaData()
{
    clear();
}

static const char * titleKey = "md_title_name";
static const char * artistKey = "md_title_artist";
static const char * commentKey = "md_title_comment";
static const char * genreKey = "md_title_genre";
static const char * yearKey = "md_title_year";
static const char * durationKey = "md_title_duration";
static const char * bitRateKey = "md_title_bitrate";
static const char * sampleKey = "md_title_samplerate";
static const char * albumKey = "md_title_album";
static const char * trackKey = "md_title_track";
static const char * widthKey = "md_video_width";
static const char * heightKey = "md_video_height";
static const char * mediaTypeKey = "md_title_mediatype";
static const char * pixelWidthKey = "md_video_pixel_height";
static const char * pixelHeightKey = "md_video_pixel_width";
static const char * seekableKey = "md_title_seekable";

static const int mediaTypeAudioFlag = 4;
static const int mediaTypeVideoFlag = 2;

bool BbMetaData::parse(const QString &contextName)
{
    clear();
    QString fileName =
            QString("/pps/services/multimedia/renderer/context/%1/metadata").arg(contextName);

    // In newer OS versions, the filename is "metadata0", not metadata, so try both.
    if (!QFile::exists(fileName))
        fileName += '0';

    QFile metaDataFile(fileName);
    if (!metaDataFile.open(QFile::ReadOnly)) {
        qWarning() << "Unable to open media metadata file" << fileName << ":"
                   << metaDataFile.errorString();
        return false;
    }

    const QString separator("::");
    QTextStream stream(&metaDataFile);
    Q_FOREVER {
        const QString line = stream.readLine();
        if (line.isNull())
            break;

        const int separatorPos = line.indexOf(separator);
        if (separatorPos != -1) {
            const QString key = line.left(separatorPos);
            const QString value = line.mid(separatorPos + separator.length());

            if (key == durationKey)
                m_duration = value.toLongLong();
            else if (key == widthKey)
                m_width = value.toInt();
            else if (key == heightKey)
                m_height = value.toInt();
            else if (key == mediaTypeKey)
                m_mediaType = value.toInt();
            else if (key == pixelWidthKey)
                m_pixelWidth = value.toFloat();
            else if (key == pixelHeightKey)
                m_pixelHeight = value.toFloat();
            else if (key == titleKey)
                m_title = value;
            else if (key == seekableKey)
                m_seekable = !(value == QLatin1String("0"));
            else if (key == artistKey)
                m_artist = value;
            else if (key == commentKey)
                m_comment = value;
            else if (key == genreKey)
                m_genre = value;
            else if (key == yearKey)
                m_year = value.toInt();
            else if (key == bitRateKey)
                m_audioBitRate = value.toInt();
            else if (key == sampleKey)
                m_sampleRate = value.toInt();
            else if (key == albumKey)
                m_album = value;
            else if (key == trackKey)
                m_track = value.toInt();
        }
    }

    return true;
}

void BbMetaData::clear()
{
    m_duration = 0;
    m_height = 0;
    m_width = 0;
    m_mediaType = -1;
    m_pixelWidth = 1;
    m_pixelHeight = 1;
    m_seekable = true;
    m_title.clear();
    m_artist.clear();
    m_comment.clear();
    m_genre.clear();
    m_year = 0;
    m_audioBitRate = 0;
    m_sampleRate = 0;
    m_album.clear();
    m_track = 0;
}

qlonglong BbMetaData::duration() const
{
    return m_duration;
}

// Handling of pixel aspect ratio
//
// If the pixel aspect ratio is different from 1:1, it means the video needs to be stretched in
// order to look natural.
// For example, if the pixel width is 2, and the pixel height is 1, it means a video of 300x200
// pixels needs to be displayed as 600x200 to look correct.
// In order to support this the easiest way, we simply pretend that the actual size of the video
// is 600x200, which will cause the video to be displayed in an aspect ratio of 3:1 instead of 3:2,
// and therefore look correct.

int BbMetaData::height() const
{
    return m_height * m_pixelHeight;
}

int BbMetaData::width() const
{
    return m_width * m_pixelWidth;
}

bool BbMetaData::hasVideo() const
{
    // By default, assume no video if we can't extract the information
    if (m_mediaType == -1)
        return false;

    return (m_mediaType & mediaTypeVideoFlag);
}

bool BbMetaData::hasAudio() const
{
    // By default, assume audio only if we can't extract the information
    if (m_mediaType == -1)
        return true;

    return (m_mediaType & mediaTypeAudioFlag);
}

QString BbMetaData::title() const
{
    return m_title;
}

bool BbMetaData::isSeekable() const
{
    return m_seekable;
}

QString BbMetaData::artist() const
{
    return m_artist;
}

QString BbMetaData::comment() const
{
    return m_comment;
}

QString BbMetaData::genre() const
{
    return m_genre;
}

int BbMetaData::year() const
{
    return m_year;
}

QString BbMetaData::mediaType() const
{
    if (hasVideo())
        return QLatin1String("video");
    else if (hasAudio())
        return QLatin1String("audio");
    else
        return QString();
}

int BbMetaData::audioBitRate() const
{
    return m_audioBitRate;
}

int BbMetaData::sampleRate() const
{
    return m_sampleRate;
}

QString BbMetaData::album() const
{
    return m_album;
}

int BbMetaData::track() const
{
    return m_track;
}

QSize BbMetaData::resolution() const
{
    return QSize(width(), height());
}

QT_END_NAMESPACE
