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

#include "xaradiosessionimpl.h"
#include "xaradiosessionimplobserver.h"
#include <xaradioitfext.h>
#include "xacommon.h"

#define MAX_NUMBER_INTERFACES 20
#define FM_STEP 100000; // Hz (.1 MHz)

/*
 * function declarations.
 * */
void EngineObjectCallback(XAObjectItf caller, const void */*pContext*/,
                        XAuint32 event, XAresult result, XAuint32 /*param*/,
                        void */*pInterface*/);

void RadioCallback(XARadioItf caller, void* pContext, XAuint32 event, XAuint32 eventIntData, XAboolean eventBooleanData);
void NokiaVolumeExtItfCallback(XANokiaVolumeExtItf caller, void* pContext, XAuint32 event, XAboolean eventBooleanData);
void NokiaLinearVolumeItfCallback(XANokiaLinearVolumeItf caller, void* pContext, XAuint32 event, XAboolean eventBooleanData);
void PlayItfCallbackForRadio(XAPlayItf caller, void* pContext, XAuint32 event);

XARadioSessionImpl::XARadioSessionImpl(XARadioSessionImplObserver& parent)
:iParent(parent),
iRadio(NULL),
iEngine(NULL),
iPlayer(NULL),
iSearching(EFalse),
iRadioAvailable(EFalse),
iState(QRadioTuner::StoppedState)
{
    iAvailabilityError = QtMultimediaKit::NoError;
}

XARadioSessionImpl::~XARadioSessionImpl()
{
    if (iRadio) {
        TRACE_LOG((_L("XARadioSessionImpl::~XARadioSessionImpl(): Deleting Radio Device...")));
        (*iRadio)->Destroy(iRadio);
        iRadio = NULL;
        TRACE_LOG((_L("XARadioSessionImpl::~XARadioSessionImpl(): Deleted Radio Device")));
    }
    if (iPlayer) {
        TRACE_LOG((_L("XARadioSessionImpl::~XARadioSessionImpl(): Deleting player...")));
        (*iPlayer)->Destroy(iPlayer);
        iPlayer = NULL;
        TRACE_LOG((_L("XARadioSessionImpl::~XARadioSessionImpl(): Deleted iPlayer")));
    }
    if ( iEngine ) {
        TRACE_LOG((_L("XARadioSessionImpl::~XARadioSessionImpl(): Deleting engine...")));
        (*iEngine)->Destroy(iEngine);
        iEngine = NULL;
        TRACE_LOG((_L("XARadioSessionImpl::~XARadioSessionImpl(): Deleted engine")));
    }
}

QRadioTuner::Error XARadioSessionImpl::PostConstruct()
{
    XAresult res = CreateEngine();
    if (res != KErrNone)
        return QRadioTuner::ResourceError;
    else
        return QRadioTuner::NoError;
}

