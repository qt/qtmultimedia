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

#include <qmediabindableinterface.h>

QT_BEGIN_NAMESPACE

/*!
    \class QMediaBindableInterface
    \inmodule QtMultimedia

    \ingroup multimedia
    \ingroup multimedia_core


    \brief The QMediaBindableInterface class is the base class for objects extending media objects functionality.

    \sa
*/

/*!
    Destroys a media helper object.
*/

QMediaBindableInterface::~QMediaBindableInterface()
{
}

/*!
    \fn QMediaBindableInterface::mediaObject() const;

    Return the currently attached media object.
*/


/*!
    \fn QMediaBindableInterface::setMediaObject(QMediaObject *object);

    Attaches to the media \a object.
    Returns true if attached successfully, otherwise returns false.
*/



QT_END_NAMESPACE

