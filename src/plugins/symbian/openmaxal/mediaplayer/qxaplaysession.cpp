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

#include <QMetaType>
#include "qxaplaysession.h"
#include "xaplaysessionimpl.h"
#include "qxacommon.h"
#include <COECNTRL.H>

QXAPlaySession::QXAPlaySession(QObject *parent):QObject(parent),
m_state(QMediaPlayer::StoppedState),
m_mediaStatus(QMediaPlayer::NoMedia),
mSeekable(-1),
mNumStreams(0),
mbAudioAvailable(EFalse),
mbVideoAvailable(EFalse),    
mImpl(NULL),
mVideowidgetControl(NULL),
mWidgetCtrlWindow(NULL),
mWidgetCtrlWindowId(NULL),
mVideoWindowControl(NULL),
mWindowCtrlWindow(NULL),
mWindowCtrlWindowId(NULL),
mWsSession(&(CCoeEnv::Static()->WsSession()))
{
    QT_TRACE_FUNCTION_ENTRY;
    mImpl = new XAPlaySessionImpl(*this);

    if (mImpl && (mImpl->postConstruct() != KErrNone)) {
        delete mImpl;
        mImpl = NULL;
        QT_TRACE1("Error initializing implementation");
    }

    if (!mImpl)
        emit error(QMediaPlayer::ResourceError, tr("Service has not been started"));

    QT_TRACE_FUNCTION_EXIT;
}

QXAPlaySession::~QXAPlaySession()
{
    QT_TRACE_FUNCTION_ENTRY;
    delete mImpl;
    QT_TRACE_FUNCTION_EXIT;
}

void QXAPlaySession::setVideoWidgetControl( QXAVideoWidgetControl * videoWidgetControl )
{
    QT_TRACE_FUNCTION_ENTRY;
    if (mVideowidgetControl) {
        disconnect(mVideowidgetControl, SIGNAL(widgetUpdated()),
                    this, SLOT(videoWidgetControlWidgetUpdated()));
        RWindow* window = static_cast<RWindow*>(mVideowidgetControl->videoWidgetWId()->DrawableWindow());
        mImpl->removeNativeDisplay(window, mWsSession);
    }
    mVideowidgetControl = videoWidgetControl;
    if (mVideowidgetControl)
        connect(mVideowidgetControl, SIGNAL(widgetUpdated()),
                    this, SLOT(videoWidgetControlWidgetUpdated()));
    QT_TRACE_FUNCTION_EXIT;
}

void QXAPlaySession::unsetVideoWidgetControl( QXAVideoWidgetControl * videoWidgetControl )
{
    QT_TRACE_FUNCTION_ENTRY;
    if ((mVideowidgetControl == videoWidgetControl) && (mImpl)) {
        disconnect(mVideowidgetControl, SIGNAL(widgetUpdated()),
                    this, SLOT(videoWidgetControlWidgetUpdated()));
        RWindow* window = static_cast<RWindow*>(mVideowidgetControl->videoWidgetWId()->DrawableWindow());
        mImpl->removeNativeDisplay(window, mWsSession);
    }
    mVideowidgetControl = NULL;
    mWidgetCtrlWindow = NULL;
    mWidgetCtrlWindowId = NULL;
    QT_TRACE_FUNCTION_EXIT;
}

void QXAPlaySession::setVideoWindowControl( QXAVideoWindowControl * videoWindowControl )
{
    QT_TRACE_FUNCTION_ENTRY;
    if (mVideoWindowControl) {
        disconnect(mVideoWindowControl, SIGNAL(windowUpdated()),
                    this, SLOT(videoWindowControlWindowUpdated()));
        RWindow* window = static_cast<RWindow*>(mVideoWindowControl->winId()->DrawableWindow());
        mImpl->removeNativeDisplay(window, mWsSession);
    }
    mVideoWindowControl = videoWindowControl;
    if (mVideoWindowControl) {
        connect(mVideoWindowControl, SIGNAL(windowUpdated()),
                    this, SLOT(videoWindowControlWindowUpdated()));
        videoWindowControlWindowUpdated();
    }
    QT_TRACE_FUNCTION_EXIT;
}

void QXAPlaySession::unsetVideoWindowControl( QXAVideoWindowControl * videoWindowControl )
{
    QT_TRACE_FUNCTION_ENTRY;
    if ((mVideoWindowControl == videoWindowControl) && (mImpl)) {
        disconnect(mVideoWindowControl, SIGNAL(windowUpdated()),
                    this, SLOT(videoWindowControlWindowUpdated()));
        RWindow* window = static_cast<RWindow*>(mVideoWindowControl->winId()->DrawableWindow());
        mImpl->removeNativeDisplay(window, mWsSession);
    }
    mVideoWindowControl = NULL;
    mWindowCtrlWindow = NULL;
    mWindowCtrlWindowId = NULL;
    QT_TRACE_FUNCTION_EXIT;
}

