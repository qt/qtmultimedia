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

#ifndef MFEVRVIDEOWINDOWCONTROL_H
#define MFEVRVIDEOWINDOWCONTROL_H

#include "evrvideowindowcontrol.h"

QT_USE_NAMESPACE

class MFEvrVideoWindowControl : public EvrVideoWindowControl
{
public:
    MFEvrVideoWindowControl(QObject *parent = 0);
    ~MFEvrVideoWindowControl();

    IMFActivate* createActivate();
    void releaseActivate();

private:
    void clear();

    IMFActivate *m_currentActivate;
    IMFMediaSink *m_evrSink;
};

#endif // MFEVRVIDEOWINDOWCONTROL_H
