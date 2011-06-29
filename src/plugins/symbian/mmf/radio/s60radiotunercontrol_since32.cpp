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

#include "DebugMacros.h"

#include "s60radiotunercontrol_since32.h"
#include "s60radiotunerservice.h"

#include <QtCore/qdebug.h>
#include <RadioFmTunerUtility.h>

S60RadioTunerControl::S60RadioTunerControl(QObject *parent)
    : QRadioTunerControl(parent)
    , m_error(0)
    , m_radioUtility(NULL)
    , m_fmTunerUtility(NULL)
    , m_playerUtility(NULL)
    , m_maxVolume(100)
    , m_audioInitializationComplete(false)
    , m_muted(false)
    , m_isStereo(true)
    , m_vol(50)
    , m_signal(0)
    , m_scanning(false)
    , m_currentBand(QRadioTuner::FM)
    , m_currentFreq(87500000)
    , m_radioError(QRadioTuner::NoError)
    , m_stereoMode(QRadioTuner::Auto)
    , m_apiTunerState(QRadioTuner::StoppedState)
    , m_previousSignal(0)
    , m_volChangeRequired(false)
    , m_signalStrengthTimer(new QTimer(this))
{
    DP0("S60RadioTunerControl::S60RadioTunerControl +++");
    bool retValue = initRadio();
    if (!retValue) {
        m_errorString = QString(tr("Initialize Error."));
        emit error(QRadioTuner::ResourceError);
    } else {
        connect(m_signalStrengthTimer, SIGNAL(timeout()), this, SLOT(changeSignalStrength()));
    }
    DP0("S60RadioTunerControl::S60RadioTunerControl ---");
}

S60RadioTunerControl::~S60RadioTunerControl()
{
    DP0("S60RadioTunerControl::~S60RadioTunerControl +++");

    if (m_fmTunerUtility) {
        m_fmTunerUtility->Close();
    }

    if (m_playerUtility) {
        m_playerUtility->Close();
    }

    delete m_radioUtility;

  DP0("S60RadioTunerControl::~S60RadioTunerControl ---");
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
    if (b == QRadioTuner::FM)
        return true;
    else if (b == QRadioTuner::LW)
        return false;
    else if (b == QRadioTuner::AM)
        return false;
    else if (b == QRadioTuner::SW)
        return false;
    else if (b == QRadioTuner::FM2)
        return false;
    else
        return false;
}

void S60RadioTunerControl::changeSignalStrength()
    {
    
    int currentSignal = signalStrength();
    if (currentSignal != m_previousSignal)
        {
        m_previousSignal = currentSignal;
        emit signalStrengthChanged(currentSignal);
        }
    }
