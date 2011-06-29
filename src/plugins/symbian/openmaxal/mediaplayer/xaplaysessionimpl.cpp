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

#include <QString>
#include <QVariant>
#include <QList>
#include <QStringList>
#include <QImage>

#include "xaplaysessionimpl.h"
#include "xaplaysessioncommon.h"
#include "xacommon.h"

#ifdef USE_VIDEOPLAYERUTILITY
#include <COECNTRL.H>
#endif

_LIT8(K8WAVMIMETYPE, "audio/wav");

#define RET_ERR_IF_ERR(e) \
    if (e != 0) {\
        return e; \
    } \

#define MAX_NUMBER_INTERFACES 20
const TUint KPlayPosUpdatePeriod = 1000;

/* Local functions for callback registation */
void MediaPlayerCallback( XAObjectItf caller, 
                          const void *pContext,
                          XAuint32 event, 
                          XAresult result, 
                          XAuint32 param,
                          void *pInterface);

void PlayItfCallback( XAPlayItf caller, 
                      void *pContext, 
                      XAuint32 event);

void PrefetchItfCallback( XAPrefetchStatusItf caller,
                          void * pContext,
                          XAuint32 event);

void StreamInformationItfCallback( XAStreamInformationItf caller,
                                   XAuint32 eventId,
                                   XAuint32 streamIndex,
                                   void * pEventData,
                                   void * pContext);

XAPlaySessionImpl::XAPlaySessionImpl(XAPlayObserver& parent)
:mParent(parent),
mEOEngine(NULL),
mMOPlayer(NULL),
mPlayItf(NULL),
mSeekItf(NULL),
mURIName(NULL),
mWAVMime(NULL),
mbMetadataAvailable(EFalse),
mbVolEnabled(EFalse),
mbMuteEnabled(EFalse),
mbPrefetchStatusChange(EFalse),
mbStreamInfoAvailable(EFalse),
mbPlaybackRateItfAvailable(EFalse),
mbScalable(EFalse),
mCurrAspectRatioMode(Qt::KeepAspectRatio)
#ifdef USE_VIDEOPLAYERUTILITY
, mVideoPlayUtil(NULL)
, mActiveSchedulerWait(NULL)
#endif
{
}

XAPlaySessionImpl::~XAPlaySessionImpl()
{
    if (mMOPlayer)
        (*mMOPlayer)->Destroy(mMOPlayer);

    if (mEOEngine)
        (*mEOEngine)->Destroy(mEOEngine);

    delete mURIName;
    delete mWAVMime;

    //clear metadata datastructures
    alKeyMap.clear();
    keyMap.clear();
    extendedKeyMap.clear();
    
#ifdef USE_VIDEOPLAYERUTILITY
    delete mVideoPlayUtil;
    if (mActiveSchedulerWait && \
        mActiveSchedulerWait->IsStarted()) {
        mActiveSchedulerWait->AsyncStop();
    }

    delete mActiveSchedulerWait;
#endif

}

TInt XAPlaySessionImpl::postConstruct()
{
    TInt retVal;
    XAresult xaRes;
    XAEngineOption engineOption[] = {     (XAuint32) XA_ENGINEOPTION_THREADSAFE, 
                                        (XAuint32) XA_BOOLEAN_TRUE
                                    };
    XAEngineItf engineItf;

    mNativeDisplay.locatorType = XA_DATALOCATOR_NATIVEDISPLAY;
    mNativeDisplay.hWindow = NULL;
    mNativeDisplay.hDisplay = NULL;
    mVideoSink.pLocator = (void*)&mNativeDisplay;
    mVideoSink.pFormat = NULL;

    // Create and realize Engine object
    xaRes = xaCreateEngine (&mEOEngine, 1, engineOption, 0, NULL, NULL);
    retVal = mapError(xaRes, ETrue);
    RET_ERR_IF_ERR(retVal);
     xaRes = (*mEOEngine)->Realize(mEOEngine, XA_BOOLEAN_FALSE);
    retVal = mapError(xaRes, ETrue);
    RET_ERR_IF_ERR(retVal);

    // Create and realize Output Mix object to be used by player 
    xaRes = (*mEOEngine)->GetInterface(mEOEngine, XA_IID_ENGINE, (void**) &engineItf);
    retVal = mapError(xaRes, ETrue);
    RET_ERR_IF_ERR(retVal);

    TRAP(retVal, mWAVMime = HBufC8::NewL(K8WAVMIMETYPE().Length() + 1));
    RET_ERR_IF_ERR(retVal);
    TPtr8 ptr = mWAVMime->Des();
    ptr = K8WAVMIMETYPE(); // copy uri name into local variable
      ptr.PtrZ(); // append zero terminator to end of URI

#ifdef USE_VIDEOPLAYERUTILITY
    TRAP(retVal, mVideoPlayUtil = 
           CVideoPlayerUtility2::NewL( *this,
                                       EMdaPriorityNormal,
                                       EMdaPriorityPreferenceTimeAndQuality)
         );
    mActiveSchedulerWait = new CActiveSchedulerWait;
#endif

    return retVal;
}

TInt XAPlaySessionImpl::addNativeDisplay(RWindow* window, RWsSession* wssession)
    {
    TInt retVal(KErrNotReady);

    if (!mMOPlayer && !mPlayItf) {
        // window can only be set before player creation 
        mNativeDisplay.locatorType = XA_DATALOCATOR_NATIVEDISPLAY;
        mNativeDisplay.hWindow = (void*)window;
        mNativeDisplay.hDisplay = (void*)wssession;
        retVal = KErrNone;
    }
    return retVal;
}

