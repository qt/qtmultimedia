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

#include "qdeclarativeradio_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmlclass Radio QDeclarativeRadio
    \since 5.0.0
    \brief The Radio element allows you to access radio functionality from a QML application.
    \ingroup qml-multimedia
    \inherits Item

    This element is part of the \bold{QtMultimedia 5.0} module.

    \qml
    import QtQuick 2.0
    import QtMultimedia 5.0

    Rectangle {
        width: 320
        height: 480

        Radio {
            id: radio
            band: Radio.FM
        }

        MouseArea {
            x: 0; y: 0
            height: parent.height
            width: parent.width / 2

            onClicked: radio.scanDown()
        }

        MouseArea {
            x: parent.width / 2; y: 0
            height: parent.height
            width: parent.width / 2

            onClicked: radio.scanUp()
        }
    }
    \endqml

    You can use the \c Radio element to tune the radio and get information about the signal.
    You can also use the Radio element to get information about tuning, for instance the
    frequency steps supported for tuning.

    The corresponding \l RadioData element gives RDS information about the current radio station.

*/


QDeclarativeRadio::QDeclarativeRadio(QObject *parent) :
    QObject(parent),
    m_radioTuner(0)
{
    m_radioTuner = new QRadioTuner(this);

    connect(m_radioTuner, SIGNAL(stateChanged(QRadioTuner::State)), this, SLOT(_q_stateChanged(QRadioTuner::State)));
    connect(m_radioTuner, SIGNAL(bandChanged(QRadioTuner::Band)), this, SLOT(_q_bandChanged(QRadioTuner::Band)));

    connect(m_radioTuner, SIGNAL(frequencyChanged(int)), this, SIGNAL(frequencyChanged(int)));
    connect(m_radioTuner, SIGNAL(stereoStatusChanged(bool)), this, SIGNAL(stereoStatusChanged(bool)));
    connect(m_radioTuner, SIGNAL(searchingChanged(bool)), this, SIGNAL(searchingChanged(bool)));
    connect(m_radioTuner, SIGNAL(signalStrengthChanged(int)), this, SIGNAL(signalStrengthChanged(int)));
    connect(m_radioTuner, SIGNAL(volumeChanged(int)), this, SIGNAL(volumeChanged(int)));
    connect(m_radioTuner, SIGNAL(mutedChanged(bool)), this, SIGNAL(mutedChanged(bool)));
    connect(m_radioTuner, SIGNAL(stationFound(int, QString)), this, SIGNAL(stationFound(int, QString)));

    connect(m_radioTuner, SIGNAL(error(QRadioTuner::Error)), this, SLOT(_q_error(QRadioTuner::Error)));
}

QDeclarativeRadio::~QDeclarativeRadio()
{
}

/*!
    \qmlproperty enumeration Radio::state

    This property holds the current state of the Radio element.

    \table
    \header \o Value \o Description
    \row \o ActiveState
        \o The radio is started and active

    \row \o StoppedState
        \o The radio is stopped

    \endtable

    \sa start, stop
*/
QDeclarativeRadio::State QDeclarativeRadio::state() const
{
    return static_cast<QDeclarativeRadio::State>(m_radioTuner->state());
}

/*!
    \qmlproperty enumeration Radio::band

    This property holds the frequency band used for the radio, which can be specified as
    any one of the values in the table below.

    \table
    \header \o Value \o Description
    \row \o AM
        \o 520 to 1610 kHz, 9 or 10kHz channel spacing, extended 1610 to 1710 kHz

    \row \o FM
        \o 87.5 to 108.0 MHz, except Japan 76-90 MHz

    \row \o SW
        \o 1.711 to 30.0 MHz, divided into 15 bands. 5kHz channel spacing

    \row \o LW
        \o 148.5 to 283.5 kHz, 9kHz channel spacing (Europe, Africa, Asia)

    \row \o FM2
        \o range not defined, used when area supports more than one FM range

    \endtable
*/
QDeclarativeRadio::Band QDeclarativeRadio::band() const
{
    return static_cast<QDeclarativeRadio::Band>(m_radioTuner->band());
}

