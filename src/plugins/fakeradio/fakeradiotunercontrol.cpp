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

#include "fakeradiotunercontrol.h"
#include "fakeradioservice.h"

#include <QtCore/qdebug.h>

FakeRadioTunerControl::FakeRadioTunerControl(QObject *parent)
    :QRadioTunerControl(parent)
{
    m_state = QRadioTuner::StoppedState;
    m_freqMin = 520000;
    m_freqMax = 108000000;
    m_currentBand = QRadioTuner::FM;
    m_currentFreq = 0;
    m_stereo = true;
    m_stereoMode = QRadioTuner::Auto;
    m_signalStrength = 0;
    m_volume = 50;
    m_muted = false;

    m_searching = false;
    m_forward = true;
    m_searchTimer = new QTimer(this);
    m_searchTimer->setSingleShot(true);
    connect(m_searchTimer, SIGNAL(timeout()), this, SLOT(searchEnded()));

    QTimer::singleShot(300, this, SLOT(delayedInit()));

    qsrand(QTime::currentTime().msec());
}

FakeRadioTunerControl::~FakeRadioTunerControl()
{
    m_searchTimer->stop();
}

bool FakeRadioTunerControl::isAvailable() const
{
    return true;
}

QtMultimediaKit::AvailabilityError FakeRadioTunerControl::availabilityError() const
{
    return QtMultimediaKit::NoError;
}

QRadioTuner::State FakeRadioTunerControl::state() const
{
    return m_state;
}

QRadioTuner::Band FakeRadioTunerControl::band() const
{
    return m_currentBand;
}

bool FakeRadioTunerControl::isBandSupported(QRadioTuner::Band b) const
{
    switch (b) {
        case QRadioTuner::FM:
            if (m_freqMin <= 87500000 && m_freqMax >= 108000000)
                return true;
            break;
        case QRadioTuner::LW:
            if (m_freqMin <= 148500 && m_freqMax >= 283500)
                return true;
        case QRadioTuner::AM:
            if (m_freqMin <= 520000 && m_freqMax >= 1610000)
                return true;
        default:
            if (m_freqMin <= 1711000 && m_freqMax >= 30000000)
                return true;
    }

    return false;
}

void FakeRadioTunerControl::setBand(QRadioTuner::Band b)
{
    if (isBandSupported(b)) {
        m_currentBand = b;
        emit bandChanged(m_currentBand);

        int f = m_currentFreq;
        QPair<int, int> fRange = frequencyRange(m_currentBand);

        if (f < fRange.first)
            f = fRange.first;
        if (f > fRange.second)
            f = fRange.second;

        if (f != m_currentFreq) {
            m_currentFreq = f;
            emit frequencyChanged(m_currentFreq);
        }
    }
}

int FakeRadioTunerControl::frequency() const
{
    return m_currentFreq;
}

int FakeRadioTunerControl::frequencyStep(QRadioTuner::Band b) const
{
    int step = 0;

    if (b == QRadioTuner::FM)
        step = 100000; // 100kHz steps
    else if (b == QRadioTuner::LW)
        step = 1000; // 1kHz steps
    else if (b == QRadioTuner::AM)
        step = 1000; // 1kHz steps
    else if (b == QRadioTuner::SW)
        step = 500; // 500Hz steps

    return step;
}

QPair<int,int> FakeRadioTunerControl::frequencyRange(QRadioTuner::Band b) const
{
    if (b == QRadioTuner::FM)
        return qMakePair<int,int>(87500000,108000000);
    else if (b == QRadioTuner::LW)
        return qMakePair<int,int>(148500,283500);
    else if (b == QRadioTuner::AM)
        return qMakePair<int,int>(520000,1710000);
    else if (b == QRadioTuner::SW)
        return qMakePair<int,int>(1711111,30000000);

    return qMakePair<int,int>(0,0);
}

void FakeRadioTunerControl::setFrequency(int frequency)
{
    qint64 f = frequency;
    QPair<int, int> fRange = frequencyRange(m_currentBand);

    if (frequency < fRange.first)
        f = fRange.first;
    if (frequency > fRange.second)
        f = fRange.second;

    m_currentFreq = f;
    emit frequencyChanged(m_currentFreq);
}

bool FakeRadioTunerControl::isStereo() const
{
    return m_stereo;
}

QRadioTuner::StereoMode FakeRadioTunerControl::stereoMode() const
{
    return m_stereoMode;
}

void FakeRadioTunerControl::setStereoMode(QRadioTuner::StereoMode mode)
{
    bool stereo = true;

    if (mode == QRadioTuner::ForceMono)
        stereo = false;
    else
        stereo = true;

    m_stereo = stereo;
    m_stereoMode = mode;

    emit stereoStatusChanged(stereo);
}

int FakeRadioTunerControl::signalStrength() const
{
    return m_signalStrength;
}

int FakeRadioTunerControl::volume() const
{
    return m_volume;
}

void FakeRadioTunerControl::setVolume(int volume)
{
    int v = volume;

    if (v < 0)
        v = 0;
    if (100 > v)
        v = 100;

    m_volume = v;
}

bool FakeRadioTunerControl::isMuted() const
{
    return m_muted;
}

void FakeRadioTunerControl::setMuted(bool muted)
{
    if (muted != m_muted) {
        m_muted = muted;
        emit mutedChanged(m_muted);
    }
}

bool FakeRadioTunerControl::isSearching() const
{
    return m_searching;
}

void FakeRadioTunerControl::cancelSearch()
{
    m_searching = false;
    m_searchTimer->stop();
    emit searchingChanged(m_searching);
}

void FakeRadioTunerControl::searchForward()
{
    m_forward = true;
    performSearch();
}

void FakeRadioTunerControl::searchBackward()
{
    m_forward = false;
    performSearch();
}

void FakeRadioTunerControl::start()
{
    if (isAvailable() && m_state != QRadioTuner::ActiveState) {
        m_state = QRadioTuner::ActiveState;
        emit stateChanged(m_state);
    }
}

void FakeRadioTunerControl::stop()
{
    if (m_state != QRadioTuner::StoppedState) {
        m_state = QRadioTuner::StoppedState;
        emit stateChanged(m_state);
    }
}

QRadioTuner::Error FakeRadioTunerControl::error() const
{
    return QRadioTuner::NoError;
}

QString FakeRadioTunerControl::errorString() const
{
    return QString();
}

void FakeRadioTunerControl::delayedInit()
{
    m_signalStrength = 50;
    emit signalStrengthChanged(m_signalStrength);
}

void FakeRadioTunerControl::performSearch()
{
    m_searching = true;
    m_searchTimer->start(qrand() % 1000);
    emit searchingChanged(m_searching);
}

void FakeRadioTunerControl::searchEnded()
{
    int minFreq, maxFreq, newFreq;
    QPair<int, int> fRange = frequencyRange(m_currentBand);

    if (m_forward) {
        minFreq = m_currentFreq;
        maxFreq = fRange.second;
    } else {
        minFreq = fRange.first;
        maxFreq = m_currentFreq;
    }

    if ((qreal)(maxFreq - minFreq) / (qreal)(fRange.second - fRange.first) < 0.02) {
        // don't change frequency if we have less than 2% of the range to scan
        m_searching = false;
        emit searchingChanged(m_searching);
        return;
    }

    newFreq = (qrand() % (maxFreq - minFreq)) + minFreq;
    newFreq -= newFreq % frequencyStep(m_currentBand);

    m_searching = false;
    m_currentFreq = newFreq;
    emit searchingChanged(m_searching);
    emit frequencyChanged(m_currentFreq);
}
