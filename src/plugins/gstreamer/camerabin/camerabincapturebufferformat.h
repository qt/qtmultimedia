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

#ifndef CAMERABINCAPTUREBUFFERFORMAT_H
#define CAMERABINCAPTUREBUFFERFORMAT_H

#include <qcamera.h>
#include <qcameracapturebufferformatcontrol.h>

#include <gst/gst.h>
#include <glib.h>

QT_BEGIN_NAMESPACE

class CameraBinSession;

QT_USE_NAMESPACE

class CameraBinCaptureBufferFormat : public QCameraCaptureBufferFormatControl
{
    Q_OBJECT
public:
    CameraBinCaptureBufferFormat(CameraBinSession *session);
    virtual ~CameraBinCaptureBufferFormat();

    QList<QVideoFrame::PixelFormat> supportedBufferFormats() const override;

    QVideoFrame::PixelFormat bufferFormat() const override;
    void setBufferFormat(QVideoFrame::PixelFormat format) override;

private:
    CameraBinSession *m_session;
    QVideoFrame::PixelFormat m_format;
};

QT_END_NAMESPACE

#endif
