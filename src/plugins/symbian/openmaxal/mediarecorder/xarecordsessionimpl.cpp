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
#include "xarecordsessionimpl.h"
#include "xarecordsessioncommon.h"
_LIT8(K8WAVMIMETYPE, "audio/x-wav");
/*
 * These codec names are not part of AL. Hence we need to define names here.
 * */
_LIT(KAUDIOCODECPCM, "pcm");
_LIT(KAUDIOCODECAMR, "amr");
_LIT(KAUDIOCODECAAC, "aac");
_LIT(KCONTAINERWAV, "audio/wav");
_LIT(KCONTAINERWAVDESC, "wav container");
_LIT(KCONTAINERAMR, "audio/amr");
_LIT(KCONTAINERAMRDESC, "amr File format");
_LIT(KCONTAINERMP4, "audio/mpeg");
_LIT(KCONTAINERMP4DESC, "mpeg container");

const TUint KRecordPosUpdatePeriod = 1000;
const TUint KMilliToHz = 1000;
const TUint KMaxNameLength = 256;

/* Local functions for callback registation */
void cbXAObjectItf(
            XAObjectItf caller,
            const void *pContext,
            XAuint32 event,
            XAresult result,
            XAuint32 param,
            void *pInterface);

void cbXARecordItf(
            XARecordItf caller,
            void *pContext,
            XAuint32 event);

void cbXAAvailableAudioInputsChanged(
            XAAudioIODeviceCapabilitiesItf caller,
            void *pContext,
            XAuint32 deviceID,
            XAint32 numInputs,
            XAboolean isNew);

XARecordSessionImpl::XARecordSessionImpl(XARecordObserver &parent) :
    m_Parent(parent),
	m_EOEngine(NULL),
    m_MORecorder(NULL),
    m_RecordItf(NULL),
    m_AudioEncItf(NULL),
    m_WAVMime(NULL),
    m_URIName(NULL),
    m_InputDeviceId(0),
    m_ContainerType(0),
    m_BitRate(0),
    m_RateControl(0),
    m_ChannelsOut(1),
    m_SampleRate(0),
    m_AudioIODevCapsItf(NULL),
    m_AudioInputDeviceNames(NULL),
    m_DefaultAudioInputDeviceNames(NULL),
    m_AudioEncCapsItf(NULL)
{
    TRACE_FUNCTION_ENTRY_EXIT;
}

XARecordSessionImpl::~XARecordSessionImpl()
{
    TRACE_FUNCTION_ENTRY;

    if (m_MORecorder)
        (*m_MORecorder)->Destroy(m_MORecorder);

    if (m_EOEngine)
        (*m_EOEngine)->Destroy(m_EOEngine);

    delete m_WAVMime;
    delete m_URIName;

    m_InputDeviceIDs.Close();
    if (m_AudioInputDeviceNames)
        m_AudioInputDeviceNames->Reset();
    delete m_AudioInputDeviceNames;
    m_DefaultInputDeviceIDs.Close();
    if (m_DefaultAudioInputDeviceNames)
        m_DefaultAudioInputDeviceNames->Reset();
    delete m_DefaultAudioInputDeviceNames;
    m_EncoderIds.Close();
    m_EncoderNames.Close();
    m_ContainerNames.Close();
    m_ContainerDescs.Close();

    TRACE_FUNCTION_EXIT;
}

TInt32 XARecordSessionImpl::postConstruct()
{
    TRACE_FUNCTION_ENTRY;

    XAEngineOption engineOption[] = { (XAuint32) XA_ENGINEOPTION_THREADSAFE, (XAuint32) XA_BOOLEAN_TRUE};

    /* Create and realize Engine object */
    TRACE_LOG(_L("XARecordSessionImpl: Creating Engine..."));
    XAresult xa_result = xaCreateEngine(&m_EOEngine, 1, engineOption, 0, NULL, NULL);
    TInt returnValue = mapError(xa_result, ETrue);
    RET_ERR_IF_ERR(returnValue);
    TRACE_LOG(_L("XARecordSessionImpl: Realizing engine..."));
    xa_result = (*m_EOEngine)->Realize(m_EOEngine, XA_BOOLEAN_FALSE);
    returnValue = mapError(xa_result, ETrue);
    RET_ERR_IF_ERR(returnValue);
    TRACE_LOG(_L("XARecordSessionImpl: OMX AL Engine realized successfully"));

    XAEngineItf engineItf;
    xa_result = (*m_EOEngine)->GetInterface(m_EOEngine, XA_IID_ENGINE, (void**) &engineItf);
    returnValue = mapError(xa_result, ETrue);
    RET_ERR_IF_ERR(returnValue);

    xa_result = (*m_EOEngine)->GetInterface(m_EOEngine,
                                         XA_IID_AUDIOIODEVICECAPABILITIES,
                                         (void**) &m_AudioIODevCapsItf);
    returnValue = mapError(xa_result, ETrue);
    RET_ERR_IF_ERR(returnValue);
    xa_result = (*m_AudioIODevCapsItf)->RegisterAvailableAudioInputsChangedCallback(
                                        m_AudioIODevCapsItf,
                                        cbXAAvailableAudioInputsChanged,
                                        (void*)this);

    xa_result = (*m_EOEngine)->GetInterface(
                                 m_EOEngine,
                                 XA_IID_AUDIOENCODERCAPABILITIES,
                                 (void**) &m_AudioEncCapsItf);
    returnValue = mapError(xa_result, ETrue);
    RET_ERR_IF_ERR(returnValue);

    TRAP(returnValue, m_WAVMime = HBufC8::NewL(K8WAVMIMETYPE().Length() + 1));
    RET_ERR_IF_ERR(returnValue);
    TPtr8 ptr = m_WAVMime->Des();
    ptr = K8WAVMIMETYPE(); // copy uri name into local variable
    ptr.PtrZ(); // append zero terminator to end of URI

    m_AudioInputDeviceNames = new CDesC16ArrayFlat(2);
    if (m_AudioInputDeviceNames == NULL)
        returnValue = KErrNoMemory;
    RET_ERR_IF_ERR(returnValue);

    m_DefaultAudioInputDeviceNames = new CDesC16ArrayFlat(2);
    if (m_DefaultAudioInputDeviceNames == NULL)
        returnValue = KErrNoMemory;
    RET_ERR_IF_ERR(returnValue);

    returnValue = initContainersList();
    RET_ERR_IF_ERR(returnValue);
    returnValue = initAudioEncodersList();
    RET_ERR_IF_ERR(returnValue);
    returnValue = initAudioInputDevicesList();
    RET_ERR_IF_ERR(returnValue);

    TRACE_FUNCTION_EXIT;
    return returnValue;
}

