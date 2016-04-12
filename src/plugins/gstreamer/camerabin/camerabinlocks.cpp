/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
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
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "camerabinlocks.h"
#include "camerabinsession.h"
#include "camerabinfocus.h"
#include "camerabinimageprocessing.h"

#include <QtCore/qcoreevent.h>

#include <gst/interfaces/photography.h>

#include <QDebug>

QT_BEGIN_NAMESPACE

CameraBinLocks::CameraBinLocks(CameraBinSession *session)
    :QCameraLocksControl(session),
     m_session(session),
     m_focus(m_session->cameraFocusControl())
{
    connect(m_focus, SIGNAL(_q_focusStatusChanged(QCamera::LockStatus,QCamera::LockChangeReason)),
            this, SLOT(updateFocusStatus(QCamera::LockStatus,QCamera::LockChangeReason)));
}

CameraBinLocks::~CameraBinLocks()
{
}

QCamera::LockTypes CameraBinLocks::supportedLocks() const
{
    QCamera::LockTypes locks = QCamera::LockFocus;

#if GST_CHECK_VERSION(1, 2, 0)
    if (GstPhotography *photography = m_session->photography()) {
        if (gst_photography_get_capabilities(photography) & GST_PHOTOGRAPHY_CAPS_WB_MODE)
            locks |= QCamera::LockWhiteBalance;

    if (GstElement *source = m_session->cameraSource()) {
            if (g_object_class_find_property(
                        G_OBJECT_GET_CLASS(source), "exposure-mode")) {
                locks |= QCamera::LockExposure;
            }
        }
    }
#endif

    return locks;
}

QCamera::LockStatus CameraBinLocks::lockStatus(QCamera::LockType lock) const
{
    switch (lock) {
    case QCamera::LockFocus:
        return m_focus->focusStatus();
#if GST_CHECK_VERSION(1, 2, 0)
    case QCamera::LockExposure:
        if (m_pendingLocks & QCamera::LockExposure)
            return QCamera::Searching;
        return isExposureLocked() ? QCamera::Locked : QCamera::Unlocked;
    case QCamera::LockWhiteBalance:
        if (m_pendingLocks & QCamera::LockWhiteBalance)
            return QCamera::Searching;
        return isWhiteBalanceLocked() ? QCamera::Locked : QCamera::Unlocked;
#endif
    default:
        return QCamera::Unlocked;
    }

    return lock == QCamera::LockFocus ? m_focus->focusStatus() : QCamera::Unlocked;
}

void CameraBinLocks::searchAndLock(QCamera::LockTypes locks)
{
    m_pendingLocks &= ~locks;

    if (locks & QCamera::LockFocus) {
        m_pendingLocks |= QCamera::LockFocus;
        m_focus->_q_startFocusing();
    }
#if GST_CHECK_VERSION(1, 2, 0)
    if (!m_pendingLocks)
        m_lockTimer.stop();

    if (locks & QCamera::LockExposure) {
        if (isExposureLocked()) {
            unlockExposure(QCamera::Searching, QCamera::UserRequest);
            m_pendingLocks |= QCamera::LockExposure;
            m_lockTimer.start(1000, this);
        } else {
            lockExposure(QCamera::UserRequest);
        }
    }
    if (locks & QCamera::LockWhiteBalance) {
        if (isWhiteBalanceLocked()) {
            unlockWhiteBalance(QCamera::Searching, QCamera::UserRequest);
            m_pendingLocks |= QCamera::LockWhiteBalance;
            m_lockTimer.start(1000, this);
        } else {
            lockWhiteBalance(QCamera::UserRequest);
        }
    }
#endif

}

void CameraBinLocks::unlock(QCamera::LockTypes locks)
{
    m_pendingLocks &= ~locks;

    if (locks & QCamera::LockFocus)
        m_focus->_q_stopFocusing();

#if GST_CHECK_VERSION(1, 2, 0)
    if (!m_pendingLocks)
        m_lockTimer.stop();

    if (locks & QCamera::LockExposure)
        unlockExposure(QCamera::Unlocked, QCamera::UserRequest);
    if (locks & QCamera::LockWhiteBalance)
        unlockWhiteBalance(QCamera::Unlocked, QCamera::UserRequest);
#endif
}

