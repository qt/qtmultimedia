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

#include "directshowcameracapturebufferformatcontrol.h"

#include "dscamerasession.h"

QT_BEGIN_NAMESPACE

DirectShowCameraCaptureBufferFormatControl::DirectShowCameraCaptureBufferFormatControl()
{
}

QList<QVideoFrame::PixelFormat> DirectShowCameraCaptureBufferFormatControl::supportedBufferFormats() const
{
    return QList<QVideoFrame::PixelFormat>() << QVideoFrame::Format_RGB32;
}

QVideoFrame::PixelFormat DirectShowCameraCaptureBufferFormatControl::bufferFormat() const
{
    return QVideoFrame::Format_RGB32;
}

void DirectShowCameraCaptureBufferFormatControl::setBufferFormat(QVideoFrame::PixelFormat format)
{
    Q_UNUSED(format);
}

QT_END_NAMESPACE