TInt32 XARecordSessionImpl::setURI(const TDesC &aURI)
{
    TRACE_FUNCTION_ENTRY;

    /* This function will only get called when aURI is different than m_URIName
     * and only when recorder is in stopped state.
     * If the recorder object was created for a different URI (than aURI), we
     * need to tear it down here.
     */
    if (m_MORecorder) {
        (*m_MORecorder)->Destroy(m_MORecorder);
        m_MORecorder = NULL;
        m_RecordItf = NULL;
    }

    delete m_URIName;
    m_URIName = NULL;
    TRAPD(returnValue, m_URIName = HBufC8::NewL(aURI.Length()+1));
    RET_ERR_IF_ERR(returnValue);

    TPtr8 uriPtr = m_URIName->Des();
    /* copy uri name into local variable */
    uriPtr.Copy(aURI);

    TRACE_FUNCTION_EXIT;
    return returnValue;
}

TInt32 XARecordSessionImpl::record()
{
    TRACE_FUNCTION_ENTRY;

    TInt32 returnValue(KErrGeneral);
    if (!m_MORecorder || !m_RecordItf) {
        TRACE_LOG(_L("XARecordSessionImpl::Record: MORecorder/RecordItf is not created"));
        returnValue = createMediaRecorderObject();
        RET_ERR_IF_ERR(returnValue);

        returnValue = setEncoderSettingsToMediaRecorder();
        RET_ERR_IF_ERR(returnValue);
    }

    XAuint32 state;
    XAresult xa_result = (*m_RecordItf)->GetRecordState(m_RecordItf, &state);
    returnValue = mapError(xa_result, ETrue);
    RET_ERR_IF_ERR(returnValue);

    if ((state == XA_RECORDSTATE_STOPPED)
        || (state == XA_RECORDSTATE_PAUSED)) {
        TRACE_LOG(_L("XARecordSessionImpl::Record: Setting State to Recording..."));
        xa_result = (*m_RecordItf)->SetRecordState(m_RecordItf, XA_RECORDSTATE_RECORDING);
        returnValue = mapError(xa_result, ETrue);
        RET_ERR_IF_ERR(returnValue);
        TRACE_LOG(_L("XARecordSessionImpl::Record: SetState to Recording"));
    }

    TRACE_FUNCTION_EXIT;
    return returnValue;
}

TInt32 XARecordSessionImpl::pause()
{
    TRACE_FUNCTION_ENTRY;

    TInt32 returnValue(KErrGeneral);
    if (!m_MORecorder || !m_RecordItf) {
        TRACE_LOG(_L("XARecordSessionImpl::Record: MORecorder/RecordItf is not created"));
        return returnValue;
    }

    XAuint32 state;
    XAresult xa_result = (*m_RecordItf)->GetRecordState(m_RecordItf, &state);
    returnValue = mapError(xa_result, ETrue);
    RET_ERR_IF_ERR(returnValue);

    if ((state == XA_RECORDSTATE_STOPPED)
        || (state == XA_RECORDSTATE_RECORDING)) {
        TRACE_LOG(_L("XARecordSessionImpl::Record: Setting State to Paused..."));
        xa_result = (*m_RecordItf)->SetRecordState(m_RecordItf, XA_RECORDSTATE_PAUSED);
        returnValue = mapError(xa_result, ETrue);
        RET_ERR_IF_ERR(returnValue);
        TRACE_LOG(_L("XARecordSessionImpl::Record: SetState to Paused"));
    }

    TRACE_FUNCTION_EXIT;
    return returnValue;
}

TInt32 XARecordSessionImpl::stop()
{
    TRACE_FUNCTION_ENTRY;

    TInt32 returnValue(KErrGeneral);
    if (!m_MORecorder || !m_RecordItf) {
        TRACE_LOG(_L("XARecordSessionImpl::Record: MORecorder/RecordItf is not created"));
        return returnValue;
    }

    XAuint32 state;
    XAresult xa_result = (*m_RecordItf)->GetRecordState(m_RecordItf, &state);
    returnValue = mapError(xa_result, ETrue);
    RET_ERR_IF_ERR(returnValue);

    if ((state == XA_RECORDSTATE_PAUSED)
        || (state == XA_RECORDSTATE_RECORDING)) {
        TRACE_LOG(_L("XARecordSessionImpl::Record: Setting State to Stopped..."));
        xa_result = (*m_RecordItf)->SetRecordState(m_RecordItf, XA_RECORDSTATE_STOPPED);
        returnValue = mapError(xa_result, ETrue);
        RET_ERR_IF_ERR(returnValue);
        TRACE_LOG(_L("XARecordSessionImpl::Record: SetState to Stopped"));
    }

    TRACE_FUNCTION_EXIT;
    return returnValue;
}

TInt32 XARecordSessionImpl::duration(TInt64 &aDur)
{
    TRACE_FUNCTION_ENTRY;

    TInt32 returnValue(KErrGeneral);

    if (!m_MORecorder || !m_RecordItf) {
        TRACE_LOG(_L("XARecordSessionImpl::Duration: MORecoder/RecordItf is not created"));
        return returnValue;
    }

    XAmillisecond milliSec;
    XAresult xa_result = (*m_RecordItf)->GetPosition(m_RecordItf, &milliSec);
    returnValue = mapError(xa_result, ETrue);
    if (returnValue == KErrNone)
        aDur = (TInt64)milliSec;

    TRACE_FUNCTION_EXIT;
    return returnValue;
}

void XARecordSessionImpl::cbMediaRecorder(
                            XAObjectItf /*caller*/,
                            const void */*pContext*/,
                            XAuint32 event,
                            XAresult result,
                            XAuint32 /*param*/,
                            void */*pInterface*/)
{
    TRACE_FUNCTION_ENTRY;

    switch (event) {
    case XA_OBJECT_EVENT_RESOURCES_LOST:
        m_Parent.cbRecordingStopped();
        break;
        case XA_OBJECT_EVENT_RUNTIME_ERROR: {
            switch (result) {
            case XA_RESULT_RESOURCE_LOST:
                m_Parent.cbRecordingStopped();
                break;
            default:
                break;
            }; /* of switch (result) */
        }
    default:
        break;
    } /* of switch (event) */

    TRACE_FUNCTION_EXIT;
}

