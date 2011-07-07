/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Mobility Components.
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

#include "DebugMacros.h"

#include "s60radiotunercontrol_31.h"
#include "s60radiotunerservice.h"

#include <QtCore/qdebug.h>
#include <QFile>

// from AudioPreference.h
const TInt KAudioPriorityFMRadio = 79;
const TUint KAudioPrefRadioAudioEvent = 0x03000001;

S60RadioTunerControl::S60RadioTunerControl(QObject *parent)
    : QRadioTunerControl(parent)
    , m_error(0)
    , m_tunerState(0)
    , m_apiTunerState(QRadioTuner::StoppedState)
    , m_audioInitializationComplete(false)
    , m_radioError(QRadioTuner::NoError)
    , m_muted(false)
    , m_isStereo(true)
    , m_stereoMode(QRadioTuner::Auto)
    , m_signal(0)
    , m_currentBand(QRadioTuner::FM)
    , m_currentFreq(87500000)
    , m_scanning(false)
    , m_vol(50)
{
    DP0("S60RadioTunerControl::S60RadioTunerControl +++");

    initRadio();

    DP0("S60RadioTunerControl::S60RadioTunerControl ---");
}

S60RadioTunerControl::~S60RadioTunerControl()
{
    DP0("S60RadioTunerControl::~S60RadioTunerControl +++");

	if (m_tunerUtility) {
	    m_tunerUtility->Close();
		m_tunerUtility->CancelNotifyChange();
		m_tunerUtility->CancelNotifySignalStrength();
		m_tunerUtility->CancelNotifyStereoChange();
		delete m_tunerUtility;
	}
	if (m_audioPlayerUtility) {
		m_audioPlayerUtility = NULL;
	}

  DP0("S60RadioTunerControl::~S60RadioTunerControl ---");
}

bool S60RadioTunerControl::initRadio()
{
    DP0("S60RadioTunerControl::initRadio +++");

	m_available = false;

	TRAPD(tunerError, m_tunerUtility = CMMTunerUtility::NewL(*this, CMMTunerUtility::ETunerBandFm, 1, 
												CMMTunerUtility::ETunerAccessPriorityNormal));
	if (tunerError != KErrNone) {
        m_radioError = QRadioTuner::OpenError;
        return m_available;
    }
	
	TRAPD(playerError, m_audioPlayerUtility = m_tunerUtility->TunerPlayerUtilityL(*this));
	if (playerError != KErrNone) {
		m_radioError = QRadioTuner::OpenError;
		return m_available;
	}
	
	TRAPD(initializeError, m_audioPlayerUtility->InitializeL(KAudioPriorityFMRadio, 
												TMdaPriorityPreference(KAudioPrefRadioAudioEvent)));
	if (initializeError != KErrNone) {
		m_radioError = QRadioTuner::OpenError;
		return m_available;
	}
		
	m_tunerUtility->NotifyChange(*this);
	m_tunerUtility->NotifyStereoChange(*this);
	m_tunerUtility->NotifySignalStrength(*this);
	
	TFrequency freq(m_currentFreq);
	m_tunerUtility->Tune(freq);
		
	m_available = true;

  DP0("S60RadioTunerControl::initRadio ---");

    return m_available;
}

void S60RadioTunerControl::start()
{
    DP0("S60RadioTunerControl::start +++");

	if (!m_audioInitializationComplete) {
		TFrequency freq(m_currentFreq);
		m_tunerUtility->Tune(freq);
	} else {
		m_audioPlayerUtility->Play();
	}

	m_apiTunerState = QRadioTuner::ActiveState;
	emit stateChanged(m_apiTunerState);

  DP0("S60RadioTunerControl::start ---");
}

void S60RadioTunerControl::stop()
{
    DP0("S60RadioTunerControl::stop +++");

    if (m_audioPlayerUtility) {
		m_audioPlayerUtility->Stop();
		m_apiTunerState = QRadioTuner::StoppedState;
		emit stateChanged(m_apiTunerState);
    }

    DP0("S60RadioTunerControl::stop ---");
}

QRadioTuner::State S60RadioTunerControl::state() const
{
    DP0("S60RadioTunerControl::state");

    return m_apiTunerState;
}

QRadioTuner::Band S60RadioTunerControl::band() const
{
    DP0("S60RadioTunerControl::band");

    return m_currentBand;
}

