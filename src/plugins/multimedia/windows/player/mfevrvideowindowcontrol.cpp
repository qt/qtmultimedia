// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "mfevrvideowindowcontrol_p.h"

#include <qdebug.h>

QT_BEGIN_NAMESPACE

MFEvrVideoWindowControl::MFEvrVideoWindowControl(QVideoSink *parent)
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

QT_END_NAMESPACE