qint64 QXAPlaySession::duration()
{
    TInt64 dur(0);
    if (mImpl)
        mImpl->duration(dur);

    return (qint64)dur;
}

qint64 QXAPlaySession::position()
{
    TInt64 pos(0);
    if (mImpl)
        mImpl->position(pos);

    return (qint64)pos;
}

void QXAPlaySession::setPosition(qint64 ms)
{
    if (mImpl) {
        qint64 currPos = position();
        mImpl->seek(ms);

        if(currPos != position()) {
             emit positionChanged(position());
             
             if(position()>=duration()) {
                 setMediaStatus(QMediaPlayer::EndOfMedia);
                 stop();
             }
        }
    }
}

int QXAPlaySession::volume()
{
    if(mImpl) {
        TInt v(0);
        
        TInt err = mImpl->volume(v);    
        if(KErrNone == err) 
            return v;
    }

    return 50;
}

void QXAPlaySession::setVolume(int v)
{
    if(mImpl) {
        if(v != volume()) {
            TInt err  = mImpl->setVolume(v);
            if(KErrNone == err) 
                emit volumeChanged(volume());
        }
    }
}


bool QXAPlaySession::isMuted()
{
    if(mImpl) {
        TBool bCurrMute = EFalse;
        TInt err = mImpl->getMute(bCurrMute);
        if(err == KErrNone)
            return bCurrMute;
    }
    
    return EFalse;
}

void QXAPlaySession::setMuted(bool muted)
{
    if(muted != isMuted())
    {
        if(mImpl)
        {
            TInt err = mImpl->setMute(muted);            

            if(KErrNone == err)
            {
                emit mutedChanged(muted);
            }
        }
    }
}

int QXAPlaySession::bufferStatus()
{
    if(mImpl) {
        TInt fillLevel = 0;
        TInt err = mImpl->bufferStatus(fillLevel);
        if(err == KErrNone)
            return fillLevel;
    }

    return 100;//default
}

bool QXAPlaySession::isAudioAvailable()
{
    return mbAudioAvailable;
}

bool QXAPlaySession::isVideoAvailable()
{
    return mbVideoAvailable;
}

bool QXAPlaySession::isSeekable()
{
    return ((mSeekable==1) || (mSeekable==-1));//default seekable
}

float QXAPlaySession::playbackRate()
{
    if(mImpl) {
        TReal32 currPBRate = 0.0;
        TInt ret = mImpl->getPlaybackRate(currPBRate);
        if(ret == KErrNone)
            return currPBRate;
    }

    return 1.0;
}

void QXAPlaySession::setPlaybackRate(float rate)
{
    if(mImpl) {
        TReal32 currPBRate = 0.0;
        TInt ret = mImpl->getPlaybackRate(currPBRate);
        if(    (ret == KErrNone) && 
            (rate!=currPBRate)) {
            ret = mImpl->setPlaybackRate(rate);
            if(ret == KErrNone)
                emit playbackRateChanged(rate);
        }
    }
}

QMediaContent QXAPlaySession::media()
{
    QT_TRACE_FUNCTION_ENTRY_EXIT;
    return mMediaContent;
}

void QXAPlaySession::setMedia(const QMediaContent& media)
{
    QT_TRACE_FUNCTION_ENTRY;
    RET_IF_p_IS_NULL_EMIT_PLAYER_RESOURCE_ERROR(mImpl);

    if (media.isNull() || 
        mMediaContent == media) {
        return;
    }

    setMediaStatus(QMediaPlayer::NoMedia);

    QString urlStr = media.canonicalUrl().toString();
    TPtrC16 urlPtr(reinterpret_cast<const TUint16*>(urlStr.utf16()));

    setMediaStatus(QMediaPlayer::LoadingMedia);
    if (mImpl->load(urlPtr) == 0) {
        setMediaStatus(QMediaPlayer::LoadedMedia);
        emit error(QMediaPlayer::NoError, "");
        mMediaContent = media;
        setPlayerState(QMediaPlayer::StoppedState);    
        emit mediaChanged(mMediaContent);
        
        if(mImpl->isMetaDataAvailable()) {
            emit metaDataAvailableChanged(true);
            emit metaDataChanged();
        }
        else {
            emit metaDataAvailableChanged(false);
        }
    }
    else {
        setMediaStatus(QMediaPlayer::NoMedia);
        emit error(QMediaPlayer::ResourceError, tr("Unable to load media"));
    }
    QT_TRACE_FUNCTION_EXIT;
}

