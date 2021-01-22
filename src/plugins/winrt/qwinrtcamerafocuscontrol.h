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

#ifndef QWINRTCAMERAFOCUSCONTROL_H
#define QWINRTCAMERAFOCUSCONTROL_H
#include <qcamerafocuscontrol.h>

QT_BEGIN_NAMESPACE

class QWinRTCameraControl;
class QWinRTCameraFocusControlPrivate;
class QWinRTCameraFocusControl : public QCameraFocusControl
{
    Q_OBJECT
public:
    explicit QWinRTCameraFocusControl(QWinRTCameraControl *parent);

    QCameraFocus::FocusModes focusMode() const override;
    void setFocusMode(QCameraFocus::FocusModes mode) override;
    bool isFocusModeSupported(QCameraFocus::FocusModes mode) const override;
    QCameraFocus::FocusPointMode focusPointMode() const override;
    void setFocusPointMode(QCameraFocus::FocusPointMode mode) override;
    bool isFocusPointModeSupported(QCameraFocus::FocusPointMode mode) const override;
    QPointF customFocusPoint() const override;
    void setCustomFocusPoint(const QPointF &point) override;
    QCameraFocusZoneList focusZones() const override;

    void setSupportedFocusMode(QCameraFocus::FocusModes flag);
    void setSupportedFocusPointMode(const QSet<QCameraFocus::FocusPointMode> &supportedFocusPointModes);

private slots:
    void imageCaptureQueueChanged(bool isEmpty);

private:
    bool changeFocusCustomPoint(const QPointF &point);

    QScopedPointer<QWinRTCameraFocusControlPrivate> d_ptr;
    Q_DECLARE_PRIVATE(QWinRTCameraFocusControl)
};

#endif // QWINRTCAMERAFOCUSCONTROL_H