void XARecordSessionImpl::cbRecordItf(
                        XARecordItf /*caller*/,
                        void */*pContext*/,
                        XAuint32 event)
{
    TRACE_FUNCTION_ENTRY;

    switch(event) {
    case XA_RECORDEVENT_HEADATLIMIT:
        TRACE_LOG(_L("XA_RECORDEVENT_HEADATLIMIT"));
        break;
    case XA_RECORDEVENT_HEADATMARKER:
        TRACE_LOG(_L("XA_RECORDEVENT_HEADATMARKER"));
        break;
    case XA_RECORDEVENT_HEADATNEWPOS: {
        TInt32 returnValue;
        XAresult xa_result;
        XAmillisecond milliSec;
        xa_result = (*m_RecordItf)->GetPosition(m_RecordItf, &milliSec);
        returnValue = mapError(xa_result, ETrue);
        if (returnValue == KErrNone)
            m_Parent.cbDurationChanged((TInt64) milliSec);
    }
        break;
    case XA_RECORDEVENT_HEADMOVING:
        TRACE_LOG(_L("XA_RECORDEVENT_HEADMOVING"));
        m_Parent.cbRecordingStarted();
        break;
    case XA_RECORDEVENT_HEADSTALLED:
        TRACE_LOG(_L("XA_RECORDEVENT_HEADSTALLED"));
        break;
    case XA_RECORDEVENT_BUFFER_FULL:
        TRACE_LOG(_L("XA_RECORDEVENT_BUFFER_FULL"));
        break;
    default:
        TRACE_LOG(_L("UNKNOWN RECORDEVENT EVENT"));
        break;
    } /* of switch(event) */

    TRACE_FUNCTION_EXIT;
}

/* For QAudioEndpointSelector begin */
void XARecordSessionImpl::getAudioInputDeviceNames(RArray<TPtrC> &aArray)
{
    TRACE_FUNCTION_ENTRY;

    for (TInt index = 0; index < m_AudioInputDeviceNames->MdcaCount(); index++)
        aArray.Append(m_AudioInputDeviceNames->MdcaPoint(index));
    TRACE_FUNCTION_EXIT;
}

TInt32 XARecordSessionImpl::defaultAudioInputDevice(TPtrC &endPoint)
{
    TRACE_FUNCTION_ENTRY;

    TInt32 err(KErrGeneral);
    if (m_DefaultAudioInputDeviceNames->MdcaCount() >= 0) {
        endPoint.Set(m_DefaultAudioInputDeviceNames->MdcaPoint(0));
        err = KErrNone;
    }

    TRACE_FUNCTION_EXIT;
    return err;
}

TInt32 XARecordSessionImpl::activeAudioInputDevice(TPtrC &endPoint)
{
    TRACE_FUNCTION_ENTRY;

    TInt32 returnValue(KErrGeneral);
    TBool found(EFalse);
    TInt index = 0;
    for (; index < m_InputDeviceIDs.Count(); index++) {
        if (m_InputDeviceIDs[index] == m_InputDeviceId) {
            found = ETrue;
            break;
        }
    }

    /* Comparing found with ETrue produces linker error */
    if (found == true) {
        endPoint.Set(m_AudioInputDeviceNames->MdcaPoint(index));
        returnValue = KErrNone;
    }

    TRACE_FUNCTION_EXIT;
    return returnValue;
}

TBool XARecordSessionImpl::setAudioInputDevice(const TDesC &aDevice)
{
    TRACE_FUNCTION_ENTRY;

    /* validate if we can set input device id */
    TBool found(EFalse);
    m_InputDeviceId = 0;
    TInt index = 0;
    for (; index < m_AudioInputDeviceNames->MdcaCount(); index++) {
        if (m_AudioInputDeviceNames->MdcaPoint(index).Compare(aDevice) == 0) {
            found = ETrue;
            break;
        }
    }
    if (found == true) {
        m_InputDeviceId = m_InputDeviceIDs[index];
    }

    TRACE_FUNCTION_EXIT;
    return found;
}

void XARecordSessionImpl::cbAvailableAudioInputsChanged(
                                    XAAudioIODeviceCapabilitiesItf /*caller*/,
                                    void */*pContext*/,
                                    XAuint32 deviceID,
                                    XAint32 /*numInputs*/,
                                    XAboolean isNew)
{
    TRACE_FUNCTION_ENTRY;

    /* If a new device is added into the system, append it to available input list */
    if (isNew == XA_BOOLEAN_TRUE) {
        XAAudioInputDescriptor audioInputDescriptor;
        m_InputDeviceIDs.Append(deviceID);

        XAresult xa_result = (*m_AudioIODevCapsItf)->QueryAudioInputCapabilities(
                m_AudioIODevCapsItf,
                deviceID,
                &audioInputDescriptor);

        if ((mapError(xa_result, ETrue)) == KErrNone) {
            TUint8* inDevNamePtr = audioInputDescriptor.deviceName;
            TUint8* tempPtr = audioInputDescriptor.deviceName;
            TInt32 inDevNameLength = 0;
            while (*tempPtr++)
                inDevNameLength++;
            TPtrC8 ptr(inDevNamePtr, inDevNameLength);
            /* Convert 8 bit to 16 bit */
            TBuf16<KMaxNameLength> name;
            name.Copy(ptr);
            /* Using TRAP with returnValue results in compiler error */
            TRAP_IGNORE(m_AudioInputDeviceNames->AppendL(name));
        }
    }
    else {
        /* an available device has been removed from the the system, remove it from
         * available input list and also default list */
        TBool found(EFalse);
        TInt index = 0;
        for (; index < m_InputDeviceIDs.Count(); index++) {
            if (deviceID == m_InputDeviceIDs[index]) {
                found = ETrue;
                break;
            }
        }
        if (found == true) {
            m_InputDeviceIDs.Remove(index);
            m_AudioInputDeviceNames->Delete(index);
        }
        if (deviceID == m_InputDeviceId)
            m_InputDeviceId = 0;

        found = EFalse;
        for (index = 0; index < m_DefaultInputDeviceIDs.Count(); index++) {
            if (deviceID == m_DefaultInputDeviceIDs[index]) {
                found = ETrue;
                break;
            }
        }
        if (found == true) {
            m_DefaultInputDeviceIDs.Remove(index);
            m_DefaultAudioInputDeviceNames->Delete(index);
        }
    }
    m_Parent.cbAvailableAudioInputsChanged();

    TRACE_FUNCTION_EXIT;
}
/* For QAudioEndpointSelector end */

