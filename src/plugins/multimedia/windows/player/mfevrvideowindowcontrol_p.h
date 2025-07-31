// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

#include "evrvideowindowcontrol_p.h"

QT_BEGIN_NAMESPACE

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

QT_END_NAMESPACE

#endif // MFEVRVIDEOWINDOWCONTROL_H
