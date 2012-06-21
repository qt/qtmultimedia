/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qmediacontent.h"

#include <QtCore/qdebug.h>

#include "mfplayercontrol.h"
#if defined(HAVE_WIDGETS) && !defined(Q_WS_SIMULATOR)
#include "evr9videowindowcontrol.h"
#endif
#include "mfvideorenderercontrol.h"
#include "mfaudioendpointcontrol.h"
#include "mfplayerservice.h"
#include "mfplayersession.h"
#include "mfmetadatacontrol.h"
int MFPlayerService::s_refCount = 0;

MFPlayerService::MFPlayerService(QObject *parent)
    : QMediaService(parent)
    , m_session(0)
#ifndef Q_WS_SIMULATOR
    , m_videoWindowControl(0)
#endif
    , m_videoRendererControl(0)
{
    s_refCount++;
    if (s_refCount == 1) {
        CoInitialize(NULL);
        MFStartup(MF_VERSION);
    }
    m_audioEndpointControl = new MFAudioEndpointControl(this);
    m_session = new MFPlayerSession(this);
    m_player = new MFPlayerControl(m_session);
    m_metaDataControl = new MFMetaDataControl(this);
}

MFPlayerService::~MFPlayerService()
{
#ifndef Q_WS_SIMULATOR
    if (m_videoWindowControl)
        delete m_videoWindowControl;
#endif

    if (m_videoRendererControl)
        delete m_videoRendererControl;

    m_session->close();
    m_session->Release();

    s_refCount--;
    if (s_refCount == 0) {
        MFShutdown();
        CoUninitialize();
    }
}

QMediaControl* MFPlayerService::requestControl(const char *name)
{
    if (qstrcmp(name, QMediaPlayerControl_iid) == 0) {
        return m_player;
    } else if (qstrcmp(name, QAudioEndpointSelector_iid) == 0) {
        return m_audioEndpointControl;
    } else if (qstrcmp(name, QMetaDataReaderControl_iid) == 0) {
        return m_metaDataControl;
    } else if (qstrcmp(name, QVideoRendererControl_iid) == 0) {
#if defined(HAVE_WIDGETS) && !defined(Q_WS_SIMULATOR)
        if (!m_videoRendererControl && !m_videoWindowControl) {
#else
        if (!m_videoRendererControl) {
#endif
            m_videoRendererControl = new MFVideoRendererControl;
            return m_videoRendererControl;
        }
#if defined(HAVE_WIDGETS) && !defined(Q_WS_SIMULATOR)
    } else if (qstrcmp(name, QVideoWindowControl_iid) == 0) {
        if (!m_videoRendererControl && !m_videoWindowControl) {
            m_videoWindowControl = new Evr9VideoWindowControl;
            return m_videoWindowControl;
        }
#endif
    }

    return 0;
}

void MFPlayerService::releaseControl(QMediaControl *control)
{
    if (!control) {
        qWarning("QMediaService::releaseControl():"
                " Attempted release of null control");
    } else if (control == m_videoRendererControl) {
        m_videoRendererControl->setSurface(0);
        delete m_videoRendererControl;
        m_videoRendererControl = 0;
#if defined(HAVE_WIDGETS) && !defined(Q_WS_SIMULATOR)
    } else if (control == m_videoWindowControl) {
        delete m_videoWindowControl;
        m_videoWindowControl = 0;
#endif
    }
}

MFAudioEndpointControl* MFPlayerService::audioEndpointControl() const
{
    return m_audioEndpointControl;
}

MFVideoRendererControl* MFPlayerService::videoRendererControl() const
{
    return m_videoRendererControl;
}

#if defined(HAVE_WIDGETS) && !defined(Q_WS_SIMULATOR)
Evr9VideoWindowControl* MFPlayerService::videoWindowControl() const
{
    return m_videoWindowControl;
}
#endif

MFMetaDataControl* MFPlayerService::metaDataControl() const
{
    return m_metaDataControl;
}