TInt XARadioSessionImpl::CreateEngine()
{
    TRACE_FUNCTION_ENTRY;
    XAboolean required[MAX_NUMBER_INTERFACES];
    XAInterfaceID iidArray[MAX_NUMBER_INTERFACES];
    XAuint32 noOfInterfaces = 0;
    int i;
    XAresult res;

    XAEngineOption EngineOption[] =
    {
        {
        (XAuint32) XA_ENGINEOPTION_THREADSAFE,
        (XAuint32) XA_BOOLEAN_TRUE
        }
    };

    /* Create XA engine */
    if (!iEngine) {
        TRACE_LOG((_L("XARadioSessionImpl::CreateEngine: Creating Engine...")));
        res = xaCreateEngine(&iEngine, 1, EngineOption, 0, NULL, NULL);
        RET_ERR_IF_ERR(CheckErr(res));
        res = (*iEngine)->RegisterCallback(iEngine, EngineObjectCallback, NULL);
        RET_ERR_IF_ERR(CheckErr(res));

        TRACE_LOG((_L("XARadioSessionImpl::CreateEngine: Realizing...")));
        res = (*iEngine)->Realize(iEngine, XA_BOOLEAN_FALSE);
        RET_ERR_IF_ERR(CheckErr(res));

        // Create Engine Interface:
        TRACE_LOG((_L("XARadioSessionImpl::CreateEngine: Creating Engine Interface")));
        RET_ERR_IF_ERR(CheckErr((*iEngine)->GetInterface(iEngine, XA_IID_ENGINE, (void*)&iEngineItf)));

        // Create Radio Device and interface(s):
        if (!iRadio) {
            TRACE_LOG((_L("XARadioSessionImpl::CreateEngine: Creating Radio Device")));
            res = (*iEngineItf)->CreateRadioDevice(iEngineItf,&iRadio, 0, NULL, NULL);
            RET_ERR_IF_ERR(CheckErr(res));

            TRACE_LOG((_L("XARadioSessionImpl::CreateEngine: Realize Radio Device")));
            res = (*iRadio)->Realize(iRadio, XA_BOOLEAN_FALSE);
            RET_ERR_IF_ERR(CheckErr(res));

            // Get Radio interface:
            TRACE_LOG((_L("XARadioSessionImpl::CreateEngine: Get Radio Interface")));
            res = (*iRadio)->GetInterface(iRadio, XA_IID_RADIO, (void*)&iRadioItf);
            RET_ERR_IF_ERR(CheckErr(res));
            iRadioAvailable = ETrue;
            // Register Radio Callback:
            TRACE_LOG((_L("XARadioSessionImpl::CreateEngine: Create Radio Callback:")));
            res = (*iRadioItf)->RegisterRadioCallback(iRadioItf, RadioCallback, (void*)this);
            RET_ERR_IF_ERR(CheckErr(res));
        }
        XADataSource            audioSource;
        XADataLocator_IODevice  locatorIODevice;
        XADataSink              audioSink;
        XADataLocator_OutputMix locator_outputmix;

        /* Init arrays required[] and iidArray[] */
        for (i = 0; i < MAX_NUMBER_INTERFACES; i++) {
            required[i] = XA_BOOLEAN_FALSE;
            iidArray[i] = XA_IID_NULL;
        }

        iidArray[0] = XA_IID_NOKIAVOLUMEEXT;
        iidArray[1] = XA_IID_NOKIALINEARVOLUME;
        noOfInterfaces = 2;

        locatorIODevice.locatorType = XA_DATALOCATOR_IODEVICE;
        locatorIODevice.deviceType  = XA_IODEVICE_RADIO;
        locatorIODevice.deviceID    = 0; /* ignored */
        locatorIODevice.device      = iRadio;
        audioSource.pLocator        = (void*) &locatorIODevice;
        audioSource.pFormat         = NULL;

        /* Setup the data sink structure */
        locator_outputmix.locatorType = XA_DEFAULTDEVICEID_AUDIOOUTPUT;
        locator_outputmix.outputMix   = NULL;
        audioSink.pLocator            = (void*) &locator_outputmix;
        audioSink.pFormat             = NULL;

        TRACE_LOG((_L("XARadioSessionImpl::CreateEngine: Create Media Player:")));
        res = (*iEngineItf)->CreateMediaPlayer(iEngineItf, &iPlayer, &audioSource, NULL, &audioSink, NULL, NULL, NULL, noOfInterfaces, iidArray, required);
        RET_ERR_IF_ERR(CheckErr(res));

        TRACE_LOG((_L("XARadioSessionImpl::CreateEngine: Realize Media Player:")));
        res = (*iPlayer)->Realize(iPlayer, XA_BOOLEAN_FALSE);
        RET_ERR_IF_ERR(CheckErr(res));
        TRACE_LOG((_L("XARadioSessionImpl::CreateEngine: Get Play Interface from player:")));
        res = (*iPlayer)->GetInterface(iPlayer, XA_IID_PLAY, (void*) &iPlayItf);
        RET_ERR_IF_ERR(CheckErr(res));
        TRACE_LOG((_L("XARadioSessionImpl::CreateEngine: Create PlayItf Callback:")));
        res = (*iPlayItf)->RegisterCallback(iPlayItf, PlayItfCallbackForRadio, (void*)this);
        RET_ERR_IF_ERR(CheckErr(res));

        // Get Volume Interfaces specific for Nokia impl:
        TRACE_LOG((_L("XARadioSessionImpl::CreateEngine: Get NokiaVolumeExt Interface")));
        res = (*iPlayer)->GetInterface(iPlayer, XA_IID_NOKIAVOLUMEEXT, (void*)&iNokiaVolumeExtItf);
        RET_ERR_IF_ERR(CheckErr(res));

        TRACE_LOG((_L("XARadioSessionImpl::CreateEngine: Get NokiaLinearVolume Interface")));
        res = (*iPlayer)->GetInterface(iPlayer, XA_IID_NOKIALINEARVOLUME, (void*)&iNokiaLinearVolumeItf);
        RET_ERR_IF_ERR(CheckErr(res));

        // Register Volume Callbacks:
        TRACE_LOG((_L("XARadioSessionImpl::CreateEngine: Create NokiaVolumeExtItf Callback:")));
        res = (*iNokiaVolumeExtItf)->RegisterVolumeCallback(iNokiaVolumeExtItf, NokiaVolumeExtItfCallback, (void*)this);
        RET_ERR_IF_ERR(CheckErr(res));
        res = (*iNokiaVolumeExtItf)->SetCallbackEventsMask(iNokiaVolumeExtItf,(XA_NOKIAVOLUMEEXT_EVENT_MUTE_CHANGED));
        RET_ERR_IF_ERR(CheckErr(res));
        TRACE_LOG((_L("XARadioSessionImpl::CreateEngine: Create NokiaLinearVolumeItf Callback:")));
        res = (*iNokiaLinearVolumeItf)->RegisterVolumeCallback(iNokiaLinearVolumeItf, NokiaLinearVolumeItfCallback, (void*)this);
        RET_ERR_IF_ERR(CheckErr(res));
        res = (*iNokiaLinearVolumeItf)->SetCallbackEventsMask(iNokiaLinearVolumeItf,(XA_NOKIALINEARVOLUME_EVENT_VOLUME_CHANGED));
        RET_ERR_IF_ERR(CheckErr(res));
    }

    TRACE_FUNCTION_EXIT;
    return EFalse;
}