/* For QAudioEncoderControl begin */
const RArray<TPtrC>& XARecordSessionImpl::getAudioEncoderNames()
{
    TRACE_FUNCTION_ENTRY_EXIT;
    return m_EncoderNames;
}

TInt32 XARecordSessionImpl::getSampleRates(
        const TDesC& aEncoder,
        RArray<TInt32> &aSampleRates,
        TBool &aIsContinuous)
{
    TRACE_FUNCTION_ENTRY;

    aSampleRates.Reset();
    aIsContinuous = EFalse;

    XAuint32 encoderId = 0;
    TBool found(EFalse);
    for (TInt index = 0; index < m_EncoderIds.Count(); index++) {
        if (m_EncoderNames[index].Compare(aEncoder) == 0) {
            found = ETrue;
            encoderId = m_EncoderIds[index];
            break;
        }
    }

    TInt32 returnValue(KErrGeneral);
    if (found == false)
        return returnValue;

    returnValue = getSampleRatesByAudioCodecID(encoderId, aSampleRates);

    TRACE_FUNCTION_EXIT;
    return returnValue;
}

TInt32 XARecordSessionImpl::getBitrates(
                        const TDesC& aEncoder,
                        RArray<TUint32> &aBitrates,
                        TBool& aContinuous)
{
    TRACE_FUNCTION_ENTRY;

    aBitrates.Reset();

    XAuint32 encoderId = 0;
    TBool found(EFalse);
    for (TInt index = 0; index < m_EncoderIds.Count(); index++) {
        if (m_EncoderNames[index].Compare(aEncoder) == 0) {
            found = ETrue;
            encoderId = m_EncoderIds[index];
            break;
        }
    }

    TInt32 returnValue(KErrNotSupported);
    XAboolean cont;
    if (found == false)
        return returnValue;

    returnValue = getBitratesByAudioCodecID(encoderId, aBitrates, cont);
    aContinuous = cont;

    TRACE_FUNCTION_EXIT;
    return returnValue;
}

/* For QAudioEncoderControl end */

/* For QMediaContainerControl begin */
const RArray<TPtrC>& XARecordSessionImpl::getContainerNames()
{
    TRACE_FUNCTION_ENTRY_EXIT;
    return m_ContainerNames;
}

const RArray<TPtrC>& XARecordSessionImpl::getContainerDescs()
{
    TRACE_FUNCTION_ENTRY_EXIT;
    return m_ContainerDescs;
}

/* For QMediaContainerControl end */

void XARecordSessionImpl::resetEncoderAttributes()
{
    m_ContainerType = 0;
    m_AudioEncoderId = 0;
    m_ProfileSetting = 0;
    m_BitRate = 0;
    m_ChannelsOut = 1;
    m_SampleRate = 0;
    m_RateControl = 0;
}

void XARecordSessionImpl::setContainerType(const TDesC &aURI)
{
    TRACE_FUNCTION_ENTRY;

    if (aURI.Compare(KCONTAINERWAV()) == 0)
        m_ContainerType = XA_CONTAINERTYPE_WAV;
    else if (aURI.Compare(KCONTAINERAMR()) == 0)
        m_ContainerType = XA_CONTAINERTYPE_AMR;
    else if (aURI.Compare(KCONTAINERMP4()) == 0)
        m_ContainerType = XA_CONTAINERTYPE_MP4;

    TRACE_FUNCTION_EXIT;
}

TBool XARecordSessionImpl::setCodec(const TDesC &aCodec)
{
    TRACE_FUNCTION_ENTRY;

    TBool returnValue(EFalse);
    if (aCodec.Compare(KAUDIOCODECPCM()) == 0) {
        m_AudioEncoderId = XA_AUDIOCODEC_PCM;
        m_ProfileSetting = XA_AUDIOPROFILE_PCM;
        returnValue = ETrue;
    }
    else if (aCodec.Compare(KAUDIOCODECAAC()) == 0) {
        m_AudioEncoderId = XA_AUDIOCODEC_AAC;
        m_ProfileSetting = XA_AUDIOPROFILE_AAC_AAC;
        returnValue = ETrue;
    }
    else if (aCodec.Compare(KAUDIOCODECAMR()) == 0) {
        m_AudioEncoderId = XA_AUDIOCODEC_AMR;
        m_ProfileSetting = XA_AUDIOPROFILE_AMR;
        returnValue = ETrue;
    }

    TRACE_FUNCTION_EXIT;
    return returnValue;
}

TUint32 XARecordSessionImpl::getBitRate()
{
    return m_BitRate;
}

void XARecordSessionImpl::setBitRate(TUint32 aBitRate)
{
    TRACE_FUNCTION_ENTRY;
    RArray<TUint32> bitrates;
    XAboolean isContinuous;
    m_BitRate = 0;
    if (getBitratesByAudioCodecID(m_AudioEncoderId, bitrates, isContinuous) == KErrNone) {
        bitrates.SortUnsigned();
        TInt loopIndex(0);
        while (loopIndex < bitrates.Count()
            && aBitRate <= bitrates[loopIndex]) {
            m_BitRate = bitrates[loopIndex];
            loopIndex++;
        }
        bitrates.Close();
    }
    TRACE_LOG((_L("BitRate[%d]"), m_BitRate));
    TRACE_FUNCTION_EXIT;
}

TUint32 XARecordSessionImpl::getChannels()
{
    return m_ChannelsOut;
}

void XARecordSessionImpl::setChannels(TUint32 aChannels)
{
    TRACE_FUNCTION_ENTRY;
    switch (m_AudioEncoderId) {
    case XA_AUDIOCODEC_PCM:
    case XA_AUDIOCODEC_AAC:
        m_ChannelsOut = 1;
        if ((aChannels >= 1) && (aChannels <= 2))
            m_ChannelsOut = aChannels;
        break;
    case XA_AUDIOCODEC_AMR:
        m_ChannelsOut = 1;
        break;
    default:
        break;
    }
    TRACE_LOG((_L("ChannelCount[%d]"), m_ChannelsOut));
    TRACE_FUNCTION_EXIT;
}

void XARecordSessionImpl::setOptimalChannelCount()
{
    TRACE_FUNCTION_ENTRY;
    m_ChannelsOut = 1;
    TRACE_FUNCTION_EXIT;
}

TUint32 XARecordSessionImpl::getSampleRate()
{
    return m_SampleRate;
}

