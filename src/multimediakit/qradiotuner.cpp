/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qradiotuner.h"
#include "qmediaservice.h"
#include "qmediaobject_p.h"
#include "qradiotunercontrol.h"

#include <QPair>


QT_BEGIN_NAMESPACE

/*!
    \class QRadioTuner
    \brief The QRadioTuner class provides an interface to the systems analog radio device.

    \inmodule QtMultimediaKit
    \ingroup multimedia
    \since 1.0

    You can control the systems analog radio device using this interface, for example:

    \snippet doc/src/snippets/multimedia-snippets/media.cpp Radio tuner

    The radio object will emit signals for any changes in state such as:
    bandChanged(), frequencyChanged(), stereoStatusChanged(), searchingChanged(),
    signalStrengthChanged(), volumeChanged(), mutedChanged().

    You can change between the frequency bands using setBand() however it is recommended
    that you check to make sure the band is available first using isBandSupported().

*/


class QRadioTunerPrivate : public QMediaObjectPrivate
{
public:
    QRadioTunerPrivate():provider(0), control(0) {}
    QMediaServiceProvider *provider;
    QRadioTunerControl* control;
};



/*!
    Constructs a radio tuner based on a media service allocated by a media service \a provider.

    The \a parent is passed to QMediaObject.
    \since 1.0
*/

QRadioTuner::QRadioTuner(QObject *parent, QMediaServiceProvider* provider):
    QMediaObject(*new QRadioTunerPrivate, parent, provider->requestService(Q_MEDIASERVICE_RADIO))
{
    Q_D(QRadioTuner);

    d->provider = provider;

    if (d->service != 0) {
        d->control = qobject_cast<QRadioTunerControl*>(d->service->requestControl(QRadioTunerControl_iid));
        if (d->control != 0) {
            connect(d->control, SIGNAL(stateChanged(QRadioTuner::State)), SIGNAL(stateChanged(QRadioTuner::State)));
            connect(d->control, SIGNAL(bandChanged(QRadioTuner::Band)), SIGNAL(bandChanged(QRadioTuner::Band)));
            connect(d->control, SIGNAL(frequencyChanged(int)), SIGNAL(frequencyChanged(int)));
            connect(d->control, SIGNAL(stereoStatusChanged(bool)), SIGNAL(stereoStatusChanged(bool)));
            connect(d->control, SIGNAL(searchingChanged(bool)), SIGNAL(searchingChanged(bool)));
            connect(d->control, SIGNAL(signalStrengthChanged(int)), SIGNAL(signalStrengthChanged(int)));
            connect(d->control, SIGNAL(volumeChanged(int)), SIGNAL(volumeChanged(int)));
            connect(d->control, SIGNAL(mutedChanged(bool)), SIGNAL(mutedChanged(bool)));
            connect(d->control, SIGNAL(error(QRadioTuner::Error)), SIGNAL(error(QRadioTuner::Error)));
        }
    }
}

/*!
    Destroys a radio tuner.
*/

QRadioTuner::~QRadioTuner()
{
    Q_D(QRadioTuner);

    if (d->service && d->control)
        d->service->releaseControl(d->control);

    d->provider->releaseService(d->service);
}

/*!
    Returns true if the radio tuner service is ready to use.
    \since 1.0
*/
bool QRadioTuner::isAvailable() const
{
    if (d_func()->control != NULL)
        return d_func()->control->isAvailable();
    else
        return false;
}

/*!
    Returns the availability error state.
    \since 1.0
*/
QtMultimediaKit::AvailabilityError QRadioTuner::availabilityError() const
{
    if (d_func()->control != NULL)
        return d_func()->control->availabilityError();
    else
        return QtMultimediaKit::ServiceMissingError;
}

/*!
    \property QRadioTuner::state
    Return the current radio tuner state.

    \since 1.0
    \sa QRadioTuner::State
*/

QRadioTuner::State QRadioTuner::state() const
{
    return d_func()->control ?
            d_func()->control->state() : QRadioTuner::StoppedState;
}

/*!
    \property QRadioTuner::band
    \brief the frequency band a radio tuner is tuned to.

    \since 1.0
    \sa QRadioTuner::Band
*/

QRadioTuner::Band QRadioTuner::band() const
{
    Q_D(const QRadioTuner);

    if (d->control != 0)
        return d->control->band();

    return QRadioTuner::FM;
}

/*!
    \property QRadioTuner::frequency
    \brief the frequency in Hertz a radio tuner is tuned to.
    \since 1.0
*/

