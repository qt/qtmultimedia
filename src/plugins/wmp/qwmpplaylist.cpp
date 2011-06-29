/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Mobility Components.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qwmpplaylist.h"

#include "qwmpevents.h"
#include "qwmpmetadata.h"
#include "qwmpglobal.h"

#include <QtCore/qstringlist.h>
#include <QtCore/qurl.h>
#include <QtCore/qvariant.h>

QWmpPlaylist::QWmpPlaylist(IWMPCore3 *player, QWmpEvents *events, QObject *parent)
    : QMediaPlaylistProvider(parent)
    , m_player(player)
    , m_playlist(0)
    , m_count(0)
{
    if (m_player && m_player->get_currentPlaylist(&m_playlist) == S_OK)
        m_playlist->get_count(&m_count);

    connect(events, SIGNAL(CurrentPlaylistChange(WMPPlaylistChangeEventType)),
            this, SLOT(currentPlaylistChangeEvent(WMPPlaylistChangeEventType)));
    connect(events, SIGNAL(OpenPlaylistSwitch(IDispatch*)),
            this, SLOT(openPlaylistChangeEvent(IDispatch*)));
    connect(events, SIGNAL(MediaChange(IDispatch*)), this, SLOT(mediaChangeEvent(IDispatch*)));
}

QWmpPlaylist::~QWmpPlaylist()
{
    if (m_playlist)
        m_playlist->Release();
}

bool QWmpPlaylist::load(const QString &location, const char *format)
{
    Q_UNUSED(location);
    Q_UNUSED(format);

    return false;
}

bool QWmpPlaylist::load(QIODevice * device, const char *format)
{
    Q_UNUSED(device);
    Q_UNUSED(format);

    return false;
}

bool QWmpPlaylist::save(const QString &location, const char *format)
{
    Q_UNUSED(location);
    Q_UNUSED(format);

    return false;
}

bool QWmpPlaylist::save(QIODevice * device, const char *format)
{
    Q_UNUSED(device);
    Q_UNUSED(format);

    return false;
}

int QWmpPlaylist::mediaCount() const
{
    return m_count;
}

QMediaContent QWmpPlaylist::media(int pos) const
{
    QMediaContent content;

    IWMPMedia *media = 0;
    if (m_playlist && m_playlist->get_item(pos, &media) == S_OK) {
        content = QWmpMetaData::resources(media);

        media->Release();
    }

    return content;
}

bool QWmpPlaylist::isReadOnly() const
{
    return false;
}

bool QWmpPlaylist::addMedia(const QMediaContent &content)
{
    bool appended = false;

    IWMPMedia *media = 0;
    if (!content.isNull() && m_playlist && m_player && m_player->newMedia(
            QAutoBStr(content.canonicalUrl()), &media) == S_OK) {
        appended = m_playlist->appendItem(media) == S_OK;

        media->Release();
    }

    return appended;
}

bool QWmpPlaylist::insertMedia(int pos, const QMediaContent &content)
{
    bool inserted = false;

    IWMPMedia *media = 0;
    if (m_playlist && m_player && m_player->newMedia(
            QAutoBStr(content.canonicalUrl()), &media) == S_OK) {
        inserted = m_playlist->insertItem(pos, media) == S_OK;

        media->Release();
    }

    return inserted;
}

bool QWmpPlaylist::removeMedia(int pos)
{
    IWMPMedia *media = 0;
    if (m_playlist->get_item(pos, &media) == S_OK) {
        bool removed = m_playlist->removeItem(media) == S_OK;

        media->Release();

        return removed;
    } else {
        return false;
    }
}

bool QWmpPlaylist::removeMedia(int start, int end)
{
    if (!m_playlist)
        return false;

    for (int i = start; i <= end; ++i) {
        IWMPMedia *media = 0;
        if (m_playlist->get_item(start, &media) == S_OK) {
            bool removed = m_playlist->removeItem(media) == S_OK;

            media->Release();

            if (!removed)
                return false;
        }
    }
    return true;
}

bool QWmpPlaylist::clear()
{
    return m_playlist && m_playlist->clear() == S_OK;
}

QStringList QWmpPlaylist::keys(int index) const
{
    QStringList keys;

    IWMPMedia *media = 0;
    if (m_playlist && m_playlist->get_item(index, &media) == S_OK) {
        keys = QWmpMetaData::keys(media);

        media->Release();
    }

    return keys;
}

QVariant QWmpPlaylist::value(int index, const QString &key) const
{
    QVariant v;
    
    IWMPMedia *media = 0;
    if (m_playlist && m_playlist->get_item(index, &media) == S_OK) {
        v = QWmpMetaData::value(media, QAutoBStr(key));

        media->Release();
    }

    return v;
}

void QWmpPlaylist::shuffle()
{
}


void QWmpPlaylist::currentPlaylistChangeEvent(WMPPlaylistChangeEventType change)
{
    Q_UNUSED(change);

    long count = 0;
    if (m_playlist && m_playlist->get_count(&count) == S_OK && count > 0) {
        if (count > m_count) {
            emit mediaAboutToBeInserted(m_count, count - 1);
            m_count = count;
            emit mediaInserted(count, m_count - 1);
        } else if (count < m_count) {
            emit mediaAboutToBeRemoved(count, m_count - 1);
            m_count = count;
            emit mediaRemoved(count, m_count - 1);
        }
    }
    if (m_count > 0)
        emit mediaChanged(0, m_count - 1);
}

void QWmpPlaylist::openPlaylistChangeEvent(IDispatch *dispatch)
{
    if (m_playlist && m_count > 0) {
        emit mediaAboutToBeRemoved(0, m_count - 1);
        m_playlist->Release();
        m_playlist = 0;
        m_count = 0;
        emit mediaRemoved(0, m_count - 1);
    } else if (m_playlist) {
        m_playlist->Release();
        m_playlist = 0;
    }

    IWMPPlaylist *playlist = 0;
    if (dispatch && dispatch->QueryInterface(
            __uuidof(IWMPPlaylist), reinterpret_cast<void **>(&playlist))) {

        long count = 0;
        if (playlist->get_count(&count) == S_OK && count > 0) {
            emit mediaAboutToBeInserted(0, count - 1);
            m_playlist = playlist;
            m_count = count;
            emit mediaInserted(0, count - 1);
        } else {
            m_playlist = playlist;
        }
    }
}

void QWmpPlaylist::mediaChangeEvent(IDispatch *dispatch)
{

    IWMPMedia *media = 0;
    if (dispatch &&  dispatch->QueryInterface(
            __uuidof(IWMPMedia), reinterpret_cast<void **>(&media)) == S_OK) {
        VARIANT_BOOL isMember = VARIANT_FALSE;

        if (media->isMemberOf(m_playlist, &isMember) == S_OK && isMember) {
            int index = QWmpMetaData::value(media, QAutoBStr(L"PlaylistIndex")).toInt();

            if (index >= 0)
                emit mediaChanged(index, index);
        }
        media->Release();
    }
}