TInt XAPlaySessionImpl::updateNativeDisplay(RWindow* /*window*/, RWsSession* /*wssession*/)
{
    return KErrNone;
}

TInt XAPlaySessionImpl::removeNativeDisplay(RWindow* /*window*/, RWsSession* /*wssession*/)
{
    return KErrNone;
}

TInt XAPlaySessionImpl::load(const TDesC& aURI)
{
    TInt retVal;
    XAresult xaRes;
    XAEngineItf engineItf;
    XADynamicSourceItf dynamicSourceItf;
    XAboolean required[MAX_NUMBER_INTERFACES];
    XAInterfaceID iidArray[MAX_NUMBER_INTERFACES];
    XAuint32 noOfInterfaces = 0;
    TInt i;

    XAmillisecond dur(0);
    TPtr8 uriPtr(0,0,0);
    TPtr8 mimeTypePtr(0,0,0);

#ifdef USE_VIDEOPLAYERUTILITY
    TRAP(m_VPError, mVideoPlayUtil->OpenFileL(_L("C:\\data\\test.3gp")));
    if (m_VPError)
        return 0;

    if(!mActiveSchedulerWait->IsStarted())
        mActiveSchedulerWait->Start();

    if (m_VPError)
        return 0;
    
    mVideoPlayUtil->Prepare();

    if(!mActiveSchedulerWait->IsStarted())
        mActiveSchedulerWait->Start();

    return 0;
#endif

    delete mURIName;
    mURIName = NULL;
    TRAP(retVal, mURIName = HBufC8::NewL(aURI.Length()+1));
    RET_ERR_IF_ERR(retVal);
    uriPtr.Set(mURIName->Des());
    
    // This has to be done here since we can not destroy the Player
    // in the Resource Lost callback.
    if (mbMediaPlayerUnrealized) {
        if (mMOPlayer) {
            (*mMOPlayer)->Destroy(mMOPlayer);
            mMOPlayer = NULL;
        }
    }
    
    //py uri name into local variable 
    //TODO fix copy issue from 16 bit to 8 bit
    uriPtr.Copy(aURI);

    //If media player object already exists, just switch source
    //using dynamic source interface
    if (mMOPlayer && mPlayItf) {
        dynamicSourceItf = NULL;
        xaRes = (*mMOPlayer)->GetInterface(mMOPlayer, XA_IID_DYNAMICSOURCE, &dynamicSourceItf);
        retVal = mapError(xaRes, ETrue);
        RET_ERR_IF_ERR(retVal);

        //Setup the data source
        //TODO Hard coded locator type
        mUri.locatorType = XA_DATALOCATOR_URI;
        
        //append zero terminator to end of URI
        mUri.URI = (XAchar*) uriPtr.PtrZ();
        
        //TODO Hard coded locator type
        mMime.containerType = XA_CONTAINERTYPE_WAV;
        
        //TODO Hard coded locator type
        mMime.formatType = XA_DATAFORMAT_MIME;
        mimeTypePtr.Set(mWAVMime->Des());
        mMime.mimeType = (XAchar*)mimeTypePtr.Ptr();
        mDataSource.pFormat = (void*)&mMime;
        mDataSource.pLocator = (void*)&mUri;
        
        xaRes = (*dynamicSourceItf)->SetSource(dynamicSourceItf, &mDataSource);
        retVal = mapError(xaRes, ETrue);
        RET_ERR_IF_ERR(retVal);
    }
    else { // Create media player object

        // Setup the data source
        // TODO Hard coded locator type
        mUri.locatorType = XA_DATALOCATOR_URI;
        
        //append zero terminator to end of URI
        mUri.URI = (XAchar*) uriPtr.PtrZ();
        
        //TODO Hard coded locator type
        mMime.containerType = XA_CONTAINERTYPE_WAV;
        
        //TODO Hard coded locator type
        mMime.formatType = XA_DATAFORMAT_MIME;
        mimeTypePtr.Set(mWAVMime->Des());
        mMime.mimeType = (XAchar*)mimeTypePtr.Ptr();
        mDataSource.pFormat = (void*)&mMime;
        mDataSource.pLocator = (void*)&mUri;
        
           //Setup the audio data sink
        mLocatorOutputDevice.locatorType = XA_DATALOCATOR_IODEVICE;
        mLocatorOutputDevice.deviceType = 6;
        mAudioSink.pLocator = (void*) &mLocatorOutputDevice;
        mAudioSink.pFormat = NULL;

        //Init arrays required[] and iidArray[]
        for (i = 0; i < MAX_NUMBER_INTERFACES; i++) {
            required[i] = XA_BOOLEAN_FALSE;
            iidArray[i] = XA_IID_NULL;
        }
        
        noOfInterfaces = 0;
        required[noOfInterfaces] = XA_BOOLEAN_FALSE;
        iidArray[noOfInterfaces] = XA_IID_SEEK;
        noOfInterfaces++;
        required[noOfInterfaces] = XA_BOOLEAN_FALSE;
        iidArray[noOfInterfaces] = XA_IID_DYNAMICSOURCE;
        noOfInterfaces++;
        required[noOfInterfaces] = XA_BOOLEAN_FALSE;
        iidArray[noOfInterfaces] = XA_IID_METADATAEXTRACTION;
        noOfInterfaces++;
        required[noOfInterfaces] = XA_BOOLEAN_FALSE;
        iidArray[noOfInterfaces] = XA_IID_NOKIALINEARVOLUME;
        noOfInterfaces++;
        required[noOfInterfaces] = XA_BOOLEAN_FALSE;
        iidArray[noOfInterfaces] = XA_IID_NOKIAVOLUMEEXT;
        noOfInterfaces++;
        required[noOfInterfaces] = XA_BOOLEAN_FALSE;
        iidArray[noOfInterfaces] = XA_IID_PREFETCHSTATUS;
        noOfInterfaces++;
        required[noOfInterfaces] = XA_BOOLEAN_FALSE;
        iidArray[noOfInterfaces] = XA_IID_STREAMINFORMATION;
        noOfInterfaces++;
        required[noOfInterfaces] = XA_BOOLEAN_FALSE;
        iidArray[noOfInterfaces] = XA_IID_PLAYBACKRATE;
        noOfInterfaces++;
        required[noOfInterfaces] = XA_BOOLEAN_FALSE;
        iidArray[noOfInterfaces] = XA_IID_VIDEOPOSTPROCESSING;
        noOfInterfaces++;

        xaRes = (*mEOEngine)->GetInterface(mEOEngine, XA_IID_ENGINE, (void**) &engineItf);
        retVal = mapError(xaRes, ETrue);
        RET_ERR_IF_ERR(retVal);

        xaRes = (*engineItf)->CreateMediaPlayer(engineItf,
                                                &mMOPlayer,
                                                &mDataSource,
                                                NULL,
                                                &mAudioSink,
                                                &mVideoSink,
                                                NULL,
                                                NULL,
                                                noOfInterfaces,
                                                iidArray,
                                                required);
        retVal = mapError(xaRes, ETrue);
        RET_ERR_IF_ERR(retVal);

        xaRes = (*mMOPlayer)->Realize(mMOPlayer, XA_BOOLEAN_FALSE);
        retVal = mapError(xaRes, ETrue);
        RET_ERR_IF_ERR(retVal);

        mbMediaPlayerUnrealized = FALSE;
        
        xaRes = (*mMOPlayer)->RegisterCallback(mMOPlayer, MediaPlayerCallback, (void*)this);
        retVal = mapError(xaRes, ETrue);
        RET_ERR_IF_ERR(retVal);

           xaRes = (*mMOPlayer)->GetInterface(mMOPlayer, XA_IID_PLAY, &mPlayItf);
        retVal = mapError(xaRes, ETrue);
        RET_ERR_IF_ERR(retVal);

        xaRes = (*mPlayItf)->RegisterCallback(mPlayItf, PlayItfCallback, (void*)this);
        retVal = mapError(xaRes, ETrue);
        RET_ERR_IF_ERR(retVal);

        xaRes = (*mPlayItf)->SetPositionUpdatePeriod(mPlayItf, (XAmillisecond)KPlayPosUpdatePeriod);
        retVal = mapError(xaRes, ETrue);
        RET_ERR_IF_ERR(retVal);

        xaRes = (*mPlayItf)->SetCallbackEventsMask(    mPlayItf,
                                                    ( XA_PLAYEVENT_HEADATEND |
                                                      XA_PLAYEVENT_HEADATNEWPOS |
                                                      XA_PLAYEVENT_HEADMOVING )
                                                  );
        retVal = mapError(xaRes, ETrue);
        RET_ERR_IF_ERR(retVal);

        xaRes = (*mMOPlayer)->GetInterface(mMOPlayer, XA_IID_SEEK, &mSeekItf);
        retVal = mapError(xaRes, ETrue);
        
        //Metadata
        xaRes = (*mMOPlayer)->GetInterface(mMOPlayer, XA_IID_METADATAEXTRACTION, &mMetadataExtItf);
        if(mapError(xaRes, ETrue)==KErrNone) {
            mbMetadataAvailable = ETrue;
            setupALKeyMap(); //done only once at creation of meadia player
        }
        
        //volume
        xaRes = (*mMOPlayer)->GetInterface(mMOPlayer, XA_IID_NOKIALINEARVOLUME, &mNokiaLinearVolumeItf);
        if(mapError(xaRes, ETrue)==KErrNone)
            mbVolEnabled = ETrue;

        xaRes = (*mMOPlayer)->GetInterface(mMOPlayer, XA_IID_NOKIAVOLUMEEXT, &mNokiaVolumeExtItf);
        if(mapError(xaRes, ETrue)==KErrNone)
            mbMuteEnabled = ETrue;
        
        //buffer status
        xaRes = (*mMOPlayer)->GetInterface(mMOPlayer, XA_IID_PREFETCHSTATUS, &mPrefetchStatusItf);
        if(mapError(xaRes, ETrue)==KErrNone) {
            mbPrefetchStatusChange = ETrue;
            (*mPrefetchStatusItf)->RegisterCallback(mPrefetchStatusItf, PrefetchItfCallback, (void*)this);
            (*mPrefetchStatusItf)->SetCallbackEventsMask(mPrefetchStatusItf, XA_PREFETCHEVENT_FILLLEVELCHANGE);
        }

        //stream information
        xaRes = (*mMOPlayer)->GetInterface(mMOPlayer, XA_IID_STREAMINFORMATION, &mStreamInformationItf);
        if(mapError(xaRes, ETrue)==KErrNone) {
            mbStreamInfoAvailable = ETrue;
            mParent.cbStreamInformation(ETrue); //indicate first time
            (*mStreamInformationItf)->RegisterStreamChangeCallback(mStreamInformationItf, StreamInformationItfCallback, (void*)this);
        }

        //playback rate 
        xaRes = (*mMOPlayer)->GetInterface(mMOPlayer, XA_IID_PLAYBACKRATE, &mPlaybackRateItf);
        if(mapError(xaRes, ETrue)==KErrNone)
            mbPlaybackRateItfAvailable = ETrue;


        //videopostprocessing 
        xaRes = (*mMOPlayer)->GetInterface(mMOPlayer, XA_IID_VIDEOPOSTPROCESSING, &mVideoPostProcessingItf);
        if(mapError(xaRes, ETrue)==KErrNone)
            mbScalable = ETrue;

    }

    if(mbMetadataAvailable) {
        keyMap.clear();
        extendedKeyMap.clear();
        setupMetaData(); //done every time source is changed
    }
    else { //send signal for seekable
        mParent.cbSeekableChanged(ETrue);
    }

    mCurPosition = 0;
    mParent.cbPositionChanged(mCurPosition);

    xaRes = (*mPlayItf)->GetDuration(mPlayItf, &dur);
    retVal = mapError(xaRes, ETrue);
    RET_ERR_IF_ERR(retVal);

    mDuration = dur;
    mParent.cbDurationChanged(mDuration);

    return retVal;
}

