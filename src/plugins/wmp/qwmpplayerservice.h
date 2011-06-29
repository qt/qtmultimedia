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

#ifndef QWMPPLAYERSERVICE_H
#define QWMPPLAYERSERVICE_H

#include "qwmpevents.h"

#include <qmediaservice.h>

QT_BEGIN_NAMESPACE
class QMediaMetaData;
class QMediaPlayerControl;
class QMediaPlaylist;
class QVideoWindowControl;
QT_END_NAMESPACE

#ifdef QWMP_EVR
class QEvrVideoOverlay;
#endif

class QWmpMetaData;
class QWmpPlayerControl;
class QWmpPlaylist;
class QWmpPlaylistControl;
class QWmpVideoOverlay;

QT_USE_NAMESPACE
class QWmpPlayerService
    : public QMediaService
    , public IOleClientSite
    , public IServiceProvider
    , public IWMPRemoteMediaServices
{
    Q_OBJECT
public:
    enum EmbedMode
    {
        LocalEmbed,
        RemoteEmbed
    };

    QWmpPlayerService(EmbedMode mode, QObject *parent = 0);
    ~QWmpPlayerService();

    QMediaControl *requestControl(const char *name);
    void releaseControl(QMediaControl *control);

    // IUnknown
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **object);
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

    // IOleClientSite
    HRESULT STDMETHODCALLTYPE SaveObject();
    HRESULT STDMETHODCALLTYPE GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker, IMoniker **ppmk);
    HRESULT STDMETHODCALLTYPE GetContainer(IOleContainer **ppContainer);
    HRESULT STDMETHODCALLTYPE ShowObject();
    HRESULT STDMETHODCALLTYPE OnShowWindow(BOOL fShow);
    HRESULT STDMETHODCALLTYPE RequestNewObjectLayout();

    // IServiceProvider
    HRESULT STDMETHODCALLTYPE QueryService(REFGUID guidService, REFIID riid, void **ppvObject);

    // IWMPRemoteMediaServices
    HRESULT STDMETHODCALLTYPE GetServiceType(BSTR *pbstrType);
    HRESULT STDMETHODCALLTYPE GetApplicationName(BSTR *pbstrName);
    HRESULT STDMETHODCALLTYPE GetScriptableObject(BSTR *pbstrName, IDispatch **ppDispatch);
    HRESULT STDMETHODCALLTYPE GetCustomUIMode(BSTR *pbstrFile);

private:
    volatile LONG m_ref;
    const EmbedMode m_embedMode;
    IWMPPlayer4 *m_player;
    IOleObject *m_oleObject;
    QWmpEvents *m_events;
    QWmpPlayerControl *m_control;
    QWmpMetaData *m_metaData;
    QWmpPlaylistControl *m_playlist;
    QVideoWindowControl *m_activeVideoOverlay;
    QWmpVideoOverlay *m_oleVideoOverlay;
#ifdef QWMP_EVR
    QEvrVideoOverlay *m_evrVideoOverlay;
#endif
};

#endif
