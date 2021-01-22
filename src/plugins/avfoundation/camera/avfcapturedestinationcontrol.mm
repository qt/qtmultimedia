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

#include "avfcapturedestinationcontrol.h"

QT_BEGIN_NAMESPACE

bool AVFCaptureDestinationControl::isCaptureDestinationSupported(QCameraImageCapture::CaptureDestinations destination) const
{
    return destination & (QCameraImageCapture::CaptureToFile | QCameraImageCapture::CaptureToBuffer);
}

QCameraImageCapture::CaptureDestinations AVFCaptureDestinationControl::captureDestination() const
{
    return m_destination;
}

void AVFCaptureDestinationControl::setCaptureDestination(QCameraImageCapture::CaptureDestinations destination)
{
    if (m_destination != destination) {
        m_destination = destination;
        Q_EMIT captureDestinationChanged(m_destination);
    }
}

QT_END_NAMESPACE
