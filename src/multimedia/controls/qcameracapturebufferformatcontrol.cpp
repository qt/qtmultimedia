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

#include <qcameracapturebufferformatcontrol.h>

QT_BEGIN_NAMESPACE

/*!
    \class QCameraCaptureBufferFormatControl
    \obsolete

    \brief The QCameraCaptureBufferFormatControl class provides a control for setting the capture buffer format.

    The format is of type QVideoFrame::PixelFormat.

    \inmodule QtMultimedia

    \ingroup multimedia_control

    The interface name of QCameraCaptureBufferFormatControl is \c org.qt-project.qt.cameracapturebufferformatcontrol/5.0 as
    defined in QCameraCaptureBufferFormatControl_iid.

    \sa QMediaService::requestControl()
*/

/*!
    \macro QCameraCaptureBufferFormatControl_iid

    \c org.qt-project.qt.cameracapturebufferformatcontrol/5.0

    Defines the interface name of the QCameraCaptureBufferFormatControl class.

    \relates QCameraCaptureBufferFormatControl
*/

/*!
    Constructs a new image buffer capture format control object with the given \a parent
*/
QCameraCaptureBufferFormatControl::QCameraCaptureBufferFormatControl(QObject *parent)
    :QMediaControl(parent)
{
}

/*!
    Destroys an image buffer capture format control.
*/
QCameraCaptureBufferFormatControl::~QCameraCaptureBufferFormatControl()
{
}

/*!
    \fn QCameraCaptureBufferFormatControl::supportedBufferFormats() const

    Returns the list of the supported buffer capture formats.
*/

/*!
    \fn QCameraCaptureBufferFormatControl::bufferFormat() const

    Returns the current buffer capture format.
*/

/*!
    \fn QCameraCaptureBufferFormatControl::setBufferFormat(QVideoFrame::PixelFormat format)

    Sets the buffer capture \a format.
*/

/*!
    \fn QCameraCaptureBufferFormatControl::bufferFormatChanged(QVideoFrame::PixelFormat format)

    Signals the buffer image capture format changed to \a format.
*/

QT_END_NAMESPACE

#include "moc_qcameracapturebufferformatcontrol.cpp"
