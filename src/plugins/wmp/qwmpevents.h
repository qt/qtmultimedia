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

#ifndef QWMPEVENTS_H
#define QWMPEVENTS_H

#include <QtCore/qobject.h>

#include <wmp.h>

class QWmpEvents : public QObject, public IWMPEvents3
{
    Q_OBJECT
public:
    QWmpEvents(IUnknown *source, QObject *parent = 0);
    ~QWmpEvents();

    // IUnknown
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject);
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

Q_SIGNALS:

#ifndef Q_MOC_RUN
    // IWMPEvents
    void STDMETHODCALLTYPE OpenStateChange(long NewState);
    void STDMETHODCALLTYPE PlayStateChange(long NewState);
    void STDMETHODCALLTYPE AudioLanguageChange(long LangID);
    void STDMETHODCALLTYPE StatusChange();
    void STDMETHODCALLTYPE ScriptCommand(BSTR scType, BSTR Param);
    void STDMETHODCALLTYPE NewStream();    
    void STDMETHODCALLTYPE Disconnect(long Result);
    void STDMETHODCALLTYPE Buffering(VARIANT_BOOL Start);
    void STDMETHODCALLTYPE Error();
    void STDMETHODCALLTYPE Warning(long WarningType, long Param, BSTR Description);
    void STDMETHODCALLTYPE EndOfStream(long Result);
    void STDMETHODCALLTYPE PositionChange(double oldPosition, double newPosition);
    void STDMETHODCALLTYPE MarkerHit(long MarkerNum);
    void STDMETHODCALLTYPE DurationUnitChange(long NewDurationUnit);
    void STDMETHODCALLTYPE CdromMediaChange(long CdromNum);
    void STDMETHODCALLTYPE PlaylistChange(IDispatch *Playlist, WMPPlaylistChangeEventType change);
    void STDMETHODCALLTYPE CurrentPlaylistChange(WMPPlaylistChangeEventType change);
    void STDMETHODCALLTYPE CurrentPlaylistItemAvailable(BSTR bstrItemName);
    void STDMETHODCALLTYPE MediaChange(IDispatch *Item);
    void STDMETHODCALLTYPE CurrentMediaItemAvailable(BSTR bstrItemName);
    void STDMETHODCALLTYPE CurrentItemChange(IDispatch *pdispMedia);
    void STDMETHODCALLTYPE MediaCollectionChange();
    void STDMETHODCALLTYPE MediaCollectionAttributeStringAdded(
            BSTR bstrAttribName, BSTR bstrAttribVal);
    void STDMETHODCALLTYPE MediaCollectionAttributeStringRemoved(
            BSTR bstrAttribName, BSTR bstrAttribVal);
    void STDMETHODCALLTYPE MediaCollectionAttributeStringChanged(
            BSTR bstrAttribName, BSTR bstrOldAttribVal, BSTR bstrNewAttribVal);
    void STDMETHODCALLTYPE PlaylistCollectionChange();
    void STDMETHODCALLTYPE PlaylistCollectionPlaylistAdded(BSTR bstrPlaylistName);
    void STDMETHODCALLTYPE PlaylistCollectionPlaylistRemoved(BSTR bstrPlaylistName);
    void STDMETHODCALLTYPE PlaylistCollectionPlaylistSetAsDeleted(
            BSTR bstrPlaylistName, VARIANT_BOOL varfIsDeleted);
    void STDMETHODCALLTYPE ModeChange(BSTR ModeName, VARIANT_BOOL NewValue);
    void STDMETHODCALLTYPE MediaError(IDispatch *pMediaObject);
    void STDMETHODCALLTYPE OpenPlaylistSwitch(IDispatch *pItem);
    void STDMETHODCALLTYPE DomainChange(BSTR strDomain);
    void STDMETHODCALLTYPE SwitchedToPlayerApplication();
    void STDMETHODCALLTYPE SwitchedToControl();
    void STDMETHODCALLTYPE PlayerDockedStateChange();
    void STDMETHODCALLTYPE PlayerReconnect();
    void STDMETHODCALLTYPE Click(short nButton, short nShiftState, long fX, long fY);
    void STDMETHODCALLTYPE DoubleClick(short nButton, short nShiftState, long fX, long fY);
    void STDMETHODCALLTYPE KeyDown(short nKeyCode, short nShiftState);
    void STDMETHODCALLTYPE KeyPress(short nKeyAscii);
    void STDMETHODCALLTYPE KeyUp(short nKeyCode, short nShiftState);
    void STDMETHODCALLTYPE MouseDown(short nButton, short nShiftState, long fX, long fY);
    void STDMETHODCALLTYPE MouseMove(short nButton, short nShiftState, long fX, long fY);
    void STDMETHODCALLTYPE MouseUp(short nButton, short nShiftState, long fX, long fY);

    // IWMPEvents2
    void STDMETHODCALLTYPE DeviceConnect(IWMPSyncDevice *pDevice);
    void STDMETHODCALLTYPE DeviceDisconnect(IWMPSyncDevice *pDevice);
    void STDMETHODCALLTYPE DeviceStatusChange(IWMPSyncDevice *pDevice, WMPDeviceStatus NewStatus);
    void STDMETHODCALLTYPE DeviceSyncStateChange(IWMPSyncDevice *pDevice, WMPSyncState NewState);
    void STDMETHODCALLTYPE DeviceSyncError(IWMPSyncDevice *pDevice, IDispatch *pMedia);
    void STDMETHODCALLTYPE CreatePartnershipComplete(IWMPSyncDevice *pDevice, HRESULT hrResult);

    // IWMPEvents3
    void STDMETHODCALLTYPE CdromRipStateChange(IWMPCdromRip *pCdromRip, WMPRipState wmprs);
    void STDMETHODCALLTYPE CdromRipMediaError(IWMPCdromRip *pCdromRip, IDispatch *pMedia);
    void STDMETHODCALLTYPE CdromBurnStateChange(IWMPCdromBurn *pCdromBurn, WMPBurnState wmpbs);
    void STDMETHODCALLTYPE CdromBurnMediaError(IWMPCdromBurn *pCdromBurn, IDispatch *pMedia);
    void STDMETHODCALLTYPE CdromBurnError(IWMPCdromBurn *pCdromBurn, HRESULT hrError);
    void STDMETHODCALLTYPE LibraryConnect(IWMPLibrary *pLibrary);
    void STDMETHODCALLTYPE LibraryDisconnect(IWMPLibrary *pLibrary);
    void STDMETHODCALLTYPE FolderScanStateChange(WMPFolderScanState wmpfss);
    void STDMETHODCALLTYPE StringCollectionChange(
            IDispatch *pdispStringCollection,
            WMPStringCollectionChangeEventType change,
            long lCollectionIndex);
    void STDMETHODCALLTYPE MediaCollectionMediaAdded(IDispatch *pdispMedia);    
    void STDMETHODCALLTYPE MediaCollectionMediaRemoved(IDispatch *pdispMedia);