QRadioTuner::State XARadioSessionImpl::State() const
{
    TRACE_FUNCTION_ENTRY_EXIT;
    return iState;
}

QtMultimediaKit::AvailabilityError XARadioSessionImpl::AvailabilityError() const
{
    TRACE_FUNCTION_ENTRY_EXIT;
    return iAvailabilityError;
}

 bool XARadioSessionImpl::IsAvailable() const
{
    TRACE_FUNCTION_ENTRY_EXIT;
    return iRadioAvailable;
}

QRadioTuner::Band XARadioSessionImpl::Band() const
{
    TRACE_FUNCTION_ENTRY_EXIT;
    return iBand;
}

void XARadioSessionImpl::SetBand(QRadioTuner::Band band)
{
    if (band != QRadioTuner::FM)
        iParent.CBError(QRadioTuner::OpenError);
    else
        iBand = band;
}

bool XARadioSessionImpl::IsBandSupported(QRadioTuner::Band band) const
{
    if (band == QRadioTuner::FM)
        return ETrue;
    else
        return EFalse;
}

// Returns the number of Hertz to increment the frequency by when stepping through frequencies within a given band.
TInt XARadioSessionImpl::FrequencyStep(QRadioTuner::Band /*band*/) const
{
    TInt freqStep = FM_STEP;
    return (int)freqStep;
}

bool XARadioSessionImpl::IsStereo() //const
{
    bool isStereo = EFalse;
    QRadioTuner::StereoMode mode = StereoMode();
    if (mode == QRadioTuner::ForceStereo || mode == QRadioTuner::Auto)
        isStereo = ETrue;
    return isStereo;
}

bool XARadioSessionImpl::IsMuted() const
{
    TRACE_FUNCTION_ENTRY;
    XAboolean isMuted = EFalse;
    (*iNokiaVolumeExtItf)->GetMute(iNokiaVolumeExtItf, &isMuted );
    TRACE_LOG((_L("XARadioSessionImpl::IsMuted: isMuted = %d"), isMuted));

    TRACE_FUNCTION_EXIT;
    return isMuted;
}

bool XARadioSessionImpl::IsSearching() const
{
    //iSearching is set when seek (QT:searchForward-backward)
    // iSearching is cleared when SearchingStatusChanged is called or StopSeeking is called
    return iSearching;
}