void QXAPlaySession::play()
{
    if (mImpl) {
        setMediaStatus(QMediaPlayer::BufferingMedia);

        TInt err = mImpl->play();
        if (err != KErrNone) {
            setMediaStatus(QMediaPlayer::NoMedia);
            RET_IF_ERROR(err);
        }
        setPlayerState(QMediaPlayer::PlayingState);

        TInt fillLevel = 0;
        err = mImpl->bufferStatus(fillLevel);
        RET_IF_ERROR(err);
        if (fillLevel == 100) {
            setMediaStatus(QMediaPlayer::BufferedMedia);
        }
    }
}

void QXAPlaySession::pause()    
{
    if (mImpl) {
        TInt err = mImpl->pause();
        RET_IF_ERROR(err);
        setPlayerState(QMediaPlayer::PausedState);
    }
}

void QXAPlaySession::stop()
{
    if (mImpl) {
        TInt err = mImpl->stop();
        RET_IF_ERROR(err);
        setPlayerState(QMediaPlayer::StoppedState);
    }
}

void QXAPlaySession::cbDurationChanged(TInt64 new_dur)
{
    emit durationChanged((qint64)new_dur);
}

void QXAPlaySession::cbPositionChanged(TInt64 new_pos)
{
    emit positionChanged((qint64)new_pos);
}

void QXAPlaySession::cbSeekableChanged(TBool seekable)
{
    if(    (mSeekable==-1) || 
        (seekable != (TBool)mSeekable)) {
        mSeekable = seekable?1:0;
        emit seekableChanged((bool)seekable);
    }
}

void QXAPlaySession::cbPlaybackStopped(TInt err)
{
    if (err) {
        emit error(QMediaPlayer::ResourceError, tr("Resources Unavailable"));
        SIGNAL_EMIT_TRACE1("emit error(QMediaPlayer::ResourceError, tr(\"Resources Unavailable\"))");
        emit positionChanged(position());
        setPlayerState(QMediaPlayer::StoppedState); 
        setMediaStatus(QMediaPlayer::NoMedia);
    }
    else {
        setMediaStatus(QMediaPlayer::EndOfMedia);
        /* Set player state to Stopped */
        stop();
    }
}

void QXAPlaySession::cbPrefetchStatusChanged()
{
    if(mImpl) {
        TInt fillLevel = 0;
        TInt err = mImpl->bufferStatus(fillLevel);
        if(err == KErrNone) {
            emit bufferStatusChanged(fillLevel);

            if(fillLevel == 100)
                setMediaStatus(QMediaPlayer::BufferedMedia);
            else if(fillLevel ==0)
                setMediaStatus(QMediaPlayer::StalledMedia);
        }
    }
}

void QXAPlaySession::cbStreamInformation(TBool bFirstTime)
{
    updateStreamInfo(bFirstTime);
}



void QXAPlaySession::videoWidgetControlWidgetUpdated()
{
    QT_TRACE_FUNCTION_ENTRY;
    if (mVideowidgetControl) {
        WId newId = mVideowidgetControl->videoWidgetWId();
        if ((newId != NULL) && (newId != mWidgetCtrlWindowId)) { 
            mWidgetCtrlWindow = static_cast<RWindow*>(newId->DrawableWindow());
            if (mWidgetCtrlWindowId == NULL)
                mImpl->addNativeDisplay(mWidgetCtrlWindow, mWsSession);
            else
                mImpl->updateNativeDisplay(mWidgetCtrlWindow, mWsSession);
            mWidgetCtrlWindowId = newId;
        }
    }
    QT_TRACE_FUNCTION_EXIT;
}

void QXAPlaySession::videoWindowControlWindowUpdated()
{
    QT_TRACE_FUNCTION_ENTRY;
    if (mVideoWindowControl) {
        WId newId = mVideoWindowControl->winId();
        if ((newId != NULL) && (newId != mWindowCtrlWindowId)) { 
            mWidgetCtrlWindow = static_cast<RWindow*>(newId->DrawableWindow());
            if (mWindowCtrlWindowId == NULL)
                mImpl->addNativeDisplay(mWidgetCtrlWindow, mWsSession);
            else
                mImpl->updateNativeDisplay(mWidgetCtrlWindow, mWsSession);
            mWindowCtrlWindowId = newId;
        }
    }
    QT_TRACE_FUNCTION_EXIT;
}

