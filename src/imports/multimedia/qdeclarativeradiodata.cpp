/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qdeclarativeradiodata_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmlclass RadioData QDeclarativeRadioData
    \inqmlmodule QtMultimedia 5
    \brief The RadioData element allows you to access RDS data from a QML application.
    \ingroup multimedia_qml
    \inherits Item

    This element is part of the \b{QtMultimedia 5.0} module.

    The \c RadioData element is your gateway to all the data available through RDS. RDS is the Radio Data System
    which allows radio stations to broadcast information like the \l stationId, \l programType, \l programTypeName,
    \l stationName, and \l radioText. This information can be read from the \c RadioData element. It also allows
    you to set whether the radio should tune to alternative frequencies if the current signal strength falls too much.

    \qml
    import QtQuick 2.0
    import QtMultimedia 5.0

    Rectangle {
        width: 480
        height: 320

        Radio {
            id: radio
            band: Radio.FM
        }

        Column {
            Text {
                text: radio.radioData.stationName
            }

            Text {
                text: radio.radioData.programTypeName
            }

            Text {
                text: radio.radioData.radioText
            }
        }
    }

    \endqml

    You use \c RadioData together with the \l Radio element, either by
    accessing the \c radioData property of the Radio element, or
    creating a separate RadioData element. The properties of the
    RadioData element will reflect the information broadcast by the
    radio station the Radio element is currently tuned to.

    \sa {Radio Overview}
*/
QDeclarativeRadioData::QDeclarativeRadioData(QObject *parent) :
    QObject(parent)
{
    m_radioTuner = new QRadioTuner(this);
    m_radioData = m_radioTuner->radioData();

    connectSignals();
}

QDeclarativeRadioData::QDeclarativeRadioData(QRadioTuner *tuner, QObject *parent) :
    QObject(parent)
{
    m_radioTuner = tuner;
    m_radioData = m_radioTuner->radioData();

    connectSignals();
}

QDeclarativeRadioData::~QDeclarativeRadioData()
{
}

/*!
    \qmlproperty enumeration QtMultimedia5::RadioData::availability

    Returns the availability state of the radio data interface.

    This is one of:

    \table
    \header \li Value \li Description
    \row \li Available
        \li The radio data interface is available to use
    \row \li Busy
        \li The radio data interface is usually available to use, but is currently busy.
    \row \li Unavailable
        \li The radio data interface is not available to use (there may be no radio
           hardware)
    \row \li ResourceMissing
        \li There is one or more resources missing, so the radio cannot
           be used.  It may be possible to try again at a later time.
    \endtable
 */
QDeclarativeRadioData::Availability QDeclarativeRadioData::availability() const
{
    return Availability(m_radioData->availabilityError());
}


/*!
    \qmlproperty string QtMultimedia5::RadioData::stationId

    This property allows you to read the station Id of the currently tuned radio
    station.
  */
QString QDeclarativeRadioData::stationId() const
{
    return m_radioData->stationId();
}

/*!
    \qmlproperty enumeration QtMultimedia5::RadioData::programType

    This property holds the type of the currently playing program as transmitted
    by the radio station. The value can be any one of the values defined in the
    table below.

    \table
    \header \li Value
        \row \li Undefined
        \row \li News
        \row \li CurrentAffairs
        \row \li Information
        \row \li Sport
        \row \li Education
        \row \li Drama
        \row \li Culture
        \row \li Science
        \row \li Varied
        \row \li PopMusic
        \row \li RockMusic
        \row \li EasyListening
        \row \li LightClassical
        \row \li SeriousClassical
        \row \li OtherMusic
        \row \li Weather
        \row \li Finance
        \row \li ChildrensProgrammes
        \row \li SocialAffairs
        \row \li Religion
        \row \li PhoneIn
        \row \li Travel
        \row \li Leisure
        \row \li JazzMusic
        \row \li CountryMusic
        \row \li NationalMusic
        \row \li OldiesMusic
        \row \li FolkMusic
        \row \li Documentary
        \row \li AlarmTest
        \row \li Alarm
        \row \li Talk
        \row \li ClassicRock
        \row \li AdultHits
        \row \li SoftRock
        \row \li Top40
        \row \li Soft
        \row \li Nostalgia
        \row \li Classical
        \row \li RhythmAndBlues
        \row \li SoftRhythmAndBlues
        \row \li Language
        \row \li ReligiousMusic
        \row \li ReligiousTalk
        \row \li Personality
        \row \li Public
        \row \li College

    \endtable
  */
