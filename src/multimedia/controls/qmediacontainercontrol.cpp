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


#include "qmediacontainercontrol.h"

QT_BEGIN_NAMESPACE

/*!
    \class QMediaContainerControl
    \obsolete

    \brief The QMediaContainerControl class provides access to the output container format of a QMediaService.

    \inmodule QtMultimedia


    \ingroup multimedia_control

    If a QMediaService supports writing encoded data it will implement
    QMediaContainerControl.  This control provides information about the output
    containers supported by a media service and allows one to be selected as
    the current output containers.

    The functionality provided by this control is exposed to application code
    through the QMediaRecorder class.

    The interface name of QMediaContainerControl is \c org.qt-project.qt.mediacontainercontrol/5.0 as
    defined in QMediaContainerControl_iid.

    \sa QMediaService::requestControl(), QMediaRecorder
*/

/*!
    \macro QMediaContainerControl_iid

    \c org.qt-project.qt.mediacontainercontrol/5.0

    Defines the interface name of the QMediaContainerControl class.

    \relates QMediaContainerControl
*/

/*!
    Constructs a new media container control with the given \a parent.
*/
QMediaContainerControl::QMediaContainerControl(QObject *parent)
    :QMediaControl(parent)
{
}

/*!
    Destroys a media container control.
*/
QMediaContainerControl::~QMediaContainerControl()
{
}


/*!
    \fn QMediaContainerControl::supportedContainers() const

    Returns a list of MIME types of supported container formats.
*/

/*!
    \fn QMediaContainerControl::containerFormat() const

    Returns the selected container format.
*/

/*!
    \fn QMediaContainerControl::setContainerFormat(const QString &format)

    Sets the current container \a format.
*/

/*!
    \fn QMediaContainerControl::containerDescription(const QString &formatMimeType) const

    Returns a description of the container \a formatMimeType.
*/

QT_END_NAMESPACE

#include "moc_qmediacontainercontrol.cpp"
