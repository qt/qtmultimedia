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

#include <QtCore/qdebug.h>

#include "dsvideorenderer.h"

QT_BEGIN_NAMESPACE

DSVideoRendererControl::DSVideoRendererControl(DSCameraSession* session, QObject *parent)
    :QVideoRendererControl(parent),
    m_session(session)
{
}

DSVideoRendererControl::~DSVideoRendererControl() = default;

QAbstractVideoSurface* DSVideoRendererControl::surface() const
{
    return m_surface;
}

void DSVideoRendererControl::setSurface(QAbstractVideoSurface *surface)
{
    m_surface = surface;
    if(m_session)
        m_session->setSurface(m_surface);
}

QT_END_NAMESPACE