bool S60RadioTunerControl::isBandSupported(QRadioTuner::Band b) const
{
    DP0("S60RadioTunerControl::isBandSupported");

	if(b == QRadioTuner::FM)
		return true;
	else if(b == QRadioTuner::LW)
		return false;
	else if(b == QRadioTuner::AM)
		return true;
	else if(b == QRadioTuner::SW)
		return false;
	else
		return false;
}

void S60RadioTunerControl::setBand(QRadioTuner::Band b)
{
    DP0("S60RadioTunerControl::setBand +++");

    QRadioTuner::Band tempBand = b;
    if (tempBand != m_currentBand) {
        m_currentBand = b;        
        emit bandChanged(m_currentBand);
    }

    DP0("S60RadioTunerControl::setBand ---");
}

int S60RadioTunerControl::frequency() const
{
    DP0("S60RadioTunerControl::frequency");

    return m_currentFreq;
}

void S60RadioTunerControl::setFrequency(int frequency)
{
    DP0("S60RadioTunerControl::setFrequency +++");

    m_currentFreq = frequency;
    TFrequency freq(m_currentFreq);
    m_tunerUtility->Tune(freq);

    DP0("S60RadioTunerControl::setFrequency ---");
}

int S60RadioTunerControl::frequencyStep(QRadioTuner::Band b) const
{
    DP0("S60RadioTunerControl::frequencyStep +++");

    int step = 0;

    if(b == QRadioTuner::FM)
        step = 100000; // 100kHz steps
    else if(b == QRadioTuner::LW)
        step = 1000; // 1kHz steps
    else if(b == QRadioTuner::AM)
        step = 1000; // 1kHz steps
    else if(b == QRadioTuner::SW)
        step = 500; // 500Hz steps

    DP1("S60RadioTunerControl::frequencyStep, Step:", step);
    DP0("S60RadioTunerControl::frequencyStep ---");

    return step;
}

QPair<int,int> S60RadioTunerControl::frequencyRange(QRadioTuner::Band band) const
{
    DP0("S60RadioTunerControl::frequencyRange +++");

	TFrequency bottomFreq;
	TFrequency topFreq;
	int bandError = KErrNone;
   
	if (m_tunerUtility){
		bandError = m_tunerUtility->GetFrequencyBandRange(bottomFreq, topFreq);	   
		if (!bandError) {
			return qMakePair<int,int>(bottomFreq.iFrequency, topFreq.iFrequency);
		}
    }

  DP0("S60RadioTunerControl::frequencyRange ---");

   return qMakePair<int,int>(0,0);
}

CMMTunerUtility::TTunerBand S60RadioTunerControl::getNativeBand(QRadioTuner::Band b) const
{
    DP0("S60RadioTunerControl::getNativeBand");

    // api match to native s60 bands
    if (b == QRadioTuner::AM)
        return CMMTunerUtility::ETunerBandAm;
    else if (b == QRadioTuner::FM)
        return CMMTunerUtility::ETunerBandFm;
    else if (b == QRadioTuner::LW)
        return CMMTunerUtility::ETunerBandLw;
    else
        return CMMTunerUtility::ETunerNoBand;
}

bool S60RadioTunerControl::isStereo() const
{
    DP0("S60RadioTunerControl::isStereo");

    return m_isStereo;
}

QRadioTuner::StereoMode S60RadioTunerControl::stereoMode() const
{
    DP0("S60RadioTunerControl::stereoMode");

    return m_stereoMode;
}

void S60RadioTunerControl::setStereoMode(QRadioTuner::StereoMode mode)
{
    DP0("S60RadioTunerControl::setStereoMode +++");

	m_stereoMode = mode;
	if (m_tunerUtility) {	    
	    if (QRadioTuner::ForceMono == mode)
	        m_tunerUtility->ForceMonoReception(true);
	    else 
	        m_tunerUtility->ForceMonoReception(false);
     }

  DP0("S60RadioTunerControl::setStereoMode ---");
}

int S60RadioTunerControl::signalStrength() const
{
    DP0("S60RadioTunerControl::signalStrength +++");

    // return value is a percentage value
    if (m_tunerUtility) {       
        TInt maxSignalStrength;
        TInt currentSignalStrength;
        m_error = m_tunerUtility->GetMaxSignalStrength(maxSignalStrength);       
        if (m_error == KErrNone) {
            m_error = m_tunerUtility->GetSignalStrength(currentSignalStrength);
            if (m_error == KErrNone) {
				if (maxSignalStrength == 0 || currentSignalStrength == 0) {
					return 0;
				}
                m_signal = ((TInt64)currentSignalStrength) * 100 / maxSignalStrength;
            }           
        }
    }

    DP1("S60RadioTunerControl::signalStrength, m_signal:", m_signal);
    DP0("S60RadioTunerControl::signalStrength ---");

    return m_signal;
}