void XAPlaySessionImpl::unload()
{
    mPlayItf = NULL;
    mSeekItf = NULL;

    //Metadata
    mbMetadataAvailable = FALSE;
    mMetadataExtItf = NULL;
    alKeyMap.clear();
    keyMap.clear();
    extendedKeyMap.clear();
    //Volume
    mNokiaLinearVolumeItf = NULL;
    mbVolEnabled = FALSE;
    mNokiaVolumeExtItf = NULL;
    mbMuteEnabled = NULL;

    //buffer status
    mPrefetchStatusItf = NULL;
    mbPrefetchStatusChange = FALSE;

    //stream information
    mStreamInformationItf = NULL;
    mbStreamInfoAvailable = FALSE;
    mbAudioStream = FALSE;
    mbVideoStream = FALSE;
    mNumStreams = 0;

    //Playbackrate
    mPlaybackRateItf = NULL;
    mbPlaybackRateItfAvailable = FALSE;

    mVideoPostProcessingItf = NULL;
    mbScalable = FALSE;
    mCurrAspectRatioMode = Qt::KeepAspectRatio;
    
    //internal
    mCurPosition = 0; // in milliseconds
    mDuration = 0; // in milliseconds

    
    mbMediaPlayerUnrealized = TRUE;
     
    delete mURIName;
    mURIName = NULL;

}