void XARecordSessionImpl::setSampleRate(TUint32 aSampleRate)
{
    TRACE_FUNCTION_ENTRY;

    m_SampleRate = 0;

    RArray<TInt32> samplerates;
    if (getSampleRatesByAudioCodecID(m_AudioEncoderId, samplerates) == KErrNone) {
        samplerates.SortUnsigned();
        TInt loopIndex(0);
        while (loopIndex < samplerates.Count()) {
            m_SampleRate = samplerates[loopIndex];
            if (samplerates[loopIndex] > aSampleRate)
                break;
            loopIndex++;
        }
        samplerates.Close();
    }

    /* convert Hz to MilliHz */
    m_SampleRate *= KMilliToHz;
    TRACE_LOG((_L("SampleRate[%d]"), m_SampleRate));
    TRACE_FUNCTION_EXIT;
}

void XARecordSessionImpl::setOptimalSampleRate()
{
    TRACE_FUNCTION_ENTRY;
    m_SampleRate = 0;

    if (m_AudioEncoderId == XA_AUDIOCODEC_AAC) {
        m_SampleRate = 32000 * KMilliToHz;
    }
    else if (m_AudioEncoderId == XA_AUDIOCODEC_AMR) {
        m_SampleRate = 8000 * KMilliToHz;
    }
    else {
        RArray<TInt32> sampleRates;
        TInt res = getSampleRatesByAudioCodecID(m_AudioEncoderId, sampleRates);
        if ((res == KErrNone) && (sampleRates.Count() > 0)) {
            /* Sort the array and pick the middle range sample rate */
            sampleRates.SortUnsigned();
            m_SampleRate = sampleRates[sampleRates.Count() / 2] * KMilliToHz;
        }
        sampleRates.Close();
    }

    TRACE_FUNCTION_EXIT;
}

TInt32 XARecordSessionImpl::setCBRMode()
{
    TRACE_FUNCTION_ENTRY;

    m_RateControl = XA_RATECONTROLMODE_CONSTANTBITRATE;

    TRACE_FUNCTION_EXIT;
    return KErrNone;
}

TInt32 XARecordSessionImpl::setVBRMode()
{
    TRACE_FUNCTION_ENTRY;

    m_RateControl = XA_RATECONTROLMODE_VARIABLEBITRATE;

    TRACE_FUNCTION_EXIT;
    return KErrNone;
}

void XARecordSessionImpl::setVeryLowQuality()
{
    /* Set to very low quality encoder preset */
    RArray<TUint32> bitrates;
    XAboolean continuous;
    TInt res = getBitratesByAudioCodecID(m_AudioEncoderId, bitrates, continuous);
    if ((res == KErrNone) && (bitrates.Count() > 0)) {
        /* Sort the array and pick the lowest bit rate */
        bitrates.SortUnsigned();
        m_BitRate = bitrates[0];
    }
    bitrates.Close();
}

void XARecordSessionImpl::setLowQuality()
{
    /* Set to low quality encoder preset */
    RArray<TUint32> bitrates;
    XAboolean continuous;
    TInt res = getBitratesByAudioCodecID(m_AudioEncoderId, bitrates, continuous);
    if ((res == KErrNone) && (bitrates.Count() > 0)) {
        /* Sort the array and pick the low quality bit rate */
        bitrates.SortUnsigned();
        if (continuous == XA_BOOLEAN_FALSE)
            m_BitRate = bitrates[bitrates.Count() / 4];
        else
            m_BitRate = (bitrates[1] - bitrates[0]) / 4;
    }
    bitrates.Close();
}

void XARecordSessionImpl::setNormalQuality()
{
    /* Set to normal quality encoder preset */
    RArray<TUint32> bitrates;
    XAboolean continuous;
    TInt res = getBitratesByAudioCodecID(m_AudioEncoderId, bitrates, continuous);
    if ((res == KErrNone) && (bitrates.Count() > 0)) {
        /* Sort the array and pick the middle range bit rate */
        bitrates.SortUnsigned();
        if (continuous == XA_BOOLEAN_FALSE)
            m_BitRate = bitrates[bitrates.Count() / 2];
        else
            m_BitRate = (bitrates[1] - bitrates[0]) / 2;
    }
    bitrates.Close();
}

void XARecordSessionImpl::setHighQuality()
{
    /* Set to high quality encoder preset */
    RArray<TUint32> bitrates;
    XAboolean continuous;
    TInt res = getBitratesByAudioCodecID(m_AudioEncoderId, bitrates, continuous);
    if ((res == KErrNone) && (bitrates.Count() > 0)) {
        /* Sort the array and pick the high quality bit rate */
        bitrates.SortUnsigned();
        if (continuous == XA_BOOLEAN_FALSE)
            m_BitRate = bitrates[bitrates.Count() * 3 / 4];
        else
            m_BitRate = (bitrates[1] - bitrates[0]) * 3 / 4;
    }
    bitrates.Close();
}

void XARecordSessionImpl::setVeryHighQuality()
{
    /* Set to very high quality encoder preset */
    RArray<TUint32> bitrates;
    XAboolean continuous;
    TInt res = getBitratesByAudioCodecID(m_AudioEncoderId, bitrates, continuous);
    if ((res == KErrNone) && (bitrates.Count() > 0)) {
        /* Sort the array and pick the highest bit rate */
        bitrates.SortUnsigned();
        m_BitRate = bitrates[bitrates.Count() - 1];
    }
    bitrates.Close();
}