int S60RadioTunerControl::volume() const
{
    DP0("S60RadioTunerControl::volume");

    return m_vol;
}

void S60RadioTunerControl::setVolume(int volume)
{
    DP0("S60RadioTunerControl::setVolume +++");
    DP1("S60RadioTunerControl::setVolume: ", volume);

    if (m_audioPlayerUtility) {
		m_vol = volume;
		TInt error = m_audioPlayerUtility->SetVolume(volume/10);
		emit volumeChanged(m_vol);
    }

    DP0("S60RadioTunerControl::setVolume ---");
}

bool S60RadioTunerControl::isMuted() const
{
    DP0("S60RadioTunerControl::isMuted");

    return m_muted;
}

void S60RadioTunerControl::setMuted(bool muted)
{
    DP0("S60RadioTunerControl::setMuted +++");

    DP1("S60RadioTunerControl::setMuted:", muted);

    if (m_audioPlayerUtility && m_audioInitializationComplete) {
        m_muted = muted;
        m_audioPlayerUtility->Mute(m_muted);
        emit mutedChanged(m_muted);           
    }

    DP0("S60RadioTunerControl::setMuted ---");
}

bool S60RadioTunerControl::isSearching() const
{
    DP0("S60RadioTunerControl::isSearching");

    if (m_tunerUtility) {
    	TUint32 tempState;
    	m_tunerUtility->GetState(tempState);
    	if (tempState == CMMTunerUtility::ETunerStateRetuning || m_scanning) {
			return true;
    	} else
    		return false;
    }
    return true;
}

void S60RadioTunerControl::cancelSearch()
{
    DP0("S60RadioTunerControl::cancelSearch +++");

	m_tunerUtility->CancelRetune();
	m_scanning = false;
	emit searchingChanged(false);

  DP0("S60RadioTunerControl::cancelSearch ---");
}

void S60RadioTunerControl::searchForward()
{
    DP0("S60RadioTunerControl::searchForward +++");

	m_scanning = true;
	setVolume(m_vol);
	m_tunerUtility->StationSeek(CMMTunerUtility::ESearchDirectionUp);
	emit searchingChanged(true);

  DP0("S60RadioTunerControl::searchForward ---");
}

void S60RadioTunerControl::searchBackward()
{
    DP0("S60RadioTunerControl::searchBackward +++");

	m_scanning = true;
	setVolume(m_vol);
	m_tunerUtility->StationSeek(CMMTunerUtility::ESearchDirectionDown);
	emit searchingChanged(true);

  DP0("S60RadioTunerControl::searchBackward ---");
}

bool S60RadioTunerControl::isValid() const
{
    DP0("S60RadioTunerControl::isValid");

    return m_available;
}

bool S60RadioTunerControl::isAvailable() const
{
    DP0("S60RadioTunerControl::isAvailable");

    return m_available;
}

QtMultimediaKit::AvailabilityError S60RadioTunerControl::availabilityError() const
{
    DP0("S60RadioTunerControl::availabilityError");

    if (m_available)
        return QtMultimediaKit::NoError;
    else
        return QtMultimediaKit::ResourceError;
}

QRadioTuner::Error S60RadioTunerControl::error() const
{
    DP1("QtMultimediaKit::NoError", m_radioError);

    return m_radioError;
}

QString S60RadioTunerControl::errorString() const
{
    DP1("S60RadioTunerControl::errorString", m_errorString);

	return m_errorString;
}

void S60RadioTunerControl::MToTuneComplete(TInt aError)
{
    DP0("S60RadioTunerControl::MToTuneComplete +++");
    DP1("S60RadioTunerControl::MToTuneComplete, aError:",aError);

	if (aError == KErrNone) {
		m_scanning = false;
		m_audioPlayerUtility->Play();
		if (!m_audioInitializationComplete) {
		TRAPD(initializeError, m_audioPlayerUtility->InitializeL(KAudioPriorityFMRadio, 
                                                        TMdaPriorityPreference(KAudioPrefRadioAudioEvent)));
			if (initializeError != KErrNone) {
				m_radioError = QRadioTuner::OpenError;
			}
		}
	}

    DP0("S60RadioTunerControl::MToTuneComplete ---");
}

