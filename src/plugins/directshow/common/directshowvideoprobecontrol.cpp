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

#include "directshowvideoprobecontrol.h"
#include "directshowglobal.h"

QT_BEGIN_NAMESPACE

DirectShowVideoProbeControl::DirectShowVideoProbeControl(QObject *p)
    : QMediaVideoProbeControl(p)
{

}

DirectShowVideoProbeControl::~DirectShowVideoProbeControl()
{
    if (m_ref.deref())
        qCWarning(qtDirectShowPlugin, "QVideoProbe control destroyed while it's still being referenced!!!");
}

void DirectShowVideoProbeControl::probeVideoFrame(const QVideoFrame &frame)
{
    emit videoFrameProbed(frame);
    m_frameProbed = true;
}

void DirectShowVideoProbeControl::flushVideoFrame()
{
    if (m_frameProbed)
        emit flush();
}

QT_END_NAMESPACE