TInt XARadioSessionImpl::GetFrequency()
{
    TRACE_FUNCTION_ENTRY;

    XAuint32 freq = 0;
    XAresult res = (*iRadioItf)->GetFrequency(iRadioItf, &freq );
    RET_ERR_IF_ERR(CheckErr(res));
    TRACE_LOG((_L("XARadioSessionImpl::GetFrequency: Frequency = %d"), freq));

    TRACE_FUNCTION_EXIT;
    return (int)freq;
}

TInt XARadioSessionImpl::GetFrequencyRange()
{
    TRACE_FUNCTION_ENTRY;
    XAuint8 range = 0;

    XAresult res = (*iRadioItf)->GetFreqRange(iRadioItf, &range);
    RET_ERR_IF_ERR(CheckErr(res));
    TRACE_LOG((_L("XARadioSessionImpl::GetFrequencyRange: Frequency Range = %d"), range));

    TRACE_FUNCTION_EXIT;
    return (int)range;
}

TInt XARadioSessionImpl::GetFrequencyRangeProperties(TInt range, TInt &minFreq, TInt &maxFreq)
{
    TRACE_FUNCTION_ENTRY;
    XAuint32 freqInterval = 0;
    XAresult res = (*iRadioItf)->GetFreqRangeProperties(iRadioItf, (XAuint8)range, (XAuint32*)&minFreq,(XAuint32*)&maxFreq, (XAuint32*)&freqInterval);
    RET_ERR_IF_ERR(CheckErr(res));
    TRACE_LOG((_L("XARadioSessionImpl::GetFrequencyRangeProperties: minFreq = %d, maxFreq = %d"), minFreq, maxFreq));

    TRACE_FUNCTION_EXIT;
    return res;
}

TInt XARadioSessionImpl::SetFrequency(TInt aFreq)
{
    TRACE_FUNCTION_ENTRY;

    TRACE_LOG((_L("XARadioSessionImpl::SetFrequency: Setting Frequency to: %d"), aFreq));
    XAresult res = (*iRadioItf)->SetFrequency(iRadioItf, aFreq );
    RET_ERR_IF_ERR(CheckErr(res));

    TRACE_FUNCTION_EXIT;
    return res;
}

QRadioTuner::StereoMode XARadioSessionImpl::StereoMode()
{
    TRACE_FUNCTION_ENTRY;
    QRadioTuner::StereoMode qtStereoMode;
    XAuint32 symStereoMode;
    (*iRadioItf)->GetStereoMode(iRadioItf, &symStereoMode);

    if (symStereoMode == XA_STEREOMODE_MONO)
        qtStereoMode = QRadioTuner::ForceMono;
    else if (symStereoMode == XA_STEREOMODE_STEREO)
        qtStereoMode = QRadioTuner::ForceStereo;
    else
        qtStereoMode = QRadioTuner::Auto;

    TRACE_FUNCTION_EXIT;
    return qtStereoMode;
}

TInt XARadioSessionImpl::SetStereoMode(QRadioTuner::StereoMode qtStereoMode)
{
    TRACE_FUNCTION_ENTRY;
    XAuint32 symStereoMode;

    if (qtStereoMode == QRadioTuner::ForceMono)
        symStereoMode = XA_STEREOMODE_MONO;
    else if (qtStereoMode == QRadioTuner::ForceStereo)
        symStereoMode = XA_STEREOMODE_STEREO;
    else
        symStereoMode = XA_STEREOMODE_AUTO;

    XAresult res = (*iRadioItf)->SetStereoMode(iRadioItf, (symStereoMode));
    TRACE_FUNCTION_EXIT;
    return res;
}

TInt XARadioSessionImpl::GetSignalStrength()
{
    TRACE_FUNCTION_ENTRY;
    XAuint32 signalStrength = 0;

    (*iRadioItf)->GetSignalStrength(iRadioItf, &signalStrength );
    TRACE_LOG((_L("XARadioSessionImpl::GetSignalStrength: Signal Strength = %d"), signalStrength));
    TRACE_FUNCTION_EXIT;
    return (int)signalStrength;
}

TInt XARadioSessionImpl::GetVolume()
{
    TRACE_FUNCTION_ENTRY;
    XAuint32 vol;
    if (iPlayer && iNokiaLinearVolumeItf) {
        (*iNokiaLinearVolumeItf)->GetVolumeLevel(iNokiaLinearVolumeItf, &vol );
        TRACE_LOG((_L("XARadioSessionImpl::GetVolume: Volume = %d"), vol));
    }
    TRACE_FUNCTION_EXIT;
    return (TInt)vol;
}