TInt XAPlaySessionImpl::play()
{
    TInt retVal(KErrGeneral);
    XAresult xaRes;
    XAuint32 state;

#ifdef USE_VIDEOPLAYERUTILITY
    mVideoPlayUtil->Play();
    return 0;
#endif

    if (!mMOPlayer || !mPlayItf)
        return retVal;

    xaRes = (*mPlayItf)->GetPlayState(mPlayItf, &state);
    retVal = mapError(xaRes, ETrue);
    RET_ERR_IF_ERR(retVal);

    if ((state == XA_PLAYSTATE_STOPPED) ||
        (state == XA_PLAYSTATE_PAUSED)) {
        xaRes = (*mPlayItf)->SetPlayState(mPlayItf, XA_PLAYSTATE_PLAYING);
        retVal = mapError(xaRes, ETrue);
        RET_ERR_IF_ERR(retVal);
    }
    return retVal;
}

TInt XAPlaySessionImpl::pause()
{
    TInt retVal(KErrGeneral);
    XAresult xaRes;
    XAuint32 state;

#ifdef USE_VIDEOPLAYERUTILITY
    TRAPD(err, mVideoPlayUtil->PauseL());
    return 0;
#endif

    if (!mMOPlayer || !mPlayItf)
        return retVal;

    xaRes = (*mPlayItf)->GetPlayState(mPlayItf, &state);
    retVal = mapError(xaRes, ETrue);
    RET_ERR_IF_ERR(retVal);

    if ((state == XA_PLAYSTATE_STOPPED) ||
        (state == XA_PLAYSTATE_PLAYING)) {
        xaRes = (*mPlayItf)->SetPlayState(mPlayItf, XA_PLAYSTATE_PAUSED);
        retVal = mapError(xaRes, ETrue);
        RET_ERR_IF_ERR(retVal);
    }

    return retVal;
}

TInt XAPlaySessionImpl::stop()
{
    TInt retVal(KErrGeneral);
    XAresult xaRes;
    XAuint32 state;

#ifdef USE_VIDEOPLAYERUTILITY
    mVideoPlayUtil->Stop();
    return 0;
#endif

    if (!mMOPlayer || !mPlayItf)
        return retVal;

    xaRes = (*mPlayItf)->GetPlayState(mPlayItf, &state);
    retVal = mapError(xaRes, ETrue);
    RET_ERR_IF_ERR(retVal);

    if ((state == XA_PLAYSTATE_PAUSED) ||
        (state == XA_PLAYSTATE_PLAYING)) {
        xaRes = (*mPlayItf)->SetPlayState(mPlayItf, XA_PLAYSTATE_STOPPED);
        retVal = mapError(xaRes, ETrue);
        RET_ERR_IF_ERR(retVal);
        mCurPosition += KPlayPosUpdatePeriod;
        mParent.cbPositionChanged(mCurPosition);
    }

    return retVal;
}

TInt XAPlaySessionImpl::duration(TInt64& aDur)
{
    TInt retVal(KErrGeneral);

#ifdef USE_VIDEOPLAYERUTILITY
    TTimeIntervalMicroSeconds dur(0);
    TRAPD(err, mVideoPlayUtil->DurationL());
    if (!err)
        aDur = dur.Int64() / 1000;
    return 0;
#endif
    if (!mMOPlayer || !mPlayItf)
        return retVal;
    aDur = mDuration;
    return retVal;
}

