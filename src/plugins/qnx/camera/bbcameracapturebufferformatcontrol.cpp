/****************************************************************************
**
** Copyright (C) 2016 Research In Motion
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
#include "bbcameracapturebufferformatcontrol.h"

QT_BEGIN_NAMESPACE

BbCameraCaptureBufferFormatControl::BbCameraCaptureBufferFormatControl(QObject *parent)
    : QCameraCaptureBufferFormatControl(parent)
{
}

QList<QVideoFrame::PixelFormat> BbCameraCaptureBufferFormatControl::supportedBufferFormats() const
{
    return (QList<QVideoFrame::PixelFormat>() << QVideoFrame::Format_Jpeg);
}

QVideoFrame::PixelFormat BbCameraCaptureBufferFormatControl::bufferFormat() const
{
    return QVideoFrame::Format_Jpeg;
}

void BbCameraCaptureBufferFormatControl::setBufferFormat(QVideoFrame::PixelFormat format)
{
    Q_UNUSED(format)
    // Do nothing, we support only Jpeg for now
}

QT_END_NAMESPACE
