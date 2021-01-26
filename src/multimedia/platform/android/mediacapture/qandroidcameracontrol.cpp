/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qandroidcameracontrol_p.h"
#include "qandroidcamerasession_p.h"
#include <qmediadevicemanager.h>
#include <qcamerainfo.h>
#include <qtimer.h>

QT_BEGIN_NAMESPACE

QAndroidCameraControl::QAndroidCameraControl(QAndroidCameraSession *session)
    : QCameraControl(0)
    , m_cameraSession(session)

{
    connect(m_cameraSession, SIGNAL(statusChanged(QCamera::Status)),
            this, SIGNAL(statusChanged(QCamera::Status)));

    connect(m_cameraSession, SIGNAL(stateChanged(QCamera::State)),
            this, SIGNAL(stateChanged(QCamera::State)));

    connect(m_cameraSession, SIGNAL(error(int,QString)), this, SIGNAL(error(int,QString)));

    connect(m_cameraSession, SIGNAL(captureModeChanged(QCamera::CaptureModes)),
            this, SIGNAL(captureModeChanged(QCamera::CaptureModes)));

    connect(m_cameraSession, SIGNAL(opened()), this, SLOT(onCameraOpened()));

    m_recalculateTimer = new QTimer(this);
    m_recalculateTimer->setInterval(1000);
    m_recalculateTimer->setSingleShot(true);
    connect(m_recalculateTimer, SIGNAL(timeout()), this, SLOT(onRecalculateTimeOut()));
}

QAndroidCameraControl::~QAndroidCameraControl()
{
}

QCamera::CaptureModes QAndroidCameraControl::captureMode() const
{
    return m_cameraSession->captureMode();
}

void QAndroidCameraControl::setCaptureMode(QCamera::CaptureModes mode)
{
    m_cameraSession->setCaptureMode(mode);
}

bool QAndroidCameraControl::isCaptureModeSupported(QCamera::CaptureModes mode) const
{
    return m_cameraSession->isCaptureModeSupported(mode);
}

void QAndroidCameraControl::setState(QCamera::State state)
{
    m_cameraSession->setState(state);
}

QCamera::State QAndroidCameraControl::state() const
{
    return m_cameraSession->state();
}

QCamera::Status QAndroidCameraControl::status() const
{
    return m_cameraSession->status();
}

void QAndroidCameraControl::setCamera(const QCameraInfo &camera)
{
    int id = 0;
    auto cameras = QMediaDeviceManager::videoInputs();
    for (int i = 0; i < cameras.size(); ++i) {
        if (cameras.at(i) == camera) {
            id = i;
            break;
        }
    }
    m_cameraSession->setSelectedCamera(id);
}


bool QAndroidCameraControl::canChangeProperty(PropertyChangeType changeType, QCamera::Status status) const
{
    Q_UNUSED(status);

    switch (changeType) {
    case QCameraControl::CaptureMode:
    case QCameraControl::ImageEncodingSettings:
    case QCameraControl::VideoEncodingSettings:
    case QCameraControl::Viewfinder:
        return true;
    default:
        return false;
    }
}


QCamera::LockTypes QAndroidCameraControl::supportedLocks() const
{
    return m_supportedLocks;
}

QCamera::LockStatus QAndroidCameraControl::lockStatus(QCamera::LockType lock) const
{
    if (!m_supportedLocks.testFlag(lock) || !m_cameraSession->camera())
        return QCamera::Unlocked;

    if (lock == QCamera::LockFocus)
        return m_focusLockStatus;

    if (lock == QCamera::LockExposure)
        return m_exposureLockStatus;

    if (lock == QCamera::LockWhiteBalance)
        return m_whiteBalanceLockStatus;

    return QCamera::Unlocked;
}

void QAndroidCameraControl::searchAndLock(QCamera::LockTypes locks)
{
    if (!m_cameraSession->camera())
        return;

    // filter out unsupported locks
    locks &= m_supportedLocks;

    if (locks.testFlag(QCamera::LockFocus)) {
        QString focusMode = m_cameraSession->camera()->getFocusMode();
        if (focusMode == QLatin1String("auto")
                || focusMode == QLatin1String("macro")
                || focusMode == QLatin1String("continuous-picture")
                || focusMode == QLatin1String("continuous-video")) {

            if (m_focusLockStatus == QCamera::Searching)
                m_cameraSession->camera()->cancelAutoFocus();
            else
                setFocusLockStatus(QCamera::Searching, QCamera::UserRequest);

            m_cameraSession->camera()->autoFocus();

        } else {
            setFocusLockStatus(QCamera::Locked, QCamera::LockAcquired);
        }
    }

    if (locks.testFlag(QCamera::LockExposure) && m_exposureLockStatus != QCamera::Searching) {
        if (m_cameraSession->camera()->getAutoExposureLock()) {
            // if already locked, unlock and give some time to recalculate exposure
            m_cameraSession->camera()->setAutoExposureLock(false);
            setExposureLockStatus(QCamera::Searching, QCamera::UserRequest);
        } else {
            m_cameraSession->camera()->setAutoExposureLock(true);
            setExposureLockStatus(QCamera::Locked, QCamera::LockAcquired);
        }
    }

    if (locks.testFlag(QCamera::LockWhiteBalance) && m_whiteBalanceLockStatus != QCamera::Searching) {
        if (m_cameraSession->camera()->getAutoWhiteBalanceLock()) {
            // if already locked, unlock and give some time to recalculate white balance
            m_cameraSession->camera()->setAutoWhiteBalanceLock(false);
            setWhiteBalanceLockStatus(QCamera::Searching, QCamera::UserRequest);
        } else {
            m_cameraSession->camera()->setAutoWhiteBalanceLock(true);
            setWhiteBalanceLockStatus(QCamera::Locked, QCamera::LockAcquired);
        }
    }

    if (m_exposureLockStatus == QCamera::Searching || m_whiteBalanceLockStatus == QCamera::Searching)
        m_recalculateTimer->start();
}

