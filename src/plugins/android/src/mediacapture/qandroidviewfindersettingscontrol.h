/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Copyright (C) 2016 Ruslan Baratov
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

#ifndef QANDROIDVIEWFINDERSETTINGSCONTROL_H
#define QANDROIDVIEWFINDERSETTINGSCONTROL_H

#include <QtMultimedia/qcameraviewfindersettingscontrol.h>
#include <QtMultimedia/qcameraviewfindersettings.h>

#include <QtCore/qpointer.h>
#include <QtCore/qglobal.h>
#include <QtCore/qsize.h>

QT_BEGIN_NAMESPACE

class QAndroidCameraSession;

class QAndroidViewfinderSettingsControl2 : public QCameraViewfinderSettingsControl2
{
    Q_OBJECT
public:
    explicit QAndroidViewfinderSettingsControl2(QAndroidCameraSession *session);

    QList<QCameraViewfinderSettings> supportedViewfinderSettings() const override;
    QCameraViewfinderSettings viewfinderSettings() const override;
    void setViewfinderSettings(const QCameraViewfinderSettings &settings) override;

private:
    QAndroidCameraSession *m_cameraSession;
};

QT_END_NAMESPACE

#endif // QANDROIDVIEWFINDERSETTINGSCONTROL_H
