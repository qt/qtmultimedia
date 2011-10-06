/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include <qmediacontrol_p.h>
#include <qmetadatareadercontrol.h>

QT_BEGIN_NAMESPACE


/*!
    \class QMetaDataReaderControl
    \inmodule QtMultimedia
    \ingroup multimedia-serv
    \since 1.0


    \brief The QMetaDataReaderControl class provides read access to the
    meta-data of a QMediaService's media.

    If a QMediaService can provide read or write access to the meta-data of
    its current media it will implement QMetaDataReaderControl.  This control
    provides functions for both retrieving and setting meta-data values.
    Meta-data may be addressed by the well defined keys in the
    QtMultimedia::MetaData enumeration using the metaData() functions, or by
    string keys using the extendedMetaData() functions.

    The functionality provided by this control is exposed to application
    code by the meta-data members of QMediaObject, and so meta-data access
    is potentially available in any of the media object classes.  Any media
    service may implement QMetaDataReaderControl.

    The interface name of QMetaDataReaderControl is
    \c com.nokia.Qt.QMetaDataReaderControl/1.0 as defined in
    QMetaDataReaderControl_iid.

    \sa QMediaService::requestControl(), QMediaObject
*/

/*!
    \macro QMetaDataReaderControl_iid

    \c com.nokia.Qt.QMetaDataReaderControl/1.0

    Defines the interface name of the QMetaDataReaderControl class.

    \relates QMetaDataReaderControl
*/

/*!
    Construct a QMetaDataReaderControl with \a parent. This class is meant as a base class
    for service specific meta data providers so this constructor is protected.
*/

QMetaDataReaderControl::QMetaDataReaderControl(QObject *parent):
    QMediaControl(*new QMediaControlPrivate, parent)
{
}

/*!
    Destroy the meta-data object.
*/

QMetaDataReaderControl::~QMetaDataReaderControl()
{
}

/*!
    \fn bool QMetaDataReaderControl::isMetaDataAvailable() const

    Identifies if meta-data is available from a media service.

    Returns true if the meta-data is available and false otherwise.
    \since 1.0
*/

/*!
    \fn QVariant QMetaDataReaderControl::metaData(QtMultimedia::MetaData key) const

    Returns the meta-data for the given \a key.
    \since 1.0
*/

/*!
    \fn QMetaDataReaderControl::availableMetaData() const

    Returns a list of keys there is meta-data available for.
    \since 1.0
*/

/*!
    \fn QMetaDataReaderControl::extendedMetaData(const QString &key) const

    Returns the metaData for an abitrary string \a key.

    The valid selection of keys for extended meta-data is determined by the provider and the meaning
    and type may differ between providers.
    \since 1.0
*/

/*!
    \fn QMetaDataReaderControl::availableExtendedMetaData() const

    Returns a list of keys there is extended meta-data available for.
    \since 1.0
*/


/*!
    \fn void QMetaDataReaderControl::metaDataChanged()

    Signal the changes of meta-data.
    \since 1.0
*/

/*!
    \fn void QMetaDataReaderControl::metaDataAvailableChanged(bool available)

    Signal the availability of meta-data has changed, \a available will
    be true if the multimedia object has meta-data.
    \since 1.0
*/

#include "moc_qmetadatareadercontrol.cpp"
QT_END_NAMESPACE

