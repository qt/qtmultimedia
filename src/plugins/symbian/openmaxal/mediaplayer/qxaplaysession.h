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

#ifndef QXAPLAYSESSION_H
#define QXAPLAYSESSION_H

#include <QObject>
#include <QUrl>
#include <qtmedianamespace.h>

#include "qxamediaplayercontrol.h"
#include "qxametadatacontrol.h"
#include "qmediaplayer.h"
#include "xaplaysessioncommon.h"
#include "qxavideowidgetcontrol.h"
#include "qxavideowindowcontrol.h"
#include "qmediastreamscontrol.h"


QT_USE_NAMESPACE

class XAPlaySessionImpl;
class RWindow;
class RWsSession;
class QXAVideoWidgetControl;

class QXAPlaySession : public QObject,
                      public XAPlayObserver
{
    Q_OBJECT
public:
    QXAPlaySession(QObject *parent);
    virtual ~QXAPlaySession();
    
    void setVideoWidgetControl( QXAVideoWidgetControl * videoWidgetControl );
    void unsetVideoWidgetControl( QXAVideoWidgetControl * videoWidgetControl );
    void setVideoWindowControl( QXAVideoWindowControl * videoWindowControl );
    void unsetVideoWindowControl( QXAVideoWindowControl * videoWindowControl );

    //QXAMediaPlayerControl
    QMediaPlayer::State state() const { return m_state; }
    QMediaPlayer::MediaStatus mediaStatus() const { return m_mediaStatus; }
    qint64 duration();
    qint64 position();
    void setPosition(qint64 position);
    int volume();
    void setVolume(int volume);
    bool isMuted();
    void setMuted(bool muted);
    int bufferStatus();
    bool isAudioAvailable();
    bool isVideoAvailable();
    bool isSeekable();
    float playbackRate();
    void setPlaybackRate(float rate);
    QMediaContent media();
    void setMedia(const QMediaContent& media);
    void play();
    void pause();
    void stop();
    
    // Callback from XAPlaySessionImpl 
    void cbDurationChanged(TInt64 new_dur);
    void cbPositionChanged(TInt64 new_pos);
    void cbSeekableChanged(TBool seekable);
    void cbPlaybackStopped(TInt error);
    void cbPrefetchStatusChanged();
    void cbStreamInformation(TBool);

    //MetadataControl methods
    QStringList availableExtendedMetaData () const;
    QList<QtMultimediaKit::MetaData>  availableMetaData () const;
    QVariant extendedMetaData(const QString & key ) const;
    bool isMetaDataAvailable() const; 
    bool isWritable() const; 
    QVariant metaData( QtMultimediaKit::MetaData key ) const;
    void setExtendedMetaData( const QString & key, const QVariant & value );
    void setMetaData( QtMultimediaKit::MetaData key, const QVariant & value ); 

    //QMediaStreamsControl
    bool isStreamActive ( int stream ) ;
    QVariant metaData ( int stream, QtMultimediaKit::MetaData key );
    int streamCount();
    QMediaStreamsControl::StreamType streamType ( int stream );

    //QVideoWidgetControl
    void setAspectRatioMode(Qt::AspectRatioMode);
    Qt::AspectRatioMode getAspectRatioMode();

public Q_SLOTS:
    void videoWidgetControlWidgetUpdated();
    void videoWindowControlWindowUpdated();
    
Q_SIGNALS:
    void mediaChanged(const QMediaContent& content);
    void durationChanged(qint64 duration);
    void positionChanged(qint64 position);
    void stateChanged(QMediaPlayer::State newState);
    void mediaStatusChanged(QMediaPlayer::MediaStatus status);
    void volumeChanged(int volume);
    void mutedChanged(bool muted);
    void audioAvailableChanged(bool audioAvailable);
    void videoAvailableChanged(bool videoAvailable);
    void bufferStatusChanged(int percentFilled);
    void seekableChanged(bool);
    void availablePlaybackRangesChanged(const QMediaTimeRange&);
    void error(int errorCode, const QString &errorString);
    void playbackRateChanged(qreal rate);
    
    //metadata
    void metaDataAvailableChanged(bool);
    void metaDataChanged();
    void writableChanged(bool);

    //QMediaStreamsControl
    void streamsChanged();
    void activeStreamsChanged();

private:
    void setMediaStatus(QMediaPlayer::MediaStatus);
    void setPlayerState(QMediaPlayer::State state);
    void updateStreamInfo(TBool emitSignal = EFalse);
    
private:
    QMediaPlayer::State m_state;
    QMediaPlayer::MediaStatus m_mediaStatus;

    QMap<QString,QVariant> m_tags;
    QList< QMap<QString,QVariant> > m_streamProperties;

    //seekable
    int mSeekable; //-1 =unintialized, 0=nonseekable, 1=seekable

    //StreamInfo
    TUint  mNumStreams;
    TBool mbAudioAvailable;
    TBool mbVideoAvailable;

    //Own
    XAPlaySessionImpl* mImpl;
    
    // Does not own
    QXAVideoWidgetControl *mVideowidgetControl;
    RWindow *mWidgetCtrlWindow;
    WId mWidgetCtrlWindowId;
    QXAVideoWindowControl *mVideoWindowControl;
    RWindow *mWindowCtrlWindow;
    WId mWindowCtrlWindowId;
    RWsSession *mWsSession;

    QMediaContent mMediaContent;
};

#endif /* QXAPLAYSESSION_H */
