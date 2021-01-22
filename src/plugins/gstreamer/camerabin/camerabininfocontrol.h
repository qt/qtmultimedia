/****************************************************************************
**
** Copyright (C) 2016 Jolla Ltd.
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

#ifndef CAMERABININFOCONTROL_H
#define CAMERABININFOCONTROL_H

#include <qcamerainfocontrol.h>

#include <gst/gst.h>

QT_BEGIN_NAMESPACE

class CameraBinInfoControl : public QCameraInfoControl
{
    Q_OBJECT
public:
    CameraBinInfoControl(GstElementFactory *sourceFactory, QObject *parent);
    ~CameraBinInfoControl();

    QCamera::Position cameraPosition(const QString &deviceName) const override;
    int cameraOrientation(const QString &deviceName) const override;

private:
    GstElementFactory * const m_sourceFactory;
};

QT_END_NAMESPACE

#endif