void QAndroidCameraControl::unlock(QCamera::LockTypes locks)
{
    if (!m_cameraSession->camera())
        return;

    if (m_recalculateTimer->isActive())
        m_recalculateTimer->stop();

    // filter out unsupported locks
    locks &= m_supportedLocks;

    if (locks.testFlag(QCamera::LockFocus)) {
        m_cameraSession->camera()->cancelAutoFocus();
        setFocusLockStatus(QCamera::Unlocked, QCamera::UserRequest);
    }

    if (locks.testFlag(QCamera::LockExposure)) {
        m_cameraSession->camera()->setAutoExposureLock(false);
        setExposureLockStatus(QCamera::Unlocked, QCamera::UserRequest);
    }

    if (locks.testFlag(QCamera::LockWhiteBalance)) {
        m_cameraSession->camera()->setAutoWhiteBalanceLock(false);
        setWhiteBalanceLockStatus(QCamera::Unlocked, QCamera::UserRequest);
    }
}

void QAndroidCameraControl::onCameraOpened()
{
    m_supportedLocks = QCamera::NoLock;
    m_focusLockStatus = QCamera::Unlocked;
    m_exposureLockStatus = QCamera::Unlocked;
    m_whiteBalanceLockStatus = QCamera::Unlocked;

    // check if focus lock is supported
    QStringList focusModes = m_cameraSession->camera()->getSupportedFocusModes();
    for (int i = 0; i < focusModes.size(); ++i) {
        const QString &focusMode = focusModes.at(i);
        if (focusMode == QLatin1String("auto")
                || focusMode == QLatin1String("continuous-picture")
                || focusMode == QLatin1String("continuous-video")
                || focusMode == QLatin1String("macro")) {

            m_supportedLocks |= QCamera::LockFocus;
            setFocusLockStatus(QCamera::Unlocked, QCamera::UserRequest);

            connect(m_cameraSession->camera(), SIGNAL(autoFocusComplete(bool)),
                    this, SLOT(onCameraAutoFocusComplete(bool)));

            break;
        }
    }

    if (m_cameraSession->camera()->isAutoExposureLockSupported()) {
        m_supportedLocks |= QCamera::LockExposure;
        setExposureLockStatus(QCamera::Unlocked, QCamera::UserRequest);
    }

    if (m_cameraSession->camera()->isAutoWhiteBalanceLockSupported()) {
        m_supportedLocks |= QCamera::LockWhiteBalance;
        setWhiteBalanceLockStatus(QCamera::Unlocked, QCamera::UserRequest);

        connect(m_cameraSession->camera(), SIGNAL(whiteBalanceChanged()),
                this, SLOT(onWhiteBalanceChanged()));
    }
}

void QAndroidCameraControl::onCameraAutoFocusComplete(bool success)
{
    m_focusLockStatus = success ? QCamera::Locked : QCamera::Unlocked;
    QCamera::LockChangeReason reason = success ? QCamera::LockAcquired : QCamera::LockFailed;
    emit lockStatusChanged(QCamera::LockFocus, m_focusLockStatus, reason);
}

void QAndroidCameraControl::onRecalculateTimeOut()
{
    if (m_exposureLockStatus == QCamera::Searching) {
        m_cameraSession->camera()->setAutoExposureLock(true);
        setExposureLockStatus(QCamera::Locked, QCamera::LockAcquired);
    }

    if (m_whiteBalanceLockStatus == QCamera::Searching) {
        m_cameraSession->camera()->setAutoWhiteBalanceLock(true);
        setWhiteBalanceLockStatus(QCamera::Locked, QCamera::LockAcquired);
    }
}

void QAndroidCameraControl::onWhiteBalanceChanged()
{
    // changing the white balance mode releases the white balance lock
    if (m_whiteBalanceLockStatus != QCamera::Unlocked)
        setWhiteBalanceLockStatus(QCamera::Unlocked, QCamera::LockLost);
}

void QAndroidCameraControl::setFocusLockStatus(QCamera::LockStatus status, QCamera::LockChangeReason reason)
{
    m_focusLockStatus = status;
    emit lockStatusChanged(QCamera::LockFocus, m_focusLockStatus, reason);
}

void QAndroidCameraControl::setWhiteBalanceLockStatus(QCamera::LockStatus status, QCamera::LockChangeReason reason)
{
    m_whiteBalanceLockStatus = status;
    emit lockStatusChanged(QCamera::LockWhiteBalance, m_whiteBalanceLockStatus, reason);
}

void QAndroidCameraControl::setExposureLockStatus(QCamera::LockStatus status, QCamera::LockChangeReason reason)
{
    m_exposureLockStatus = status;
    emit lockStatusChanged(QCamera::LockExposure, m_exposureLockStatus, reason);
}

QT_END_NAMESPACE