void S60RadioTunerControl::setBand(QRadioTuner::Band b)
{
    DP0("S60RadioTunerControl::setBand +++");
    QRadioTuner::Band tempBand = b;
    if (tempBand != m_currentBand ) {
        if (isBandSupported(tempBand)){
            m_currentBand = b;  
            emit bandChanged(m_currentBand);
        }
        else {
            switch(tempBand)
                {
                case QRadioTuner::FM :
                    m_errorString = QString(tr("Band FM not Supported"));
                    break;
                case QRadioTuner::AM :
                    m_errorString = QString(tr("Band AM not Supported"));
                    break;
                case QRadioTuner::SW :
                    m_errorString = QString(tr("Band SW not Supported"));
                    break;
                case QRadioTuner::LW :
                    m_errorString = QString(tr("Band LW not Supported"));
                    break;
                case QRadioTuner::FM2 :
                    m_errorString = QString(tr("Band FM2 not Supported"));
                    break;
                default :
                    m_errorString = QString("Band %1 not Supported").arg(tempBand);
                    break;
                }
            emit error(QRadioTuner::OutOfRangeError);
        }
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
    DP1("S60RadioTunerControl::setFrequency, frequency:", frequency);

    m_currentFreq = frequency;
    m_fmTunerUtility->SetFrequency(m_currentFreq);

    DP0("S60RadioTunerControl::setFrequency ---");
}
int S60RadioTunerControl::frequencyStep(QRadioTuner::Band b) const
{
    DP0("S60RadioTunerControl::frequencyStep +++");

    int step = 0;
    if (b == QRadioTuner::FM)
        step = 100000; // 100kHz steps
    else if(b == QRadioTuner::LW)
        step = 1000; // 1kHz steps
    else if (b == QRadioTuner::AM)
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

    int bottomFreq;
    int topFreq;

    int bandError = KErrNone;
    TFmRadioFrequencyRange range;

    if (m_fmTunerUtility) {
        bandError = m_fmTunerUtility->GetFrequencyRange(range, bottomFreq, topFreq);
    }
    if (!bandError) {
        return qMakePair<int,int>(bottomFreq, topFreq);
    }

    DP0("S60RadioTunerControl::frequencyRange ---");

    return qMakePair<int,int>(0,0);
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

    if (m_fmTunerUtility) {
        if (QRadioTuner::ForceMono == mode) {
            m_fmTunerUtility->ForceMonoReception(true);
            m_stereoMode = QRadioTuner::ForceMono;
            m_isStereo = false;
        } else {
            m_fmTunerUtility->ForceMonoReception(false);
            m_isStereo = true;
            m_stereoMode = QRadioTuner::ForceStereo;
        }
    }

    DP0("S60RadioTunerControl::setStereoMode ---");
}

int S60RadioTunerControl::signalStrength() const
{
     DP0("S60RadioTunerControl::signalStrength +++");

    // return value is a percentage value
    if (m_fmTunerUtility) {
        TInt maxSignalStrength;
        TInt currentSignalStrength;
        m_error = m_fmTunerUtility->GetMaxSignalStrength(maxSignalStrength);

        if (m_error == KErrNone) {
            m_error = m_fmTunerUtility->GetSignalStrength(currentSignalStrength);
            if (m_error == KErrNone) {
                if (currentSignalStrength == 0 || maxSignalStrength == 0) {
                    return currentSignalStrength;
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
    DP1("S60RadioTunerControl::setVolume, Volume:", volume);

    int boundVolume = qBound(0, volume, 100);

    if (m_vol == boundVolume )
        return;

    if (!m_muted && m_playerUtility) {
        m_vol = boundVolume;
        // Don't set volume until State is in Active State.
        if (state() == QRadioTuner::ActiveState ) {
            m_playerUtility->SetVolume(m_vol*m_volMultiplier);

        } else {
            m_volChangeRequired = TRUE;
            emit volumeChanged(boundVolume);
        }
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
    DP1("S60RadioTunerControl::setMuted, Muted:", muted);
    if (m_playerUtility) {
        m_muted = muted;
        m_playerUtility->Mute(m_muted);
    }
  DP0("S60RadioTunerControl::setMuted ---");
}

bool S60RadioTunerControl::isSearching() const
{
    DP0("S60RadioTunerControl::isSearching");

    return m_scanning;
}

void S60RadioTunerControl::cancelSearch()
{
    DP0("S60RadioTunerControl::cancelSearch +++");

    m_fmTunerUtility->CancelStationSeek();
    m_scanning = false;
    emit searchingChanged(false);

  DP0("S60RadioTunerControl::cancelSearch ---");
}

void S60RadioTunerControl::searchForward()
{
    DP0("S60RadioTunerControl::searchForward +++");
    m_fmTunerUtility->StationSeek(true);
    m_scanning = true;
    emit searchingChanged(m_scanning);
  DP0("S60RadioTunerControl::searchForward ---");
}

void S60RadioTunerControl::searchBackward()
{
    DP0("S60RadioTunerControl::searchBackward +++");
    m_fmTunerUtility->StationSeek(false);
    m_scanning = true;
    emit searchingChanged(m_scanning);
  DP0("S60RadioTunerControl::searchBackward ---");
}

bool S60RadioTunerControl::isValid() const
{
    DP0("S60RadioTunerControl::isValid");

    return m_available;
}

bool S60RadioTunerControl::initRadio()
{
    DP0("S60RadioTunerControl::initRadio +++");
    m_available = false;
    // create an instance of Radio Utility factory and indicate
    // FM Radio is a primary client
    TRAPD(utilityError,
        m_radioUtility = CRadioUtility::NewL(ETrue);
        // Get a tuner utility
        m_fmTunerUtility = &m_radioUtility->RadioFmTunerUtilityL(*this);
        // we want to listen radio in offline mode too
        m_fmTunerUtility->EnableTunerInOfflineMode(ETrue);
        // Get a player utility
        m_playerUtility = &m_radioUtility->RadioPlayerUtilityL(*this);
    );
    if (utilityError != KErrNone) {
        m_radioError = QRadioTuner::ResourceError;
        return m_available;
    }

    m_tunerControl = false;

    m_available = true;
  DP1("S60RadioTunerControl::initRadio, m_available:", m_available);
  DP0("S60RadioTunerControl::initRadio ---");
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

void S60RadioTunerControl::start()
{
    DP0("S60RadioTunerControl::start +++");
    if (!m_tunerControl) {
        m_fmTunerUtility->RequestTunerControl();
    } else {
        m_playerUtility->Play();
    }
    m_signalStrengthTimer->start(3000);

  DP0("S60RadioTunerControl::start ---");
}

void S60RadioTunerControl::stop()
{
    DP0("S60RadioTunerControl::stop +++");
    if (m_playerUtility) {
        m_playerUtility->Stop();
    }
    m_signalStrengthTimer->stop();
   DP0("S60RadioTunerControl::stop ---");
}

QRadioTuner::Error S60RadioTunerControl::error() const
{
    DP1("S60RadioTunerControl::error", m_radioError);

    return m_radioError;
}
QString S60RadioTunerControl::errorString() const
{
    DP1("S60RadioTunerControl::errorString", m_errorString);

    return m_errorString;
}

void S60RadioTunerControl::MrpoStateChange(TPlayerState aState, TInt aError)
{
    DP0("S60RadioTunerControl::MrpoStateChange +++");
    if (aError == KErrNone){
        m_radioError = QRadioTuner::NoError;
        if (aState == ERadioPlayerIdle) {
            m_apiTunerState = QRadioTuner::StoppedState;
        } else if (aState == ERadioPlayerPlaying) {
            m_apiTunerState = QRadioTuner::ActiveState;
            //Apply pending volume changes.
            if(m_volChangeRequired){
                setVolume(m_vol);
                }
        }
    } else {
        m_apiTunerState = QRadioTuner::StoppedState;
    }
    emit stateChanged(m_apiTunerState);
  DP0("S60RadioTunerControl::MrpoStateChange ---");
}

void S60RadioTunerControl::MrpoVolumeChange(TInt aVolume)
{
    DP0("S60RadioTunerControl::MrpoVolumeChange +++");
    DP1("S60RadioTunerControl::MrpoVolumeChange, aVolume:", aVolume);
    m_vol = (aVolume/m_volMultiplier);
    if (!m_volChangeRequired) {
        emit volumeChanged(m_vol);

    } else {
        m_volChangeRequired = false;
    }
    DP0("S60RadioTunerControl::MrpoVolumeChange ---");
}

void S60RadioTunerControl::MrpoMuteChange(TBool aMute)
{
    DP0("S60RadioTunerControl::MrpoMuteChange +++");
    DP1("S60RadioTunerControl::MrpoMuteChange, aMute:", aMute);
    m_muted = aMute;
    emit mutedChanged(m_muted);
  DP0("S60RadioTunerControl::MrpoMuteChange ---");
}

void S60RadioTunerControl::MrpoBalanceChange(TInt aLeftPercentage, TInt aRightPercentage)
{
    DP0("S60RadioTunerControl::MrpoBalanceChange +++");

    DP0("S60RadioTunerControl::MrpoBalanceChange ---");

    // no actions
}

void S60RadioTunerControl::MrftoRequestTunerControlComplete(TInt aError)
{
    DP0("S60RadioTunerControl::MrftoRequestTunerControlComplete +++");
    DP1("S60RadioTunerControl::MrftoRequestTunerControlComplete, aError:", aError);
    if (aError == KErrNone) {
        m_playerUtility->GetMaxVolume(m_maxVolume);
        m_volMultiplier = float(m_maxVolume)/float(100);
        m_radioError = QRadioTuner::NoError;
        m_tunerControl = true;
        m_available = true;
        m_fmTunerUtility->SetFrequency(m_currentFreq);
        m_playerUtility->Play();
        int signal = signalStrength();
        if (m_signal != signal) {
            emit signalStrengthChanged(signal);
            m_signal = signal;
        }

    } else if (aError == KFmRadioErrAntennaNotConnected) {
        m_radioError = QRadioTuner::OpenError;
        m_errorString = QString(tr("Antenna Not Connected"));
        emit error(m_radioError);
    } else if (aError == KErrAlreadyExists){
        m_radioError = QRadioTuner::ResourceError;
        m_errorString = QString(tr("Resource Error."));
        emit error(m_radioError);
    } else if (aError == KFmRadioErrFrequencyOutOfBandRange) {
        m_radioError = QRadioTuner::OutOfRangeError;
        m_errorString = QString(tr("Frequency out of band range"));
        emit error(m_radioError);
    }else{
        m_radioError = QRadioTuner::OpenError;
        m_errorString = QString(tr("Unknown Error."));
        emit error(m_radioError);
    }

  DP0("S60RadioTunerControl::MrftoRequestTunerControlComplete ---");
}

void S60RadioTunerControl::MrftoSetFrequencyRangeComplete(TInt aError)
{
    DP0("S60RadioTunerControl::MrftoSetFrequencyRangeComplete +++");
    DP1("S60RadioTunerControl::MrftoSetFrequencyRangeComplete, aError:", aError);
    if (aError == KFmRadioErrFrequencyOutOfBandRange || KFmRadioErrFrequencyNotValid) {
        m_radioError = QRadioTuner::OutOfRangeError;
        m_errorString = QString(tr("Frequency Out of Band Range or Frequency Not Valid"));
        emit error(m_radioError);
    } else if (aError == KFmRadioErrHardwareFaulty || KFmRadioErrOfflineMode) {
        m_radioError = QRadioTuner::OpenError;
        m_errorString = QString(tr("Hardware failure or RadioInOfflineMode"));
        emit error(m_radioError);
    }
  DP0("S60RadioTunerControl::MrftoSetFrequencyRangeComplete ---");
}

void S60RadioTunerControl::MrftoSetFrequencyComplete(TInt aError)
{
    DP0("S60RadioTunerControl::MrftoSetFrequencyComplete +++");
    DP1("S60RadioTunerControl::MrftoSetFrequencyComplete, aError", aError);
    if (aError == KErrNone) {
        m_radioError = QRadioTuner::NoError;
    } else if (aError == KFmRadioErrFrequencyOutOfBandRange || KFmRadioErrFrequencyNotValid) {
        m_radioError = QRadioTuner::OutOfRangeError;
        m_errorString = QString(tr("Frequency Out of range or not Valid."));
        emit error(m_radioError);
    } else if (aError == KFmRadioErrHardwareFaulty || KFmRadioErrOfflineMode) {
        m_radioError = QRadioTuner::OpenError;
        m_errorString = QString("Hardware failure or Radio In Offline Mode");
        emit error(m_radioError);
    }
  DP0("S60RadioTunerControl::MrftoSetFrequencyComplete ---");
}

void S60RadioTunerControl::MrftoStationSeekComplete(TInt aError, TInt aFrequency)
{
    DP0("S60RadioTunerControl::MrftoStationSeekComplete +++");
    DP3("S60RadioTunerControl::MrftoStationSeekComplete, aError:", aError, " Frequency:", aFrequency);
    m_scanning = false;
    if (aError == KErrNone) {
        m_radioError = QRadioTuner::NoError;
        m_currentFreq = aFrequency;
        emit searchingChanged(m_scanning);
    } else {
        m_radioError = QRadioTuner::OpenError;
        emit searchingChanged(m_scanning);
        m_errorString = QString("Scanning Error");
        emit error(m_radioError);
    }
  DP0("S60RadioTunerControl::MrftoStationSeekComplete ---");
}

void S60RadioTunerControl::MrftoFmTransmitterStatusChange(TBool aActive)
{
    DP0("S60RadioTunerControl::MrftoFmTransmitterStatusChange +++");

    DP0("S60RadioTunerControl::MrftoFmTransmitterStatusChange ---");

    //no actions
}

void S60RadioTunerControl::MrftoAntennaStatusChange(TBool aAttached)
{
    DP0("S60RadioTunerControl::MrftoAntennaStatusChange +++");
    DP1("S60RadioTunerControl::MrftoAntennaStatusChange, aAttached:", aAttached);
    if (aAttached && m_tunerControl) {
        m_playerUtility->Play();
    }
  DP0("S60RadioTunerControl::MrftoAntennaStatusChange ---");
}

void S60RadioTunerControl::MrftoOfflineModeStatusChange(TBool /*aOfflineMode*/)
{
    DP0("S60RadioTunerControl::MrftoOfflineModeStatusChange +++");

    DP0("S60RadioTunerControl::MrftoOfflineModeStatusChange ---");


}

void S60RadioTunerControl::MrftoFrequencyRangeChange(TFmRadioFrequencyRange aBand /*, TInt aMinFreq, TInt aMaxFreq*/)
{
    DP0("S60RadioTunerControl::MrftoFrequencyRangeChange +++");
    if (aBand == EFmRangeEuroAmerica) {
        setBand(QRadioTuner::FM);
    }
  DP0("S60RadioTunerControl::MrftoFrequencyRangeChange ---");
}

void S60RadioTunerControl::MrftoFrequencyChange(TInt aNewFrequency)
{
    DP0("S60RadioTunerControl::MrftoFrequencyChange +++");
    DP1("S60RadioTunerControl::MrftoFrequencyChange, aNewFrequency:", aNewFrequency);
    m_currentFreq = aNewFrequency;
    emit frequencyChanged(m_currentFreq);

    int signal = signalStrength();
    if (m_signal != signal) {
        emit signalStrengthChanged(signal);
        m_signal = signal;
    }
  DP0("S60RadioTunerControl::MrftoFrequencyChange ---");
}

void S60RadioTunerControl::MrftoForcedMonoChange(TBool aForcedMono)
{
    DP0("S60RadioTunerControl::MrftoForcedMonoChange +++");
    DP1("S60RadioTunerControl::MrftoForcedMonoChange, aForcedMono:", aForcedMono);
    if (aForcedMono) {
        m_stereoMode = QRadioTuner::ForceMono;
    } else {
        m_stereoMode = QRadioTuner::ForceStereo;
    }
    emit stereoStatusChanged(!aForcedMono);
  DP0("S60RadioTunerControl::MrftoForcedMonoChange ---");
}

void S60RadioTunerControl::MrftoSquelchChange(TBool aSquelch)
{
    DP0("S60RadioTunerControl::MrftoSquelchChange");

    DP1("S60RadioTunerControl::MrftoSquelchChange, aSquelch:", aSquelch);

    // no actions
}
