/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the examples of the Qt Mobility Components.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
** $QT_END_LICENSE$
**
****************************************************************************/

#include "camerakeyevent_symbian.h"

#include <QtGui/QWidget>    // WId
#include <eikon.hrh>        // EKeyCamera
#include <coecntrl.h>       // CCoeControl (WId)
#include <w32std.h>         // RWindowbase, RWindowGroup, RWsSession

QSymbianCameraKeyListener::QSymbianCameraKeyListener(QWidget *widget):
    QObject(widget),
    m_widget(widget)
{
    if (!m_widget)
        return;

    // Get view's native Symbian window
    WId windowId = 0;
    if (m_widget->internalWinId())
        windowId = m_widget->internalWinId();
    else if (m_widget->parentWidget() && m_widget->effectiveWinId())
        windowId = m_widget->effectiveWinId();
    RWindowBase *window = windowId ? static_cast<RWindowBase*>(windowId->DrawableWindow()) : 0;

    // Get hold of the window group
    TInt wGroupId = window ? window->WindowGroupId() : 0;
    if (!wGroupId)
        return;
    RWsSession &wsSession = CCoeEnv::Static()->WsSession();
    TUint wGroupHandle = wsSession.GetWindowGroupHandle(wGroupId);
    if (wGroupHandle) {
        RWindowGroup wGroup(wsSession);
        wGroup.Construct(wGroupHandle);
        if (wGroup.CaptureKey(EKeyCamera, 0, 0, 100) < 0)
            qWarning("Unable to register for camera capture key events, SwEvent capability may be missing");
    }
}

QSymbianCameraKeyListener::~QSymbianCameraKeyListener()
{
    if (!m_widget)
        return;

    // Get view's native Symbian window
    WId windowId = 0;
    if (m_widget->internalWinId())
        windowId = m_widget->internalWinId();
    else if (m_widget->parentWidget() && m_widget->effectiveWinId())
        windowId = m_widget->effectiveWinId();
    RWindowBase *window = windowId ? static_cast<RWindowBase*>(windowId->DrawableWindow()) : 0;

    // Get hold of the window group
    TInt wGroupId = window ? window->WindowGroupId() : 0;
    if (!wGroupId)
        return;
    RWsSession &wsSession = CCoeEnv::Static()->WsSession();
    TUint wGroupHandle = wsSession.GetWindowGroupHandle(wGroupId);
    if (wGroupHandle) {
        RWindowGroup wGroup(wsSession);
        wGroup.Construct(wGroupHandle);
        wGroup.CancelCaptureKey(EKeyCamera);
    }
}