TInt XAPlaySessionImpl::position(TInt64& aPos)
{
    TInt retVal(KErrGeneral);
#ifdef USE_VIDEOPLAYERUTILITY
    TTimeIntervalMicroSeconds dur(0);
    TRAPD(err, mVideoPlayUtil->PositionL());
    if (!err)
        aPos = dur.Int64() / 1000;
    return 0;
#endif

/*    XAresult xaRes;
    XAmillisecond pos(0);*/

    if (!mMOPlayer || !mPlayItf)
        return retVal;

    aPos = mCurPosition;
    return retVal;
}

TInt XAPlaySessionImpl::getSeekable(TBool& seekable)
{
    TInt retVal(KErrGeneral);

    if (!mMOPlayer || !mSeekItf)
        return retVal;

    retVal = ETrue;
    seekable = ETrue;

    return retVal;
}

TInt XAPlaySessionImpl::seek(TInt64 pos)
{
    TInt retVal(KErrGeneral);
    XAresult xaRes;

    if (!mMOPlayer || !mSeekItf)
        return retVal;

    xaRes = (*mSeekItf)->SetPosition(mSeekItf, (XAmillisecond)pos, XA_SEEKMODE_FAST);
    retVal = mapError(xaRes, ETrue);
    RET_ERR_IF_ERR(retVal);
    mCurPosition = pos;
    
    return retVal;
}

void XAPlaySessionImpl::cbMediaPlayer(XAObjectItf /*caller*/,
                                         const void */*pContext*/,
                                         XAuint32 event,
                                         XAresult result,
                                         XAuint32 /*param*/,
                                         void */*pInterface*/) 

{
    switch (event) {
    case XA_OBJECT_EVENT_RESOURCES_LOST:
        unload();
        mParent.cbPlaybackStopped(result);
        break;
    case XA_OBJECT_EVENT_RUNTIME_ERROR:
    {
        switch (result) {
        case XA_RESULT_RESOURCE_LOST:
            unload();
            mParent.cbPlaybackStopped(result);
            break;
        default:
            break;
        }; /* of switch (result) */
    }
    default:
        break;
    } /* of switch (event) */
}

void XAPlaySessionImpl::cbPlayItf(XAPlayItf /*caller*/,
                                  void */*pContext*/,
                                  XAuint32 event)
{
    switch(event) {
    case XA_PLAYEVENT_HEADATEND:
        mParent.cbPlaybackStopped(KErrNone);
        break;
    case XA_PLAYEVENT_HEADATMARKER:
        break;
    case XA_PLAYEVENT_HEADATNEWPOS:
        mCurPosition += KPlayPosUpdatePeriod;
        mParent.cbPositionChanged(mCurPosition);
        break;
    case XA_PLAYEVENT_HEADMOVING:
        break;
    case XA_PLAYEVENT_HEADSTALLED:
        break;
    default:
        break;
    }
}

void XAPlaySessionImpl::cbPrefetchItf(XAuint32 event)
{
    if(event == XA_PREFETCHEVENT_FILLLEVELCHANGE)
        mParent.cbPrefetchStatusChanged();
}

void XAPlaySessionImpl::cbStreamInformationItf(    XAuint32 /*eventId*/,
                                                    XAuint32 /*streamIndex*/,
                                                    void * /*pEventData*/)
{
    mParent.cbStreamInformation(EFalse);
}

void XAPlaySessionImpl::setAspectRatioMode(Qt::AspectRatioMode aspectRatioMode)
{
    if( !mbScalable || 
        (mCurrAspectRatioMode == aspectRatioMode)) {
        return;
    }

    XAuint32 scaleOptions;;
    XAuint32 backgrndColor = 1;
    XAuint32 renderingHints = 1;
        
    switch(aspectRatioMode) {
    case Qt::IgnoreAspectRatio:
        scaleOptions = XA_VIDEOSCALE_STRETCH;
        break;
    case Qt::KeepAspectRatio:
        scaleOptions = XA_VIDEOSCALE_FIT;
        break;
    case Qt::KeepAspectRatioByExpanding:
        scaleOptions = XA_VIDEOSCALE_CROP;
        break;
    default:
        return;
    }

    XAresult xaRes = (*mVideoPostProcessingItf)->SetScaleOptions(mVideoPostProcessingItf, \
                                                scaleOptions, backgrndColor, renderingHints);
    if(mapError(xaRes, ETrue) == KErrNone)
        xaRes = (*mVideoPostProcessingItf)->Commit(mVideoPostProcessingItf);

    if(mapError(xaRes, ETrue) == KErrNone)
        mCurrAspectRatioMode = aspectRatioMode;
}

Qt::AspectRatioMode XAPlaySessionImpl::getAspectRatioMode()
{
    return mCurrAspectRatioMode;
}


#ifdef USE_VIDEOPLAYERUTILITY
void XAPlaySessionImpl::MvpuoOpenComplete(TInt aError)
{
    TRACE_FUNCTION_ENTRY;
    m_VPError = aError;
    if (mActiveSchedulerWait->IsStarted())
        mActiveSchedulerWait->AsyncStop();
    TRACE_FUNCTION_EXIT;
}