int QRadioTuner::frequency() const
{
    Q_D(const QRadioTuner);

    if (d->control != 0)
        return d->control->frequency();

    return 0;
}

/*!
    Returns the number of Hertz to increment the frequency by when stepping through frequencies
    within a given \a band.
    \since 1.0
*/

int QRadioTuner::frequencyStep(QRadioTuner::Band band) const
{
    Q_D(const QRadioTuner);

    if(d->control != 0)
        return d->control->frequencyStep(band);

    return 0;
}

/*!
    Returns a frequency \a band's minimum and maximum frequency.
    \since 1.0
*/

QPair<int,int> QRadioTuner::frequencyRange(QRadioTuner::Band band) const
{
    Q_D(const QRadioTuner);

    if(d->control != 0)
        return d->control->frequencyRange(band);

    return qMakePair<int,int>(0,0);
}

/*!
    \property QRadioTuner::stereo
    \brief whether a radio tuner is receiving a stereo signal.
    \since 1.0
*/

bool QRadioTuner::isStereo() const
{
    Q_D(const QRadioTuner);

    if (d->control != 0)
        return d->control->isStereo();

    return false;
}


/*!
    \property QRadioTuner::stereoMode
    \brief the stereo mode of a radio tuner.
    \since 1.0
*/

QRadioTuner::StereoMode QRadioTuner::stereoMode() const
{
    Q_D(const QRadioTuner);

    if (d->control != 0)
        return d->control->stereoMode();

    return QRadioTuner::Auto;
}

void QRadioTuner::setStereoMode(QRadioTuner::StereoMode mode)
{
    Q_D(QRadioTuner);

    if (d->control != 0)
        return d->control->setStereoMode(mode);
}

/*!
    Identifies if a frequency \a band is supported by a radio tuner.

    Returns true if the band is supported, and false if it is not.
    \since 1.0
*/

bool QRadioTuner::isBandSupported(QRadioTuner::Band band) const
{
    Q_D(const QRadioTuner);

    if (d->control != 0)
        return d->control->isBandSupported(band);

    return false;
}

/*!
    Activate the radio device.
    \since 1.0
*/

void QRadioTuner::start()
{
    Q_D(const QRadioTuner);

    if (d->control != 0)
        d->control->start();
}

/*!
    Deactivate the radio device.
    \since 1.0
*/

void QRadioTuner::stop()
{
    Q_D(const QRadioTuner);

    if (d->control != 0)
        d->control->stop();
}

/*!
    \property QRadioTuner::signalStrength
    \brief the strength of the current radio signal as a percentage.
    \since 1.0
*/

int QRadioTuner::signalStrength() const
{
    Q_D(const QRadioTuner);

    if (d->control != 0)
        return d->control->signalStrength();

    return 0;
}

/*!
    \property QRadioTuner::volume
    \brief the volume of a radio tuner's audio output as a percentage.
    \since 1.0
*/


int QRadioTuner::volume() const
{
    Q_D(const QRadioTuner);

    if (d->control != 0)
        return d->control->volume();

    return 0;
}

/*!
    \property QRadioTuner::muted
    \brief whether a radio tuner's audio output is muted.
    \since 1.0
*/

bool QRadioTuner::isMuted() const
{
    Q_D(const QRadioTuner);

    if (d->control != 0)
        return d->control->isMuted();

    return false;
}

/*!
    Sets a radio tuner's frequency \a band.

    Changing the band will reset the \l frequency to the new band's minimum frequency.
    \since 1.0
*/

void QRadioTuner::setBand(QRadioTuner::Band band)
{
    Q_D(QRadioTuner);

    if (d->control != 0)
        d->control->setBand(band);
}

/*!
    Sets a radio tuner's \a frequency.

    If the tuner is set to a frequency outside the current \l band, the band will be changed to
    one occupied by the new frequency.
    \since 1.0
*/

void QRadioTuner::setFrequency(int frequency)
{
    Q_D(QRadioTuner);

    if (d->control != 0)
        d->control->setFrequency(frequency);
}

void QRadioTuner::setVolume(int volume)
{
    Q_D(QRadioTuner);

    if (d->control != 0)
        d->control->setVolume(volume);
}

void QRadioTuner::setMuted(bool muted)
{
    Q_D(QRadioTuner);

    if (d->control != 0)
        d->control->setMuted(muted);
}

/*!
    \property QRadioTuner::searching
    \brief whether a radio tuner is currently scanning for a signal.

    \sa searchForward(), searchBackward(), cancelSearch()
    \since 1.0
*/

