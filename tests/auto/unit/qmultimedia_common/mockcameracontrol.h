/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef MOCKCAMERACONTROL_H
#define MOCKCAMERACONTROL_H

#include "qcameracontrol.h"
#include <qtimer.h>

class MockCameraControl : public QCameraControl
{
    friend class MockCaptureControl;
    Q_OBJECT
public:
    MockCameraControl(QObject *parent = 0):
            QCameraControl(parent),
            m_state(QCamera::UnloadedState),
            m_captureMode(QCamera::CaptureStillImage),
            m_status(QCamera::UnloadedStatus),
            m_propertyChangesSupported(false)
    {
        QCameraViewfinderSettings s;
        s.setResolution(640, 480);
        s.setMinimumFrameRate(30);
        s.setMaximumFrameRate(30);
        s.setPixelFormat(QVideoFrame::Format_NV12);
        s.setPixelAspectRatio(1, 1);
        supportedSettings.append(s);

        s.setResolution(1280, 720);
        s.setMinimumFrameRate(10);
        s.setMaximumFrameRate(10);
        s.setPixelFormat(QVideoFrame::Format_NV12);
        s.setPixelAspectRatio(1, 1);
        supportedSettings.append(s);

        s.setResolution(1920, 1080);
        s.setMinimumFrameRate(5);
        s.setMaximumFrameRate(10);
        s.setPixelFormat(QVideoFrame::Format_BGR32);
        s.setPixelAspectRatio(2, 1);
        supportedSettings.append(s);

        s.setResolution(1280, 720);
        s.setMinimumFrameRate(10);
        s.setMaximumFrameRate(10);
        s.setPixelFormat(QVideoFrame::Format_YV12);
        s.setPixelAspectRatio(1, 1);
        supportedSettings.append(s);

        s.setResolution(1280, 720);
        s.setMinimumFrameRate(30);
        s.setMaximumFrameRate(30);
        s.setPixelFormat(QVideoFrame::Format_YV12);
        s.setPixelAspectRatio(1, 1);
        supportedSettings.append(s);

        s.setResolution(320, 240);
        s.setMinimumFrameRate(30);
        s.setMaximumFrameRate(30);
        s.setPixelFormat(QVideoFrame::Format_NV12);
        s.setPixelAspectRatio(1, 1);
        supportedSettings.append(s);
    }

    ~MockCameraControl() {}

    void start() { m_state = QCamera::ActiveState; }
    virtual void stop() { m_state = QCamera::UnloadedState; }
    QCamera::State state() const { return m_state; }
    void setState(QCamera::State state) {
        if (m_state != state) {
            m_state = state;

            switch (state) {
            case QCamera::UnloadedState:
                m_status = QCamera::UnloadedStatus;
                break;
            case QCamera::LoadedState:
                m_status = QCamera::LoadedStatus;
                break;
            case QCamera::ActiveState:
                m_status = QCamera::ActiveStatus;
                break;
            default:
                emit error(QCamera::NotSupportedFeatureError, "State not supported.");
                return;
            }

            emit stateChanged(m_state);
            emit statusChanged(m_status);
        }
    }

    QCamera::Status status() const { return m_status; }

    QCamera::CaptureModes captureMode() const { return m_captureMode; }
    void setCaptureMode(QCamera::CaptureModes mode)
    {
        if (m_captureMode != mode) {
            if (m_state == QCamera::ActiveState && !m_propertyChangesSupported)
                return;
            m_captureMode = mode;
            emit captureModeChanged(mode);
        }
    }

    bool isCaptureModeSupported(QCamera::CaptureModes mode) const
    {
        return mode == QCamera::CaptureStillImage || mode == QCamera::CaptureVideo;
    }

    bool canChangeProperty(PropertyChangeType changeType, QCamera::Status status) const
    {
        Q_UNUSED(status);
        if (changeType == QCameraControl::ImageEncodingSettings && m_captureMode == QCamera::CaptureVideo)
            return true;
        else if (changeType== QCameraControl::VideoEncodingSettings)
            return true;
        else
            return m_propertyChangesSupported;
    }

    QCamera::LockTypes supportedLocks() const
    {
        return QCamera::LockExposure | QCamera::LockFocus;
    }

    QCamera::LockStatus lockStatus(QCamera::LockType lock) const
    {
        switch (lock) {
        case QCamera::LockExposure:
            return m_exposureLock;
        case QCamera::LockFocus:
            return m_focusLock;
        default:
            return QCamera::Unlocked;
        }
    }

    void searchAndLock(QCamera::LockTypes locks)
    {
        if (locks & QCamera::LockExposure) {
            QCamera::LockStatus newStatus = locks & QCamera::LockFocus ? QCamera::Searching : QCamera::Locked;

            if (newStatus != m_exposureLock)
                emit lockStatusChanged(QCamera::LockExposure,
                                       m_exposureLock = newStatus,
                                       QCamera::UserRequest);
        }

        if (locks & QCamera::LockFocus) {
            emit lockStatusChanged(QCamera::LockFocus,
                                   m_focusLock = QCamera::Searching,
                                   QCamera::UserRequest);

            QTimer::singleShot(5, this, SLOT(focused()));
        }
    }

    void unlock(QCamera::LockTypes locks) {
        if (locks & QCamera::LockFocus && m_focusLock != QCamera::Unlocked) {
            emit lockStatusChanged(QCamera::LockFocus,
                                   m_focusLock = QCamera::Unlocked,
                                   QCamera::UserRequest);
        }

        if (locks & QCamera::LockExposure && m_exposureLock != QCamera::Unlocked) {
            emit lockStatusChanged(QCamera::LockExposure,
                                   m_exposureLock = QCamera::Unlocked,
                                   QCamera::UserRequest);
        }
    }

    QCameraViewfinderSettings viewfinderSettings() const
    {
        return settings;
    }

    void setViewfinderSettings(const QCameraViewfinderSettings &s)
    {
        settings = s;
    }

    QList<QCameraViewfinderSettings> supportedViewfinderSettings() const
    {
        return supportedSettings;
    }

    QCameraViewfinderSettings settings;
    QList<QCameraViewfinderSettings> supportedSettings;

    /* helper method to emit the signal with LockChangeReason */
    void setLockChangeReason (QCamera::LockChangeReason lockChangeReason)
    {
        emit lockStatusChanged(QCamera::NoLock,
                               QCamera::Unlocked,
                               lockChangeReason);

    }

    /* helper method to emit the signal error */
    void setError(QCamera::Error err, QString errorString)
    {
        emit error(err, errorString);
    }

    /* helper method to emit the signal statusChaged */
    void setStatus(QCamera::Status newStatus)
    {
        m_status = newStatus;
        emit statusChanged(newStatus);
    }

    QCamera::State m_state;
    QCamera::CaptureModes m_captureMode;
    QCamera::Status m_status;
    bool m_propertyChangesSupported;


private slots:
    void focused()
    {
        if (m_focusLock == QCamera::Searching) {
            emit lockStatusChanged(QCamera::LockFocus,
                                   m_focusLock = QCamera::Locked,
                                   QCamera::UserRequest);
        }

        if (m_exposureLock == QCamera::Searching) {
            emit lockStatusChanged(QCamera::LockExposure,
                                   m_exposureLock = QCamera::Locked,
                                   QCamera::UserRequest);
        }
    }


private:
    QCamera::LockStatus m_focusLock = QCamera::Unlocked;
    QCamera::LockStatus m_exposureLock = QCamera::Unlocked;
};



#endif // MOCKCAMERACONTROL_H
