/****************************************************************************
**
** Copyright (C) 2016 Research In Motion
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
#ifndef MMRENDERERMETADATA_H
#define MMRENDERERMETADATA_H

#include <QtCore/qglobal.h>
#include <QtCore/QSize>
#include <QtCore/QString>

typedef struct strm_dict strm_dict_t;

QT_BEGIN_NAMESPACE

class MmRendererMetaData
{
public:
    MmRendererMetaData();
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