/*!
    \qmlproperty int Radio::frequency

    Sets the frequency in Hertz that the radio is tuned to. The frequency must be within the frequency
    range for the current band, otherwise it will be changed to be within the frequency range.

    \sa maximumFrequency, minimumFrequency
*/
int QDeclarativeRadio::frequency() const
{
    return m_radioTuner->frequency();
}

/*!
    \qmlproperty enumeration Radio::stereoMode

    This property holds the stereo mode of the radio, which can be set to any one of the
    values in the table below.

    \table
    \header \o Value \o Description
    \row \o Auto
        \o Uses stereo mode matching the station

    \row \o ForceStereo
        \o Forces the radio to play the station in stereo, converting the sound signal if necessary

    \row \o ForceMono
        \o Forces the radio to play the station in mono, converting the sound signal if necessary

    \endtable
*/
QDeclarativeRadio::StereoMode QDeclarativeRadio::stereoMode() const
{
    return static_cast<QDeclarativeRadio::StereoMode>(m_radioTuner->stereoMode());
}

/*!
    \qmlproperty int Radio::volume

    Set this property to control the volume of the radio. The valid range of the volume is from 0 to 100.
*/
int QDeclarativeRadio::volume() const
{
    return m_radioTuner->volume();
}

/*!
    \qmlproperty bool Radio::muted

    This property reflects whether the radio is muted or not.
*/
bool QDeclarativeRadio::muted() const
{
    return m_radioTuner->isMuted();
}

/*!
    \qmlproperty bool Radio::stereo

    This property holds whether the radio is receiving a stereo signal or not. If \l stereoMode is
    set to ForceMono the value will always be false. Likewise, it will always be true if stereoMode
    is set to ForceStereo.

    \sa stereoMode
*/
bool QDeclarativeRadio::stereo() const
{
    return m_radioTuner->isStereo();
}

/*!
    \qmlproperty int Radio::signalStrength

    The strength of the current radio signal as a percentage where 0% equals no signal, and 100% is a
    very good signal.
*/
int QDeclarativeRadio::signalStrength() const
{
    return m_radioTuner->signalStrength();
}

/*!
    \qmlproperty bool Radio::searching

    This property is true if the radio is currently searching for radio stations, for instance using the \l scanUp,
    \l scanDown, and \l searchAllStations methods. Once the search completes, or if it is cancelled using
    \l cancelScan, this property will be false.
*/
bool QDeclarativeRadio::searching() const
{
    return m_radioTuner->isSearching();
}

/*!
    \qmlproperty int Radio::frequencyStep

    The number of Hertz for each step when tuning the radio manually. The value is for the current \l band.
 */
int QDeclarativeRadio::frequencyStep() const
{
    return m_radioTuner->frequencyStep(m_radioTuner->band());
}

/*!
    \qmlproperty int Radio::minimumFrequency

    The minimum frequency for the current \l band.
 */
int QDeclarativeRadio::minimumFrequency() const
{
    return m_radioTuner->frequencyRange(m_radioTuner->band()).first;
}

/*!
    \qmlproperty int Radio::maximumFrequency

    The maximum frequency for the current \l band.
 */
int QDeclarativeRadio::maximumFrequency() const
{
    return m_radioTuner->frequencyRange(m_radioTuner->band()).second;
}

/*!
    \qmlmethod bool Radio::isAvailable()

    Returns whether the radio is ready to use.
 */
bool QDeclarativeRadio::isAvailable() const
{
    return m_radioTuner->isAvailable();
}

void QDeclarativeRadio::setBand(QDeclarativeRadio::Band band)
{
    m_radioTuner->setBand(static_cast<QRadioTuner::Band>(band));
}

void QDeclarativeRadio::setFrequency(int frequency)
{
    m_radioTuner->setFrequency(frequency);
}

void QDeclarativeRadio::setStereoMode(QDeclarativeRadio::StereoMode stereoMode)
{
    m_radioTuner->setStereoMode(static_cast<QRadioTuner::StereoMode>(stereoMode));
}

void QDeclarativeRadio::setVolume(int volume)
{
    m_radioTuner->setVolume(volume);
}

void QDeclarativeRadio::setMuted(bool muted)
{
    m_radioTuner->setMuted(muted);
}

/*!
    \qmlmethod Radio::cancelScan()

    Cancel the current scan. Will also cancel a search started with \l searchAllStations.
 */
