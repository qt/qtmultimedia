/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtMultimedia/private/qtmultimediaglobal_p.h>
#include <QtCore/qvariant.h>
#include <QtCore/qdebug.h>


#include "qgstreamerplayerservice_p.h"
#include "qgstreamermetadataprovider_p.h"

#include <private/qgstreamervideowindow_p.h>
#include <private/qgstreamervideorenderer_p.h>

#include "qgstreamerstreamscontrol_p.h"
#include <private/qgstreamerplayersession_p.h>
#include <private/qgstreamerplayercontrol_p.h>

QT_BEGIN_NAMESPACE

QGstreamerPlayerService::QGstreamerPlayerService()
    : QMediaPlatformPlayerInterface()
{
    m_session = new QGstreamerPlayerSession(this);
    m_control = new QGstreamerPlayerControl(m_session, this);
    m_metaData = new QGstreamerMetaDataProvider(m_session, this);
    m_streamsControl = new QGstreamerStreamsControl(m_session,this);
    m_videoRenderer = new QGstreamerVideoRenderer(this);
    m_videoWindow = new QGstreamerVideoWindow(this);
   // If the GStreamer video sink is not available, don't provide the video window control since
    // it won't work anyway.
    if (!m_videoWindow->videoSink()) {
        delete m_videoWindow;
        m_videoWindow = 0;
    }
}

QGstreamerPlayerService::~QGstreamerPlayerService()
{
}

QObject *QGstreamerPlayerService::requestControl(const char *name)
{
    if (qstrcmp(name,QMediaPlayerControl_iid) == 0)
        return m_control;

    if (qstrcmp(name,QMetaDataReaderControl_iid) == 0)
        return m_metaData;

    if (!m_videoOutput) {
        if (qstrcmp(name, QVideoRendererControl_iid) == 0)
            m_videoOutput = m_videoRenderer;
        else if (qstrcmp(name, QVideoWindowControl_iid) == 0)
            m_videoOutput = m_videoWindow;

        if (m_videoOutput) {
            increaseVideoRef();
            m_control->setVideoOutput(m_videoOutput);
            return m_videoOutput;
        }
    }

    return 0;
}

void QGstreamerPlayerService::releaseControl(QObject *control)
{
    if (!control)
        return;

    if (control == m_videoOutput) {
        m_videoOutput = 0;
        m_control->setVideoOutput(0);
        decreaseVideoRef();
    }
}

QMediaPlayerControl *QGstreamerPlayerService::player()
{
    return m_control;
}

QMetaDataReaderControl *QGstreamerPlayerService::dataReader()
{
    return m_metaData;
}

QMediaStreamsControl *QGstreamerPlayerService::streams()
{
    return m_streamsControl;
}

QVideoRendererControl *QGstreamerPlayerService::createVideoRenderer()
{
    if (!m_videoOutput) {
        m_videoOutput = m_videoRenderer;

        increaseVideoRef();
        m_control->setVideoOutput(m_videoOutput);
        return m_videoRenderer;
    }
    return nullptr;
}

QVideoWindowControl *QGstreamerPlayerService::createVideoWindow()
{
    if (!m_videoOutput) {
        m_videoOutput = m_videoWindow;

        increaseVideoRef();
        m_control->setVideoOutput(m_videoOutput);
        return m_videoWindow;
    }
    return nullptr;
}

void QGstreamerPlayerService::increaseVideoRef()
{
    m_videoReferenceCount++;
}

void QGstreamerPlayerService::decreaseVideoRef()
{
    m_videoReferenceCount--;
}

#if 0
// ### Re-add something similar to be able to check for support of certain file formats
QMultimedia::SupportEstimate QGstreamerPlayerServicePlugin::hasSupport(const QString &mimeType,
                                                                     const QStringList &codecs) const
{
    if (m_supportedMimeTypeSet.isEmpty())
        updateSupportedMimeTypes();

    return QGstUtils::hasSupport(mimeType, codecs, m_supportedMimeTypeSet);
}

static bool isDecoderOrDemuxer(GstElementFactory *factory)
{
    return gst_element_factory_list_is_type(factory, GST_ELEMENT_FACTORY_TYPE_DEMUXER)
                || gst_element_factory_list_is_type(factory, GST_ELEMENT_FACTORY_TYPE_DECODER);
}

void QGstreamerPlayerServicePlugin::updateSupportedMimeTypes() const
{
     m_supportedMimeTypeSet = QGstUtils::supportedMimeTypes(isDecoderOrDemuxer);
}

QStringList QGstreamerPlayerServicePlugin::supportedMimeTypes() const
{
    return QStringList();
}
#endif

QT_END_NAMESPACE
