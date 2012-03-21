/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the Qt Toolkit.
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

#include <QtCore/qvariant.h>
#include <QtCore/qdebug.h>

#if defined(HAVE_WIDGETS)
#include <QtWidgets/qwidget.h>
#endif

#include "qgstreamerplayerservice.h"
#include "qgstreamerplayercontrol.h"
#include "qgstreamerplayersession.h"
#include "qgstreamermetadataprovider.h"
#include "qgstreameravailabilitycontrol.h"

#if defined(HAVE_WIDGETS)
#include "qgstreamervideooverlay.h"
#include "qgstreamervideowindow.h"
#include "qgstreamervideowidget.h"
#endif

#include "qgstreamervideorenderer.h"

#if defined(Q_WS_MAEMO_6) && defined(__arm__)
#include "qgstreamergltexturerenderer.h"
#endif

#include "qgstreamerstreamscontrol.h"
#include "qgstreameraudioprobecontrol.h"
#include "qgstreamervideoprobecontrol.h"

#include <private/qmediaplaylistnavigator_p.h>
#include <qmediaplaylist.h>

QT_BEGIN_NAMESPACE

QGstreamerPlayerService::QGstreamerPlayerService(QObject *parent):
     QMediaService(parent)
     , m_videoOutput(0)
     , m_videoRenderer(0)
#if defined(HAVE_XVIDEO) && defined(HAVE_WIDGETS)
     , m_videoWindow(0)
     , m_videoWidget(0)
#endif
{
    m_session = new QGstreamerPlayerSession(this);
    m_control = new QGstreamerPlayerControl(m_session, this);
    m_metaData = new QGstreamerMetaDataProvider(m_session, this);
    m_streamsControl = new QGstreamerStreamsControl(m_session,this);
    m_availabilityControl = new QGStreamerAvailabilityControl(m_control->resources(), this);

#if defined(Q_WS_MAEMO_6) && defined(__arm__)
    m_videoRenderer = new QGstreamerGLTextureRenderer(this);
#else
    m_videoRenderer = new QGstreamerVideoRenderer(this);
#endif

#if defined(HAVE_XVIDEO) && defined(HAVE_WIDGETS)
#ifdef Q_WS_MAEMO_6
    m_videoWindow = new QGstreamerVideoWindow(this, "omapxvsink");
#else
    m_videoWindow = new QGstreamerVideoOverlay(this);
#endif
    m_videoWidget = new QGstreamerVideoWidgetControl(this);
#endif
}

QGstreamerPlayerService::~QGstreamerPlayerService()
{
}

QMediaControl *QGstreamerPlayerService::requestControl(const char *name)
{
    if (qstrcmp(name,QMediaPlayerControl_iid) == 0)
        return m_control;

    if (qstrcmp(name,QMetaDataReaderControl_iid) == 0)
        return m_metaData;

    if (qstrcmp(name,QMediaStreamsControl_iid) == 0)
        return m_streamsControl;

    if (qstrcmp(name, QMediaAvailabilityControl_iid) == 0)
        return m_availabilityControl;

    if (qstrcmp(name,QMediaVideoProbeControl_iid) == 0) {
        if (m_session) {
            QGstreamerVideoProbeControl *probe = new QGstreamerVideoProbeControl(this);
            m_session->addProbe(probe);
            return probe;
        }
        return 0;
    }

    if (qstrcmp(name,QMediaAudioProbeControl_iid) == 0) {
        if (m_session) {
            QGstreamerAudioProbeControl *probe = new QGstreamerAudioProbeControl(this);
            m_session->addProbe(probe);
            return probe;
        }
        return 0;
    }

    if (!m_videoOutput) {
        if (qstrcmp(name, QVideoRendererControl_iid) == 0)
            m_videoOutput = m_videoRenderer;
#if defined(HAVE_XVIDEO) && defined(HAVE_WIDGETS)
        else if (qstrcmp(name, QVideoWidgetControl_iid) == 0)
            m_videoOutput = m_videoWidget;
        else  if (qstrcmp(name, QVideoWindowControl_iid) == 0)
            m_videoOutput = m_videoWindow;
#endif

        if (m_videoOutput) {
            m_control->setVideoOutput(m_videoOutput);
            return m_videoOutput;
        }
    }

    return 0;
}

void QGstreamerPlayerService::releaseControl(QMediaControl *control)
{
    if (control == m_videoOutput) {
        m_videoOutput = 0;
        m_control->setVideoOutput(0);
    }

    QGstreamerVideoProbeControl* videoProbe = qobject_cast<QGstreamerVideoProbeControl*>(control);
    if (videoProbe) {
        if (m_session)
            m_session->removeProbe(videoProbe);
        delete videoProbe;
        return;
    }

    QGstreamerAudioProbeControl* audioProbe = qobject_cast<QGstreamerAudioProbeControl*>(control);
    if (audioProbe) {
        if (m_session)
            m_session->removeProbe(audioProbe);
        delete audioProbe;
        return;
    }
}

QT_END_NAMESPACE