void QDeclarativeRadio::cancelScan()
{
    m_radioTuner->cancelSearch();
}

/*!
    \qmlmethod Radio::scanDown()

    Searches backward in the frequency range for the current band.
 */
void QDeclarativeRadio::scanDown()
{
    m_radioTuner->searchBackward();
}

/*!
    \qmlmethod Radio::scanUp()

    Searches forward in the frequency range for the current band.
 */
void QDeclarativeRadio::scanUp()
{
    m_radioTuner->searchForward();
}

/*!
    \qmlmethod Radio::searchAllStations(enumeration searchMode)

    Start searching the complete frequency range for the current band, and save all the
    radio stations found. The search mode can be either of the values described in the
    table below.

    \table
    \header \o Value \o Description
    \row \o SearchFast
        \o Stores each radio station for later retrival and tuning

    \row \o SearchGetStationId
        \o Does the same as SearchFast, but also emits the station Id with the \l stationFound signal.

    \endtable

    The snippet below uses \c searchAllStations with \c SearchGetStationId to receive all the radio
    stations in the current band, and store them in a ListView. The station Id is shown to the user
    and if the user presses a station, the radio is tuned to this station.

    \qml
    Radio {
        id: radio
        onStationFound: radioStations.append({"frequency": frequency, "stationId": stationId})
    }

    ListModel {
        id: radioStations
    }

    ListView {
        model: radioStations
        delegate: Rectangle {
                MouseArea {
                    anchors.fill: parent
                    onClicked: radio.frequency = frequency
                }

                Text {
                    anchors.fill: parent
                    text: stationId
                }
            }
    }

    Rectangle {
        MouseArea {
            anchors.fill: parent
            onClicked: radio.searchAllStations(Radio.SearchGetStationId)
        }
    }
    \endqml
 */
void QDeclarativeRadio::searchAllStations(QDeclarativeRadio::SearchMode searchMode)
{
    m_radioTuner->searchAllStations(static_cast<QRadioTuner::SearchMode>(searchMode));
}

/*!
    \qmlmethod Radio::tuneDown()

    Decrements the frequency by the frequency step for the current band. If the frequency is already set
    to the minimum frequency, calling this function has no effect.

    \sa band, frequencyStep, minimumFrequency
 */
void QDeclarativeRadio::tuneDown()
{
    int f = frequency();
    f = f - frequencyStep();
    setFrequency(f);
}

/*!
    \qmlmethod Radio::tuneUp()

    Increments the frequency by the frequency step for the current band. If the frequency is already set
    to the maximum frequency, calling this function has no effect.

    \sa band, frequencyStep, maximumFrequency
 */
void QDeclarativeRadio::tuneUp()
{
    int f = frequency();
    f = f + frequencyStep();
    setFrequency(f);
}

/*!
    \qmlmethod Radio::start()

    Starts the radio. If the radio is available, as determined by the \l isAvailable method,
    this will result in the \l state becoming \c ActiveState.
 */
void QDeclarativeRadio::start()
{
    m_radioTuner->start();
}

/*!
    \qmlmethod Radio::stop()

    Stops the radio. After calling this method the \l state will be \c StoppedState.
 */
void QDeclarativeRadio::stop()
{
    m_radioTuner->stop();
}

void QDeclarativeRadio::_q_stateChanged(QRadioTuner::State state)
{
    emit stateChanged(static_cast<QDeclarativeRadio::State>(state));
}

void QDeclarativeRadio::_q_bandChanged(QRadioTuner::Band band)
{
    emit bandChanged(static_cast<QDeclarativeRadio::Band>(band));
}

void QDeclarativeRadio::_q_error(QRadioTuner::Error errorCode)
{
    emit error(static_cast<QDeclarativeRadio::Error>(errorCode));
    emit errorChanged();
}

/*!
    \qmlsignal Radio::stationFound(int frequency, string stationId)

    This signal is emitted when a new radio station is found. This signal is only emitted
    if \l searchAllStations is called with \c SearchGetStationId.

    The \a frequency is returned in Hertz, and the \a stationId corresponds to the station Id
    in the \l RadioData element for this radio station.
  */

QT_END_NAMESPACE
