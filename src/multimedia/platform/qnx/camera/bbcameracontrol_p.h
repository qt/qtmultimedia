/****************************************************************************
**
** Copyright (C) 2016 Research In Motion
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef BBCAMERACONTROL_H
#define BBCAMERACONTROL_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qcameracontrol.h>

QT_BEGIN_NAMESPACE

class BbCameraSession;

class BbCameraControl : public QCameraControl
{
    Q_OBJECT
public:
    explicit BbCameraControl(BbCameraSession *session, QObject *parent = 0);

    QCamera::State state() const override;
    void setState(QCamera::State state) override;

    QCamera::Status status() const override;

    void setCamera(const QCameraInfo &camera) override;

    QCamera::CaptureModes captureMode() const override;
    void setCaptureMode(QCamera::CaptureModes) override;
    bool isCaptureModeSupported(QCamera::CaptureModes mode) const override;

    bool canChangeProperty(PropertyChangeType changeType, QCamera::Status status) const override;

    enum LocksApplyMode
    {
        IndependentMode,
        FocusExposureBoundMode,
        AllBoundMode,
        FocusOnlyMode
    };

    QCamera::LockTypes supportedLocks() const override;
    QCamera::LockStatus lockStatus(QCamera::LockType lock) const override;
    void searchAndLock(QCamera::LockTypes locks) override;
    void unlock(QCamera::LockTypes locks) override;

private Q_SLOTS:
    void cameraOpened();
    void focusStatusChanged(int value);

private:
    BbCameraSession *m_session;

    LocksApplyMode m_locksApplyMode = IndependentMode;
    QCamera::LockStatus m_focusLockStatus = QCamera::Unlocked;
    QCamera::LockStatus m_exposureLockStatus = QCamera::Unlocked;
    QCamera::LockStatus m_whiteBalanceLockStatus = QCamera::Unlocked;
    QCamera::LockTypes m_currentLockTypes = QCamera::NoLock;
    QCamera::LockTypes m_supportedLockTypes = QCamera::NoLock;

private:
    BbCameraSession *m_session;
};

QT_END_NAMESPACE

#endif
