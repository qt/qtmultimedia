/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd and/or its subsidiary(-ies).
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

#include "avfmediaplayerservice.h"
#include "avfmediaplayersession.h"
#include "avfmediaplayercontrol.h"
#include "avfmediaplayermetadatacontrol.h"
#include "avfvideooutput.h"
#if QT_CONFIG(opengl)
#include "avfvideorenderercontrol.h"
#endif
#ifndef QT_NO_WIDGETS
# include "avfvideowidgetcontrol.h"
#endif
#include "avfvideowindowcontrol.h"

#import <AVFoundation/AVFoundation.h>

QT_USE_NAMESPACE

AVFMediaPlayerService::AVFMediaPlayerService(QObject *parent)
    : QMediaService(parent)
    , m_videoOutput(nullptr)
{
    m_session = new AVFMediaPlayerSession(this);
    m_control = new AVFMediaPlayerControl(this);
    m_control->setSession(m_session);
    m_playerMetaDataControl = new AVFMediaPlayerMetaDataControl(m_session, this);

    connect(m_control, SIGNAL(mediaChanged(QMediaContent)), m_playerMetaDataControl, SLOT(updateTags()));
}

AVFMediaPlayerService::~AVFMediaPlayerService()
{
#ifdef QT_DEBUG_AVF
    qDebug() << Q_FUNC_INFO;
#endif
    delete m_session;
}

QMediaControl *AVFMediaPlayerService::requestControl(const char *name)
{
#ifdef QT_DEBUG_AVF
    qDebug() << Q_FUNC_INFO << name;
#endif

    if (qstrcmp(name, QMediaPlayerControl_iid) == 0)
        return m_control;

    if (qstrcmp(name, QMetaDataReaderControl_iid) == 0)
        return m_playerMetaDataControl;

#if QT_CONFIG(opengl)
    if (qstrcmp(name, QVideoRendererControl_iid) == 0) {
        if (!m_videoOutput)
            m_videoOutput = new AVFVideoRendererControl(this);

        m_session->setVideoOutput(qobject_cast<AVFVideoOutput*>(m_videoOutput));
        return m_videoOutput;
    }
#endif
#ifndef QT_NO_WIDGETS
    if (qstrcmp(name, QVideoWidgetControl_iid) == 0) {
        if (!m_videoOutput)
            m_videoOutput = new AVFVideoWidgetControl(this);

        m_session->setVideoOutput(qobject_cast<AVFVideoOutput*>(m_videoOutput));
        return m_videoOutput;
    }
#endif
    if (qstrcmp(name, QVideoWindowControl_iid) == 0) {
        if (!m_videoOutput)
            m_videoOutput = new AVFVideoWindowControl(this);

        m_session->setVideoOutput(qobject_cast<AVFVideoOutput*>(m_videoOutput));
        return m_videoOutput;
    }
    return nullptr;
}

void AVFMediaPlayerService::releaseControl(QMediaControl *control)
{
#ifdef QT_DEBUG_AVF
    qDebug() << Q_FUNC_INFO << control;
#endif
    if (m_videoOutput == control) {
#if QT_CONFIG(opengl)
        AVFVideoRendererControl *renderControl = qobject_cast<AVFVideoRendererControl*>(m_videoOutput);

        if (renderControl)
            renderControl->setSurface(nullptr);
#endif
        m_videoOutput = nullptr;
        m_session->setVideoOutput(nullptr);

        delete control;
    }
}