/* Internal function */
TInt32 XARecordSessionImpl::createMediaRecorderObject()
{
    TRACE_FUNCTION_ENTRY;

    if (!m_EOEngine)
        return KErrGeneral;

    TInt32 returnValue(KErrNone);

    TRACE_LOG(_L("XARecordSessionImpl::CreateMediaRecorderObject"));
    if (!m_MORecorder && !m_RecordItf) {

        /* Setup the data source */
        m_LocatorMic.locatorType = XA_DATALOCATOR_IODEVICE;
        m_LocatorMic.deviceType = XA_IODEVICE_AUDIOINPUT;
        m_LocatorMic.deviceID = m_InputDeviceId;
        m_LocatorMic.device = NULL;
        m_DataSource.pLocator = (void*) &m_LocatorMic;
        m_DataSource.pFormat = NULL;

        /* Setup the data sink structure */
        m_Uri.locatorType = XA_DATALOCATOR_URI;
        /* append zero terminator to end of URI */
        TPtr8 uriPtr = m_URIName->Des();
        m_Uri.URI = (XAchar*) uriPtr.PtrZ();
        m_Mime.formatType = XA_DATAFORMAT_MIME;
        m_Mime.containerType = m_ContainerType;
        TPtr8 mimeTypePtr(m_WAVMime->Des());
        m_Mime.mimeType = (XAchar*) mimeTypePtr.Ptr();
        m_DataSink.pLocator = (void*) &m_Uri;
        m_DataSink.pFormat = (void*) &m_Mime;

        /* Init arrays required[] and iidArray[] */
        XAboolean required[MAX_NUMBER_INTERFACES];
        XAInterfaceID iidArray[MAX_NUMBER_INTERFACES];
        for (TInt32 i = 0; i < MAX_NUMBER_INTERFACES; i++) {
            required[i] = XA_BOOLEAN_FALSE;
            iidArray[i] = XA_IID_NULL;
        }
        XAuint32 noOfInterfaces = 0;
        required[noOfInterfaces] = XA_BOOLEAN_FALSE;
        iidArray[noOfInterfaces] = XA_IID_RECORD;
        noOfInterfaces++;
        required[noOfInterfaces] = XA_BOOLEAN_FALSE;
        iidArray[noOfInterfaces] = XA_IID_AUDIOENCODER;
        noOfInterfaces++;

        XAEngineItf engineItf;
        XAresult xa_result = (*m_EOEngine)->GetInterface(m_EOEngine, XA_IID_ENGINE, (void**) &engineItf);
        returnValue = mapError(xa_result, ETrue);
        RET_ERR_IF_ERR(returnValue);

        TRACE_LOG(_L("XARecordSessionImpl::CreateMediaRecorderObject: Create Media Recorder..."));

        /* Create recorder with NULL for a the image/video source, since this is for audio-only recording */
        xa_result = (*engineItf)->CreateMediaRecorder(
                                    engineItf,
                                    &m_MORecorder,
                                    &m_DataSource,
                                    NULL,
                                    &m_DataSink,
                                    noOfInterfaces,
                                    iidArray,
                                    required);
        returnValue = mapError(xa_result, ETrue);
        RET_ERR_IF_ERR(returnValue);

        TRACE_LOG(_L("XARecordSessionImpl::CreateMediaRecorderObject: Realize Media Recorder..."));
        xa_result = (*m_MORecorder)->Realize(m_MORecorder, XA_BOOLEAN_FALSE);
        returnValue = mapError(xa_result, ETrue);
        RET_ERR_IF_ERR(returnValue);

        TRACE_LOG(_L("XARecordSessionImpl::CreateMediaRecorderObject: Register Callback on recorder..."));
        xa_result = (*m_MORecorder)->RegisterCallback(m_MORecorder, cbXAObjectItf, (void*) this);
        returnValue = mapError(xa_result, ETrue);
        RET_ERR_IF_ERR(returnValue);

        TRACE_LOG(_L("XARecordSessionImpl::CreateMediaRecorderObject: Getting Record Interface..."));
        xa_result = (*m_MORecorder)->GetInterface(m_MORecorder, XA_IID_RECORD, &m_RecordItf);
        returnValue = mapError(xa_result, ETrue);
        RET_ERR_IF_ERR(returnValue);

        TRACE_LOG(_L("XARecordSessionImpl::CreateMediaRecorderObject: Registering Callback on record Interface..."));
        xa_result = (*m_RecordItf)->RegisterCallback(m_RecordItf, cbXARecordItf, (void*) this);
        returnValue = mapError(xa_result, ETrue);
        RET_ERR_IF_ERR(returnValue);

        TRACE_LOG(_L("XARecordSessionImpl::CreateMediaRecorderObject: SetPositionUpdatePeriod on record Interface..."));
        xa_result = (*m_RecordItf)->SetPositionUpdatePeriod(m_RecordItf, (XAmillisecond)KRecordPosUpdatePeriod);
        returnValue = mapError(xa_result, ETrue);
        RET_ERR_IF_ERR(returnValue);

        TRACE_LOG(_L("XARecordSessionImpl::CreateMediaRecorderObject: SetCallbackEventsMask on record Interface..."));
        xa_result = (*m_RecordItf)->SetCallbackEventsMask(m_RecordItf, XA_RECORDEVENT_HEADATNEWPOS |
                                                                    XA_RECORDEVENT_HEADMOVING |
                                                                    XA_RECORDEVENT_HEADSTALLED);
        returnValue = mapError(xa_result, ETrue);
        RET_ERR_IF_ERR(returnValue);

        TRACE_LOG(_L("XARecordSessionImpl::CreateMediaRecorderObject: Getting Audio Encoder Interface..."));
        xa_result = (*m_MORecorder)->GetInterface(m_MORecorder, XA_IID_AUDIOENCODER, &m_AudioEncItf);
        returnValue = mapError(xa_result, ETrue);
        RET_ERR_IF_ERR(returnValue);
    }

    TRACE_FUNCTION_EXIT;
    return returnValue;
}

