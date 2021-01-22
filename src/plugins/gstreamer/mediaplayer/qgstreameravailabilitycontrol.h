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

#ifndef QGSTREAMERAVAILABILITYCONTROL_H
#define QGSTREAMERAVAILABILITYCONTROL_H

#include <QObject>
#include <qmediaavailabilitycontrol.h>

QT_BEGIN_NAMESPACE

class QMediaPlayerResourceSetInterface;
class QGStreamerAvailabilityControl : public QMediaAvailabilityControl
{
    Q_OBJECT
public:
    QGStreamerAvailabilityControl(QMediaPlayerResourceSetInterface *resources, QObject *parent = 0);
    QMultimedia::AvailabilityStatus availability() const override;

private Q_SLOTS:
    void handleAvailabilityChanged();

private:
    QMediaPlayerResourceSetInterface *m_resources = nullptr;
};

QT_END_NAMESPACE

#endif // QGSTREAMERAVAILABILITYCONTROL_H
