/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
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

#ifndef MOCKRADIOTUNERCONTROL_H
#define MOCKRADIOTUNERCONTROL_H

#include "qradiotunercontrol.h"

class MockRadioTunerControl : public QRadioTunerControl
{
    Q_OBJECT

public:
    MockRadioTunerControl(QObject *parent):
        QRadioTunerControl(parent),
        m_active(false),
        m_searching(false),m_muted(false),m_stereo(true),
        m_volume(100),m_signal(0),m_frequency(0),
        m_band(QRadioTuner::FM),m_err(QRadioTuner::NoError),
        m_errstr("")
    {
    }

    using QRadioTunerControl::error;

    QRadioTuner::State state() const
    {
        return m_active ? QRadioTuner::ActiveState : QRadioTuner::StoppedState;
    }

    bool isAvailable() const
    {
        return true;
    }
    QtMultimedia::AvailabilityError availabilityError() const
    {
        return QtMultimedia::NoError;
    }

    QRadioTuner::Band band() const
    {
        return m_band;
    }

    void setBand(QRadioTuner::Band b)
    {
        if (b == QRadioTuner::FM2 || b == QRadioTuner::LW) {
            m_err = QRadioTuner::OutOfRangeError;
            m_errstr.clear();
            m_errstr = QString("band and range not supported");
        } else {
            m_err = QRadioTuner::NoError;
            m_errstr.clear();
            m_band = b;
            emit bandChanged(m_band);
        }
        emit error(m_err);

    }

    bool isBandSupported(QRadioTuner::Band b) const
    {
        if (b == QRadioTuner::FM || b == QRadioTuner::AM)
            return true;

        return false;
    }

    int frequency() const
    {
        return m_frequency;
    }

    QPair<int,int> frequencyRange(QRadioTuner::Band) const
    {
        return qMakePair<int,int>(1,2);
    }

    int frequencyStep(QRadioTuner::Band) const
    {
        return 1;
    }

    void setFrequency(int frequency)
    {
        if (frequency >= 148500000) {
            m_err = QRadioTuner::OutOfRangeError;
            m_errstr.clear();
            m_errstr = QString("band and range not supported");
        } else {
            m_err = QRadioTuner::NoError;
            m_errstr.clear();
            m_frequency = frequency;
            emit frequencyChanged(m_frequency);
        }

        emit error(m_err);
    }

    bool isStereo() const
    {
        return m_stereo;
    }

    void setStereo(bool stereo)
    {
        emit stereoStatusChanged(m_stereo = stereo);
    }


    QRadioTuner::StereoMode stereoMode() const
    {
        return m_stereoMode;
    }

    void setStereoMode(QRadioTuner::StereoMode mode)
    {
        m_stereoMode = mode;
    }

    QRadioTuner::Error error() const
    {
        return m_err;
    }

    QString errorString() const
    {
        return m_errstr;
    }

    int signalStrength() const
    {
        return m_signal;
    }

    int volume() const
    {
        return m_volume;
    }

    void setVolume(int volume)
    {
        m_volume = volume;
        emit volumeChanged(m_volume);
    }

    bool isMuted() const
    {
        return m_muted;
    }

    void setMuted(bool muted)
    {
        m_muted = muted;
        emit mutedChanged(m_muted);
    }

    bool isSearching() const
    {
        return m_searching;
    }

    void searchForward()
    {
        m_searching = true;
        emit searchingChanged(m_searching);
    }

    void searchBackward()
    {
        m_searching = true;
        emit searchingChanged(m_searching);
    }

    void cancelSearch()
    {
        m_searching = false;
        emit searchingChanged(m_searching);
    }

    void start()
    {
        if (!m_active) {
            m_active = true;
            emit stateChanged(state());
        }
    }

    void stop()
    {
        if (m_active) {
            m_active = false;
            emit stateChanged(state());
        }
    }

public:
    bool m_active;
    bool m_searching;
    bool m_muted;
    bool m_stereo;
    int  m_volume;
    int  m_signal;
    int  m_frequency;
    QRadioTuner::StereoMode m_stereoMode;
    QRadioTuner::Band m_band;
    QRadioTuner::Error m_err;
    QString m_errstr;
};

#endif // MOCKRADIOTUNERCONTROL_H