TInt32 XARecordSessionImpl::mapError(XAresult xa_err, TBool debPrn)
{
    TInt32 returnValue(KErrGeneral);
    switch (xa_err) {
    case XA_RESULT_SUCCESS:
        returnValue = KErrNone;
        break;
    case XA_RESULT_PRECONDITIONS_VIOLATED:
        if (debPrn)
            TRACE_LOG(_L("XA_RESULT_PRECONDITIONS_VIOLATED"));
        break;
    case XA_RESULT_PARAMETER_INVALID:
        if (debPrn)
            TRACE_LOG(_L("XA_RESULT_PARAMETER_INVALID"));
        break;
    case XA_RESULT_MEMORY_FAILURE:
        if (debPrn)
            TRACE_LOG(_L("XA_RESULT_MEMORY_FAILURE"));
        break;
    case XA_RESULT_RESOURCE_ERROR:
        if (debPrn)
            TRACE_LOG(_L("XA_RESULT_RESOURCE_ERROR"));
        break;
    case XA_RESULT_RESOURCE_LOST:
        if (debPrn)
            TRACE_LOG(_L("XA_RESULT_RESOURCE_LOST"));
        break;
    case XA_RESULT_IO_ERROR:
        if (debPrn)
            TRACE_LOG(_L("XA_RESULT_IO_ERROR"));
        break;
    case XA_RESULT_BUFFER_INSUFFICIENT:
        if (debPrn)
            TRACE_LOG(_L("XA_RESULT_BUFFER_INSUFFICIENT"));
        break;
    case XA_RESULT_CONTENT_CORRUPTED:
        if (debPrn)
            TRACE_LOG(_L("XA_RESULT_CONTENT_CORRUPTED"));
        break;
    case XA_RESULT_CONTENT_UNSUPPORTED:
        if (debPrn)
            TRACE_LOG(_L("XA_RESULT_CONTENT_UNSUPPORTED"));
        break;
    case XA_RESULT_CONTENT_NOT_FOUND:
        if (debPrn)
            TRACE_LOG(_L("XA_RESULT_CONTENT_NOT_FOUND"));
        break;
    case XA_RESULT_PERMISSION_DENIED:
        if (debPrn)
            TRACE_LOG(_L("XA_RESULT_PERMISSION_DENIED"));
        break;
    case XA_RESULT_FEATURE_UNSUPPORTED:
        if (debPrn)
            TRACE_LOG(_L("XA_RESULT_FEATURE_UNSUPPORTED"));
        break;
    case XA_RESULT_INTERNAL_ERROR:
        if (debPrn)
            TRACE_LOG(_L("XA_RESULT_INTERNAL_ERROR"));
        break;
    case XA_RESULT_UNKNOWN_ERROR:
        if (debPrn)
            TRACE_LOG(_L("XA_RESULT_UNKNOWN_ERROR"));
        break;
    case XA_RESULT_OPERATION_ABORTED:
        if (debPrn)
            TRACE_LOG(_L("XA_RESULT_OPERATION_ABORTED"));
        break;
    case XA_RESULT_CONTROL_LOST:
        if (debPrn)
            TRACE_LOG(_L("XA_RESULT_CONTROL_LOST"));
        break;
    default:
        if (debPrn)
            TRACE_LOG(_L("Unknown Error!!!"));
        break;
    }
    return returnValue;
}

TInt32 XARecordSessionImpl::initContainersList()
{
    TRACE_FUNCTION_ENTRY;

    m_ContainerNames.Reset();
    m_ContainerDescs.Reset();

    m_ContainerNames.Append(KCONTAINERWAV());
    m_ContainerNames.Append(KCONTAINERAMR());
    m_ContainerNames.Append(KCONTAINERMP4());

    m_ContainerDescs.Append(KCONTAINERWAVDESC());
    m_ContainerDescs.Append(KCONTAINERAMRDESC());
    m_ContainerDescs.Append(KCONTAINERMP4DESC());

    TRACE_FUNCTION_EXIT;
    return KErrNone;
}

TInt32 XARecordSessionImpl::initAudioEncodersList()
{
    TRACE_FUNCTION_ENTRY;

    m_EncoderIds.Reset();
    m_EncoderNames.Reset();

    XAuint32 encoderIds[MAX_NUMBER_ENCODERS];

    for (TInt index = 0; index < MAX_NUMBER_ENCODERS; index++)
        encoderIds[index] = 0;

    XAuint32 numEncoders = MAX_NUMBER_ENCODERS;
    XAresult xa_result = (*m_AudioEncCapsItf)->GetAudioEncoders(
                                        m_AudioEncCapsItf,
                                        &numEncoders,
                                        encoderIds);
    TInt32 returnValue = mapError(xa_result, ETrue);
    RET_ERR_IF_ERR(returnValue);

    for (TInt index = 0; index < numEncoders; index++) {
        m_EncoderIds.Append(encoderIds[index]);
        switch (encoderIds[index]) {
        case XA_AUDIOCODEC_PCM:
            m_EncoderNames.Append(KAUDIOCODECPCM());
            break;
        case XA_AUDIOCODEC_AMR:
            m_EncoderNames.Append(KAUDIOCODECAMR());
            break;
        case XA_AUDIOCODEC_AAC:
            m_EncoderNames.Append(KAUDIOCODECAAC());
            break;
        default:
            break;
        };
    }

    TRACE_FUNCTION_EXIT;
    return returnValue;
}

TInt32 XARecordSessionImpl::initAudioInputDevicesList()
{
    TRACE_FUNCTION_ENTRY;

    m_InputDeviceIDs.Reset();

    XAuint32 deviceIds[MAX_NUMBER_INPUT_DEVICES];
    for (TInt index = 0; index < MAX_NUMBER_INPUT_DEVICES; index++)
        deviceIds[index] = 0;

    XAint32 numInputs = MAX_NUMBER_INPUT_DEVICES;
    XAresult xa_result = (*m_AudioIODevCapsItf)->GetAvailableAudioInputs(
                                        m_AudioIODevCapsItf,
                                        &numInputs,
                                        deviceIds);
    TInt32 returnValue = mapError(xa_result, ETrue);
    RET_ERR_IF_ERR(returnValue);

    XAAudioInputDescriptor audioInputDescriptor;
    for (TInt index = 0; index < numInputs; index++) {
        xa_result = (*m_AudioIODevCapsItf)->QueryAudioInputCapabilities(
                m_AudioIODevCapsItf,
                deviceIds[index],
                &audioInputDescriptor);
        returnValue = mapError(xa_result, ETrue);
        if (returnValue != KErrNone)
            continue;

        TUint8 * inDevNamePtr = audioInputDescriptor.deviceName;
        TUint8 * tempPtr = audioInputDescriptor.deviceName;
        TInt32 inDevNameLength = 0;
        while (*tempPtr++)
            inDevNameLength++;
        TPtrC8 ptr(inDevNamePtr, inDevNameLength);
        /* Convert 8 bit to 16 bit */
        TBuf16<KMaxNameLength> name;
        name.Copy(ptr);
        /* Using TRAP with returnValue results in compiler error */
        TRAPD(err2, m_AudioInputDeviceNames->AppendL(name));
        returnValue = err2;
        if (returnValue != KErrNone)
            continue;
        m_InputDeviceIDs.Append(deviceIds[index]);
    }

    numInputs = MAX_NUMBER_INPUT_DEVICES;
    for (TInt index = 0; index < MAX_NUMBER_INPUT_DEVICES; index++)
        deviceIds[index] = 0;
    xa_result = (*m_AudioIODevCapsItf)->GetDefaultAudioDevices(
                                        m_AudioIODevCapsItf,
                                        XA_DEFAULTDEVICEID_AUDIOINPUT,
                                        &numInputs,
                                        deviceIds);
    returnValue = mapError(xa_result, ETrue);
    RET_ERR_IF_ERR(returnValue);

    for (TInt index = 0; index < numInputs; index++) {
        xa_result = (*m_AudioIODevCapsItf)->QueryAudioInputCapabilities(
                m_AudioIODevCapsItf,
                deviceIds[index],
                &audioInputDescriptor);
        returnValue = mapError(xa_result, ETrue);
        if (returnValue != KErrNone)
            continue;
        TUint8* inDevNamePtr = audioInputDescriptor.deviceName;
        TUint8* tempPtr = audioInputDescriptor.deviceName;
        TInt32 inDevNameLength = 0;
        while (*tempPtr++)
            inDevNameLength++;
        TPtrC8 ptr(inDevNamePtr, inDevNameLength);
        /* Convert 8 bit to 16 bit */
        TBuf16<KMaxNameLength> name;
        name.Copy(ptr);
        /* Using TRAP with returnValue results in compiler error */
        TRAPD(err2, m_DefaultAudioInputDeviceNames->AppendL(name));
        returnValue = err2;
        if (returnValue != KErrNone)
            continue;
        m_DefaultInputDeviceIDs.Append(deviceIds[index]);
        m_InputDeviceId = deviceIds[index];
    }

    TRACE_FUNCTION_EXIT;
    return returnValue;
}

