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

#ifndef QEVRVIDEOOVERLAY_H
#define QEVRVIDEOOVERLAY_H

#include <qvideowindowcontrol.h>

#include "qmfactivate.h"

#include <evr.h>

QT_USE_NAMESPACE

class QEvrVideoOverlay : public QVideoWindowControl, public QMFActivate
{
    Q_OBJECT
public:
    QEvrVideoOverlay(HINSTANCE evrHwnd);
    ~QEvrVideoOverlay();

    WId winId() const;
    void setWinId(WId id);

    QRect displayRect() const;
    void setDisplayRect(const QRect &rect);

    bool isFullScreen() const;
    void setFullScreen(bool fullScreen);

    QSize nativeSize() const;

    Qt::AspectRatioMode aspectRatioMode() const;
    void setAspectRatioMode(Qt::AspectRatioMode mode);

    void repaint();

    int brightness() const;
    void setBrightness(int brightness);

    int contrast() const;
    void setContrast(int contrast);

    int hue() const;
    void setHue(int hue);

    int saturation() const;
    void setSaturation(int saturation);

    void setDisplayControl(IMFVideoDisplayControl *control);

    // IUnknown
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **object);
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

    // IMFActivate
    HRESULT STDMETHODCALLTYPE ActivateObject(REFIID riid, void **ppv);
    HRESULT STDMETHODCALLTYPE ShutdownObject();
    HRESULT STDMETHODCALLTYPE DetachObject();

public Q_SLOTS:
    void openStateChanged(long state);

private:
    typedef HRESULT (WINAPI *PtrMFCreateVideoPresenter)(IUnknown*, REFIID, REFIID, void**);

    volatile LONG m_ref;
    HINSTANCE m_evrHwnd;
    PtrMFCreateVideoPresenter ptrMFCreateVideoPresenter;
    IMFVideoPresenter *m_presenter;
    IMFVideoDisplayControl *m_displayControl;
    Qt::AspectRatioMode m_aspectRatioMode;
    QSize m_sizeHint;
    QRect m_displayRect;
    WId m_winId;
    COLORREF m_windowColor;
    bool m_fullScreen;
};

#endif
