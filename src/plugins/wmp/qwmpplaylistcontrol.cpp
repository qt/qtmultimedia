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

#include "qwmpplaylistcontrol.h"

#include "qwmpevents.h"
#include "qwmpglobal.h"
#include "qwmpmetadata.h"
#include "qwmpplaylist.h"

QWmpPlaylistControl::QWmpPlaylistControl(IWMPCore3 *player, QWmpEvents *events, QObject *parent)
    : QMediaPlaylistControl(parent)
    , m_player(player)
    , m_controls(0)
    , m_playlist(new QWmpPlaylist(player, events))
    , m_playbackMode(QMediaPlaylist::Sequential)
{
    m_player->get_controls(&m_controls);

    connect(events, SIGNAL(CurrentItemChange(IDispatch*)),
            this, SLOT(currentItemChangeEvent(IDispatch*)));
}

QWmpPlaylistControl::~QWmpPlaylistControl()
{
    if (m_controls)
        m_controls->Release();

    delete m_playlist;
}

QMediaPlaylistProvider *QWmpPlaylistControl::playlistProvider() const
{
    return m_playlist;
}

bool QWmpPlaylistControl::setPlaylistProvider(QMediaPlaylistProvider *playlist)
{
    Q_UNUSED(playlist);

    return false;
}

int QWmpPlaylistControl::currentIndex() const
{
    int position = 0;

    IWMPMedia *media = 0;
    if (m_controls && m_player->get_currentMedia(&media) == S_OK) {
        position = QWmpMetaData::value(media, QAutoBStr(L"PlaylistIndex")).toInt();

        media->Release();
    }

    return position;
}

void QWmpPlaylistControl::setCurrentIndex(int position)
{

    IWMPPlaylist *playlist = 0;
    if (m_player->get_currentPlaylist(&playlist) == S_OK) {
        IWMPMedia *media = 0;
        if (playlist->get_item(position, &media) == S_OK) {
            m_player->put_currentMedia(media);

            media->Release();
        }
        playlist->Release();
    }
}

int QWmpPlaylistControl::nextIndex(int steps) const
{
    return currentIndex() + steps;
}

int QWmpPlaylistControl::previousIndex(int steps) const
{
    return currentIndex() - steps;
}

void QWmpPlaylistControl::next()
{
    if (m_controls)
        m_controls->next();
}

void QWmpPlaylistControl::previous()
{
    if (m_controls)
        m_controls->previous();
}

QMediaPlaylist::PlaybackMode QWmpPlaylistControl::playbackMode() const
{
    return m_playbackMode;
}

void QWmpPlaylistControl::setPlaybackMode(QMediaPlaylist::PlaybackMode mode)
{
    m_playbackMode = mode;
}

void QWmpPlaylistControl::currentItemChangeEvent(IDispatch *dispatch)
{
    IWMPMedia *media = 0;
    if (dispatch && dispatch->QueryInterface(
            __uuidof(IWMPMedia), reinterpret_cast<void **>(&media)) == S_OK) {
        int index = QWmpMetaData::value(media, QAutoBStr(L"PlaylistIndex")).toInt();

        emit currentIndexChanged(index);
        emit currentMediaChanged(m_playlist->media(index));
    }
}
