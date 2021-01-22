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

#include <QtCore/qdebug.h>

#include "dscameracontrol.h"
#include "dscameraservice.h"
#include "dscamerasession.h"

QT_BEGIN_NAMESPACE

DSCameraControl::DSCameraControl(QObject *parent)
    : QCameraControl(parent)
{
    m_session = qobject_cast<DSCameraSession*>(parent);
    connect(m_session, &DSCameraSession::statusChanged, this,
            [&](QCamera::Status status) {
                if (status == QCamera::UnloadedStatus)
                    m_state = QCamera::UnloadedState;
                emit statusChanged(status);
            });
    connect(m_session, &DSCameraSession::cameraError,
            this, &DSCameraControl::error);
}

DSCameraControl::~DSCameraControl() = default;

void DSCameraControl::setState(QCamera::State state)
{
    if (m_state == state)
        return;

    bool succeeded = false;
    switch (state) {
    case QCamera::UnloadedState:
        succeeded = m_session->unload();
        break;
    case QCamera::LoadedState:
    case QCamera::ActiveState:
        if (m_state == QCamera::UnloadedState && !m_session->load())
            return;

        if (state == QCamera::ActiveState)
            succeeded = m_session->startPreview();
        else
            succeeded = m_session->stopPreview();

        break;
    }

    if (succeeded) {
        m_state = state;
        emit stateChanged(m_state);
    }
}

bool DSCameraControl::isCaptureModeSupported(QCamera::CaptureModes mode) const
{
    bool bCaptureSupported = false;
    switch (mode) {
    case QCamera::CaptureStillImage:
        bCaptureSupported = true;
        break;
    case QCamera::CaptureVideo:
        bCaptureSupported = false;
        break;
    }
    return bCaptureSupported;
}

void DSCameraControl::setCaptureMode(QCamera::CaptureModes mode)
{
    if (m_captureMode != mode && isCaptureModeSupported(mode)) {
        m_captureMode = mode;
        emit captureModeChanged(mode);
    }
}

QCamera::Status DSCameraControl::status() const
{
    return m_session->status();
}

QT_END_NAMESPACE
