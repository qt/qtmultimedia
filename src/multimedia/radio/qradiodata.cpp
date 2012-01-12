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

#include "qradiodata.h"
#include "qmediaservice.h"
#include "qmediaobject_p.h"
#include "qradiodatacontrol.h"

#include <QPair>


QT_BEGIN_NAMESPACE


namespace
{
    class QRadioDataPrivateRegisterMetaTypes
    {
    public:
        QRadioDataPrivateRegisterMetaTypes()
        {
            qRegisterMetaType<QRadioData::Error>();
            qRegisterMetaType<QRadioData::ProgramType>();
        }
    } _registerMetaTypes;
}

/*!
    \class QRadioData
    \brief The QRadioData class provides interfaces to the RDS functionality of the system radio.

    \inmodule QtMultimedia
    \ingroup multimedia
    \since 5.0

    The radio data object will emit signals for any changes in radio data. You can enable or disable
    alternative frequency with setAlternativeFrequenciesEnabled().

    \sa {Radio Overview}

*/


class QRadioDataPrivate : public QMediaObjectPrivate
{
public:
    QRadioDataPrivate():provider(0), control(0) {}
    QMediaServiceProvider *provider;
    QRadioDataControl* control;
};

/*!
    Constructs a radio data based on a media service allocated by a media service \a provider.

    The \a parent is passed to QMediaObject.
    \since 5.0
*/

QRadioData::QRadioData(QObject *parent, QMediaServiceProvider* provider):
    QMediaObject(*new QRadioDataPrivate, parent, provider->requestService(Q_MEDIASERVICE_RADIO))
{
    Q_D(QRadioData);

    d->provider = provider;

    if (d->service != 0) {
        d->control = qobject_cast<QRadioDataControl*>(d->service->requestControl(QRadioDataControl_iid));
        if (d->control != 0) {
            connect(d->control, SIGNAL(stationIdChanged(QString)), SIGNAL(stationIdChanged(QString)));
            connect(d->control, SIGNAL(programTypeChanged(QRadioData::ProgramType)),
                                SIGNAL(programTypeChanged(QRadioData::ProgramType)));
            connect(d->control, SIGNAL(programTypeNameChanged(QString)), SIGNAL(programTypeNameChanged(QString)));
            connect(d->control, SIGNAL(stationNameChanged(QString)), SIGNAL(stationNameChanged(QString)));
            connect(d->control, SIGNAL(radioTextChanged(QString)), SIGNAL(radioTextChanged(QString)));
            connect(d->control, SIGNAL(alternativeFrequenciesEnabledChanged(bool)), SIGNAL(alternativeFrequenciesEnabledChanged(bool)));
            connect(d->control, SIGNAL(error(QRadioData::Error)), SIGNAL(error(QRadioData::Error)));
        }
    }
}

/*!
    Destroys a radio data.
*/

QRadioData::~QRadioData()
{
    Q_D(QRadioData);

    if (d->service && d->control)
        d->service->releaseControl(d->control);

    d->provider->releaseService(d->service);
}

/*!
    Returns true if the radio data service is ready to use.
    \since 5.0
*/
bool QRadioData::isAvailable() const
{
    Q_D(const QRadioData);

    if (d->control != 0)
        return d_func()->control->isAvailable();
    else
        return false;
}

/*!
    Returns the availability error state.
    \since 5.0
*/
QtMultimedia::AvailabilityError QRadioData::availabilityError() const
{
    Q_D(const QRadioData);

    if (d->control != 0)
        return d_func()->control->availabilityError();
    else
        return QtMultimedia::ServiceMissingError;
}

/*!
    \property QRadioData::stationId
    \brief Current Program Identification

    \since 5.0
*/

QString QRadioData::stationId() const
{
    Q_D(const QRadioData);

    if (d->control != 0)
        return d->control->stationId();
    return QString();
}

/*!
    \property QRadioData::programType
    \brief Current Program Type

    \since 5.0
*/

QRadioData::ProgramType QRadioData::programType() const
{
    Q_D(const QRadioData);

    if (d->control != 0)
        return d->control->programType();

    return QRadioData::Undefined;
}

/*!
    \property QRadioData::programTypeName
    \brief Current Program Type Name

    \since 5.0
*/

QString QRadioData::programTypeName() const
{
    Q_D(const QRadioData);

    if (d->control != 0)
        return d->control->programTypeName();
    return QString();
}

/*!
    \property QRadioData::stationName
    \brief Current Program Service

    \since 5.0
*/

QString QRadioData::stationName() const
{
    Q_D(const QRadioData);

    if (d->control != 0)
        return d->control->stationName();
    return QString();
}

/*!
    \property QRadioData::radioText
    \brief Current Radio Text

    \since 5.0
*/

QString QRadioData::radioText() const
{
    Q_D(const QRadioData);

    if (d->control != 0)
        return d->control->radioText();
    return QString();
}

/*!
    \property QRadioData::alternativeFrequenciesEnabled
    \brief Is Alternative Frequency currently enabled

    \since 5.0
*/

bool QRadioData::isAlternativeFrequenciesEnabled() const
{
    Q_D(const QRadioData);

    if (d->control != 0)
        return d->control->isAlternativeFrequenciesEnabled();
    return false;
}

void QRadioData::setAlternativeFrequenciesEnabled( bool enabled )
{
    Q_D(const QRadioData);

    if (d->control != 0)
        return d->control->setAlternativeFrequenciesEnabled(enabled);
}

/*!
    Returns the error state of a radio data.

    \since 5.0
    \sa errorString()
*/

QRadioData::Error QRadioData::error() const
{
    Q_D(const QRadioData);

    if (d->control != 0)
        return d->control->error();
    return QRadioData::ResourceError;
}

/*!
    Returns a description of a radio data's error state.

    \since 5.0
    \sa error()
*/
QString QRadioData::errorString() const
{
    Q_D(const QRadioData);

    if (d->control != 0)
        return d->control->errorString();
    return QString();
}

/*!
    \fn void QRadioData::stationIdChanged(QString stationId)

    Signals that the Program Identification code has changed to \a stationId
    \since 5.0
*/

/*!
    \fn void QRadioData::programTypeChanged(QRadioData::ProgramType programType)

    Signals that the Program Type code has changed to \a programType
    \since 5.0
*/

/*!
    \fn void QRadioData::programTypeNameChanged(QString programTypeName)

    Signals that the Program Type Name has changed to \a programTypeName
    \since 5.0
*/

/*!
    \fn void QRadioData::stationNameChanged(int stationName)

    Signals that the Program Service has changed to \a stationName
    \since 5.0
*/

/*!
    \fn void QRadioData::alternativeFrequenciesEnabledChanged(bool enabled)

    Signals that the AF has been enabled or disabled
    \since 5.0
*/

/*!
    \fn void QRadioData::error(QRadioData::Error error)

    Signals that an \a error occurred.
    \since 5.0
*/

/*!
    \enum QRadioData::Error

    Enumerates radio data error conditions.

    \value NoError         No errors have occurred.
    \value ResourceError   There is no radio service available.
    \value OpenError       Unable to open radio device.
    \value OutOfRangeError An attempt to set a frequency or band that is not supported by radio device.
*/

/*! \fn void QRadioData::stateChanged(QRadioData::State state)
  This signal is emitted when the state changes to \a state.
  \since 5.0
 */

#include "moc_qradiodata.cpp"
QT_END_NAMESPACE