void XAPlaySessionImpl::MvpuoPrepareComplete(TInt aError)
{
    TRACE_FUNCTION_ENTRY;
    m_VPError = aError;
    if (mActiveSchedulerWait->IsStarted())
        mActiveSchedulerWait->AsyncStop();

    RWindow* window = (RWindow*)mNativeDisplay.hWindow;
    RWsSession* wssession = (RWsSession*)mNativeDisplay.hDisplay;
    
    if (window) {
        TRect videoExtent = TRect(window->Size());
        TRect clipRect = TRect(window->Size());        
        TRAP_IGNORE(mVideoPlayUtil->AddDisplayWindowL(*wssession, *(CCoeEnv::Static()->ScreenDevice()), *window, videoExtent, clipRect));
        TRAP_IGNORE(mVideoPlayUtil->SetAutoScaleL(*window, EAutoScaleBestFit));
    }
    TRACE_FUNCTION_EXIT;
}

void XAPlaySessionImpl::MvpuoFrameReady(CFbsBitmap& /*aFrame*/,TInt /*aError*/)
{
    TRACE_FUNCTION_ENTRY_EXIT;
}

void XAPlaySessionImpl::MvpuoPlayComplete(TInt /*aError*/)
{
    TRACE_FUNCTION_ENTRY;
    mParent.cbPlaybackStopped_EOS();
    TRACE_FUNCTION_EXIT;
}

void XAPlaySessionImpl::MvpuoEvent(const TMMFEvent& /*aEvent*/)
{
    TRACE_FUNCTION_ENTRY_EXIT;
}

#endif

TInt XAPlaySessionImpl::mapError(XAresult xa_err, TBool /*debPrn*/)
{
    TInt retVal(KErrGeneral);

    switch(xa_err) {
    case XA_RESULT_SUCCESS:
        retVal = KErrNone;
        break;
    case XA_RESULT_PRECONDITIONS_VIOLATED:
        break;
    case XA_RESULT_PARAMETER_INVALID:
        break;
    case XA_RESULT_MEMORY_FAILURE:
        break;
    case XA_RESULT_RESOURCE_ERROR:
        break;
    case XA_RESULT_RESOURCE_LOST:
        break;
    case XA_RESULT_IO_ERROR:
        break;
    case XA_RESULT_BUFFER_INSUFFICIENT:
        break;
    case XA_RESULT_CONTENT_CORRUPTED:
        break;
    case XA_RESULT_CONTENT_UNSUPPORTED:
        break;
    case XA_RESULT_CONTENT_NOT_FOUND:
        break;
    case XA_RESULT_PERMISSION_DENIED:
        break;
    case XA_RESULT_FEATURE_UNSUPPORTED:
        break;
    case XA_RESULT_INTERNAL_ERROR:
        break;
    case XA_RESULT_UNKNOWN_ERROR:
        break;
    case XA_RESULT_OPERATION_ABORTED:
        break;
    case XA_RESULT_CONTROL_LOST:
        break;
    default:
        break;
    }

    return retVal;
}


QStringList XAPlaySessionImpl::availableExtendedMetaData () const
{
    QStringList retList;

    //create a qlist with all keys in keyMap hash
    QHashIterator<QString, int> it(extendedKeyMap);
    while(it.hasNext())    {
        it.next();
        retList << it.key();
    }

    return retList;    
}

QList<QtMultimediaKit::MetaData>  XAPlaySessionImpl::availableMetaData () const
{
    QList<QtMultimediaKit::MetaData> retList;

    //create a qlist with all keys in keyMap hash
    QHashIterator<QtMultimediaKit::MetaData, int> it(keyMap);
    while( it.hasNext() ) {
        it.next();
        retList << it.key();
    }

    return retList;
}

QVariant XAPlaySessionImpl::getMetaData( int alIndex ) const
{
    QVariant ret; //invalid variant
    
    //find index for the given key
    if(mMetadataExtItf) {
        XAuint32 valueSize = 0;
        XAresult res = (*mMetadataExtItf)->GetValueSize(mMetadataExtItf, alIndex, &valueSize);
        if(res == XA_RESULT_SUCCESS) {
            XAMetadataInfo * value = (XAMetadataInfo*)calloc(valueSize, 1);
            if(value) {
                res = (*mMetadataExtItf)->GetValue(mMetadataExtItf, alIndex, valueSize, value);
                if(res == XA_RESULT_SUCCESS) {
                    if(value->encoding == XA_CHARACTERENCODING_ASCII)
                        ret = QVariant ((const char*)value->data);
                    else if(value->encoding == XA_CHARACTERENCODING_UTF16LE)
                        ret = QVariant(QString::fromUtf16((ushort*)value->data, (value->size/2)-1)); //dont include null terminating character
                    else if(value->encoding == XA_CHARACTERENCODING_BINARY)
                        ret = QVariant(QImage::fromData(value->data, value->size));
                }

                free(value);
            }        
        }
    }

    return ret;
}

QVariant XAPlaySessionImpl::metaData( QtMultimediaKit::MetaData key ) const
{
    QVariant ret;
    if(keyMap.contains(key))
        ret = getMetaData(keyMap[key]);
    return ret;
}

QVariant XAPlaySessionImpl::extendedMetaData(const QString & key ) const
{
    QVariant ret;
    if(extendedKeyMap.contains(key))
        ret = getMetaData(extendedKeyMap[key]);

    return ret;
}

bool XAPlaySessionImpl::isMetaDataAvailable() const
{
    return ((keyMap.size()>0) || (extendedKeyMap.size()>0));
}

bool XAPlaySessionImpl::isWritable() const
{
    return false;
}

void XAPlaySessionImpl::setExtendedMetaData( const QString&, const QVariant&)
{
    //Do Nothing
}

