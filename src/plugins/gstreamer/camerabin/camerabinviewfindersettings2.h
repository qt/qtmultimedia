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

#ifndef CAMERABINVIEWFINDERSETTINGS2_H
#define CAMERABINVIEWFINDERSETTINGS2_H

#include <qcameraviewfindersettingscontrol.h>

QT_BEGIN_NAMESPACE

class CameraBinSession;

class CameraBinViewfinderSettings2 : public QCameraViewfinderSettingsControl2
{
    Q_OBJECT
public:
    CameraBinViewfinderSettings2(CameraBinSession *session);
    ~CameraBinViewfinderSettings2();

    QList<QCameraViewfinderSettings> supportedViewfinderSettings() const override;

    QCameraViewfinderSettings viewfinderSettings() const override;
    void setViewfinderSettings(const QCameraViewfinderSettings &settings) override;

private:
    CameraBinSession *m_session;
};

QT_END_NAMESPACE

#endif // CAMERABINVIEWFINDERSETTINGS2_H
