/****************************************************************************
**
** Copyright (C) 2016 Research In Motion
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
#ifndef BBCAMERAFOCUSCONTROL_H
#define BBCAMERAFOCUSCONTROL_H

#include <qcamerafocuscontrol.h>

QT_BEGIN_NAMESPACE

class BbCameraSession;

class BbCameraFocusControl : public QCameraFocusControl
{
    Q_OBJECT
public:
    explicit BbCameraFocusControl(BbCameraSession *session, QObject *parent = 0);

    QCameraFocus::FocusModes focusMode() const override;
    void setFocusMode(QCameraFocus::FocusModes mode) override;
    bool isFocusModeSupported(QCameraFocus::FocusModes mode) const override;
    QCameraFocus::FocusPointMode focusPointMode() const override;
    void setFocusPointMode(QCameraFocus::FocusPointMode mode) override;
    bool isFocusPointModeSupported(QCameraFocus::FocusPointMode mode) const override;
    QPointF customFocusPoint() const override;
    void setCustomFocusPoint(const QPointF &point) override;
    QCameraFocusZoneList focusZones() const override;

private:
    void updateCustomFocusRegion();
    bool retrieveViewfinderSize(int *width, int *height);

    BbCameraSession *m_session;

    QCameraFocus::FocusModes m_focusMode;
    QCameraFocus::FocusPointMode m_focusPointMode;
    QPointF m_customFocusPoint;
};

QT_END_NAMESPACE

#endif