void QXAPlaySession::setMediaStatus(QMediaPlayer::MediaStatus status)
{
    if (m_mediaStatus != status) {
        m_mediaStatus = status;
        emit mediaStatusChanged(status);
    }
}

void QXAPlaySession::setPlayerState(QMediaPlayer::State state)
{
    if (m_state != state) {
        m_state = state;
        emit stateChanged(m_state);
    }
}

QStringList QXAPlaySession::availableExtendedMetaData () const
{
    QStringList list;
    RET_s_IF_p_IS_NULL(mImpl, list);
    list = mImpl->availableExtendedMetaData();
    return list;
}

QList<QtMultimediaKit::MetaData>  QXAPlaySession::availableMetaData () const
{
    QList<QtMultimediaKit::MetaData> list;
    RET_s_IF_p_IS_NULL(mImpl, list);
    return mImpl->availableMetaData();
}

QVariant QXAPlaySession::extendedMetaData(const QString & key ) const
{
    QVariant var;
    RET_s_IF_p_IS_NULL(mImpl, var);
    return mImpl->extendedMetaData(key);
}

bool QXAPlaySession::isMetaDataAvailable() const
{
    RET_s_IF_p_IS_NULL(mImpl, false);
    return mImpl->isMetaDataAvailable();
}

bool QXAPlaySession::isWritable() const
{
    RET_s_IF_p_IS_NULL(mImpl, false);
    return mImpl->isWritable();
}

QVariant QXAPlaySession::metaData( QtMultimediaKit::MetaData key ) const
{
    QVariant var;
    RET_s_IF_p_IS_NULL(mImpl, var);
    return mImpl->metaData(key);
}

void QXAPlaySession::setExtendedMetaData( const QString & key, const QVariant & value )
{
    RET_IF_p_IS_NULL(mImpl);
    mImpl->setExtendedMetaData(key, value);
}

void QXAPlaySession::setMetaData( QtMultimediaKit::MetaData key, const QVariant & value )
{
    RET_IF_p_IS_NULL(mImpl);
    mImpl->setMetaData(key, value);
}

void QXAPlaySession::updateStreamInfo(TBool emitSignal)
{
    if(mImpl) {
        mNumStreams = 0;
        TInt ret = mImpl->numMediaStreams(mNumStreams);
        if(ret == KErrNone) {
            TBool bAudioAvailable = EFalse;//lcoal variable
            TBool bVideoAvailable = EFalse;//lcvoal variable
            
            for(TUint i = 0; i < mNumStreams; i++) {
                QMediaStreamsControl::StreamType strType;
                mImpl->streamType(i, strType);
                if(strType == QMediaStreamsControl::AudioStream)
                    bAudioAvailable = ETrue;
                else if(strType == QMediaStreamsControl::VideoStream)
                    bVideoAvailable = ETrue;
            }

            if(emitSignal || (bAudioAvailable != mbAudioAvailable)) {
                emit audioAvailableChanged(bAudioAvailable);
                mbAudioAvailable = bAudioAvailable;
            }

            if(emitSignal || (bVideoAvailable != mbVideoAvailable)) {
                emit videoAvailableChanged(bVideoAvailable);
                mbVideoAvailable = bVideoAvailable;
            }

            emit streamsChanged();
        }    
    }
}

bool QXAPlaySession::isStreamActive ( int stream ) 
{
    RET_s_IF_p_IS_NULL(mImpl, false);
    TBool isActive = EFalse;
    mImpl->isStreamActive(stream,isActive);
    return isActive;
}

QVariant QXAPlaySession::metaData ( int /*stream*/, QtMultimediaKit::MetaData key )
{
    return this->metaData(key);
}

int QXAPlaySession::streamCount()
{
    return mNumStreams;
}

QMediaStreamsControl::StreamType QXAPlaySession::streamType ( int stream )
{
    QMediaStreamsControl::StreamType strType = QMediaStreamsControl::UnknownStream;
     RET_s_IF_p_IS_NULL(mImpl, strType);
    if(mImpl->streamType(stream, strType) == KErrNone) {
        return strType;
    }

    return QMediaStreamsControl::UnknownStream;
}    

////AspectRatioMode
void QXAPlaySession::setAspectRatioMode(Qt::AspectRatioMode aspectRatioMode)
{
    RET_IF_p_IS_NULL(mImpl);
    mImpl->setAspectRatioMode(aspectRatioMode);
}

Qt::AspectRatioMode QXAPlaySession::getAspectRatioMode()
{
    RET_s_IF_p_IS_NULL(mImpl, Qt::KeepAspectRatio);
    return mImpl->getAspectRatioMode();
}