bool QRadioTuner::isSearching() const
{
    Q_D(const QRadioTuner);

    if (d->control != 0)
        return d->control->isSearching();

    return false;
}

/*!
    Starts a forward scan for a signal, starting from the current \l frequency.

    \since 1.0
    \sa searchBackward(), cancelSearch(), searching
*/

void QRadioTuner::searchForward()
{
    Q_D(QRadioTuner);

    if (d->control != 0)
        d->control->searchForward();
}

/*!
    Starts a backwards scan for a signal, starting from the current \l frequency.

    \since 1.0
    \sa searchForward(), cancelSearch(), searching
*/

void QRadioTuner::searchBackward()
{
    Q_D(QRadioTuner);

    if (d->control != 0)
        d->control->searchBackward();
}

/*!
    Stops scanning for a signal.

    \since 1.0
    \sa searchForward(), searchBackward(), searching
*/

void QRadioTuner::cancelSearch()
{
    Q_D(QRadioTuner);

    if (d->control != 0)
        d->control->cancelSearch();
}

/*!
    Returns the error state of a radio tuner.

    \since 1.0
    \sa errorString()
*/

QRadioTuner::Error QRadioTuner::error() const
{
    Q_D(const QRadioTuner);

    if (d->control != 0)
        return d->control->error();

    return QRadioTuner::ResourceError;
}

/*!
    Returns a description of a radio tuner's error state.

    \since 1.0
    \sa error()
*/

QString QRadioTuner::errorString() const
{
    Q_D(const QRadioTuner);

    if (d->control != 0)
        return d->control->errorString();

    return QString();
}

/*!
    \fn void QRadioTuner::bandChanged(QRadioTuner::Band band)

    Signals a radio tuner's \a band has changed.
    \since 1.0
*/

/*!
    \fn void QRadioTuner::frequencyChanged(int frequency)

    Signals that the \a frequency a radio tuner is tuned to has changed.
    \since 1.0
*/

/*!
    \fn void QRadioTuner::mutedChanged(bool muted)

    Signals that the \a muted state of a radio tuner's audio output has changed.
    \since 1.0
*/

/*!
    \fn void QRadioTuner::volumeChanged(int volume)

    Signals that the \a volume of a radio tuner's audio output has changed.
    \since 1.0
*/

/*!
    \fn void QRadioTuner::searchingChanged(bool searching)

    Signals that the \a searching state of a radio tuner has changed.
    \since 1.0
*/

/*!
    \fn void QRadioTuner::stereoStatusChanged(bool stereo)

    Signals that the \a stereo state of a radio tuner has changed.
    \since 1.0
*/

/*!
    \fn void QRadioTuner::signalStrengthChanged(int strength)

    Signals that the \a strength of the signal received by a radio tuner has changed.
    \since 1.0
*/

/*!
    \fn void QRadioTuner::error(QRadioTuner::Error error)

    Signals that an \a error occurred.
    \since 1.0
*/

/*!
    \enum QRadioTuner::State

    Enumerates radio tuner states.

    \value ActiveState The tuner is started and active.
    \value StoppedState The tuner device is stopped.
*/


/*!
    \enum QRadioTuner::Band

    Enumerates radio frequency bands.

    \value AM 520 to 1610 kHz, 9 or 10kHz channel spacing, extended 1610 to 1710 kHz
    \value FM 87.5 to 108.0 MHz, except Japan 76-90 MHz
    \value SW 1.711 to 30.0 MHz, divided into 15 bands. 5kHz channel spacing
    \value LW 148.5 to 283.5 kHz, 9kHz channel spacing (Europe, Africa, Asia)
    \value FM2 range not defined, used when area supports more than one FM range.
*/

/*!
    \enum QRadioTuner::Error

    Enumerates radio tuner error conditions.

    \value NoError         No errors have occurred.
    \value ResourceError   There is no radio service available.
    \value OpenError       Unable to open radio device.
    \value OutOfRangeError An attempt to set a frequency or band that is not supported by radio device.
*/

/*!
    \enum QRadioTuner::StereoMode

    Enumerates radio tuner policy for receiving stereo signals.

    \value Auto        Uses the stereo mode matching the station.
    \value ForceStereo Provide stereo mode, converting if required.
    \value ForceMono   Provide mono mode, converting if required.
*/

/*! \fn void QRadioTuner::stateChanged(QRadioTuner::State state)
  This signal is emitted when the state changes to \a state.
  \since 1.0
 */

#include "moc_qradiotuner.cpp"
QT_END_NAMESPACE

