/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Mobility Components.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qradiotuner.h>
#include "qxaradiosession.h"
#include "xaradiosessionimpl.h"
#include "qxacommon.h"

QXARadioSession::QXARadioSession(QObject *parent)
:QObject(parent)
{
    QT_TRACE_FUNCTION_ENTRY;
    m_impl = new XARadioSessionImpl(*this);
    if (!m_impl) {
        QT_TRACE1("RadioSession::RadioSession(): ERROR creating RadioSessionImpl...");
        return;
    }
    if (m_impl->PostConstruct() != QRadioTuner::NoError) {
        QT_TRACE1("RadioSession::RadioSession(): ERROR from RadioSessionImpl::PostContstruct...");
        delete m_impl;
        m_impl = NULL;
    }
    QT_TRACE_FUNCTION_EXIT;
}

QXARadioSession::~QXARadioSession()
{
    delete m_impl;
}

QRadioTuner::State QXARadioSession::state() const
{
    QRadioTuner::State state = QRadioTuner::StoppedState;
    if (m_impl)
        state = m_impl->State();
    return state;
    }
QtMultimediaKit::AvailabilityError QXARadioSession::availabilityError() const
{
    QtMultimediaKit::AvailabilityError error = QtMultimediaKit::NoError;
    if (m_impl)
        error = m_impl->AvailabilityError();
    return error;
}

QRadioTuner::Band QXARadioSession::band() const
{
    QRadioTuner::Band band = QRadioTuner::FM;
    if (m_impl)
        band = m_impl->Band();
    return band;
}

void QXARadioSession::setBand(QRadioTuner::Band band)
{
    if (m_impl)
        m_impl->SetBand(band);
}

bool QXARadioSession::isBandSupported(QRadioTuner::Band band) const
{
    if (m_impl)
        return m_impl->IsBandSupported(band);
    return false;
}

bool QXARadioSession::isAvailable() const
{
    if (m_impl)
        return m_impl->IsAvailable();
    return false;
}

int QXARadioSession::frequency() const
{
    TInt frequency = 0;
    if (m_impl)
        frequency = m_impl->GetFrequency();
    return (int)frequency;
}

int QXARadioSession::frequencyStep(QRadioTuner::Band band) const
{
    TInt freqStep = 0;
    if (m_impl)
        freqStep = m_impl->FrequencyStep(band);
    return (int)freqStep;
}

QPair<int, int> QXARadioSession::frequencyRange(QRadioTuner::Band /*band*/) const
{
    QPair<int, int> freqRange;
    freqRange.first = 0;
    freqRange.second  =0;

    if (m_impl) {
        TInt freqRangeType = m_impl->GetFrequencyRange();
        m_impl->GetFrequencyRangeProperties(freqRangeType, freqRange.first, freqRange.second);
    }

    return freqRange;
}

void QXARadioSession::setFrequency(int frequency)
{
    if (m_impl)
        m_impl->SetFrequency(frequency);
}

bool QXARadioSession::isStereo() const
{
    bool isStereo = false;
    if (m_impl)
        isStereo = m_impl->IsStereo();
    return isStereo;
}

QRadioTuner::StereoMode QXARadioSession::stereoMode() const
{
    QRadioTuner::StereoMode mode(QRadioTuner::Auto);
    if (m_impl)
        mode = m_impl->StereoMode();
    return mode;
}

void QXARadioSession::setStereoMode(QRadioTuner::StereoMode mode)
{
    if (m_impl)
        m_impl->SetStereoMode(mode);
}

int QXARadioSession::signalStrength() const
{
    TInt signalStrength = 0;
    if (m_impl)
        signalStrength = m_impl->GetSignalStrength();
    return (int)signalStrength;
}

int QXARadioSession::volume() const
{
    TInt volume = 0;
    if (m_impl)
        volume = m_impl->GetVolume();
    return volume;
}

int QXARadioSession::setVolume(int volume)
{
    TInt newVolume = 0;
    if (m_impl) {
        m_impl->SetVolume(volume);
        newVolume = m_impl->GetVolume();
    }
    return newVolume;
}

bool QXARadioSession::isMuted() const
{
    bool isMuted = false;
    if (m_impl)
        isMuted = m_impl->IsMuted();
    return isMuted;
}

void QXARadioSession::setMuted(bool muted)
{
    if (m_impl)
        m_impl->SetMuted(muted);
}

bool QXARadioSession::isSearching() const
{
    bool isSearching = false;
    if (m_impl)
        isSearching = m_impl->IsSearching();
    return isSearching;
}

void QXARadioSession::searchForward()
{
    if (m_impl)
        m_impl->Seek(true);
}

void QXARadioSession::searchBackward()
{
    if (m_impl)
        m_impl->Seek(false);
}

void QXARadioSession::cancelSearch()
{
    if (m_impl)
        m_impl->StopSeeking();
}

void QXARadioSession::start()
{
    if (m_impl)
        m_impl->Start();
}

void QXARadioSession::stop()
{
    if (m_impl)
        m_impl->Stop();
}

QRadioTuner::Error QXARadioSession::error() const
{
    QRadioTuner::Error err(QRadioTuner::NoError);
    if (m_impl)
        err = m_impl->Error();
    return err;
}

QString QXARadioSession::errorString() const
{
    QString str = NULL;
    switch (iError) {
    case QRadioTuner::ResourceError:
        str = "Resource Error";
        break;
    case QRadioTuner::OpenError:
        str = "Open Error";
        break;
    case QRadioTuner::OutOfRangeError:
        str = "Out of Range Error";
        break;
    default:
        break;
    }

    return str;
}

// Callbacks, which will emit signals to client:
void QXARadioSession::CBStateChanged(QRadioTuner::State state)
{
    emit stateChanged(state);
}

void QXARadioSession::CBBandChanged(QRadioTuner::Band band)
{
    emit bandChanged(band);
}

void QXARadioSession::CBFrequencyChanged(TInt newFrequency)
{
    emit frequencyChanged(newFrequency);
}

void QXARadioSession::CBStereoStatusChanged(bool isStereo)
{
    emit stereoStatusChanged(isStereo);
}

void QXARadioSession::CBSignalStrengthChanged(int signalStrength)
{
    emit signalStrengthChanged(signalStrength);
}

void QXARadioSession::CBVolumeChanged(int volume)
{
    emit volumeChanged(volume);
}

void QXARadioSession::CBMutedChanged(bool isMuted)
{
    emit mutedChanged(isMuted);
}

void QXARadioSession::CBSearchingChanged(bool isSearching)
{
    emit searchingChanged(isSearching);
}

void QXARadioSession::CBError(QRadioTuner::Error err)
{
    iError = err;
    emit error((int)err, errorString());
}


