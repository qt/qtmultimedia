/****************************************************************************
**
** Copyright (C) 2016 Research In Motion
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

#include "bbvideorenderercontrol.h"

#include "bbcamerasession.h"

QT_BEGIN_NAMESPACE

BbVideoRendererControl::BbVideoRendererControl(BbCameraSession *session, QObject *parent)
    : QVideoRendererControl(parent)
    , m_session(session)
{
}

QAbstractVideoSurface* BbVideoRendererControl::surface() const
{
    return m_session->surface();
}

void BbVideoRendererControl::setSurface(QAbstractVideoSurface *surface)
{
    m_session->setSurface(surface);
}

QT_END_NAMESPACE
