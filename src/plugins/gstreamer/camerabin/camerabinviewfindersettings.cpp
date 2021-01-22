/****************************************************************************
**
** Copyright (C) 2016 Jolla Ltd.
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


#include "camerabinviewfindersettings.h"
#include "camerabinsession.h"

QT_BEGIN_NAMESPACE


CameraBinViewfinderSettings::CameraBinViewfinderSettings(CameraBinSession *session)
    : QCameraViewfinderSettingsControl(session)
    , m_session(session)
{
}

CameraBinViewfinderSettings::~CameraBinViewfinderSettings()
{
}

bool CameraBinViewfinderSettings::isViewfinderParameterSupported(ViewfinderParameter parameter) const
{
    switch (parameter) {
    case Resolution:
    case PixelAspectRatio:
    case MinimumFrameRate:
    case MaximumFrameRate:
    case PixelFormat:
        return true;
    case UserParameter:
        return false;
    }
    return false;
}

QVariant CameraBinViewfinderSettings::viewfinderParameter(ViewfinderParameter parameter) const
{
    switch (parameter) {
    case Resolution:
        return m_session->viewfinderSettings().resolution();
    case PixelAspectRatio:
        return m_session->viewfinderSettings().pixelAspectRatio();
    case MinimumFrameRate:
        return m_session->viewfinderSettings().minimumFrameRate();
    case MaximumFrameRate:
        return m_session->viewfinderSettings().maximumFrameRate();
    case PixelFormat:
        return m_session->viewfinderSettings().pixelFormat();
    case UserParameter:
        return QVariant();
    }
    return false;
}

void CameraBinViewfinderSettings::setViewfinderParameter(ViewfinderParameter parameter, const QVariant &value)
{
    QCameraViewfinderSettings settings = m_session->viewfinderSettings();

    switch (parameter) {
    case Resolution:
        settings.setResolution(value.toSize());
        break;
    case PixelAspectRatio:
        settings.setPixelAspectRatio(value.toSize());
        break;
    case MinimumFrameRate:
        settings.setMinimumFrameRate(value.toReal());
        break;
    case MaximumFrameRate:
        settings.setMaximumFrameRate(value.toReal());
        break;
    case PixelFormat:
        settings.setPixelFormat(qvariant_cast<QVideoFrame::PixelFormat>(value));
    case UserParameter:
        break;
    }

    m_session->setViewfinderSettings(settings);
}

QT_END_NAMESPACE
