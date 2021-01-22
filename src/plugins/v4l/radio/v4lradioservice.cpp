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

#include <QtCore/qvariant.h>
#include <QtCore/qdebug.h>
#include <QtCore/qfile.h>
#include <QtWidgets/qwidget.h>

#include "v4lradioservice.h"
#include "v4lradiocontrol.h"

V4LRadioService::V4LRadioService(QObject *parent):
    QMediaService(parent)
{
    m_control = new V4LRadioControl(this);
}

V4LRadioService::~V4LRadioService()
{
}

QMediaControl *V4LRadioService::requestControl(const char* name)
{
    if (qstrcmp(name,QRadioTunerControl_iid) == 0)
        return m_control;

    return 0;
}


void V4LRadioService::releaseControl(QMediaControl *)
{
}
