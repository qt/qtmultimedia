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

#include <qtmultimediadefs.h>
#include "qradiodatacontrol.h"
#include "qmediacontrol_p.h"

QT_BEGIN_NAMESPACE


/*!
    \class QRadioDataControl
    \inmodule QtMultimedia
    \ingroup multimedia-serv
    \since 5.0


    \brief The QRadioDataControl class provides access to the RDS functionality of the
    radio in the QMediaService.

    The functionality provided by this control is exposed to application code
    through the QRadioData class.

    The interface name of QRadioDataControl is \c com.nokia.Qt.QRadioDataControl/5.0 as
    defined in QRadioDataControl_iid.

    \sa QMediaService::requestControl(), QRadioData
*/

/*!
    \macro QRadioDataControl_iid

    \c com.nokia.Qt.QRadioDataControl/5.0

    Defines the interface name of the QRadioDataControl class.

    \relates QRadioDataControl
*/

/*!
    Constructs a radio data control with the given \a parent.
*/

QRadioDataControl::QRadioDataControl(QObject *parent):
    QMediaControl(*new QMediaControlPrivate, parent)
{
}

/*!
    Destroys a radio data control.
*/

QRadioDataControl::~QRadioDataControl()
{
}

/*!
    \fn bool QRadioDataControl::isAvailable() const

    Returns true if the radio service is ready to use.
    \since 5.0
*/

/*!
    \fn QtMultimedia::AvailabilityError QRadioDataControl::availabilityError() const

    Returns the error state of the radio service.
    \since 5.0
*/

/*!
    \fn QRadioData::Error QRadioDataControl::error() const

    Returns the error state of a radio data.
    \since 5.0
*/

/*!
    \fn QString QRadioDataControl::errorString() const

    Returns a string describing a radio data's error state.
    \since 5.0
*/

/*!
    \fn void QRadioDataControl::error(QRadioData::Error error)

    Signals that an \a error has occurred.
    \since 5.0
*/

/*!
    \fn int QRadioDataControl::stationId()

    Returns the current Program Identification
    \since 5.0
*/

/*!
    \fn QRadioData::ProgramType QRadioDataControl::programType()

    Returns the current Program Type
    \since 5.0
*/

/*!
    \fn QString QRadioDataControl::programTypeName()

    Returns the current Program Type Name
    \since 5.0
*/

/*!
    \fn QString QRadioDataControl::stationName()

    Returns the current Program Service
    \since 5.0
*/

/*!
    \fn QString QRadioDataControl::radioText()

    Returns the current Radio Text
    \since 5.0
*/

/*!
    \fn void QRadioDataControl::setAlternativeFrequenciesEnabled(bool enabled)

    Sets the Alternative Frequency to \a enabled
    \since 5.0
*/

/*!
    \fn bool QRadioDataControl::isAlternativeFrequenciesEnabled()

    Returns true if Alternative Frequency is currently enabled
    \since 5.0
*/

/*!
    \fn void QRadioDataControl::stationIdChanged(QString stationId)

    Signals that the Program Identification \a stationId has changed
    \since 5.0
*/

/*!
    \fn void QRadioDataControl::programTypeChanged(QRadioData::ProgramType programType)

    Signals that the Program Type \a programType has changed
    \since 5.0
*/

/*!
    \fn void QRadioDataControl::programTypeNameChanged(QString programTypeName)

    Signals that the Program Type Name \a programTypeName has changed
    \since 5.0
*/

/*!
    \fn void QRadioDataControl::stationNameChanged(QString stationName)

    Signals that the Program Service \a stationName has changed
    \since 5.0
*/

/*!
    \fn void QRadioDataControl::radioTextChanged(QString radioText)

    Signals that the Radio Text \a radioText has changed
    \since 5.0
*/

#include "moc_qradiodatacontrol.cpp"
QT_END_NAMESPACE

