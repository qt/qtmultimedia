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

#include "camerabincapturebufferformat.h"
#include "camerabinsession.h"

QT_BEGIN_NAMESPACE

CameraBinCaptureBufferFormat::CameraBinCaptureBufferFormat(CameraBinSession *session)
    :QCameraCaptureBufferFormatControl(session)
     , m_session(session)
     , m_format(QVideoFrame::Format_Jpeg)
{
}

CameraBinCaptureBufferFormat::~CameraBinCaptureBufferFormat()
{
}

QList<QVideoFrame::PixelFormat> CameraBinCaptureBufferFormat::supportedBufferFormats() const
{
    //the exact YUV format is unknown with camerabin until the first capture is requested
    return QList<QVideoFrame::PixelFormat>()
            << QVideoFrame::Format_Jpeg;
}

QVideoFrame::PixelFormat CameraBinCaptureBufferFormat::bufferFormat() const
{
    return m_format;
}

void CameraBinCaptureBufferFormat::setBufferFormat(QVideoFrame::PixelFormat format)
{
    if (m_format != format) {
        m_format = format;
        emit bufferFormatChanged(format);
    }
}

QT_END_NAMESPACE
