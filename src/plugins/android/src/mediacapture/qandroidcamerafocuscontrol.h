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

#ifndef QANDROIDCAMERAFOCUSCONTROL_H
#define QANDROIDCAMERAFOCUSCONTROL_H

#include <qcamerafocuscontrol.h>

QT_BEGIN_NAMESPACE

class QAndroidCameraSession;

class QAndroidCameraFocusControl : public QCameraFocusControl
{
    Q_OBJECT
public:
    explicit QAndroidCameraFocusControl(QAndroidCameraSession *session);

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
    void onCameraOpened();
    void onViewportSizeChanged();
    void onCameraCaptureModeChanged();
    void onAutoFocusStarted();
    void onAutoFocusComplete(bool success);

private:
    inline void setFocusModeHelper(QCameraFocus::FocusModes mode)
    {
        if (m_focusMode != mode) {
            m_focusMode = mode;
            emit focusModeChanged(mode);
        }
    }

    inline void setFocusPointModeHelper(QCameraFocus::FocusPointMode mode)
    {
        if (m_focusPointMode != mode) {
            m_focusPointMode = mode;
            emit focusPointModeChanged(mode);
        }
    }

    void updateFocusZones(QCameraFocusZone::FocusZoneStatus status = QCameraFocusZone::Selected);
    void setCameraFocusArea();

    QAndroidCameraSession *m_session;

    QCameraFocus::FocusModes m_focusMode;
    QCameraFocus::FocusPointMode m_focusPointMode;
    QPointF m_actualFocusPoint;
    QPointF m_customFocusPoint;
    QCameraFocusZoneList m_focusZones;

    QList<QCameraFocus::FocusModes> m_supportedFocusModes;
    bool m_continuousPictureFocusSupported;
    bool m_continuousVideoFocusSupported;

    QList<QCameraFocus::FocusPointMode> m_supportedFocusPointModes;
};

QT_END_NAMESPACE

#endif // QANDROIDCAMERAFOCUSCONTROL_H
