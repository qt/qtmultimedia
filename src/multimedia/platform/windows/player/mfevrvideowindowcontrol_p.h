/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

#ifndef MFEVRVIDEOWINDOWCONTROL_H
#define MFEVRVIDEOWINDOWCONTROL_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "private/evrvideowindowcontrol_p.h"

QT_USE_NAMESPACE

class MFEvrVideoWindowControl : public EvrVideoWindowControl
{
public:
    MFEvrVideoWindowControl(QVideoSink *parent = 0);
    ~MFEvrVideoWindowControl();

    IMFActivate* createActivate();
    void releaseActivate();

private:
    void clear();

    IMFActivate *m_currentActivate;
    IMFMediaSink *m_evrSink;
};

#endif // MFEVRVIDEOWINDOWCONTROL_H
