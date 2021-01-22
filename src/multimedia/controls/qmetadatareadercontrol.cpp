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

#include "qmediacontrol_p.h"
#include <qmetadatareadercontrol.h>

QT_BEGIN_NAMESPACE


/*!
    \class QMetaDataReaderControl
    \obsolete
    \inmodule QtMultimedia


    \ingroup multimedia_control


    \brief The QMetaDataReaderControl class provides read access to the
    meta-data of a QMediaService's media.

    If a QMediaService can provide read or write access to the meta-data of
    its current media it will implement QMetaDataReaderControl.  This control
    provides functions for both retrieving and setting meta-data values.
    Meta-data may be addressed by the keys defined in the
    QMediaMetaData namespace.

    The functionality provided by this control is exposed to application
    code by the meta-data members of QMediaObject, and so meta-data access
    is potentially available in any of the media object classes.  Any media
    service may implement QMetaDataReaderControl.

    The interface name of QMetaDataReaderControl is
    \c org.qt-project.qt.metadatareadercontrol/5.0 as defined in
    QMetaDataReaderControl_iid.

    \sa QMediaService::requestControl(), QMediaObject
*/

/*!
    \macro QMetaDataReaderControl_iid

    \c org.qt-project.qt.metadatareadercontrol/5.0

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
*/

/*!
    \fn QVariant QMetaDataReaderControl::metaData(const QString &key) const

    Returns the meta-data for the given \a key.
*/

/*!
    \fn QMetaDataReaderControl::availableMetaData() const

    Returns a list of keys there is meta-data available for.
*/

/*!
    \fn void QMetaDataReaderControl::metaDataChanged()

    Signal the changes of meta-data.

    If multiple meta-data elements are changed,
    metaDataChanged(const QString &key, const QVariant &value) signal is emitted
    for each of them with metaDataChanged() changed emitted once.
*/

/*!
    \fn void QMetaDataReaderControl::metaDataChanged(const QString &key, const QVariant &value)

    Signal the changes of one meta-data element \a value with the given \a key.
*/

/*!
    \fn void QMetaDataReaderControl::metaDataAvailableChanged(bool available)

    Signal the availability of meta-data has changed, \a available will
    be true if the multimedia object has meta-data.
*/

QT_END_NAMESPACE

#include "moc_qmetadatareadercontrol.cpp"