TInt XARadioSessionImpl::SetVolume(TInt aVolume)
{
    TRACE_FUNCTION_ENTRY;
    XAuint32 newVolume = 0;
    TRACE_LOG((_L("XARadioSessionImpl::SetVolume: Setting volume to: %d"), aVolume));
    if (iPlayer && iNokiaLinearVolumeItf) {
        newVolume = aVolume;
        XAresult res = (*iNokiaLinearVolumeItf)->SetVolumeLevel(iNokiaLinearVolumeItf, &newVolume);
    }
    TRACE_FUNCTION_EXIT;
    return (TInt)newVolume;
}

TInt XARadioSessionImpl::SetMuted(TBool aMuted)
{
    TRACE_FUNCTION_ENTRY;
    XAresult res = (*iNokiaVolumeExtItf)->SetMute(iNokiaVolumeExtItf, aMuted);
    TRACE_FUNCTION_EXIT;
    return res;
}

TInt XARadioSessionImpl::Seek(TBool aDirection)
{
    TRACE_FUNCTION_ENTRY;
    iSearching = true;
    XAresult res = (*iRadioItf)->Seek(iRadioItf, aDirection );
    TRACE_FUNCTION_EXIT;
    return res;
}

TInt XARadioSessionImpl::StopSeeking()
{
    TRACE_FUNCTION_ENTRY;
    XAresult res = (*iRadioItf)->StopSeeking(iRadioItf);
    iSearching = EFalse;
    TRACE_FUNCTION_EXIT;
    return res;
}

void XARadioSessionImpl::Start()
{
    TRACE_FUNCTION_ENTRY;
    if (iPlayItf) {
        XAresult res = (*iPlayItf)->SetPlayState(iPlayItf, XA_PLAYSTATE_PLAYING);
        // add error handling if res != 0 (call errorCB)
    }
    TRACE_FUNCTION_EXIT;
}

void XARadioSessionImpl::Stop()
{
    TRACE_FUNCTION_ENTRY;
    if (iPlayItf) {
        XAresult res = (*iPlayItf)->SetPlayState(iPlayItf, XA_PLAYSTATE_STOPPED);
        // add error handling if res != 0 (call errorCB)
    }
    TRACE_FUNCTION_EXIT;
}

QRadioTuner::Error XARadioSessionImpl::Error()
{
    TRACE_FUNCTION_ENTRY_EXIT;
    return QRadioTuner::NoError;
}

//TInt XARadioSessionImpl::ErrorString();
//    {
//    TRACE_FUNCTION_ENTRY;

//    TRACE_FUNCTION_EXIT;
//    }

void XARadioSessionImpl::StateChanged(QRadioTuner::State state)
{
    TRACE_FUNCTION_ENTRY;
    iState = state;
    iParent.CBStateChanged(state);
    TRACE_FUNCTION_EXIT;
}

void XARadioSessionImpl::FrequencyChanged(XAuint32 freq)
{
    TRACE_FUNCTION_ENTRY;
    iParent.CBFrequencyChanged(freq);
    TRACE_FUNCTION_EXIT;
}

void XARadioSessionImpl::SearchingChanged(TBool isSearching)
{
    TRACE_FUNCTION_ENTRY;
    iSearching = EFalse;
    iParent.CBSearchingChanged(isSearching);
    TRACE_FUNCTION_EXIT;
}

void XARadioSessionImpl::StereoStatusChanged(TBool stereoStatus)
{
    TRACE_FUNCTION_ENTRY;
    iParent.CBStereoStatusChanged(stereoStatus);
    TRACE_FUNCTION_EXIT;
}

void XARadioSessionImpl::SignalStrengthChanged(TBool stereoStatus)
{
    TRACE_FUNCTION_ENTRY;
    iParent.CBSignalStrengthChanged(stereoStatus);
    TRACE_FUNCTION_EXIT;
}

void XARadioSessionImpl::VolumeChanged()
{
    TRACE_FUNCTION_ENTRY;
    int vol = 0;
    iParent.CBVolumeChanged(vol);
    TRACE_FUNCTION_EXIT;
}

