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

#include "mfevrvideowindowcontrol.h"

#include <qdebug.h>

MFEvrVideoWindowControl::MFEvrVideoWindowControl(QObject *parent)
    : EvrVideoWindowControl(parent)
    , m_currentActivate(NULL)
    , m_evrSink(NULL)
{
}

MFEvrVideoWindowControl::~MFEvrVideoWindowControl()
{
   clear();
}

void MFEvrVideoWindowControl::clear()
{
    setEvr(NULL);

    if (m_evrSink)
        m_evrSink->Release();
    if (m_currentActivate) {
        m_currentActivate->ShutdownObject();
        m_currentActivate->Release();
    }
    m_evrSink = NULL;
    m_currentActivate = NULL;
}

IMFActivate* MFEvrVideoWindowControl::createActivate()
{
    clear();

    if (FAILED(MFCreateVideoRendererActivate(0, &m_currentActivate))) {
        qWarning() << "Failed to create evr video renderer activate!";
        return NULL;
    }
    if (FAILED(m_currentActivate->ActivateObject(IID_IMFMediaSink, (LPVOID*)(&m_evrSink)))) {
        qWarning() << "Failed to activate evr media sink!";
        return NULL;
    }
    if (!setEvr(m_evrSink))
        return NULL;

    return m_currentActivate;
}

void MFEvrVideoWindowControl::releaseActivate()
{
    clear();
}