void S60RadioTunerControl::MTcoFrequencyChanged(const TFrequency& aOldFrequency, const TFrequency& aNewFrequency)
{
    DP0("S60RadioTunerControl::MTcoFrequencyChanged +++");

	m_currentFreq = aNewFrequency.iFrequency;
	m_scanning = false;
	emit frequencyChanged(m_currentFreq);

  DP0("S60RadioTunerControl::MTcoFrequencyChanged ---");
}

void S60RadioTunerControl::MTcoStateChanged(const TUint32& aOldState, const TUint32& aNewState)
{
    DP0("S60RadioTunerControl::MTcoStateChanged +++");

	if (aNewState == CMMTunerUtility::ETunerStateActive) {
		m_apiTunerState = QRadioTuner::ActiveState;
	}
	if (aNewState == CMMTunerUtility::ETunerStatePlaying) {
		m_apiTunerState = QRadioTuner::ActiveState;
	}	
	if (aOldState != aNewState){
		emit stateChanged(m_apiTunerState);
	}

  DP0("S60RadioTunerControl::MTcoStateChanged ---");
}

void S60RadioTunerControl::MTcoAntennaDetached()
{
    DP0("S60RadioTunerControl::MTcoAntennaDetached +++");

    DP0("S60RadioTunerControl::MTcoAntennaDetached ---");

	// no actions
}

void S60RadioTunerControl::MTcoAntennaAttached()
{
    DP0("S60RadioTunerControl::MTcoAntennaAttached +++");

    DP0("S60RadioTunerControl::MTcoAntennaAttached ---");

	// no actions
}

void S60RadioTunerControl::FlightModeChanged(TBool aFlightMode)
{
    DP0("S60RadioTunerControl::FlightModeChanged +++");

    DP0("S60RadioTunerControl::FlightModeChanged ---");

	// no actions
}

void S60RadioTunerControl::MTsoStereoReceptionChanged(TBool aStereo)
{
    DP0("S60RadioTunerControl::MTsoStereoReceptionChanged +++");
    DP1("S60RadioTunerControl::MTsoStereoReceptionChanged, aStereo:", aStereo);
	m_isStereo = aStereo;
	emit stereoStatusChanged(aStereo);

  DP0("S60RadioTunerControl::MTsoStereoReceptionChanged ---");
}

void S60RadioTunerControl::MTsoForcedMonoChanged(TBool aForcedMono)
{
    DP0("S60RadioTunerControl::MTsoForcedMonoChanged +++");
    DP1("S60RadioTunerControl::MTsoForcedMonoChanged, aForcedMono:", aForcedMono);

	if (aForcedMono) {
		m_stereoMode = QRadioTuner::ForceMono;
	}

  DP0("S60RadioTunerControl::MTsoForcedMonoChanged ---");
}

void S60RadioTunerControl::MssoSignalStrengthChanged(TInt aNewSignalStrength)
{
    DP0("S60RadioTunerControl::MssoSignalStrengthChanged +++");
    DP1("S60RadioTunerControl::MssoSignalStrengthChanged, aNewSignalStrength:", aNewSignalStrength);

	m_signal = aNewSignalStrength;
	emit signalStrengthChanged(m_signal);

  DP0("S60RadioTunerControl::MssoSignalStrengthChanged ---");
}

void S60RadioTunerControl::MTapoInitializeComplete(TInt aError)
{
    DP0("S60RadioTunerControl::MTapoInitializeComplete +++");
    DP1("S60RadioTunerControl::MTapoInitializeComplete, aError:", aError);
	if (aError == KErrNone) {
		m_audioInitializationComplete = true;
		m_available = true;
		m_audioPlayerUtility->Play();
		m_apiTunerState = QRadioTuner::ActiveState;
		emit stateChanged(m_apiTunerState);
	} else if (aError != KErrNone) {
		m_radioError = QRadioTuner::OpenError;
	}

  DP0("S60RadioTunerControl::MTapoInitializeComplete ---");
}

void S60RadioTunerControl::MTapoPlayEvent(TEventType aEvent, TInt aError, TAny* aAdditionalInfo)
{
    DP0("S60RadioTunerControl::MTapoPlayEvent +++");

    DP0("S60RadioTunerControl::MTapoPlayEvent ---");

	// no actions
}



