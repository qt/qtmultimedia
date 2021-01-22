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

#ifndef QCAMERACAPTUREBUFFERFORMATCONTROL_H
#define QCAMERACAPTUREBUFFERFORMATCONTROL_H

#include <QtMultimedia/qmediacontrol.h>
#include <QtMultimedia/qcameraimagecapture.h>

QT_BEGIN_NAMESPACE

// Required for QDoc workaround
class QString;

class Q_MULTIMEDIA_EXPORT QCameraCaptureBufferFormatControl : public QMediaControl
{
    Q_OBJECT
public:
    ~QCameraCaptureBufferFormatControl();

    virtual QList<QVideoFrame::PixelFormat> supportedBufferFormats() const = 0;
    virtual QVideoFrame::PixelFormat bufferFormat() const = 0;
    virtual void setBufferFormat(QVideoFrame::PixelFormat format) = 0;

Q_SIGNALS:
    void bufferFormatChanged(QVideoFrame::PixelFormat format);

protected:
    explicit QCameraCaptureBufferFormatControl(QObject *parent = nullptr);
};

#define QCameraCaptureBufferFormatControl_iid "org.qt-project.qt.cameracapturebufferformatcontrol/5.0"
Q_MEDIA_DECLARE_CONTROL(QCameraCaptureBufferFormatControl, QCameraCaptureBufferFormatControl_iid)

QT_END_NAMESPACE


#endif

