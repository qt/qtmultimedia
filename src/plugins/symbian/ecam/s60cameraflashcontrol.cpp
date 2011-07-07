/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Mobility Components.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/qstring.h>

#include "s60cameraflashcontrol.h"
#include "s60cameraservice.h"
#include "s60imagecapturesession.h"

S60CameraFlashControl::S60CameraFlashControl(QObject *parent) :
    QCameraFlashControl(parent)
{
}

S60CameraFlashControl::S60CameraFlashControl(S60ImageCaptureSession *session, QObject *parent) :
    QCameraFlashControl(parent),
    m_session(0),
    m_service(0),
    m_advancedSettings(0),
    m_flashMode(QCameraExposure::FlashOff)
{
    m_session = session;

    connect(m_session, SIGNAL(advancedSettingChanged()), this, SLOT(resetAdvancedSetting()));
    m_advancedSettings = m_session->advancedSettings();

    if (m_advancedSettings)
        connect(m_advancedSettings, SIGNAL(flashReady(bool)), this, SIGNAL(flashReady(bool)));
}

S60CameraFlashControl::~S60CameraFlashControl()
{
    m_advancedSettings = 0;
}

void S60CameraFlashControl::resetAdvancedSetting()
{
    m_advancedSettings = m_session->advancedSettings();
    if (m_advancedSettings)
        connect(m_advancedSettings, SIGNAL(flashReady(bool)), this, SIGNAL(flashReady(bool)));
}

QCameraExposure::FlashModes S60CameraFlashControl::flashMode() const
{
    return m_session->flashMode();
}

void S60CameraFlashControl::setFlashMode(QCameraExposure::FlashModes mode)
{
    if (isFlashModeSupported(mode)) {
        m_flashMode = mode;
        m_session->setFlashMode(m_flashMode);
    }
    else
        m_session->setError(KErrNotSupported, tr("Requested flash mode is not supported."));
}

bool S60CameraFlashControl::isFlashModeSupported(QCameraExposure::FlashModes mode) const
{
    return m_session->supportedFlashModes() & mode;
}

bool S60CameraFlashControl::isFlashReady() const
{
    if (m_advancedSettings)
        return m_advancedSettings->isFlashReady();

    return false;
}

// End of file