void CameraBinLocks::updateFocusStatus(QCamera::LockStatus status, QCamera::LockChangeReason reason)
{
    if (status != QCamera::Searching)
        m_pendingLocks &= ~QCamera::LockFocus;

#if GST_CHECK_VERSION(1, 2, 0)
    if (status == QCamera::Locked && !m_lockTimer.isActive()) {
        if (m_pendingLocks & QCamera::LockExposure)
            lockExposure(QCamera::LockAcquired);
        if (m_pendingLocks & QCamera::LockWhiteBalance)
            lockWhiteBalance(QCamera::LockAcquired);
    }
#endif
    emit lockStatusChanged(QCamera::LockFocus, status, reason);
}

#if GST_CHECK_VERSION(1, 2, 0)

void CameraBinLocks::timerEvent(QTimerEvent *event)
{
    if (event->timerId() != m_lockTimer.timerId())
        return QCameraLocksControl::timerEvent(event);

    m_lockTimer.stop();

    if (!(m_pendingLocks & QCamera::LockFocus)) {
        if (m_pendingLocks & QCamera::LockExposure)
            lockExposure(QCamera::LockAcquired);
        if (m_pendingLocks & QCamera::LockWhiteBalance)
            lockWhiteBalance(QCamera::LockAcquired);
    }
}

bool CameraBinLocks::isExposureLocked() const
{
    if (GstElement *source = m_session->cameraSource()) {
        GstPhotographyExposureMode exposureMode = GST_PHOTOGRAPHY_EXPOSURE_MODE_AUTO;
        g_object_get (G_OBJECT(source), "exposure-mode", &exposureMode, NULL);
        return exposureMode == GST_PHOTOGRAPHY_EXPOSURE_MODE_MANUAL;
    } else {
        return false;
    }
}

void CameraBinLocks::lockExposure(QCamera::LockChangeReason reason)
{
    GstElement *source = m_session->cameraSource();
    if (!source)
        return;

    m_pendingLocks &= ~QCamera::LockExposure;
    g_object_set(
                G_OBJECT(source),
                "exposure-mode",
                GST_PHOTOGRAPHY_EXPOSURE_MODE_MANUAL,
                NULL);
    emit lockStatusChanged(QCamera::LockExposure, QCamera::Locked, reason);
}

void CameraBinLocks::unlockExposure(QCamera::LockStatus status, QCamera::LockChangeReason reason)
{
    GstElement *source = m_session->cameraSource();
    if (!source)
        return;

    g_object_set(
                G_OBJECT(source),
                "exposure-mode",
                GST_PHOTOGRAPHY_EXPOSURE_MODE_AUTO,
                NULL);
    emit lockStatusChanged(QCamera::LockExposure, status, reason);
}

bool CameraBinLocks::isWhiteBalanceLocked() const
{
    if (GstPhotography *photography = m_session->photography()) {
        GstPhotographyWhiteBalanceMode whiteBalanceMode;
        return gst_photography_get_white_balance_mode(photography, &whiteBalanceMode)
                && whiteBalanceMode == GST_PHOTOGRAPHY_WB_MODE_MANUAL;
    } else {
        return false;
    }
}

void CameraBinLocks::lockWhiteBalance(QCamera::LockChangeReason reason)
{
    m_pendingLocks &= ~QCamera::LockWhiteBalance;
    m_session->imageProcessingControl()->lockWhiteBalance();
    emit lockStatusChanged(QCamera::LockWhiteBalance, QCamera::Locked, reason);
}

void CameraBinLocks::unlockWhiteBalance(
        QCamera::LockStatus status, QCamera::LockChangeReason reason)
{
    m_session->imageProcessingControl()->lockWhiteBalance();
    emit lockStatusChanged(QCamera::LockWhiteBalance, status, reason);
}

#endif

QT_END_NAMESPACE
