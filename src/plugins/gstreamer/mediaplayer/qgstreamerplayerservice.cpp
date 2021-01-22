/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#include <QtMultimedia/private/qtmultimediaglobal_p.h>
#include <QtCore/qvariant.h>
#include <QtCore/qdebug.h>

#if defined(HAVE_WIDGETS)
#include <QtWidgets/qwidget.h>
#endif

#include "qgstreamerplayerservice.h"
#include "qgstreamermetadataprovider.h"
#include "qgstreameravailabilitycontrol.h"

#if defined(HAVE_WIDGETS)
#include <private/qgstreamervideowidget_p.h>
#endif
#include <private/qgstreamervideowindow_p.h>
#include <private/qgstreamervideorenderer_p.h>

#include "qgstreamerstreamscontrol.h"
#include <private/qgstreameraudioprobecontrol_p.h>
#include <private/qgstreamervideoprobecontrol_p.h>
#include <private/qgstreamerplayersession_p.h>
#include <private/qgstreamerplayercontrol_p.h>

#include <private/qmediaplaylistnavigator_p.h>
#include <qmediaplaylist.h>
#include <private/qmediaresourceset_p.h>

QT_BEGIN_NAMESPACE

QGstreamerPlayerService::QGstreamerPlayerService(QObject *parent)
    : QMediaService(parent)
{
    m_session = new QGstreamerPlayerSession(this);
    m_control = new QGstreamerPlayerControl(m_session, this);
    m_metaData = new QGstreamerMetaDataProvider(m_session, this);
    m_streamsControl = new QGstreamerStreamsControl(m_session,this);
    m_availabilityControl = new QGStreamerAvailabilityControl(m_control->resources(), this);
    m_videoRenderer = new QGstreamerVideoRenderer(this);
    m_videoWindow = new QGstreamerVideoWindow(this);
   // If the GStreamer video sink is not available, don't provide the video window control since
    // it won't work anyway.
    if (!m_videoWindow->videoSink()) {
        delete m_videoWindow;
        m_videoWindow = 0;
    }

#if defined(HAVE_WIDGETS)
    m_videoWidget = new QGstreamerVideoWidgetControl(this);

    // If the GStreamer video sink is not available, don't provide the video widget control since
    // it won't work anyway.
    // QVideoWidget will fall back to QVideoRendererControl in that case.
    if (!m_videoWidget->videoSink()) {
        delete m_videoWidget;
        m_videoWidget = 0;
    }
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

    if (qstrcmp(name, QMediaVideoProbeControl_iid) == 0) {
        if (!m_videoProbeControl) {
            increaseVideoRef();
            m_videoProbeControl = new QGstreamerVideoProbeControl(this);
            m_session->addProbe(m_videoProbeControl);
        }
        m_videoProbeControl->ref.ref();
        return m_videoProbeControl;
    }

    if (qstrcmp(name, QMediaAudioProbeControl_iid) == 0) {
        if (!m_audioProbeControl) {
            m_audioProbeControl = new QGstreamerAudioProbeControl(this);
            m_session->addProbe(m_audioProbeControl);
        }
        m_audioProbeControl->ref.ref();
        return m_audioProbeControl;
    }

    if (!m_videoOutput) {
        if (qstrcmp(name, QVideoRendererControl_iid) == 0)
            m_videoOutput = m_videoRenderer;
        else if (qstrcmp(name, QVideoWindowControl_iid) == 0)
            m_videoOutput = m_videoWindow;
#if defined(HAVE_WIDGETS)
        else if (qstrcmp(name, QVideoWidgetControl_iid) == 0)
            m_videoOutput = m_videoWidget;
#endif

        if (m_videoOutput) {
            increaseVideoRef();
            m_control->setVideoOutput(m_videoOutput);
            return m_videoOutput;
        }
    }

    return 0;
}

void QGstreamerPlayerService::releaseControl(QMediaControl *control)
{
    if (!control) {
        return;
    } else if (control == m_videoOutput) {
        m_videoOutput = 0;
        m_control->setVideoOutput(0);
        decreaseVideoRef();
    } else if (control == m_videoProbeControl && !m_videoProbeControl->ref.deref()) {
        m_session->removeProbe(m_videoProbeControl);
        delete m_videoProbeControl;
        m_videoProbeControl = 0;
        decreaseVideoRef();
    } else if (control == m_audioProbeControl && !m_audioProbeControl->ref.deref()) {
        m_session->removeProbe(m_audioProbeControl);
        delete m_audioProbeControl;
        m_audioProbeControl = 0;
    }
}

void QGstreamerPlayerService::increaseVideoRef()
{
    m_videoReferenceCount++;
    if (m_videoReferenceCount == 1) {
        m_control->resources()->setVideoEnabled(true);
    }
}

void QGstreamerPlayerService::decreaseVideoRef()
{
    m_videoReferenceCount--;
    if (m_videoReferenceCount == 0) {
        m_control->resources()->setVideoEnabled(false);
    }
}

QT_END_NAMESPACE
