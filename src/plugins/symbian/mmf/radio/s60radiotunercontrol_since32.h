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

#ifndef S60RADIOTUNERCONTROL_H
#define S60RADIOTUNERCONTROL_H

#include <QtCore/qobject.h>
#include <QtCore/qtimer.h>
#include <qradiotunercontrol.h>
#include <qradiotuner.h>

#include <RadioUtility.h>
#include <RadioFmTunerUtility.h>
#include <RadioPlayerUtility.h>

class S60RadioTunerService;
class CFMRadioEngineCallObserver;

QT_USE_NAMESPACE

class S60RadioTunerControl 
    : public QRadioTunerControl
    , public MRadioPlayerObserver
    , public MRadioFmTunerObserver
{
    Q_OBJECT
public:
    S60RadioTunerControl(QObject *parent = 0);
    ~S60RadioTunerControl();
    
    QRadioTuner::State state() const;

    QRadioTuner::Band band() const;
    void setBand(QRadioTuner::Band b);
    bool isBandSupported(QRadioTuner::Band b) const;

    int frequency() const;
    int frequencyStep(QRadioTuner::Band b) const;
    QPair<int,int> frequencyRange(QRadioTuner::Band b) const;
    void setFrequency(int frequency);

    bool isStereo() const;
    QRadioTuner::StereoMode stereoMode() const;
    void setStereoMode(QRadioTuner::StereoMode mode);

    int signalStrength() const;
   
    int volume() const;
    void setVolume(int volume);

    bool isMuted() const;
    void setMuted(bool muted);

    bool isSearching() const;
    void searchForward();
    void searchBackward();
    void cancelSearch();

    bool isValid() const;

    bool isAvailable() const;
    QtMultimediaKit::AvailabilityError availabilityError() const;
    
    void start();
    void stop();

    QRadioTuner::Error error() const;
    QString errorString() const;
    
    /**
	 * From MRadioPlayerObserver.
	 * Called when Radio state changed.
	 *
	 * @since S60 3.2
	 * @param aState Radio player state
	 * @param aError A standard system error code, only used when aState is ERadioPlayerIdle
	 */
	void MrpoStateChange(TPlayerState aState, TInt aError);

	/**
	 * From MRadioPlayerObserver.
	 * Called when volume changes. This may be caused by other applications.
	 *
	 * @since S60 3.2
	 * @param aVolume Current volume.
	 */
	void MrpoVolumeChange(TInt aVolume);

	/**
	 * From MRadioPlayerObserver.
	 * Called when mute setting changes. This may be caused by other applications.
	 *
	 * @since S60 3.2
	 * @param aMute ETrue indicates audio is muted.
	 */
	void MrpoMuteChange(TBool aMute);

	/**
	 * From MRadioPlayerObserver.
	 * Called when mute setting changes. This may be caused by other applications.
	 *
	 * Called when balance setting changes. This may be caused by other applications.
	 *
	 * @since S60 3.2
	 *        Left speaker volume percentage. This can be any value from zero to 100.
	 *        Zero value means left speaker is muted.
	 * @param aRightPercentage
	 *        Right speaker volume percentage. This can be any value from zero to 100.
	 *        Zero value means right speaker is muted.
	 */
	void MrpoBalanceChange(TInt aLeftPercentage, TInt aRightPercentage);

	
	/**
	 * From MRadioFmTunerObserver.
	 * Called when Request for tuner control completes.
	 *
	 * @since S60 3.2
	 * @param aError A standard system error code or FM tuner error (TFmRadioTunerError).
	 */
	void MrftoRequestTunerControlComplete(TInt aError);

	/**
	 * From MRadioFmTunerObserver.
	 * Set frequency range complete event. This event is asynchronous and is received after
	 * a call to CRadioFmTunerUtility::SetFrequencyRange.
	 *
	 * @since S60 3.2
	 * @param aError A standard system error code or FM tuner error (TFmRadioTunerError).
	 */
	void MrftoSetFrequencyRangeComplete(TInt aError);

	/**
	 * From MRadioFmTunerObserver.
	 * Set frequency complete event. This event is asynchronous and is received after a call to
	 * CRadioFmTunerUtility::SetFrequency.
	 *
	 * @since S60 3.2
	 * @param aError A standard system error code or FM tuner error (TFmRadioTunerError).
	 */
	void MrftoSetFrequencyComplete(TInt aError);

	/**
	 * From MRadioFmTunerObserver.
	 * Station seek complete event. This event is asynchronous and is received after a call to
	 * CRadioFmTunerUtility::StationSeek.
	 *
	 * @since S60 3.2
	 * @param aError A standard system error code or FM tuner error (TFmRadioTunerError).
	 * @param aFrequency The frequency(Hz) of the radio station that was found.
	 */
	void MrftoStationSeekComplete(TInt aError, TInt aFrequency);

	/**
	 * From MRadioFmTunerObserver.
	 * Called when FM Transmitter status changes (if one is present in the device). Tuner receiver
	 * is forced to be turned off due to hardware conflicts when FM transmitter is activated.
	 *
	 * @since S60 3.2
	 * @param aActive ETrue if FM transmitter is active; EFalse otherwise.
	 */
	void MrftoFmTransmitterStatusChange(TBool aActive);

	/**
	 * From MRadioFmTunerObserver.
	 * Called when antenna status changes.
	 *
	 * @since S60 3.2
	 * @param aAttached ETrue if antenna is attached; EFalse otherwise.
	 */
	void MrftoAntennaStatusChange(TBool aAttached);

	/**
	 * From MRadioFmTunerObserver.
	 * Called when offline mode status changes.
	 * @since S60 3.2
	 *
	 ** @param aAttached ETrue if offline mode is enabled; EFalse otherwise.
	 */
	void MrftoOfflineModeStatusChange(TBool aOfflineMode);

	/**
	 * From MRadioFmTunerObserver.
	 * Called when the frequency range changes. This may be caused by other applications.
	 *
	 * @since S60 3.2
	 * @param aNewRange New frequency range.
	 */
	void MrftoFrequencyRangeChange(TFmRadioFrequencyRange aBand /*, TInt aMinFreq, TInt aMaxFreq*/);

	/**
	 * From MRadioFmTunerObserver.
	 * Called when the tuned frequency changes. This may be caused by other
	 * applications or RDS if AF/TA is enabled.
	 *
	 * @since S60 3.2
	 * @param aNewFrequency The new tuned frequency(Hz).
	 */
	void MrftoFrequencyChange(TInt aNewFrequency);

	/**
	 * From MRadioFmTunerObserver.
	 * Called when the forced mono status change. This may be caused by other applications.
	 *
	 * @since S60 3.2
	 * @param aForcedMono ETrue if forced mono mode is enabled; EFalse otherwise.
	 */
	void MrftoForcedMonoChange(TBool aForcedMono);

	/**
	 * From MRadioFmTunerObserver.
	 * Called when the squelch (muting the frequencies without broadcast) status change.
	 * This may be caused by other applications.
	 *
	 * @since S60 3.2
	 * @param aSquelch ETrue if squelch is enabled; EFalse otherwise.
	 */
	void MrftoSquelchChange(TBool aSquelch);

private:
    bool initRadio();

    mutable int m_error;

    CRadioUtility* m_radioUtility;   
    CRadioFmTunerUtility* m_fmTunerUtility;
    CRadioPlayerUtility* m_playerUtility;
    TInt m_maxVolume;
    TReal m_volMultiplier;

	bool m_tunerControl;
    bool m_audioInitializationComplete;
    bool m_muted;
    bool m_isStereo;
    bool m_available;
    int  m_vol;
    bool m_volChangeRequired;
    mutable int m_signal;
    int m_previousSignal;
    bool m_scanning;
    QRadioTuner::Band m_currentBand;
    qint64 m_currentFreq;
    
    QRadioTuner::Error m_radioError;
    QRadioTuner::StereoMode m_stereoMode;
    QString m_errorString;
    QRadioTuner::State m_apiTunerState;
    QTimer *m_signalStrengthTimer;
    
Q_SIGNALS:
     void error(QRadioTuner::Error) const;
     
protected slots:
    void changeSignalStrength();
};

#endif

