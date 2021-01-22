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

#include "directshowevrvideowindowcontrol.h"

#include "directshowglobal.h"

DirectShowEvrVideoWindowControl::DirectShowEvrVideoWindowControl(QObject *parent)
    : EvrVideoWindowControl(parent)
{
}

DirectShowEvrVideoWindowControl::~DirectShowEvrVideoWindowControl()
{
    if (m_evrFilter)
        m_evrFilter->Release();
}

IBaseFilter *DirectShowEvrVideoWindowControl::filter()
{
    if (!m_evrFilter) {
        m_evrFilter = com_new<IBaseFilter>(clsid_EnhancedVideoRenderer);
        if (!setEvr(m_evrFilter)) {
            m_evrFilter->Release();
            m_evrFilter = nullptr;
        }
    }

    return m_evrFilter;
}
