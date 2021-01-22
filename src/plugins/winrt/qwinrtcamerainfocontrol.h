/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd and/or its subsidiary(-ies).
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

#ifndef QWINRTCAMERAINFOCONTROL_H
#define QWINRTCAMERAINFOCONTROL_H

#include <QtMultimedia/QCameraInfoControl>

QT_BEGIN_NAMESPACE

class QWinRTCameraInfoControl : public QCameraInfoControl
{
    Q_OBJECT
public:
    explicit QWinRTCameraInfoControl(QObject *parent = nullptr);

    QCamera::Position cameraPosition(const QString &deviceName) const override;
    int cameraOrientation(const QString &deviceName) const override;
};

QT_END_NAMESPACE

#endif // QWINRTCAMERAINFOCONTROL_H
