/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Mobility Components.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QWMPVIDEOOVERLAY_H
#define QWMPVIDEOOVERLAY_H

#include <qvideowindowcontrol.h>

#include "qwmpplayerservice.h"

#include <wmp.h>

QT_USE_NAMESPACE
class QWmpVideoOverlay
    : public QVideoWindowControl
    , public IOleInPlaceSite
    , public IOleInPlaceFrame
{
    Q_OBJECT
public:
    QWmpVideoOverlay(IWMPPlayer4 *player, IOleObject *object, QWmpPlayerService *service);
    ~QWmpVideoOverlay();

    WId winId() const;
    void setWinId(WId id);

    QRect displayRect() const;
    void setDisplayRect(const QRect &rect);

    bool isFullScreen() const;
    void setFullScreen(bool fullScreen);

    void repaint();

    QSize nativeSize() const;
    void setNativeSize(const QSize &size);

    Qt::AspectRatioMode aspectRatioMode() const;
    void setAspectRatioMode(Qt::AspectRatioMode mode);

    int brightness() const;
    void setBrightness(int brightness);

    int contrast() const;
    void setContrast(int contrast);

    int hue() const;
    void setHue(int hue);

    int saturation() const;
    void setSaturation(int saturation);

    // IUnknown
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **object);
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

    // IOleWindow
    HRESULT STDMETHODCALLTYPE GetWindow(HWND *phwnd);
    HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(BOOL fEnterMode);

    // IOleInPlaceSite
    HRESULT STDMETHODCALLTYPE CanInPlaceActivate();
    HRESULT STDMETHODCALLTYPE OnInPlaceActivate();
    HRESULT STDMETHODCALLTYPE OnUIActivate();
    HRESULT STDMETHODCALLTYPE GetWindowContext(
            IOleInPlaceFrame **ppFrame,
            IOleInPlaceUIWindow **ppDoc,
            LPRECT lprcPosRect,
            LPRECT lprcClipRect,
            LPOLEINPLACEFRAMEINFO lpFrameInfo);
    HRESULT STDMETHODCALLTYPE Scroll(SIZE scrollExtant);
    HRESULT STDMETHODCALLTYPE OnUIDeactivate(BOOL fUndoable);
    HRESULT STDMETHODCALLTYPE OnInPlaceDeactivate();
    HRESULT STDMETHODCALLTYPE DiscardUndoState();
    HRESULT STDMETHODCALLTYPE DeactivateAndUndo();
    HRESULT STDMETHODCALLTYPE OnPosRectChange(LPCRECT lprcPosRect);

    // IOleInPlaceUIWindow
    HRESULT STDMETHODCALLTYPE GetBorder(LPRECT lprectBorder);
    HRESULT STDMETHODCALLTYPE RequestBorderSpace(LPCBORDERWIDTHS pborderwidths);
    HRESULT STDMETHODCALLTYPE SetBorderSpace(LPCBORDERWIDTHS pborderwidths);
    HRESULT STDMETHODCALLTYPE SetActiveObject(
            IOleInPlaceActiveObject *pActiveObject, LPCOLESTR pszObjName);

    // IOleInPlaceFrame
    HRESULT STDMETHODCALLTYPE InsertMenus(HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths);
    HRESULT STDMETHODCALLTYPE SetMenu(HMENU hmenuShared, HOLEMENU holemenu, HWND hwndActiveObject);
    HRESULT STDMETHODCALLTYPE RemoveMenus(HMENU hmenuShared);
    HRESULT STDMETHODCALLTYPE SetStatusText(LPCOLESTR pszStatusText);
    HRESULT STDMETHODCALLTYPE EnableModeless(BOOL fEnable);
    HRESULT STDMETHODCALLTYPE TranslateAccelerator(LPMSG lpmsg, WORD wID);

private:
    QWmpPlayerService *m_service;
	IWMPPlayer4 *m_player;
    IOleObject *m_object;
    IOleInPlaceObject *m_inPlaceObject;
    WId m_winId;
    COLORREF m_windowColor;
    Qt::AspectRatioMode m_aspectRatioMode;
    QSize m_sizeHint;
    QRect m_displayRect;
    bool m_fullScreen;
};

#endif
