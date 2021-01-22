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

#ifndef AVFCAMERAFOCUSCONTROL_H
#define AVFCAMERAFOCUSCONTROL_H

#include <QtCore/qscopedpointer.h>
#include <QtCore/qglobal.h>

#include <qcamerafocuscontrol.h>

@class AVCaptureDevice;

QT_BEGIN_NAMESPACE

class AVFCameraService;
class AVFCameraSession;

class AVFCameraFocusControl : public QCameraFocusControl
{
    Q_OBJECT
public:
    explicit AVFCameraFocusControl(AVFCameraService *service);

    QCameraFocus::FocusModes focusMode() const override;
    void setFocusMode(QCameraFocus::FocusModes mode) override;
    bool isFocusModeSupported(QCameraFocus::FocusModes mode) const override;

    QCameraFocus::FocusPointMode focusPointMode() const override;
    void setFocusPointMode(QCameraFocus::FocusPointMode mode) override;
    bool isFocusPointModeSupported(QCameraFocus::FocusPointMode mode) const override;
    QPointF customFocusPoint() const override;
    void setCustomFocusPoint(const QPointF &point) override;

    QCameraFocusZoneList focusZones() const override;

private Q_SLOTS:
    void cameraStateChanged();

private:

    AVFCameraSession *m_session;
    QCameraFocus::FocusModes m_focusMode;
    QCameraFocus::FocusPointMode m_focusPointMode;
    QPointF m_customFocusPoint;
    QPointF m_actualFocusPoint;
};

QT_END_NAMESPACE

#endif // AVFCAMERAFOCUSCONTROL_H