QDeclarativeRadioData::ProgramType QDeclarativeRadioData::programType() const
{
    return static_cast<QDeclarativeRadioData::ProgramType>(m_radioData->programType());
}

/*!
    \qmlproperty string QtMultimedia5::RadioData::programTypeName

    This property holds a string representation of the \l programType.
  */
QString QDeclarativeRadioData::programTypeName() const
{
    return m_radioData->programTypeName();
}

/*!
    \qmlproperty string QtMultimedia5::RadioData::stationName

    This property has the name of the currently tuned radio station.
  */
QString QDeclarativeRadioData::stationName() const
{
    return m_radioData->stationName();
}

/*!
    \qmlproperty string QtMultimedia5::RadioData::radioText

    This property holds free-text transmitted by the radio station. This is typically used to
    show supporting information for the currently playing content, for instance song title or
    artist name.
  */
QString QDeclarativeRadioData::radioText() const
{
    return m_radioData->radioText();
}

/*!
    \qmlproperty bool QtMultimedia5::RadioData::alternativeFrequenciesEnabled

    This property allows you to specify whether the radio should try and tune to alternative
    frequencies if the signal strength of the current station becomes too weak. The alternative
    frequencies are emitted over RDS by the radio station, and the tuning happens automatically.
  */
bool QDeclarativeRadioData::alternativeFrequenciesEnabled() const
{
    return m_radioData->isAlternativeFrequenciesEnabled();
}

void QDeclarativeRadioData::setAlternativeFrequenciesEnabled(bool enabled)
{
    m_radioData->setAlternativeFrequenciesEnabled(enabled);
}

void QDeclarativeRadioData::_q_programTypeChanged(QRadioData::ProgramType programType)
{
    emit programTypeChanged(static_cast<QDeclarativeRadioData::ProgramType>(programType));
}

void QDeclarativeRadioData::_q_error(QRadioData::Error errorCode)
{
    emit error(static_cast<QDeclarativeRadioData::Error>(errorCode));
    emit errorChanged();
}

void QDeclarativeRadioData::_q_availabilityChanged(QtMultimedia::AvailabilityError error)
{
    emit availabilityChanged(Availability(error));
}

void QDeclarativeRadioData::connectSignals()
{
    connect(m_radioData, SIGNAL(programTypeChanged(QRadioData::ProgramType)), this,
                                 SLOT(_q_programTypeChanged(QRadioData::ProgramType)));

    connect(m_radioData, SIGNAL(stationIdChanged(QString)), this, SIGNAL(stationIdChanged(QString)));
    connect(m_radioData, SIGNAL(programTypeNameChanged(QString)), this, SIGNAL(programTypeNameChanged(QString)));
    connect(m_radioData, SIGNAL(stationNameChanged(QString)), this, SIGNAL(stationNameChanged(QString)));
    connect(m_radioData, SIGNAL(radioTextChanged(QString)), this, SIGNAL(radioTextChanged(QString)));
    connect(m_radioData, SIGNAL(alternativeFrequenciesEnabledChanged(bool)), this,
                         SIGNAL(alternativeFrequenciesEnabledChanged(bool)));

    // Note we map availabilityError->availability
    // Since the radio data element depends on the service for the tuner, the availability is also dictated from the tuner
    connect(m_radioTuner, SIGNAL(availabilityErrorChanged(QtMultimedia::AvailabilityError)), this, SLOT(_q_availabilityChanged(QtMultimedia::AvailabilityError)));

    connect(m_radioData, SIGNAL(error(QRadioData::Error)), this, SLOT(_q_error(QRadioData::Error)));
}

QT_END_NAMESPACE