#else
    // Declare again without STDMETHODCALLTYPE for moc's benefit.

    // IWMPEvents
    void OpenStateChange(long NewState);
    void PlayStateChange(long NewState);
    void AudioLanguageChange(long LangID);
    void StatusChange();
    void ScriptCommand(BSTR scType, BSTR Param);
    void NewStream();
    void Disconnect(long Result);
    void Buffering(VARIANT_BOOL Start);
    void Error();
    void Warning(long WarningType, long Param, BSTR Description);
    void EndOfStream(long Result);
    void PositionChange(double oldPosition, double newPosition);
    void MarkerHit(long MarkerNum);
    void DurationUnitChange(long NewDurationUnit);
    void CdromMediaChange(long CdromNum);
    void PlaylistChange(IDispatch *Playlist, WMPPlaylistChangeEventType change);
    void CurrentPlaylistChange(WMPPlaylistChangeEventType change);
    void CurrentPlaylistItemAvailable(BSTR bstrItemName);
    void MediaChange(IDispatch *Item);
    void CurrentMediaItemAvailable(BSTR bstrItemName);
    void CurrentItemChange(IDispatch *pdispMedia);
    void MediaCollectionChange();
    void MediaCollectionAttributeStringAdded(
            BSTR bstrAttribName, BSTR bstrAttribVal);
    void MediaCollectionAttributeStringRemoved(
            BSTR bstrAttribName, BSTR bstrAttribVal);
    void MediaCollectionAttributeStringChanged(
            BSTR bstrAttribName, BSTR bstrOldAttribVal, BSTR bstrNewAttribVal);
    void PlaylistCollectionChange();
    void PlaylistCollectionPlaylistAdded(BSTR bstrPlaylistName);
    void PlaylistCollectionPlaylistRemoved(BSTR bstrPlaylistName);
    void PlaylistCollectionPlaylistSetAsDeleted(
            BSTR bstrPlaylistName, VARIANT_BOOL varfIsDeleted);
    void ModeChange(BSTR ModeName, VARIANT_BOOL NewValue);
    void MediaError(IDispatch *pMediaObject);
    void OpenPlaylistSwitch(IDispatch *pItem);
    void DomainChange(BSTR strDomain);
    void SwitchedToPlayerApplication();
    void SwitchedToControl();
    void PlayerDockedStateChange();
    void PlayerReconnect();
    void Click(short nButton, short nShiftState, long fX, long fY);
    void DoubleClick(short nButton, short nShiftState, long fX, long fY);
    void KeyDown(short nKeyCode, short nShiftState);
    void KeyPress(short nKeyAscii);
    void KeyUp(short nKeyCode, short nShiftState);
    void MouseDown(short nButton, short nShiftState, long fX, long fY);
    void MouseMove(short nButton, short nShiftState, long fX, long fY);
    void MouseUp(short nButton, short nShiftState, long fX, long fY);

    // IWMPEvents2
    void DeviceConnect(IWMPSyncDevice *pDevice);
    void DeviceDisconnect(IWMPSyncDevice *pDevice);
    void DeviceStatusChange(IWMPSyncDevice *pDevice, WMPDeviceStatus NewStatus);
    void DeviceSyncStateChange(IWMPSyncDevice *pDevice, WMPSyncState NewState);
    void DeviceSyncError(IWMPSyncDevice *pDevice, IDispatch *pMedia);
    void CreatePartnershipComplete(IWMPSyncDevice *pDevice, HRESULT hrResult);

    // IWMPEvents3
    void CdromRipStateChange(IWMPCdromRip *pCdromRip, WMPRipState wmprs);
    void CdromRipMediaError(IWMPCdromRip *pCdromRip, IDispatch *pMedia);
    void CdromBurnStateChange(IWMPCdromBurn *pCdromBurn, WMPBurnState wmpbs);
    void CdromBurnMediaError(IWMPCdromBurn *pCdromBurn, IDispatch *pMedia);
    void CdromBurnError(IWMPCdromBurn *pCdromBurn, HRESULT hrError);
    void LibraryConnect(IWMPLibrary *pLibrary);
    void LibraryDisconnect(IWMPLibrary *pLibrary);
    void FolderScanStateChange(WMPFolderScanState wmpfss);
    void StringCollectionChange(
            IDispatch *pdispStringCollection,
            WMPStringCollectionChangeEventType change,
            long lCollectionIndex);
    void MediaCollectionMediaAdded(IDispatch *pdispMedia);
    void MediaCollectionMediaRemoved(IDispatch *pdispMedia);
#endif
private:
    volatile LONG m_ref;
    IConnectionPoint *m_connectionPoint;
    DWORD m_adviseCookie;
};

#endif
