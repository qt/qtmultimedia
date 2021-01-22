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

#include "qandroidmediaplayervideorenderercontrol.h"

#include "qandroidmediaplayercontrol.h"
#include "qandroidvideooutput.h"
#include <qabstractvideosurface.h>

QT_BEGIN_NAMESPACE

QAndroidMediaPlayerVideoRendererControl::QAndroidMediaPlayerVideoRendererControl(QAndroidMediaPlayerControl *mediaPlayer, QObject *parent)
    : QVideoRendererControl(parent)
    , m_mediaPlayerControl(mediaPlayer)
    , m_surface(0)
    , m_textureOutput(new QAndroidTextureVideoOutput(this))
{
    m_mediaPlayerControl->setVideoOutput(m_textureOutput);
}

QAndroidMediaPlayerVideoRendererControl::~QAndroidMediaPlayerVideoRendererControl()
{
    m_mediaPlayerControl->setVideoOutput(0);
}

QAbstractVideoSurface *QAndroidMediaPlayerVideoRendererControl::surface() const
{
    return m_surface;
}

void QAndroidMediaPlayerVideoRendererControl::setSurface(QAbstractVideoSurface *surface)
{
    if (m_surface == surface)
        return;

    m_surface = surface;
    m_textureOutput->setSurface(m_surface);
}

QT_END_NAMESPACE