void XAPlaySessionImpl::setMetaData( QtMultimediaKit::MetaData, const QVariant& )
{
    //Do Nothing
}

void XAPlaySessionImpl::setupALKeyMap()
{
    alKeyMap["KhronosTitle"]          = QtMultimediaKit::Title;
    alKeyMap["KhronosComment"]        = QtMultimediaKit::Comment;
    alKeyMap["KhronosTrackNumber"]    = QtMultimediaKit::TrackNumber;
    alKeyMap["KhronosAlbumArtJPEG"]   = QtMultimediaKit::CoverArtImage;
    alKeyMap["KhronosAlbumArtPNG"]    = QtMultimediaKit::CoverArtImage;
    alKeyMap["KhronosAlbum"]          = QtMultimediaKit::AlbumTitle;
    alKeyMap["KhronosArtist"]         = QtMultimediaKit::AlbumArtist;
    alKeyMap["KhronosGenre"]          = QtMultimediaKit::Genre;
    alKeyMap["KhronosYear"]           = QtMultimediaKit::Year;
    alKeyMap["KhronosYear"]           = QtMultimediaKit::Date;
    alKeyMap["KhronosRating"]         = QtMultimediaKit::UserRating;
    alKeyMap["KhronosCopyright"]      = QtMultimediaKit::Copyright;
    alKeyMap["Author"]                = QtMultimediaKit::Author;
    alKeyMap["Duration"]              = QtMultimediaKit::Duration;
    alKeyMap["Stream Count"]          = QtMultimediaKit::ChannelCount;
    alKeyMap["Composer"]              = QtMultimediaKit::Composer;
    alKeyMap["Resolution"]            = QtMultimediaKit::Resolution;
    alKeyMap["FrameRate"]             = QtMultimediaKit::VideoFrameRate;
    alKeyMap["ClipBitRate"]           = QtMultimediaKit::VideoBitRate;
    alKeyMap["Codec"]                 = QtMultimediaKit::VideoCodec;
    alKeyMap["attachedpicture"]       = QtMultimediaKit::CoverArtImage;
    
    /*Keys not available    
        QtMedia::SubTitle            
        QtMedia::Description
        QtMedia::Category            
        QtMedia::Keywords            
        QtMedia::Language            
        QtMedia::Publisher            
        QtMedia::ParentalRating         
        QtMedia::RatingOrganisation            
        QtMedia::Size            
        QtMedia::MediaType            
        QtMedia::AudioBitrate            
        QtMedia::AudioCodec         
        QtMedia::AverageLevel            
        QtMedia::PeakValue            
        QtMedia::Frequency            
        QtMedia::ContributingArtist         
        QtMedia::Conductor            
        QtMedia::Lyrics         
        QtMedia::Mood            
        QtMedia::TrackCount         
        QtMedia::PixelAspectRatio            
        QtMedia::PosterUri            
        QtMedia::ChapterNumber            
        QtMedia::Director            
        QtMedia::LeadPerformer            
        QtMedia::Writer         */
}

TInt XAPlaySessionImpl::mapMetaDataKey(const char* asckey, QtMultimediaKit::MetaData& key)
{
    if(alKeyMap.contains(asckey)) {
        key = alKeyMap[asckey];
        return KErrNone;
    }
    
    return KErrNotFound;
}

TInt XAPlaySessionImpl::setupMetaData()
{
    XAresult res;
    if(mMetadataExtItf) {
        XAuint32 numItems = 0;
        res = (*mMetadataExtItf)->GetItemCount(mMetadataExtItf, &numItems);
        RET_ERR_IF_ERR(mapError(res, ETrue));

        for(int i=0; i<numItems; ++i) {
            XAuint32 keySize;
            res = (*mMetadataExtItf)->GetKeySize(mMetadataExtItf, i, &keySize);
            RET_ERR_IF_ERR(mapError(res, ETrue));

            XAMetadataInfo *key = (XAMetadataInfo *)calloc(keySize,1);
            if(key) {
                res = (*mMetadataExtItf)->GetKey(mMetadataExtItf, i, keySize, key);
                RET_ERR_IF_ERR(mapError(res, ETrue));

                if(key->encoding == XA_CHARACTERENCODING_ASCII) { //only handle ASCII keys ignore others
                    QtMultimediaKit::MetaData qtKey;
                    if(mapMetaDataKey((const char*)key->data, qtKey) == KErrNone)//qt metadata
                        keyMap[qtKey] = i;
                    else //extended metadata
                        extendedKeyMap[(const char*)key->data] = i;
                }

                free(key);
            }
        }

        //check for seek property to generate seekable signal
        QVariant var = extendedMetaData("Seekable");
        if(!var.isValid() || (var.toString() == "1"))
            mParent.cbSeekableChanged(ETrue);
        else
            mParent.cbSeekableChanged(EFalse);

    }

    return KErrGeneral;
}

//Volume
TInt XAPlaySessionImpl::volume(TInt& v)
{
    if(mbVolEnabled) {
        XAresult res = (*mNokiaLinearVolumeItf)->GetVolumeLevel(mNokiaLinearVolumeItf, (XAuint32 *)&v);    
        return mapError(res, ETrue);
    }

    return KErrNotFound;
}

TInt XAPlaySessionImpl::setVolume(TInt v)
{
    if(mbVolEnabled) {
        XAresult res = (*mNokiaLinearVolumeItf)->SetVolumeLevel(mNokiaLinearVolumeItf, (XAuint32*)&v);
        return mapError(res, ETrue);
    }

    return KErrNotFound;
}

