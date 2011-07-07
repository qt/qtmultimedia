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

#ifndef XARADIOSESSIONIMPL_H
#define XARADIOSESSIONIMPL_H

#include <OpenMAXAL.h>
#include <xanokialinearvolumeitf.h>
#include <xanokiavolumeextitf.h>
#include <qradiotuner.h>
#include <qtmedianamespace.h>

QT_USE_NAMESPACE

class XARadioSessionImplObserver;

class XARadioSessionImpl
{
public:
    XARadioSessionImpl(XARadioSessionImplObserver& parent);
    ~XARadioSessionImpl();
    QRadioTuner::Error PostConstruct();
    QRadioTuner::Band Band() const;
    QRadioTuner::State State() const;
    QtMultimediaKit::AvailabilityError AvailabilityError() const;
    bool IsAvailable() const;
    void SetBand(QRadioTuner::Band band);
    bool IsBandSupported(QRadioTuner::Band band) const;
    TInt FrequencyStep(QRadioTuner::Band band) const;
    bool IsStereo(); //const;
    bool IsMuted() const;
    bool IsSearching() const;
    TInt GetFrequency();
    TInt GetFrequencyRange();
    TInt GetFrequencyRangeProperties(TInt range, TInt &minFreq, TInt &maxFreq);
    TInt SetFrequency(TInt aFreq);
    QRadioTuner::StereoMode StereoMode();
    TInt SetStereoMode(QRadioTuner::StereoMode stereoMode);
    TInt GetSignalStrength();
    TInt GetVolume();
    TInt SetVolume(TInt aVolume);
    TInt SetMuted(TBool aMuted);
    TInt Seek(TBool aDirection);
    TInt StopSeeking();
    void Start();
    void Stop();
    QRadioTuner::Error Error();
//TInt ErrorString();
    void StateChanged(QRadioTuner::State state);
    void FrequencyChanged(XAuint32 freq);
    void SearchingChanged(TBool isSearching);
    void StereoStatusChanged(TBool stereoStatus);
    void SignalStrengthChanged(TBool stereoStatus);
    void VolumeChanged();
    void MutedChanged(TBool mute);

private:
    TInt CreateEngine();
    TInt CheckErr(XAresult res);


private:
    XARadioSessionImplObserver& iParent;
    XAObjectItf iRadio;
    XAObjectItf iEngine;
    XAObjectItf iPlayer;
    XAEngineItf iEngineItf;
    XARecordItf iRecordItf;
    XAPlayItf   iPlayItf;
    XARadioItf iRadioItf;
    XARDSItf iRdsItf;
    XANokiaVolumeExtItf iNokiaVolumeExtItf; // used for mute functionality
    XANokiaLinearVolumeItf iNokiaLinearVolumeItf; // used for volume functionality

    /* Audio Source */
    XADataSource iDataSource;

    /*Audio Sink*/
    XADataSink iAudioSink;
    XADataLocator_OutputMix iLocator_outputmix;

    TBool iAutoFlag;
    TBool iSearching;
    TBool iRadioAvailable;
    QtMultimediaKit::AvailabilityError iAvailabilityError;
    QRadioTuner::Band iBand;
    QRadioTuner::State iState;
};

#endif /* XARADIOSESSIONIMPL_H */