void XARadioSessionImpl::MutedChanged(TBool mute)
{
    TRACE_FUNCTION_ENTRY;
    iParent.CBMutedChanged(mute);
    TRACE_FUNCTION_EXIT;
}

void EngineObjectCallback(XAObjectItf /*caller*/,
                          const void */*pContext*/,
#ifdef PLUGIN_SYMBIAN_TRACE_ENABLED
                          XAuint32 event,
#else
                          XAuint32 /*event*/,
#endif /*PLUGIN_SYMBIAN_TRACE_ENABLED*/
                          XAresult /*result*/,
                          XAuint32 /*param*/,
                          void */*pInterface*/)
{
#ifdef PLUGIN_SYMBIAN_TRACE_ENABLED
    TRACE_LOG((_L("Engine object event: 0x%x\n"), (int)event));
#endif /*PLUGIN_SYMBIAN_TRACE_ENABLED*/
}

void RadioCallback(XARadioItf /*caller*/,
                   void* pContext,
                   XAuint32 event,
                   XAuint32 eventIntData,
                   XAboolean eventBooleanData)
{
    XAuint32 freq;
    XAboolean stereoStatus(XA_BOOLEAN_FALSE);

    switch (event) {
    case XA_RADIO_EVENT_ANTENNA_STATUS_CHANGED:
        TRACE_LOG((_L("RadioCallback: XA_RADIO_EVENT_ANTENNA_STATUS_CHANGED")));
        // Qt API has no callback defined for this event.
        break;
    case XA_RADIO_EVENT_FREQUENCY_CHANGED:
        freq = eventIntData;
        TRACE_LOG((_L("RadioCallback: XA_RADIO_EVENT_FREQUENCY_CHANGED to: %d"), freq));
        if (pContext)
            ((XARadioSessionImpl*)pContext)->FrequencyChanged(freq);
        break;
    case XA_RADIO_EVENT_FREQUENCY_RANGE_CHANGED:
        TRACE_LOG((_L("RadioCallback: XA_RADIO_EVENT_FREQUENCY_RANGE_CHANGED")));
        // Qt API has no callback defined for this event.
        break;
    case XA_RADIO_EVENT_PRESET_CHANGED:
        TRACE_LOG((_L("RadioCallback: XA_RADIO_EVENT_PRESET_CHANGED")));
        // Qt API has no callback defined for this event.
        break;
    case XA_RADIO_EVENT_SEEK_COMPLETED:
        TRACE_LOG((_L("RadioCallback: XA_RADIO_EVENT_SEEK_COMPLETED")));
        if (pContext)
            ((XARadioSessionImpl*)pContext)->SearchingChanged(false);
        break;
     case XA_RADIO_EVENT_STEREO_STATUS_CHANGED:
        stereoStatus = eventBooleanData;
        TRACE_LOG((_L("RadioCallback: XA_RADIO_EVENT_STEREO_STATUS_CHANGED: %d"), stereoStatus));
        if (pContext)
            ((XARadioSessionImpl*)pContext)->StereoStatusChanged(stereoStatus);
        break;
    case XA_RADIO_EVENT_SIGNAL_STRENGTH_CHANGED:
        TRACE_LOG((_L("RadioCallback: XA_RADIO_EVENT_SIGNAL_STRENGTH_CHANGED")));
        if (pContext)
            ((XARadioSessionImpl*)pContext)->SignalStrengthChanged(stereoStatus);
        break;
    default:
        TRACE_LOG((_L("RadioCallback: default")));
        break;
    }
}

void NokiaVolumeExtItfCallback(XANokiaVolumeExtItf /*caller*/,
                               void* pContext,
                               XAuint32 event,
                               XAboolean eventBooleanData)
{
    XAboolean mute;
    switch (event) {
    case XA_NOKIAVOLUMEEXT_EVENT_MUTE_CHANGED:
        mute = eventBooleanData;
        TRACE_LOG((_L("NokiaVolumeExtItfCallback: XA_NOKIAVOLUMEEXT_EVENT_MUTE_CHANGED to: %d"), mute));
        if (pContext)
            ((XARadioSessionImpl*)pContext)->MutedChanged(mute);
        break;
    default:
        TRACE_LOG((_L("NokiaVolumeExtItfCallback: default")));
        break;
    }
}

