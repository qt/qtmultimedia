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

#include "qdeclarativeradiodata_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmlclass RadioData QDeclarativeRadioData
    \since 5.0.0
    \brief The RadioData element allows you to access RDS data from a QML application.
    \ingroup qml-multimedia
    \inherits Item

    This element is part of the \bold{QtMultimedia 5.0} module.

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

        RadioData {
            id: radioData
        }

        Column {
            Text {
                text: radioData.stationName
            }

            Text {
                text: radioData.programTypeName
            }

            Text {
                text: radioData.radioText
            }
        }
    }

    \endqml

    You use \c RadioData together with the \l Radio element. The properties of the RadioData element will reflect the
    information broadcast by the radio station the Radio element is currently tuned to.
*/
QDeclarativeRadioData::QDeclarativeRadioData(QObject *parent) :
    QObject(parent),
    m_radioData(0)
{
    m_radioData = new QRadioData(this);

    connect(m_radioData, SIGNAL(programTypeChanged(QRadioData::ProgramType)), this,
                                 SLOT(_q_programTypeChanged(QRadioData::ProgramType)));

    connect(m_radioData, SIGNAL(stationIdChanged(QString)), this, SIGNAL(stationIdChanged(QString)));
    connect(m_radioData, SIGNAL(programTypeNameChanged(QString)), this, SIGNAL(programTypeNameChanged(QString)));
    connect(m_radioData, SIGNAL(stationNameChanged(QString)), this, SIGNAL(stationNameChanged(QString)));
    connect(m_radioData, SIGNAL(radioTextChanged(QString)), this, SIGNAL(radioTextChanged(QString)));
    connect(m_radioData, SIGNAL(alternativeFrequenciesEnabledChanged(bool)), this,
                         SIGNAL(alternativeFrequenciesEnabledChanged(bool)));

    connect(m_radioData, SIGNAL(error(QRadioData::Error)), this, SLOT(_q_error(QRadioData::Error)));
}

QDeclarativeRadioData::~QDeclarativeRadioData()
{
}

/*!
    \qmlmethod bool RadioData::isAvailable()

    Returns whether the radio data element is ready to use.
  */
bool QDeclarativeRadioData::isAvailable() const
{
    return m_radioData->isAvailable();
}

/*!
    \qmlproperty string RadioData::stationId

    This property allows you to read the station Id of the currently tuned radio
    station.
  */
QString QDeclarativeRadioData::stationId() const
{
    return m_radioData->stationId();
}

/*!
    \qmlproperty enumeration RadioData::programType

    This property holds the type of the currently playing program as transmitted
    by the radio station. The value can be any one of the values defined in the
    table below.

    \table
    \header \o Value
        \row \o Undefined
        \row \o News
        \row \o CurrentAffairs
        \row \o Information
        \row \o Sport
        \row \o Education
        \row \o Drama
        \row \o Culture
        \row \o Science
        \row \o Varied
        \row \o PopMusic
        \row \o RockMusic
        \row \o EasyListening
        \row \o LightClassical
        \row \o SeriousClassical
        \row \o OtherMusic
        \row \o Weather
        \row \o Finance
        \row \o ChildrensProgrammes
        \row \o SocialAffairs
        \row \o Religion
        \row \o PhoneIn
        \row \o Travel
        \row \o Leisure
        \row \o JazzMusic
        \row \o CountryMusic
        \row \o NationalMusic
        \row \o OldiesMusic
        \row \o FolkMusic
        \row \o Documentary
        \row \o AlarmTest
        \row \o Alarm
        \row \o Talk
        \row \o ClassicRock
        \row \o AdultHits
        \row \o SoftRock
        \row \o Top40
        \row \o Soft
        \row \o Nostalgia
        \row \o Classical
        \row \o RhythmAndBlues
        \row \o SoftRhythmAndBlues
        \row \o Language
        \row \o ReligiousMusic
        \row \o ReligiousTalk
        \row \o Personality
        \row \o Public
        \row \o College

    \endtable
  */
QDeclarativeRadioData::ProgramType QDeclarativeRadioData::programType() const
{
    return static_cast<QDeclarativeRadioData::ProgramType>(m_radioData->programType());
}

/*!
    \qmlproperty string RadioData::programTypeName

    This property holds a string representation of the \l programType.
  */
QString QDeclarativeRadioData::programTypeName() const
{
    return m_radioData->programTypeName();
}

/*!
    \qmlproperty string RadioData::stationName

    This property has the name of the currently tuned radio station.
  */
QString QDeclarativeRadioData::stationName() const
{
    return m_radioData->stationName();
}

/*!
    \qmlproperty string RadioData::radioText

    This property holds free-text transmitted by the radio station. This is typically used to
    show supporting information for the currently playing content, for instance song title or
    artist name.
  */
QString QDeclarativeRadioData::radioText() const
{
    return m_radioData->radioText();
}

/*!
    \qmlproperty bool RadioData::alternativeFrequenciesEnabled

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

QT_END_NAMESPACE
