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

#ifndef QCAMERAFOCUSCONTROL_H
#define QCAMERAFOCUSCONTROL_H

#include <QtMultimedia/qmediacontrol.h>
#include <QtMultimedia/qmediaobject.h>

#include <QtMultimedia/qcamerafocus.h>

QT_BEGIN_NAMESPACE

// Required for QDoc workaround
class QString;

class Q_MULTIMEDIA_EXPORT QCameraFocusControl : public QMediaControl
{
    Q_OBJECT

public:
    ~QCameraFocusControl();

    virtual QCameraFocus::FocusModes focusMode() const = 0;
    virtual void setFocusMode(QCameraFocus::FocusModes mode) = 0;
    virtual bool isFocusModeSupported(QCameraFocus::FocusModes mode) const = 0;

    virtual QCameraFocus::FocusPointMode focusPointMode() const = 0;
    virtual void setFocusPointMode(QCameraFocus::FocusPointMode mode) = 0;
    virtual bool isFocusPointModeSupported(QCameraFocus::FocusPointMode mode) const = 0;
    virtual QPointF customFocusPoint() const = 0;
    virtual void setCustomFocusPoint(const QPointF &point) = 0;

    virtual QCameraFocusZoneList focusZones() const = 0;

Q_SIGNALS:
    void focusModeChanged(QCameraFocus::FocusModes mode);
    void focusPointModeChanged(QCameraFocus::FocusPointMode mode);
    void customFocusPointChanged(const QPointF &point);

    void focusZonesChanged();

protected:
    explicit QCameraFocusControl(QObject *parent = nullptr);
};

#define QCameraFocusControl_iid "org.qt-project.qt.camerafocuscontrol/5.0"
Q_MEDIA_DECLARE_CONTROL(QCameraFocusControl, QCameraFocusControl_iid)

QT_END_NAMESPACE


#endif  // QCAMERAFOCUSCONTROL_H