void NokiaLinearVolumeItfCallback(XANokiaLinearVolumeItf /*caller*/,
                                  void* pContext,
                                  XAuint32 event,
                                  XAboolean /*eventBooleanData*/)
{
    switch (event) {
    case XA_NOKIALINEARVOLUME_EVENT_VOLUME_CHANGED:
        if (pContext)
            ((XARadioSessionImpl*)pContext)->VolumeChanged();
        break;
    default:
        TRACE_LOG((_L("NokiaLinearVolumeItfCallback: default")));
        break;
    }
}

void PlayItfCallbackForRadio(XAPlayItf /*caller*/,
                             void* pContext,
                             XAuint32 event)
{
    switch (event) {
    case XA_PLAYEVENT_HEADMOVING:
        if (pContext)
            ((XARadioSessionImpl*)pContext)->StateChanged(QRadioTuner::ActiveState);
        break;
    case XA_PLAYEVENT_HEADSTALLED:
        if (pContext)
            ((XARadioSessionImpl*)pContext)->StateChanged(QRadioTuner::StoppedState);
        break;
    default:
        TRACE_LOG((_L("NokiaLinearVolumeItfCallback: default")));
        break;
    }
}

TInt XARadioSessionImpl::CheckErr(XAresult res)
{
    TInt status(KErrGeneral);
    switch(res) {
    case XA_RESULT_SUCCESS:
        //TRACE_LOG((_L("XA_RESULT_SUCCESS")));
        status = KErrNone;
        break;
    case XA_RESULT_PRECONDITIONS_VIOLATED:
        TRACE_LOG((_L("XA_RESULT_PRECONDITIONS_VIOLATED")));
        break;
    case XA_RESULT_PARAMETER_INVALID:
        TRACE_LOG((_L("XA_RESULT_PARAMETER_INVALID")));
        break;
    case XA_RESULT_MEMORY_FAILURE:
        TRACE_LOG((_L("XA_RESULT_MEMORY_FAILURE")));
        iAvailabilityError = QtMultimediaKit::ResourceError;
        break;
    case XA_RESULT_RESOURCE_ERROR:
        TRACE_LOG((_L("XA_RESULT_RESOURCE_ERROR")));
        iAvailabilityError = QtMultimediaKit::ResourceError;
        break;
    case XA_RESULT_RESOURCE_LOST:
        TRACE_LOG((_L("XA_RESULT_RESOURCE_LOST")));
        iAvailabilityError = QtMultimediaKit::ResourceError;
        break;
    case XA_RESULT_IO_ERROR:
        TRACE_LOG((_L("XA_RESULT_IO_ERROR")));
        break;
    case XA_RESULT_BUFFER_INSUFFICIENT:
        TRACE_LOG((_L("XA_RESULT_BUFFER_INSUFFICIENT")));
        break;
    case XA_RESULT_CONTENT_CORRUPTED:
        TRACE_LOG((_L("XA_RESULT_CONTENT_CORRUPTED")));
        break;
    case XA_RESULT_CONTENT_UNSUPPORTED:
        TRACE_LOG((_L("XA_RESULT_CONTENT_UNSUPPORTED")));
        break;
    case XA_RESULT_CONTENT_NOT_FOUND:
        TRACE_LOG((_L("XA_RESULT_CONTENT_NOT_FOUND")));
        break;
    case XA_RESULT_PERMISSION_DENIED:
        TRACE_LOG((_L("XA_RESULT_PERMISSION_DENIED")));
        break;
    case XA_RESULT_FEATURE_UNSUPPORTED:
        TRACE_LOG((_L("XA_RESULT_FEATURE_UNSUPPORTED")));
        break;
    case XA_RESULT_INTERNAL_ERROR:
        TRACE_LOG((_L("XA_RESULT_INTERNAL_ERROR")));
        break;
    case XA_RESULT_UNKNOWN_ERROR:
        TRACE_LOG((_L("XA_RESULT_UNKNOWN_ERROR")));
        break;
    case XA_RESULT_OPERATION_ABORTED:
        TRACE_LOG((_L("XA_RESULT_OPERATION_ABORTED")));
        break;
    case XA_RESULT_CONTROL_LOST:
        TRACE_LOG((_L("XA_RESULT_CONTROL_LOST")));
        break;
    default:
        TRACE_LOG((_L("Unknown Error!!!")));
    }
    return status;
}
