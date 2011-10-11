/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

QDeclarativeRadio::State QDeclarativeRadio::state() const
{
    return static_cast<QDeclarativeRadio::State>(m_radioTuner->state());
}

QDeclarativeRadio::Band QDeclarativeRadio::band() const
{
    return static_cast<QDeclarativeRadio::Band>(m_radioTuner->band());
}

int QDeclarativeRadio::frequency() const
{
    return m_radioTuner->frequency();
}

QDeclarativeRadio::StereoMode QDeclarativeRadio::stereoMode() const
{
    return static_cast<QDeclarativeRadio::StereoMode>(m_radioTuner->stereoMode());
}

int QDeclarativeRadio::volume() const
{
    return m_radioTuner->volume();
}

bool QDeclarativeRadio::muted() const
{
    return m_radioTuner->isMuted();
}

bool QDeclarativeRadio::stereo() const
{
    return m_radioTuner->isStereo();
}

int QDeclarativeRadio::signalStrength() const
{
    return m_radioTuner->signalStrength();
}

bool QDeclarativeRadio::searching() const
{
    return m_radioTuner->isSearching();
}

int QDeclarativeRadio::frequencyStep() const
{
    return m_radioTuner->frequencyStep(m_radioTuner->band());
}

int QDeclarativeRadio::minimumFrequency() const
{
    return m_radioTuner->frequencyRange(m_radioTuner->band()).first;
}

int QDeclarativeRadio::maximumFrequency() const
{
    return m_radioTuner->frequencyRange(m_radioTuner->band()).second;
}

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

void QDeclarativeRadio::cancelScan()
{
    m_radioTuner->cancelSearch();
}

void QDeclarativeRadio::scanDown()
{
    m_radioTuner->searchBackward();
}

void QDeclarativeRadio::scanUp()
{
    m_radioTuner->searchForward();
}

void QDeclarativeRadio::searchAllStations(QDeclarativeRadio::SearchMode searchMode)
{
    m_radioTuner->searchAllStations(static_cast<QRadioTuner::SearchMode>(searchMode));
}

void QDeclarativeRadio::tuneDown()
{
    int f = frequency();
    f = f - frequencyStep();
    setFrequency(f);
}

void QDeclarativeRadio::tuneUp()
{
    int f = frequency();
    f = f + frequencyStep();
    setFrequency(f);
}

void QDeclarativeRadio::start()
{
    m_radioTuner->start();
}

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
