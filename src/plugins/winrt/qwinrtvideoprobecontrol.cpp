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
#include "qwinrtvideoprobecontrol.h"
#include "qwinrtcameravideorenderercontrol.h"

QT_BEGIN_NAMESPACE

QWinRTVideoProbeControl::QWinRTVideoProbeControl(QWinRTCameraVideoRendererControl *parent)
    : QMediaVideoProbeControl(parent)
{
    QObject::connect(parent, &QWinRTCameraVideoRendererControl::videoFrameProbed,
                     this, &QMediaVideoProbeControl::videoFrameProbed, Qt::QueuedConnection);
    parent->incrementProbe();
}

QWinRTVideoProbeControl::~QWinRTVideoProbeControl()
{
    if (QWinRTCameraVideoRendererControl *renderer = qobject_cast<QWinRTCameraVideoRendererControl *>(parent()))
        renderer->decrementProbe();
}

QT_END_NAMESPACE
