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

#ifndef MFPLAYERSERVICE_H
#define MFPLAYERSERVICE_H

#include <mfapi.h>
#include <mfidl.h>

#include "qmediaplayer.h"
#include "qmediaresource.h"
#include "qmediaservice.h"
#include "qmediatimerange.h"

QT_BEGIN_NAMESPACE
class QMediaContent;
QT_END_NAMESPACE

QT_USE_NAMESPACE

#if defined(HAVE_WIDGETS) && !defined(Q_WS_SIMULATOR)
class Evr9VideoWindowControl;
#endif
class MFAudioEndpointControl;
class MFVideoRendererControl;
class MFPlayerControl;
class MFMetaDataControl;
class MFPlayerSession;

class MFPlayerService : public QMediaService
{
    Q_OBJECT
public:
    MFPlayerService(QObject *parent = 0);
    ~MFPlayerService();

    QMediaControl* requestControl(const char *name);
    void releaseControl(QMediaControl *control);

    MFAudioEndpointControl* audioEndpointControl() const;
    MFVideoRendererControl* videoRendererControl() const;
#if defined(HAVE_WIDGETS) && !defined(Q_WS_SIMULATOR)
    Evr9VideoWindowControl* videoWindowControl() const;
#endif
    MFMetaDataControl* metaDataControl() const;

private:
    MFPlayerSession *m_session;
    MFVideoRendererControl *m_videoRendererControl;
    MFAudioEndpointControl *m_audioEndpointControl;
#if defined(HAVE_WIDGETS) && !defined(Q_WS_SIMULATOR)
    Evr9VideoWindowControl *m_videoWindowControl;
#endif
    MFPlayerControl        *m_player;
    MFMetaDataControl      *m_metaDataControl;
    static int                s_refCount;
};

#endif
