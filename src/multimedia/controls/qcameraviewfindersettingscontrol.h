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



#ifndef QCAMERAVIEWFINDERSETTINGSCONTROL_H
#define QCAMERAVIEWFINDERSETTINGSCONTROL_H

#include <QtMultimedia/qmediacontrol.h>
#include <QtMultimedia/qcamera.h>

QT_BEGIN_NAMESPACE

// Required for QDoc workaround
class QString;

class Q_MULTIMEDIA_EXPORT QCameraViewfinderSettingsControl : public QMediaControl
{
    Q_OBJECT
public:
    enum ViewfinderParameter {
        Resolution,
        PixelAspectRatio,
        MinimumFrameRate,
        MaximumFrameRate,
        PixelFormat,
        UserParameter = 1000
    };

    ~QCameraViewfinderSettingsControl();

    virtual bool isViewfinderParameterSupported(ViewfinderParameter parameter) const = 0;
    virtual QVariant viewfinderParameter(ViewfinderParameter parameter) const = 0;
    virtual void setViewfinderParameter(ViewfinderParameter parameter, const QVariant &value) = 0;

protected:
    explicit QCameraViewfinderSettingsControl(QObject *parent = nullptr);
};

#define QCameraViewfinderSettingsControl_iid "org.qt-project.qt.cameraviewfindersettingscontrol/5.0"
Q_MEDIA_DECLARE_CONTROL(QCameraViewfinderSettingsControl, QCameraViewfinderSettingsControl_iid)


// Required for QDoc workaround
class QString;

class Q_MULTIMEDIA_EXPORT QCameraViewfinderSettingsControl2 : public QMediaControl
{
    Q_OBJECT
public:
    virtual ~QCameraViewfinderSettingsControl2();

    virtual QList<QCameraViewfinderSettings> supportedViewfinderSettings() const = 0;

    virtual QCameraViewfinderSettings viewfinderSettings() const = 0;
    virtual void setViewfinderSettings(const QCameraViewfinderSettings &settings) = 0;

protected:
    explicit QCameraViewfinderSettingsControl2(QObject *parent = nullptr);
};

#define QCameraViewfinderSettingsControl2_iid "org.qt-project.qt.cameraviewfindersettingscontrol2/5.5"
Q_MEDIA_DECLARE_CONTROL(QCameraViewfinderSettingsControl2, QCameraViewfinderSettingsControl2_iid)

QT_END_NAMESPACE

#endif // QCAMERAVIEWFINDERSETTINGSCONTROL_H