TInt32 XARecordSessionImpl::setEncoderSettingsToMediaRecorder()
{
    TRACE_FUNCTION_EXIT;

    /* Get current settings */
    XAAudioEncoderSettings settings;
    XAresult xa_result = (*m_AudioEncItf)->GetEncoderSettings(
            m_AudioEncItf,
            &settings);
    TInt32 returnValue = mapError(xa_result, ETrue);

    settings.encoderId = m_AudioEncoderId;
    settings.channelsOut = m_ChannelsOut;
    if ((m_SampleRate != 0) && (m_SampleRate != 0xffffffff))
        settings.sampleRate = m_SampleRate;
    if ((m_BitRate != 0) && (m_BitRate != 0xffffffff))
        settings.bitRate = m_BitRate;
    if (m_RateControl != 0)
        settings.rateControl = m_RateControl;
    settings.profileSetting = m_ProfileSetting;
    xa_result = (*m_AudioEncItf)->SetEncoderSettings(
            m_AudioEncItf,
            &settings);
    returnValue = mapError(xa_result, ETrue);

    TRACE_FUNCTION_EXIT;
    return returnValue;
}

TInt32 XARecordSessionImpl::getBitratesByAudioCodecID(
        XAuint32 encoderId,
        RArray<TUint32> &aBitrates,
        XAboolean& aContinuous)
{
    TRACE_FUNCTION_ENTRY;

    if (!m_AudioEncCapsItf)
        return KErrGeneral;

    XAuint32 numCaps = 0;
    XAAudioCodecDescriptor codecDesc;
    XAresult xa_result = (*m_AudioEncCapsItf)->GetAudioEncoderCapabilities(
                                        m_AudioEncCapsItf,
                                        encoderId,
                                        &numCaps,
                                        &codecDesc);
    TInt32 returnValue = mapError(xa_result, ETrue);
    RET_ERR_IF_ERR(returnValue);
    aContinuous = codecDesc.isBitrateRangeContinuous;
    /* TODO What do we do if we have more than one caps?? */
    if (codecDesc.isBitrateRangeContinuous == XA_BOOLEAN_TRUE) {
        aBitrates.Append(codecDesc.minBitRate);
        aBitrates.Append(codecDesc.maxBitRate);
    }
    else {
        XAuint32 numBrSupported = codecDesc.numBitratesSupported;
        XAuint32 * pBitratesSupported(NULL);
        pBitratesSupported = codecDesc.pBitratesSupported;
        TInt32 index = 0;
        for (index = 0; index < numBrSupported; index++)
            aBitrates.Append(*(pBitratesSupported + index));
    }

    TRACE_FUNCTION_ENTRY;
    return returnValue;
}

TInt32 XARecordSessionImpl::getSampleRatesByAudioCodecID(XAuint32 encoderId,
    RArray<TInt32> &aSampleRates)
{
    TRACE_FUNCTION_ENTRY;

    if (!m_AudioEncCapsItf)
        return KErrGeneral;

    XAuint32 numCaps = 0;
    XAAudioCodecDescriptor codecDesc;
    XAresult xa_result = (*m_AudioEncCapsItf)->GetAudioEncoderCapabilities(
                                        m_AudioEncCapsItf,
                                        encoderId,
                                        &numCaps,
                                        &codecDesc);
    TInt returnValue = mapError(xa_result, ETrue);
    RET_ERR_IF_ERR(returnValue);

    /* TODO What do we do if we have more than one caps?? */
    if (codecDesc.isFreqRangeContinuous == XA_BOOLEAN_TRUE) {
        aSampleRates.Append(codecDesc.minSampleRate / KMilliToHz);
        aSampleRates.Append(codecDesc.maxSampleRate / KMilliToHz);
    }
    else {
        XAuint32 numSRSupported = codecDesc.numSampleRatesSupported;
        XAmilliHertz *pSampleRatesSupported(NULL);
        pSampleRatesSupported = codecDesc.pSampleRatesSupported;
        for (TInt index = 0; index < numSRSupported; index++)
            aSampleRates.Append((*(pSampleRatesSupported + index)) / KMilliToHz);
    }

    TRACE_FUNCTION_ENTRY;
    return returnValue;
}

/* Local function implementation */
void cbXAObjectItf(
            XAObjectItf caller,
            const void *pContext,
            XAuint32 event,
            XAresult result,
            XAuint32 param,
            void *pInterface)
{
    if (pContext) {
        ((XARecordSessionImpl*)pContext)->cbMediaRecorder(
                                                        caller,
                                                        pContext,
                                                        event,
                                                        result,
                                                        param,
                                                        pInterface);
    }
}

void cbXARecordItf(
            XARecordItf caller,
            void *pContext,
            XAuint32 event)
{
    if (pContext) {
        ((XARecordSessionImpl*)pContext)->cbRecordItf(
                                                    caller,
                                                    pContext,
                                                    event);
    }
}

void cbXAAvailableAudioInputsChanged(
            XAAudioIODeviceCapabilitiesItf caller,
            void * pContext,
            XAuint32 deviceID,
            XAint32 numInputs,
            XAboolean isNew)
{
    if (pContext) {
        ((XARecordSessionImpl*)pContext)->cbAvailableAudioInputsChanged(
                                                    caller,
                                                    pContext,
                                                    deviceID,
                                                    numInputs,
                                                    isNew);
    }
}
