/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd and/or its subsidiary(-ies).
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
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

    QCameraFocus::FocusModes focusMode() const Q_DECL_OVERRIDE;
    void setFocusMode(QCameraFocus::FocusModes mode) Q_DECL_OVERRIDE;
    bool isFocusModeSupported(QCameraFocus::FocusModes mode) const Q_DECL_OVERRIDE;
    QCameraFocus::FocusPointMode focusPointMode() const Q_DECL_OVERRIDE;
    void setFocusPointMode(QCameraFocus::FocusPointMode mode) Q_DECL_OVERRIDE;
    bool isFocusPointModeSupported(QCameraFocus::FocusPointMode mode) const Q_DECL_OVERRIDE;
    QPointF customFocusPoint() const Q_DECL_OVERRIDE;
    void setCustomFocusPoint(const QPointF &point) Q_DECL_OVERRIDE;
    QCameraFocusZoneList focusZones() const Q_DECL_OVERRIDE;

    void setSupportedFocusMode(QCameraFocus::FocusModes flag);
    void setSupportedFocusPointMode(const QSet<QCameraFocus::FocusPointMode> &supportedFocusPointModes);

private slots:
    void imageCaptureQueueChanged(bool isEmpty);

private:
    Q_INVOKABLE void applyFocusCustomPoint(const QPointF &point);
    Q_INVOKABLE void applyFocusMode(QCameraFocus::FocusModes modes);
    Q_INVOKABLE void applyFocusPointMode(QCameraFocus::FocusPointMode mode);
    bool changeFocusCustomPoint(const QPointF &point);

    QScopedPointer<QWinRTCameraFocusControlPrivate> d_ptr;
    Q_DECLARE_PRIVATE(QWinRTCameraFocusControl)
};

#endif // QWINRTCAMERAFOCUSCONTROL_H