TInt XAPlaySessionImpl::setMute(TBool bMute)
{
    if(mbMuteEnabled) {
        XAresult res = (*mNokiaVolumeExtItf)->SetMute(mNokiaVolumeExtItf, (XAboolean)bMute);
        return mapError(res, ETrue);
    }

    return KErrNotFound;

}

TInt XAPlaySessionImpl::getMute(TBool& bIsMute)
{
    if(mbMuteEnabled) {
        XAboolean xaMute;
        XAresult res = (*mNokiaVolumeExtItf)->GetMute(mNokiaVolumeExtItf, &xaMute);
        bIsMute = xaMute;
        return mapError(res, ETrue);
    }

    return KErrNotFound;
}


TInt XAPlaySessionImpl::bufferStatus(TInt &bs)
{
    TInt ret = KErrNotFound;
    
    if(mbPrefetchStatusChange) {
        XApermille satusPerThousand;
        XAresult res = (*mPrefetchStatusItf)->GetFillLevel(mPrefetchStatusItf, &satusPerThousand);
        ret = mapError(res, ETrue);
        if(ret == KErrNone)
            bs = satusPerThousand/10.0; //convert to parts per hundred
    }
    return ret;
}

QMediaStreamsControl::StreamType XAPlaySessionImpl::mapStreamType(XAuint32& alStreamType)
{
    switch(alStreamType) {
    case XA_DOMAINTYPE_AUDIO: 
        return QMediaStreamsControl::AudioStream;
    case XA_DOMAINTYPE_VIDEO: 
        return QMediaStreamsControl::VideoStream;
    case XA_DOMAINTYPE_IMAGE: 
        return QMediaStreamsControl::DataStream;
    }
    return QMediaStreamsControl::UnknownStream;
}


TInt XAPlaySessionImpl::numMediaStreams(TUint& numStreams)
{
    TInt ret = KErrNotFound;
    numStreams = 0;
    if(mbStreamInfoAvailable) {
        XAMediaContainerInformation mediaContainerInfo;
        XAresult res = (*mStreamInformationItf)->QueryMediaContainerInformation(mStreamInformationItf, &mediaContainerInfo);
        ret = mapError(res, ETrue);
        if(ret == KErrNone)
            numStreams = mediaContainerInfo.numStreams;
    }
    return ret;
}

TInt XAPlaySessionImpl::streamType(TUint index, QMediaStreamsControl::StreamType& type)
{
    TInt ret = KErrNotFound;
    type = QMediaStreamsControl::UnknownStream; 
    if(mbStreamInfoAvailable) {
        XAuint32 strType;
        XAresult res = (*mStreamInformationItf)->QueryStreamType(mStreamInformationItf, (XAuint32)(index+1), &strType);
        ret = mapError(res, ETrue);
        if(ret == KErrNone)
            type = mapStreamType(strType);
    }
    return ret;
}

TInt XAPlaySessionImpl::isStreamActive(TUint index, TBool& isActive)
{
    TUint numStreams;
    TInt ret = numMediaStreams(numStreams);
    if((ret == KErrNone) && (index < numStreams)) {
        isActive = EFalse;
        if(numStreams > 0)     {
            //create array of bools 
            XAboolean *activeStreams = new XAboolean[numStreams+1];
            XAresult res = (*mStreamInformationItf)->QueryActiveStreams(mStreamInformationItf, (XAuint32*)&numStreams, activeStreams);
            ret = mapError(res, ETrue);
            if(ret == KErrNone)
                isActive = activeStreams[index+1];
            delete[] activeStreams;
        }
    }
    return ret;
}

TInt XAPlaySessionImpl::getPlaybackRate(TReal32 &rate)
{
    TInt ret = KErrNotFound;
    
    if(mbPlaybackRateItfAvailable) {
        XApermille perMilleRate = 0;
        ret = (*mPlaybackRateItf)->GetRate(mPlaybackRateItf, &perMilleRate);
        rate = perMilleRate / 1000.0;
    }
    return ret;
}

TInt XAPlaySessionImpl::setPlaybackRate(TReal32 rate)
{
    TInt ret = KErrNotFound;
    if(mbPlaybackRateItfAvailable)
        ret = (*mPlaybackRateItf)->SetRate(mPlaybackRateItf, (XApermille)(rate * 1000.0));
    return ret;
}


/* Local function implementation */
void MediaPlayerCallback(   XAObjectItf caller,
                                const void *pContext,
                                XAuint32 event,
                                XAresult result,
                                XAuint32 param,
                                void *pInterface)
{
    if (pContext)
        ((XAPlaySessionImpl*)pContext)->cbMediaPlayer(  caller,
                                                        pContext,
                                                        event,
                                                        result,
                                                        param,
                                                        pInterface);
}

void PlayItfCallback(  XAPlayItf caller,
                        void *pContext,
                        XAuint32 event)
{
    if (pContext)
        ((XAPlaySessionImpl*)pContext)->cbPlayItf(caller,
                    pContext,
                    event);
}

void PrefetchItfCallback( XAPrefetchStatusItf /*caller*/,
                            void *pContext,
                            XAuint32 event)
{
    if (pContext)
        ((XAPlaySessionImpl*)pContext)->cbPrefetchItf(event);
}

void StreamInformationItfCallback(  XAStreamInformationItf /*caller*/,
                                        XAuint32 eventId,
                                        XAuint32 streamIndex,
                                        void * pEventData,
                                        void * pContext)
{
    if (pContext)
        ((XAPlaySessionImpl*)pContext)->cbStreamInformationItf( eventId, 
                                                                streamIndex, 
                                                                pEventData);
}



// End of file
