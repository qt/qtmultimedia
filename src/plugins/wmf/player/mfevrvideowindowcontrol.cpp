/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
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
