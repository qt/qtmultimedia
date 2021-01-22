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

#ifndef BBCAMERAINFOCONTROL_H
#define BBCAMERAINFOCONTROL_H

#include <qcamerainfocontrol.h>

QT_BEGIN_NAMESPACE

class BbCameraInfoControl : public QCameraInfoControl
{
    Q_OBJECT
public:
    explicit BbCameraInfoControl(QObject *parent = 0);

    QCamera::Position cameraPosition(const QString &deviceName) const;
    int cameraOrientation(const QString &deviceName) const;

    static QCamera::Position position(const QString &deviceName);
    static int orientation(const QString &deviceName);
};

QT_END_NAMESPACE

#endif // BBCAMERAINFOCONTROL_H

