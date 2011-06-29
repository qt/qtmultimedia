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

#include "qwmpplayerservice.h"

#ifdef QWMP_EVR
#include "qevrvideooverlay.h"
#endif

#include "qwmpglobal.h"
#include "qwmpmetadata.h"
#include "qwmpplayercontrol.h"
#include "qwmpplaylist.h"
#include "qwmpplaylistcontrol.h"
#include "qwmpvideooverlay.h"

#include <qmediaplayer.h>

#include <QtCore/qcoreapplication.h>
#include <QtCore/quuid.h>
#include <QtCore/qvariant.h>
#include <QtGui/qevent.h>

#include <d3d9.h>
#include <wmprealestate.h>

QWmpPlayerService::QWmpPlayerService(EmbedMode mode, QObject *parent)
    : QMediaService(parent)
    , m_ref(1)
    , m_embedMode(mode)
    , m_player(0)
    , m_oleObject(0)
    , m_events(0)
    , m_control(0)
    , m_metaData(0)
    , m_playlist(0)
    , m_activeVideoOverlay(0)
    , m_oleVideoOverlay(0)
#ifdef QWMP_EVR
    , m_evrVideoOverlay(0)
#endif
{
    HRESULT hr;

    if ((hr = CoCreateInstance(
            __uuidof(WindowsMediaPlayer),
            0,
            CLSCTX_INPROC_SERVER,
            __uuidof(IWMPPlayer4),
            reinterpret_cast<void **>(&m_player))) != S_OK) {
        qWarning("failed to create media player control, %x: %s", hr, qwmp_error_string(hr));
    } else {
        m_events = new QWmpEvents(m_player);

        if ((hr = m_player->QueryInterface(
                __uuidof(IOleObject), reinterpret_cast<void **>(&m_oleObject))) != S_OK) {
            qWarning("No IOleObject interface, %x: %s", hr, qwmp_error_string(hr));
        } else if ((hr = m_oleObject->SetClientSite(this)) != S_OK) {
            qWarning("Failed to set site, %x: %s", hr, qwmp_error_string(hr));
        }

        if (m_embedMode == LocalEmbed)
            m_oleVideoOverlay = new QWmpVideoOverlay(m_player, m_oleObject, this);

        m_metaData = new QWmpMetaData(m_player, m_events);
        m_playlist = new QWmpPlaylistControl(m_player, m_events);
        m_control = new QWmpPlayerControl(m_player, m_events);
    }
}

QWmpPlayerService::~QWmpPlayerService()
{
    delete m_control;
    delete m_metaData;
    delete m_playlist;
    delete m_events;

    if (m_oleObject) {
        m_oleObject->SetClientSite(0);
        m_oleObject->Release();
        delete m_oleVideoOverlay;
    }

#ifdef QWMP_EVR
    delete m_evrVideoOverlay;
#endif


    if (m_player)
        m_player->Release();

    Q_ASSERT(m_ref == 1);
}

QMediaControl *QWmpPlayerService::requestControl(const char *name)
{
    if (qstrcmp(name, QMediaPlayerControl_iid) == 0) {
        return m_control;
    } else if (qstrcmp(name, QMetaDataReaderControl_iid) == 0) {
        return m_metaData;
    } else if (qstrcmp(name, QMediaPlaylistControl_iid) == 0) {
        return m_playlist;
    } else if (qstrcmp(name, QVideoWindowControl_iid) == 0
            && m_embedMode == LocalEmbed
            && m_player
            && !m_activeVideoOverlay) {
#ifdef QWMP_EVR
        IWMPVideoRenderConfig *config = 0;
        if (m_player->QueryInterface(
                __uuidof(IWMPVideoRenderConfig), reinterpret_cast<void **>(&config)) == S_OK) {
            if (HINSTANCE evrHwnd = LoadLibrary(L"evr")) {
                m_evrVideoOverlay = new QEvrVideoOverlay(evrHwnd);

                if (SUCCEEDED(config->put_presenterActivate(
                        static_cast<IMFActivate *>(m_evrVideoOverlay)))) {
                    connect(m_events, SIGNAL(OpenStateChange(long)),
                            m_evrVideoOverlay, SLOT(openStateChanged(long)));
                } else {
                    delete m_evrVideoOverlay;

                    m_evrVideoOverlay = 0;
                }
            }
            config->Release();
        }

        if (m_evrVideoOverlay) {
            m_activeVideoOverlay = m_evrVideoOverlay;

            return m_evrVideoOverlay;
        } else
#endif
        if (SUCCEEDED(m_player->put_uiMode(QAutoBStr(L"none")))) {
            m_activeVideoOverlay = m_oleVideoOverlay;

            return m_oleVideoOverlay;
        }
    }
    return 0;
}

