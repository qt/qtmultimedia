/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd and/or its subsidiary(-ies).
** Copyright (C) 2016 Integrated Computer Solutions, Inc
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

#include "avfmediavideoprobecontrol.h"
#include <qvideoframe.h>

QT_BEGIN_NAMESPACE

AVFMediaVideoProbeControl::AVFMediaVideoProbeControl(QObject *parent) :
    QMediaVideoProbeControl(parent)
{
}

AVFMediaVideoProbeControl::~AVFMediaVideoProbeControl()
{

}

void AVFMediaVideoProbeControl::newFrameProbed(const QVideoFrame &frame)
{
    Q_EMIT videoFrameProbed(frame);
}

QT_END_NAMESPACE
