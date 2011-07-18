/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Toolkit.
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

#include <qcameracapturebufferformatcontrol.h>

QT_BEGIN_NAMESPACE

/*!
    \class QCameraCaptureBufferFormatControl

    \brief The QCameraCaptureBufferFormatControl class provides a control for setting the capture buffer format.

    The format is of type QVideoFrame::PixelFormat.

    \inmodule QtMultimediaKit
    \ingroup camera

    The interface name of QCameraCaptureBufferFormatControl is \c com.nokia.Qt.QCameraCaptureBufferFormatControl/1.0 as
    defined in QCameraCaptureBufferFormatControl_iid.

    \sa QMediaService::requestControl()
*/

/*!
    \macro QCameraCaptureBufferFormatControl_iid

    \c com.nokia.Qt.QCameraCaptureBufferFormatControl/1.0

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
    \since 1.2
*/

/*!
    \fn QCameraCaptureBufferFormatControl::bufferFormat() const

    Returns the current buffer capture format.
    \since 1.2
*/

/*!
    \fn QCameraCaptureBufferFormatControl::setBufferFormat(QVideoFrame::PixelFormat format)

    Sets the buffer capture \a format.
    \since 1.2
*/

/*!
    \fn QCameraCaptureBufferFormatControl::bufferFormatChanged(QVideoFrame::PixelFormat format)

    Signals the buffer image capture format changed to \a format.
    \since 1.2
*/

#include "moc_qcameracapturebufferformatcontrol.cpp"
QT_END_NAMESPACE

