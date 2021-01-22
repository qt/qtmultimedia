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

#ifndef V4LRADIOSERVICE_H
#define V4LRADIOSERVICE_H

#include <QtCore/qobject.h>

#include <qmediaservice.h>
QT_USE_NAMESPACE

class V4LRadioControl;

class V4LRadioService : public QMediaService
{
    Q_OBJECT

public:
    V4LRadioService(QObject *parent = 0);
    ~V4LRadioService();

    QMediaControl *requestControl(const char* name);
    void releaseControl(QMediaControl *);

private:
    V4LRadioControl *m_control;
};

#endif
