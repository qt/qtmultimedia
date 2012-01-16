/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qmediacontrol_p.h"
#include <qmetadatawritercontrol.h>

QT_BEGIN_NAMESPACE


/*!
    \class QMetaDataWriterControl
    \inmodule QtMultimedia

    \ingroup multimedia
    \ingroup multimedia_control


    \brief The QMetaDataWriterControl class provides write access to the
    meta-data of a QMediaService's media.

    If a QMediaService can provide write access to the meta-data of its
    current media it will implement QMetaDataWriterControl.  This control
    provides functions for both retrieving and setting meta-data values.
    Meta-data may be addressed by the keys defined in the
    QtMultimedia::MetaData namespace.

    The functionality provided by this control is exposed to application code
    by the meta-data members of QMediaObject, and so meta-data access is
    potentially available in any of the media object classes.  Any media
    service may implement QMetaDataControl.

    The interface name of QMetaDataWriterControl is \c com.nokia.Qt.QMetaDataWriterControl/1.0 as
    defined in QMetaDataWriterControl_iid.

    \sa QMediaService::requestControl(), QMediaObject
*/

/*!
    \macro QMetaDataWriterControl_iid

    \c com.nokia.Qt.QMetaDataWriterControl/1.0

    Defines the interface name of the QMetaDataWriterControl class.

    \relates QMetaDataWriterControl
*/

/*!
    Construct a QMetaDataWriterControl with \a parent. This class is meant as a base class
    for service specific meta data providers so this constructor is protected.
*/

QMetaDataWriterControl::QMetaDataWriterControl(QObject *parent):
    QMediaControl(*new QMediaControlPrivate, parent)
{
}

/*!
    Destroy the meta-data writer control.
*/

QMetaDataWriterControl::~QMetaDataWriterControl()
{
}

/*!
    \fn bool QMetaDataWriterControl::isMetaDataAvailable() const

    Identifies if meta-data is available from a media service.

    Returns true if the meta-data is available and false otherwise.
*/

/*!
    \fn bool QMetaDataWriterControl::isWritable() const

    Identifies if a media service's meta-data can be edited.

    Returns true if the meta-data is writable and false otherwise.
*/

/*!
    \fn QVariant QMetaDataWriterControl::metaData(const QString &key) const

    Returns the meta-data for the given \a key.
*/

/*!
    \fn void QMetaDataWriterControl::setMetaData(const QString &key, const QVariant &value)

    Sets the \a value of the meta-data element with the given \a key.
*/

/*!
    \fn QMetaDataWriterControl::availableMetaData() const

    Returns a list of keys there is meta-data available for.
*/

/*!
    \fn void QMetaDataWriterControl::metaDataChanged()

    Signal the changes of meta-data.
*/

/*!
    \fn void QMetaDataWriterControl::metaDataAvailableChanged(bool available)

    Signal the availability of meta-data has changed, \a available will
    be true if the multimedia object has meta-data.
*/

/*!
    \fn void QMetaDataWriterControl::writableChanged(bool writable)

    Signal a change in the writable status of meta-data, \a writable will be
    true if meta-data elements can be added or adjusted.
*/

#include "moc_qmetadatawritercontrol.cpp"
QT_END_NAMESPACE

