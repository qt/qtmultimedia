/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#ifndef QANDROIDMEDIAVIDEOPROBECONTROL_H
#define QANDROIDMEDIAVIDEOPROBECONTROL_H

#include <qmediavideoprobecontrol.h>

QT_BEGIN_NAMESPACE

class QAndroidMediaVideoProbeControl : public QMediaVideoProbeControl
{
    Q_OBJECT
public:
    explicit QAndroidMediaVideoProbeControl(QObject *parent = 0);
    virtual ~QAndroidMediaVideoProbeControl();

    void newFrameProbed(const QVideoFrame& frame);

};

QT_END_NAMESPACE

#endif // QANDROIDMEDIAVIDEOPROBECONTROL_H
