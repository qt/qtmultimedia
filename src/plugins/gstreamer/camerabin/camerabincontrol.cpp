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

#include <QtMultimedia/private/qtmultimediaglobal_p.h>
#include "camerabincontrol.h"
#include "camerabincontainer.h"
#include "camerabinaudioencoder.h"
#include "camerabinvideoencoder.h"
#include "camerabinimageencoder.h"
#include "camerabinfocus.h"
#include "camerabinimageprocessing.h"

#include <QtCore/qdebug.h>
#include <QtCore/qfile.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qcoreevent.h>

QT_BEGIN_NAMESPACE

//#define CAMEABIN_DEBUG 1
#define ENUM_NAME(c,e,v) (c::staticMetaObject.enumerator(c::staticMetaObject.indexOfEnumerator(e)).valueToKey((v)))

CameraBinControl::CameraBinControl(CameraBinSession *session)
    :QCameraControl(session),
    m_session(session),
    m_state(QCamera::UnloadedState),
    m_reloadPending(false),
    m_focus(m_session->cameraFocusControl())
{
    connect(m_session, SIGNAL(statusChanged(QCamera::Status)),
            this, SIGNAL(statusChanged(QCamera::Status)));

    connect(m_session, SIGNAL(viewfinderChanged()),
            SLOT(reloadLater()));
    connect(m_session, SIGNAL(readyChanged(bool)),
            SLOT(reloadLater()));
    connect(m_session, SIGNAL(error(int,QString)),
            SLOT(handleCameraError(int,QString)));

    connect(m_session, SIGNAL(busyChanged(bool)),
            SLOT(handleBusyChanged(bool)));

    connect(m_focus, SIGNAL(_q_focusStatusChanged(QCamera::LockStatus,QCamera::LockChangeReason)),
            this, SLOT(updateFocusStatus(QCamera::LockStatus,QCamera::LockChangeReason)));
}

CameraBinControl::~CameraBinControl()
{
}

QCamera::CaptureModes CameraBinControl::captureMode() const
{
    return m_session->captureMode();
}

void CameraBinControl::setCaptureMode(QCamera::CaptureModes mode)
{
    if (m_session->captureMode() != mode) {
        m_session->setCaptureMode(mode);

        emit captureModeChanged(mode);
    }
}

bool CameraBinControl::isCaptureModeSupported(QCamera::CaptureModes mode) const
{
    return mode == QCamera::CaptureStillImage || mode == QCamera::CaptureVideo;
}

void CameraBinControl::setState(QCamera::State state)
{
#ifdef CAMEABIN_DEBUG
    qDebug() << Q_FUNC_INFO << ENUM_NAME(QCamera, "State", state);
#endif
    if (m_state != state) {
        m_state = state;

        //special case for stopping the camera while it's busy,
        //it should be delayed until the camera is idle
        if ((state == QCamera::LoadedState || state == QCamera::UnloadedState) &&
                m_session->status() == QCamera::ActiveStatus &&
                m_session->isBusy()) {
#ifdef CAMEABIN_DEBUG
            qDebug() << Q_FUNC_INFO << "Camera is busy,"
                     << state == QCamera::LoadedState ? "QCamera::stop()" : "QCamera::unload()"
                     << "is delayed";
#endif
            emit stateChanged(m_state);
            return;
        }

        //postpone changing to Active if the session is nor ready yet
        if (state == QCamera::ActiveState) {
            if (m_session->isReady()) {
                m_session->setState(state);
            } else {
#ifdef CAMEABIN_DEBUG
                qDebug() << "Camera session is not ready yet, postpone activating";
#endif
            }
        } else
            m_session->setState(state);

        emit stateChanged(m_state);
    }
}

QCamera::State CameraBinControl::state() const
{
    return m_state;
}

QCamera::Status CameraBinControl::status() const
{
    return m_session->status();
}

void CameraBinControl::reloadLater()
{
#ifdef CAMEABIN_DEBUG
    qDebug() << "CameraBinControl: reload pipeline requested" << ENUM_NAME(QCamera, "State", m_state);
#endif
    if (!m_reloadPending && m_state == QCamera::ActiveState) {
        m_reloadPending = true;

        if (!m_session->isBusy()) {
            m_session->setState(QCamera::LoadedState);
            QMetaObject::invokeMethod(this, "delayedReload", Qt::QueuedConnection);
        }
    }
}

void CameraBinControl::handleBusyChanged(bool busy)
{
    if (!busy && m_session->status() == QCamera::ActiveStatus) {
        if (m_state != QCamera::ActiveState) {
            m_session->setState(m_state);
        } else if (m_reloadPending) {
            //handle delayed reload because of busy camera
            m_session->setState(QCamera::LoadedState);
            QMetaObject::invokeMethod(this, "delayedReload", Qt::QueuedConnection);
        }
    }
}

void CameraBinControl::handleCameraError(int errorCode, const QString &errorString)
{
    emit error(errorCode, errorString);
    setState(QCamera::UnloadedState);
}

void CameraBinControl::delayedReload()
{
#ifdef CAMEABIN_DEBUG
    qDebug() << "CameraBinControl: reload pipeline";
#endif
    if (m_reloadPending) {
        m_reloadPending = false;
        if (m_state == QCamera::ActiveState && m_session->isReady())
                m_session->setState(QCamera::ActiveState);
    }
}

bool CameraBinControl::canChangeProperty(PropertyChangeType changeType, QCamera::Status status) const
{
    Q_UNUSED(status);

    switch (changeType) {
    case QCameraControl::Viewfinder:
        return true;
    case QCameraControl::CaptureMode:
    case QCameraControl::ImageEncodingSettings:
    case QCameraControl::VideoEncodingSettings:
    case QCameraControl::ViewfinderSettings:
    default:
        return status != QCamera::ActiveStatus;
    }
}