void QWmpPlayerService::releaseControl(QMediaControl *control)
{
    if (!control) {
        qWarning("QMediaService::releaseControl():"
                " Attempted release of null control");
#ifdef QWMP_EVR
    } else if (control == m_evrVideoOverlay) {

        IWMPVideoRenderConfig *config = 0;
        if (m_player->QueryInterface(
                __uuidof(IWMPVideoRenderConfig), reinterpret_cast<void **>(&config)) == S_OK) {
            config->put_presenterActivate(0);
            config->Release();
        }

        delete m_evrVideoOverlay;

        m_evrVideoOverlay = 0;
        m_activeVideoOverlay = 0;
#endif
    } else if (control == m_oleVideoOverlay) {
        m_player->put_uiMode(QAutoBStr(L"invisible"));
        m_oleVideoOverlay->setWinId(0);

        m_activeVideoOverlay = 0;
    }
}

// IUnknown
HRESULT QWmpPlayerService::QueryInterface(REFIID riid, void **object)
{
    if (!object) {
        return E_POINTER;
    } else if (riid == __uuidof(IUnknown)
            || riid == __uuidof(IOleClientSite)) {
        *object = static_cast<IOleClientSite *>(this);
    } else if (riid == __uuidof(IServiceProvider)) {
        *object = static_cast<IServiceProvider *>(this);
    } else if (riid == __uuidof(IWMPRemoteMediaServices)) {
        *object = static_cast<IWMPRemoteMediaServices *>(this);
    } else if (riid == __uuidof(IOleWindow)
            || riid == __uuidof(IOleInPlaceSite)) {
        *object = static_cast<IOleInPlaceSite *>(m_oleVideoOverlay);
    } else if (riid == __uuidof(IOleInPlaceUIWindow)
            || riid == __uuidof(IOleInPlaceFrame)) {
        *object = static_cast<IOleInPlaceFrame *>(m_oleVideoOverlay);
    } else {
        *object = 0;
    }

    if (*object) {
        AddRef();

        return S_OK;
    } else {
        return E_NOINTERFACE;
    }
}

ULONG QWmpPlayerService::AddRef()
{
    return InterlockedIncrement(&m_ref);
}

ULONG QWmpPlayerService::Release()
{
    ULONG ref = InterlockedDecrement(&m_ref);

    Q_ASSERT(ref != 0);

    return ref;
}

// IOleClientSite
HRESULT QWmpPlayerService::SaveObject()
{
    return E_NOTIMPL;
}

HRESULT QWmpPlayerService::GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker, IMoniker **ppmk)
{
    Q_UNUSED(dwAssign);
    Q_UNUSED(dwWhichMoniker);
    Q_UNUSED(ppmk);

    return E_NOTIMPL;
}

HRESULT QWmpPlayerService::GetContainer(IOleContainer **ppContainer)
{
    if (!ppContainer) {
        return E_POINTER;
    } else {
        *ppContainer = 0;

        return E_NOINTERFACE;
    }
}

HRESULT QWmpPlayerService::ShowObject()
{
    return S_OK;
}

HRESULT QWmpPlayerService::OnShowWindow(BOOL fShow)
{
    Q_UNUSED(fShow);

    return S_OK;
}

HRESULT QWmpPlayerService::RequestNewObjectLayout()
{
    return E_NOTIMPL;
}

// IServiceProvider
HRESULT QWmpPlayerService::QueryService(REFGUID guidService, REFIID riid, void **ppvObject)
{
    Q_UNUSED(guidService);

    if (!ppvObject) {
        return E_POINTER;
    } else if (riid == __uuidof(IWMPRemoteMediaServices)) {
        *ppvObject = static_cast<IWMPRemoteMediaServices *>(this);

        AddRef();

        return S_OK;
    } else {
        return E_NOINTERFACE;
    }
}

// IWMPRemoteMediaServices
HRESULT QWmpPlayerService::GetServiceType(BSTR *pbstrType)
{
    if (!pbstrType) {
        return E_POINTER;
    } else if (m_embedMode == RemoteEmbed) {
        *pbstrType = ::SysAllocString(L"Remote");

        return S_OK;
    } else {
        *pbstrType = ::SysAllocString(L"Local");

        return S_OK;
    }
}

HRESULT QWmpPlayerService::GetApplicationName(BSTR *pbstrName)
{
    if (!pbstrName) {
        return E_POINTER;
    } else {
        *pbstrName = ::SysAllocString(static_cast<const wchar_t *>(
                QCoreApplication::applicationName().utf16()));

        return S_OK;
    }
}

HRESULT QWmpPlayerService::GetScriptableObject(BSTR *pbstrName, IDispatch **ppDispatch)
{
    Q_UNUSED(pbstrName);
    Q_UNUSED(ppDispatch);

    return E_NOTIMPL;
}

HRESULT QWmpPlayerService::GetCustomUIMode(BSTR *pbstrFile)
{
    Q_UNUSED(pbstrFile);

    return E_NOTIMPL;
}
