/****************************************************************************
**
** Copyright (C) 2016 Jolla Ltd.
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

#ifndef CAMERABINVIEWFINDERSETTINGS_H
#define CAMERABINVIEWFINDERSETTINGS_H

#include <qcameraviewfindersettingscontrol.h>

QT_BEGIN_NAMESPACE

class CameraBinSession;

class CameraBinViewfinderSettings : public QCameraViewfinderSettingsControl
{
    Q_OBJECT
public:
    CameraBinViewfinderSettings(CameraBinSession *session);
    ~CameraBinViewfinderSettings();

    bool isViewfinderParameterSupported(ViewfinderParameter parameter) const override;
    QVariant viewfinderParameter(ViewfinderParameter parameter) const override;
    void setViewfinderParameter(ViewfinderParameter parameter, const QVariant &value) override;

private:
    CameraBinSession *m_session;
};

QT_END_NAMESPACE

#endif