#define VIEWFINDER_COLORSPACE_CONVERSION 0x00000004

bool CameraBinControl::viewfinderColorSpaceConversion() const
{
    gint flags = 0;
    g_object_get(G_OBJECT(m_session->cameraBin()), "flags", &flags, NULL);

    return flags & VIEWFINDER_COLORSPACE_CONVERSION;
}

void CameraBinControl::setViewfinderColorSpaceConversion(bool enabled)
{
    gint flags = 0;
    g_object_get(G_OBJECT(m_session->cameraBin()), "flags", &flags, NULL);

    if (enabled)
        flags |= VIEWFINDER_COLORSPACE_CONVERSION;
    else
        flags &= ~VIEWFINDER_COLORSPACE_CONVERSION;

    g_object_set(G_OBJECT(m_session->cameraBin()), "flags", flags, NULL);
}

QCamera::LockTypes CameraBinControl::supportedLocks() const
{
    QCamera::LockTypes locks = QCamera::LockFocus;

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

    return locks;
}

QCamera::LockStatus CameraBinControl::lockStatus(QCamera::LockType lock) const
{
    switch (lock) {
    case QCamera::LockFocus:
        return m_focus->focusStatus();
    case QCamera::LockExposure:
        if (m_pendingLocks & QCamera::LockExposure)
            return QCamera::Searching;
        return isExposureLocked() ? QCamera::Locked : QCamera::Unlocked;
    case QCamera::LockWhiteBalance:
        if (m_pendingLocks & QCamera::LockWhiteBalance)
            return QCamera::Searching;
        return isWhiteBalanceLocked() ? QCamera::Locked : QCamera::Unlocked;
    default:
        return QCamera::Unlocked;
    }
}

void CameraBinControl::searchAndLock(QCamera::LockTypes locks)
{
    m_pendingLocks &= ~locks;

    if (locks & QCamera::LockFocus) {
        m_pendingLocks |= QCamera::LockFocus;
        m_focus->_q_startFocusing();
    }
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
}

void CameraBinControl::unlock(QCamera::LockTypes locks)
{
    m_pendingLocks &= ~locks;

    if (locks & QCamera::LockFocus)
        m_focus->_q_stopFocusing();

    if (!m_pendingLocks)
        m_lockTimer.stop();

    if (locks & QCamera::LockExposure)
        unlockExposure(QCamera::Unlocked, QCamera::UserRequest);
    if (locks & QCamera::LockWhiteBalance)
        unlockWhiteBalance(QCamera::Unlocked, QCamera::UserRequest);
}

void CameraBinControl::updateFocusStatus(QCamera::LockStatus status, QCamera::LockChangeReason reason)
{
    if (status != QCamera::Searching)
        m_pendingLocks &= ~QCamera::LockFocus;

    if (status == QCamera::Locked && !m_lockTimer.isActive()) {
        if (m_pendingLocks & QCamera::LockExposure)
            lockExposure(QCamera::LockAcquired);
        if (m_pendingLocks & QCamera::LockWhiteBalance)
            lockWhiteBalance(QCamera::LockAcquired);
    }
    emit lockStatusChanged(QCamera::LockFocus, status, reason);
}

void CameraBinControl::timerEvent(QTimerEvent *event)
{
    if (event->timerId() != m_lockTimer.timerId())
        return QCameraControl::timerEvent(event);

    m_lockTimer.stop();

    if (!(m_pendingLocks & QCamera::LockFocus)) {
        if (m_pendingLocks & QCamera::LockExposure)
            lockExposure(QCamera::LockAcquired);
        if (m_pendingLocks & QCamera::LockWhiteBalance)
            lockWhiteBalance(QCamera::LockAcquired);
    }
}

bool CameraBinControl::isExposureLocked() const
{
    if (GstElement *source = m_session->cameraSource()) {
        GstPhotographyExposureMode exposureMode = GST_PHOTOGRAPHY_EXPOSURE_MODE_AUTO;
        g_object_get (G_OBJECT(source), "exposure-mode", &exposureMode, NULL);
        return exposureMode == GST_PHOTOGRAPHY_EXPOSURE_MODE_MANUAL;
    }

    return false;
}

void CameraBinControl::lockExposure(QCamera::LockChangeReason reason)
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

void CameraBinControl::unlockExposure(QCamera::LockStatus status, QCamera::LockChangeReason reason)
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

bool CameraBinControl::isWhiteBalanceLocked() const
{
    if (GstPhotography *photography = m_session->photography()) {
        GstPhotographyWhiteBalanceMode whiteBalanceMode;
        return gst_photography_get_white_balance_mode(photography, &whiteBalanceMode)
                && whiteBalanceMode == GST_PHOTOGRAPHY_WB_MODE_MANUAL;
    }

    return false;
}

void CameraBinControl::lockWhiteBalance(QCamera::LockChangeReason reason)
{
    m_pendingLocks &= ~QCamera::LockWhiteBalance;
    m_session->imageProcessingControl()->lockWhiteBalance();
    emit lockStatusChanged(QCamera::LockWhiteBalance, QCamera::Locked, reason);
}

void CameraBinControl::unlockWhiteBalance(
        QCamera::LockStatus status, QCamera::LockChangeReason reason)
{
    m_session->imageProcessingControl()->lockWhiteBalance();
    emit lockStatusChanged(QCamera::LockWhiteBalance, status, reason);
}

QT_END_NAMESPACE
