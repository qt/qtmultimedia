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

#ifndef XAPLAYSESSIONIMPL_H
#define XAPLAYSESSIONIMPL_H

#include <OpenMAXAL.h>
#include <xanokiavolumeextitf.h>
#include <xanokialinearvolumeitf.h>
#ifdef USE_VIDEOPLAYERUTILITY
#include <VideoPlayer2.h>
#endif

#include "qtmedianamespace.h"
#include "qmediastreamscontrol.h"

class XAPlayObserver;
class RWindow;
class RWsSession;

class XAPlaySessionImpl
#ifdef USE_VIDEOPLAYERUTILITY
                      : public MVideoPlayerUtilityObserver
#endif
{
public:
    XAPlaySessionImpl(XAPlayObserver& parent);
    ~XAPlaySessionImpl();
    TInt postConstruct();
    TInt addNativeDisplay(RWindow* window, RWsSession* wssession);
    TInt updateNativeDisplay(RWindow* window, RWsSession* wssession);
    TInt removeNativeDisplay(RWindow* window, RWsSession* wssession);
    TInt load(const TDesC& aURI);
    void unload();
    TInt play();
    TInt pause();
    TInt stop();
    TInt duration(TInt64& aDur);
    TInt position(TInt64& aPos);
    TInt getSeekable(TBool& seekable);
    TInt seek(TInt64 pos);

    //Metadata    
    QStringList availableExtendedMetaData () const;
    QList<QtMultimediaKit::MetaData>  availableMetaData () const;
    QVariant extendedMetaData(const QString & key ) const;
    bool isMetaDataAvailable() const; 
    bool isWritable() const; 
    QVariant metaData( QtMultimediaKit::MetaData key ) const;
    void setExtendedMetaData( const QString & key, const QVariant & value );
    void setMetaData( QtMultimediaKit::MetaData key, const QVariant & value ); 

    TInt volume(TInt&);
    TInt setVolume(TInt);
    TInt setMute(TBool);
    TInt getMute(TBool&);

    TInt bufferStatus(TInt &);

    
    TInt numMediaStreams(TUint& numStreams);
    TInt streamType(TUint index, QMediaStreamsControl::StreamType& type);
    TInt isStreamActive(TUint index, TBool& isActive);

    TInt getPlaybackRate(TReal32 &rate);
    TInt setPlaybackRate(TReal32 rate);

    //AspectRatioMode
    void setAspectRatioMode(Qt::AspectRatioMode);
    Qt::AspectRatioMode getAspectRatioMode();

public:
    void cbMediaPlayer( XAObjectItf caller,
                        const void *pContext,
                        XAuint32 event,
                        XAresult result,
                        XAuint32 param,
                        void *pInterface);

    void cbPlayItf(XAPlayItf caller,
                   void *pContext,
                   XAuint32 event);

    
    void cbPrefetchItf(XAuint32);
    
    void cbStreamInformationItf(XAuint32, XAuint32, void*);

#ifdef USE_VIDEOPLAYERUTILITY
    //MVideoPlayerUtilityObserver
    void MvpuoOpenComplete(TInt aError);
    void MvpuoPrepareComplete(TInt aError);
    void MvpuoFrameReady(CFbsBitmap& aFrame,TInt aError);
    void MvpuoPlayComplete(TInt aError);
    void MvpuoEvent(const TMMFEvent& aEvent);
#endif

private:
    TInt mapError(XAresult xa_err,
                  TBool debPrn);
    void setupALKeyMap();
    TInt setupMetaData();
    TInt mapMetaDataKey(const char* asckey, QtMultimediaKit::MetaData& key);
    QVariant getMetaData( int alIndex ) const;

    QMediaStreamsControl::StreamType mapStreamType(XAuint32& alStreamType);

        
private:
    XAPlayObserver& mParent;
    XAObjectItf mEOEngine;
    XAObjectItf mMOPlayer;
    XAPlayItf mPlayItf;
    XASeekItf mSeekItf;
    HBufC8* mURIName;
    HBufC8* mWAVMime;
    // Audio Source
      XADataSource mDataSource;
     XADataFormat_MIME mMime;
       XADataLocator_URI mUri;
    //Audio Sink
    XADataSink mAudioSink;
    XADataLocator_IODevice mLocatorOutputDevice;
    //Video Sink
    XADataSink mVideoSink;
    XADataLocator_NativeDisplay mNativeDisplay;
    
    //Metadata
    TBool mbMetadataAvailable;
    XAMetadataExtractionItf mMetadataExtItf;
    QHash<QString, QtMultimediaKit::MetaData> alKeyMap;
    QHash<QtMultimediaKit::MetaData, int> keyMap;
    QHash<QString,int> extendedKeyMap;

    //Volume
    XANokiaLinearVolumeItf  mNokiaLinearVolumeItf;
    TBool mbVolEnabled;
    XANokiaVolumeExtItf mNokiaVolumeExtItf;
    TBool mbMuteEnabled;

    //buffer status
    XAPrefetchStatusItf mPrefetchStatusItf;
    TBool mbPrefetchStatusChange;

    //stream information
    XAStreamInformationItf mStreamInformationItf;
    TBool mbStreamInfoAvailable;
    TBool mbAudioStream;
    TBool mbVideoStream;
    TInt  mNumStreams;

    //Playbackrate
    XAPlaybackRateItf mPlaybackRateItf;
    TBool mbPlaybackRateItfAvailable;

    XAVideoPostProcessingItf mVideoPostProcessingItf;
    TBool mbScalable;
    Qt::AspectRatioMode mCurrAspectRatioMode;
    
    //internal
    TInt64  mCurPosition; // in milliseconds
    TInt64  mDuration; // in milliseconds
    
    TBool   mbMediaPlayerUnrealized;
#ifdef USE_VIDEOPLAYERUTILITY
    CVideoPlayerUtility2* mVideoPlayUtil;
    CActiveSchedulerWait* mActiveSchedulerWait;
    TInt m_VPError;
#endif
};

#endif /* XAPLAYSESSIONIMPL_H */
