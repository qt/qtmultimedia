// Copyright (C) 2016 The Qt Company Ltd and/or its subsidiary(-ies).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "avfmediaplayer_p.h"
#include "avfvideorenderercontrol_p.h"
#include <avfvideosink_p.h>
#include <avfmetadata_p.h>

#include "qaudiooutput.h"
#include "private/qplatformaudiooutput_p.h"

#include <qpointer.h>
#include <QFileInfo>
#include <QtCore/qmath.h>

#import <AVFoundation/AVFoundation.h>

QT_USE_NAMESPACE

//AVAsset Keys
static NSString* const AVF_TRACKS_KEY       = @"tracks";
static NSString* const AVF_PLAYABLE_KEY     = @"playable";

//AVPlayerItem keys
static NSString* const AVF_STATUS_KEY                   = @"status";
static NSString* const AVF_BUFFER_LIKELY_KEEP_UP_KEY    = @"playbackLikelyToKeepUp";

//AVPlayer keys
static NSString* const AVF_RATE_KEY                     = @"rate";
static NSString* const AVF_CURRENT_ITEM_KEY             = @"currentItem";
static NSString* const AVF_CURRENT_ITEM_DURATION_KEY    = @"currentItem.duration";

static void *AVFMediaPlayerObserverRateObservationContext = &AVFMediaPlayerObserverRateObservationContext;
static void *AVFMediaPlayerObserverStatusObservationContext = &AVFMediaPlayerObserverStatusObservationContext;
static void *AVFMediaPlayerObserverPresentationSizeContext = &AVFMediaPlayerObserverPresentationSizeContext;
static void *AVFMediaPlayerObserverBufferLikelyToKeepUpContext = &AVFMediaPlayerObserverBufferLikelyToKeepUpContext;
static void *AVFMediaPlayerObserverTracksContext = &AVFMediaPlayerObserverTracksContext;
static void *AVFMediaPlayerObserverCurrentItemObservationContext = &AVFMediaPlayerObserverCurrentItemObservationContext;
static void *AVFMediaPlayerObserverCurrentItemDurationObservationContext = &AVFMediaPlayerObserverCurrentItemDurationObservationContext;

@interface AVFMediaPlayerObserver : NSObject<AVAssetResourceLoaderDelegate>

@property (readonly, getter=player) AVPlayer* m_player;
@property (readonly, getter=playerItem) AVPlayerItem* m_playerItem;
@property (readonly, getter=playerLayer) AVPlayerLayer* m_playerLayer;
@property (readonly, getter=session) AVFMediaPlayer* m_session;
@property (retain) AVPlayerItemTrack *videoTrack;

- (AVFMediaPlayerObserver *) initWithMediaPlayerSession:(AVFMediaPlayer *)session;
- (void) setURL:(NSURL *)url mimeType:(NSString *)mimeType;
- (void) unloadMedia;
- (void) prepareToPlayAsset:(AVURLAsset *)asset withKeys:(NSArray *)requestedKeys;
- (void) assetFailedToPrepareForPlayback:(NSError *)error;
- (void) playerItemDidReachEnd:(NSNotification *)notification;
- (void) observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object
                         change:(NSDictionary *)change context:(void *)context;
- (void) detatchSession;
- (void) dealloc;
- (BOOL) resourceLoader:(AVAssetResourceLoader *)resourceLoader shouldWaitForLoadingOfRequestedResource:(AVAssetResourceLoadingRequest *)loadingRequest;
@end

@implementation AVFMediaPlayerObserver
{
@private
    AVFMediaPlayer *m_session;
    AVPlayer *m_player;
    AVPlayerItem *m_playerItem;
    AVPlayerLayer *m_playerLayer;
    NSURL *m_URL;
    BOOL m_bufferIsLikelyToKeepUp;
    NSData *m_data;
    NSString *m_mimeType;
}

@synthesize m_player, m_playerItem, m_playerLayer, m_session;

- (AVFMediaPlayerObserver *) initWithMediaPlayerSession:(AVFMediaPlayer *)session
{
    if (!(self = [super init]))
        return nil;

    m_session = session;
    m_bufferIsLikelyToKeepUp = FALSE;

    m_playerLayer = [AVPlayerLayer playerLayerWithPlayer:nil];
    [m_playerLayer retain];
    m_playerLayer.videoGravity = AVLayerVideoGravityResizeAspectFill;
    m_playerLayer.anchorPoint = CGPointMake(0.0f, 0.0f);
    return self;
}

- (void) setURL:(NSURL *)url mimeType:(NSString *)mimeType
{
    if (!m_session)
        return;

    [m_mimeType release];
    m_mimeType = [mimeType retain];

    if (m_URL != url)
    {
        [m_URL release];
        m_URL = [url copy];

        //Create an asset for inspection of a resource referenced by a given URL.
        //Load the values for the asset keys "tracks", "playable".

        // use __block to avoid maintaining strong references on variables captured by the
        // following block callback
#if defined(Q_OS_IOS)
        BOOL isAccessing = [m_URL startAccessingSecurityScopedResource];
#endif
        __block AVURLAsset *asset = [[AVURLAsset URLAssetWithURL:m_URL options:nil] retain];
        [asset.resourceLoader setDelegate:self queue:dispatch_get_main_queue()];

        __block NSArray *requestedKeys = [[NSArray arrayWithObjects:AVF_TRACKS_KEY, AVF_PLAYABLE_KEY, nil] retain];

        __block AVFMediaPlayerObserver *blockSelf = [self retain];

        // Tells the asset to load the values of any of the specified keys that are not already loaded.
        [asset loadValuesAsynchronouslyForKeys:requestedKeys completionHandler:
         ^{
             dispatch_async( dispatch_get_main_queue(),
                           ^{
#if defined(Q_OS_IOS)
                                 if (isAccessing)
                                    [m_URL stopAccessingSecurityScopedResource];
#endif
                                 [blockSelf prepareToPlayAsset:asset withKeys:requestedKeys];
                                 [asset release];
                                 [requestedKeys release];
                                 [blockSelf release];
                            });
         }];
    }
}

- (void) unloadMedia
{
    if (m_playerItem) {
        [m_playerItem removeObserver:self forKeyPath:@"presentationSize"];
        [m_playerItem removeObserver:self forKeyPath:AVF_STATUS_KEY];
        [m_playerItem removeObserver:self forKeyPath:AVF_BUFFER_LIKELY_KEEP_UP_KEY];
        [m_playerItem removeObserver:self forKeyPath:AVF_TRACKS_KEY];

        [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:AVPlayerItemDidPlayToEndTimeNotification
                                                    object:m_playerItem];
        m_playerItem = 0;
    }
    if (m_player) {
        [m_player setRate:0.0];
        [m_player removeObserver:self forKeyPath:AVF_CURRENT_ITEM_DURATION_KEY];
        [m_player removeObserver:self forKeyPath:AVF_CURRENT_ITEM_KEY];
        [m_player removeObserver:self forKeyPath:AVF_RATE_KEY];
        [m_player release];
        m_player = 0;
    }
    if (m_playerLayer)
        m_playerLayer.player = nil;
#if defined(Q_OS_IOS)
    [[AVAudioSession sharedInstance] setActive:NO error:nil];
#endif
}

- (void) prepareToPlayAsset:(AVURLAsset *)asset
                   withKeys:(NSArray *)requestedKeys
{
    if (!m_session)
        return;

    //Make sure that the value of each key has loaded successfully.
    for (NSString *thisKey in requestedKeys)
    {
        NSError *error = nil;
        AVKeyValueStatus keyStatus = [asset statusOfValueForKey:thisKey error:&error];
#ifdef QT_DEBUG_AVF
        qDebug() << Q_FUNC_INFO << [thisKey UTF8String] << " status: " << keyStatus;
#endif
        if (keyStatus == AVKeyValueStatusFailed)
        {
            [self assetFailedToPrepareForPlayback:error];
            return;
        }
    }

    //Use the AVAsset playable property to detect whether the asset can be played.
#ifdef QT_DEBUG_AVF
    qDebug() << Q_FUNC_INFO << "isPlayable: " << [asset isPlayable];
#endif
    if (!asset.playable)
        qWarning() << "Asset reported to be not playable. Playback of this asset may not be possible.";

    //At this point we're ready to set up for playback of the asset.
    //Stop observing our prior AVPlayerItem, if we have one.
    if (m_playerItem)
    {
        //Remove existing player item key value observers and notifications.
        [self unloadMedia];
    }

    //Create a new instance of AVPlayerItem from the now successfully loaded AVAsset.
    m_playerItem = [AVPlayerItem playerItemWithAsset:asset];
    if (!m_playerItem) {
        qWarning() << "Failed to create player item";
        //Generate an error describing the failure.
        NSString *localizedDescription = NSLocalizedString(@"Item cannot be played", @"Item cannot be played description");
        NSString *localizedFailureReason = NSLocalizedString(@"The assets tracks were loaded, but couldn't create player item.", @"Item cannot be played failure reason");
        NSDictionary *errorDict = [NSDictionary dictionaryWithObjectsAndKeys:
                localizedDescription, NSLocalizedDescriptionKey,
                localizedFailureReason, NSLocalizedFailureReasonErrorKey,
                nil];
        NSError *assetCannotBePlayedError = [NSError errorWithDomain:@"StitchedStreamPlayer" code:0 userInfo:errorDict];

        [self assetFailedToPrepareForPlayback:assetCannotBePlayedError];
        return;
    }

    //Observe the player item "status" key to determine when it is ready to play.
    [m_playerItem addObserver:self
                   forKeyPath:AVF_STATUS_KEY
                      options:NSKeyValueObservingOptionInitial | NSKeyValueObservingOptionNew
                      context:AVFMediaPlayerObserverStatusObservationContext];

    [m_playerItem addObserver:self
                   forKeyPath:@"presentationSize"
                      options:NSKeyValueObservingOptionInitial | NSKeyValueObservingOptionNew
                      context:AVFMediaPlayerObserverPresentationSizeContext];

    [m_playerItem addObserver:self
                   forKeyPath:AVF_BUFFER_LIKELY_KEEP_UP_KEY
                      options:NSKeyValueObservingOptionInitial | NSKeyValueObservingOptionNew
                      context:AVFMediaPlayerObserverBufferLikelyToKeepUpContext];

    [m_playerItem addObserver:self
                   forKeyPath:AVF_TRACKS_KEY
                      options:NSKeyValueObservingOptionInitial | NSKeyValueObservingOptionNew
                      context:AVFMediaPlayerObserverTracksContext];

    //When the player item has played to its end time we'll toggle
    //the movie controller Pause button to be the Play button
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(playerItemDidReachEnd:)
                                                 name:AVPlayerItemDidPlayToEndTimeNotification
                                               object:m_playerItem];

    //Get a new AVPlayer initialized to play the specified player item.
    m_player = [AVPlayer playerWithPlayerItem:m_playerItem];
    [m_player retain];

    //Set the initial volume on new player object
    if (self.session) {
        auto *audioOutput = m_session->m_audioOutput;
        m_player.volume = (audioOutput ? audioOutput->volume : 1.);
        m_player.muted = (audioOutput ? audioOutput->muted : true);
    }

    //Assign the output layer to the new player
    m_playerLayer.player = m_player;

    //Observe the AVPlayer "currentItem" property to find out when any
    //AVPlayer replaceCurrentItemWithPlayerItem: replacement will/did
    //occur.
    [m_player addObserver:self
                          forKeyPath:AVF_CURRENT_ITEM_KEY
                          options:NSKeyValueObservingOptionInitial | NSKeyValueObservingOptionNew
                          context:AVFMediaPlayerObserverCurrentItemObservationContext];

    //Observe the AVPlayer "rate" property to update the scrubber control.
    [m_player addObserver:self
                          forKeyPath:AVF_RATE_KEY
                          options:NSKeyValueObservingOptionInitial | NSKeyValueObservingOptionNew
                          context:AVFMediaPlayerObserverRateObservationContext];

    //Observe the duration for getting the buffer state
    [m_player addObserver:self
                          forKeyPath:AVF_CURRENT_ITEM_DURATION_KEY
                          options:0
                          context:AVFMediaPlayerObserverCurrentItemDurationObservationContext];
#if defined(Q_OS_IOS)
    [[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryPlayback withOptions:AVAudioSessionCategoryOptionMixWithOthers error:nil];
    [[AVAudioSession sharedInstance] setActive:YES error:nil];
#endif
}

-(void) assetFailedToPrepareForPlayback:(NSError *)error
{
    Q_UNUSED(error);
    QMetaObject::invokeMethod(m_session, "processMediaLoadError", Qt::AutoConnection);
#ifdef QT_DEBUG_AVF
    qDebug() << Q_FUNC_INFO;
    qDebug() << [[error localizedDescription] UTF8String];
    qDebug() << [[error localizedFailureReason] UTF8String];
    qDebug() << [[error localizedRecoverySuggestion] UTF8String];
#endif
}

- (void) playerItemDidReachEnd:(NSNotification *)notification
{
    Q_UNUSED(notification);
    if (self.session)
        QMetaObject::invokeMethod(m_session, "processEOS", Qt::AutoConnection);
}

- (void) observeValueForKeyPath:(NSString*) path
                       ofObject:(id)object
                         change:(NSDictionary*)change
                        context:(void*)context
{
    //AVPlayerItem "status" property value observer.
    if (context == AVFMediaPlayerObserverStatusObservationContext)
    {
        AVPlayerStatus status = (AVPlayerStatus)[[change objectForKey:NSKeyValueChangeNewKey] integerValue];
        switch (status)
        {
            //Indicates that the status of the player is not yet known because
            //it has not tried to load new media resources for playback
            case AVPlayerStatusUnknown:
            {
                //QMetaObject::invokeMethod(m_session, "processLoadStateChange", Qt::AutoConnection);
            }
            break;

            case AVPlayerStatusReadyToPlay:
            {
                //Once the AVPlayerItem becomes ready to play, i.e.
                //[playerItem status] == AVPlayerItemStatusReadyToPlay,
                //its duration can be fetched from the item.
                if (self.session)
                    QMetaObject::invokeMethod(m_session, "processLoadStateChange", Qt::AutoConnection);
            }
            break;

            case AVPlayerStatusFailed:
            {
                AVPlayerItem *playerItem = static_cast<AVPlayerItem*>(object);
                [self assetFailedToPrepareForPlayback:playerItem.error];

                if (self.session)
                    QMetaObject::invokeMethod(m_session, "processLoadStateFailure", Qt::AutoConnection);
            }
            break;
        }
    } else if (context == AVFMediaPlayerObserverPresentationSizeContext) {
        QSize size(m_playerItem.presentationSize.width, m_playerItem.presentationSize.height);
        QMetaObject::invokeMethod(m_session, "nativeSizeChanged", Qt::AutoConnection, Q_ARG(QSize, size));
    } else if (context == AVFMediaPlayerObserverBufferLikelyToKeepUpContext)
    {
        const bool isPlaybackLikelyToKeepUp = [m_playerItem isPlaybackLikelyToKeepUp];
        if (isPlaybackLikelyToKeepUp != m_bufferIsLikelyToKeepUp) {
            m_bufferIsLikelyToKeepUp = isPlaybackLikelyToKeepUp;
            QMetaObject::invokeMethod(m_session, "processBufferStateChange", Qt::AutoConnection,
                                      Q_ARG(int, isPlaybackLikelyToKeepUp ? 100 : 0));
        }
    }
    else if (context == AVFMediaPlayerObserverTracksContext)
    {
        QMetaObject::invokeMethod(m_session, "updateTracks", Qt::AutoConnection);
    }
    //AVPlayer "rate" property value observer.
    else if (context == AVFMediaPlayerObserverRateObservationContext)
    {
        //QMetaObject::invokeMethod(m_session, "setPlaybackRate",  Qt::AutoConnection, Q_ARG(qreal, [m_player rate]));
    }
    //AVPlayer "currentItem" property observer.
    //Called when the AVPlayer replaceCurrentItemWithPlayerItem:
    //replacement will/did occur.
    else if (context == AVFMediaPlayerObserverCurrentItemObservationContext)
    {
        AVPlayerItem *newPlayerItem = [change objectForKey:NSKeyValueChangeNewKey];
        if (m_playerItem != newPlayerItem)
            m_playerItem = newPlayerItem;
    }
    else if (context == AVFMediaPlayerObserverCurrentItemDurationObservationContext)
    {
        const CMTime time = [m_playerItem duration];
        const qint64 duration =  static_cast<qint64>(float(time.value) / float(time.timescale) * 1000.0f);
        if (self.session)
            QMetaObject::invokeMethod(m_session, "processDurationChange",  Qt::AutoConnection, Q_ARG(qint64, duration));
    }
    else
    {
        [super observeValueForKeyPath:path ofObject:object change:change context:context];
    }
}

- (void) detatchSession
{
#ifdef QT_DEBUG_AVF
        qDebug() << Q_FUNC_INFO;
#endif
        m_session = 0;
}

- (void) dealloc
{
#ifdef QT_DEBUG_AVF
        qDebug() << Q_FUNC_INFO;
#endif
    [self unloadMedia];

    if (m_URL) {
        [m_URL release];
    }

    [m_mimeType release];
    [m_playerLayer release];
    [super dealloc];
}

- (BOOL) resourceLoader:(AVAssetResourceLoader *)resourceLoader shouldWaitForLoadingOfRequestedResource:(AVAssetResourceLoadingRequest *)loadingRequest
{
    Q_UNUSED(resourceLoader);

    if (![loadingRequest.request.URL.scheme isEqualToString:@"iodevice"])
        return NO;

    QIODevice *device = m_session->mediaStream();
    if (!device)
        return NO;

    device->seek(loadingRequest.dataRequest.requestedOffset);
    if (loadingRequest.contentInformationRequest) {
        loadingRequest.contentInformationRequest.contentType = m_mimeType;
        loadingRequest.contentInformationRequest.contentLength = device->size();
        loadingRequest.contentInformationRequest.byteRangeAccessSupported = YES;
    }

    if (loadingRequest.dataRequest) {
        NSInteger requestedLength = loadingRequest.dataRequest.requestedLength;
        int maxBytes = qMin(32 * 1064, int(requestedLength));
        char buffer[maxBytes];
        NSInteger submitted = 0;
        while (submitted < requestedLength) {
            qint64 len = device->read(buffer, maxBytes);
            if (len < 1)
                break;

            [loadingRequest.dataRequest respondWithData:[NSData dataWithBytes:buffer length:len]];
            submitted += len;
        }

        // Finish loading even if not all bytes submitted.
        [loadingRequest finishLoading];
    }

    return YES;
}
@end

AVFMediaPlayer::AVFMediaPlayer(QMediaPlayer *player)
    : QObject(player),
      QPlatformMediaPlayer(player),
      m_state(QMediaPlayer::StoppedState),
      m_mediaStatus(QMediaPlayer::NoMedia),
      m_mediaStream(nullptr),
      m_rate(1.0),
      m_requestedPosition(-1),
      m_duration(0),
      m_bufferProgress(0),
      m_videoAvailable(false),
      m_audioAvailable(false),
      m_seekable(false)
{
    m_observer = [[AVFMediaPlayerObserver alloc] initWithMediaPlayerSession:this];
    connect(&m_playbackTimer, &QTimer::timeout, this, &AVFMediaPlayer::processPositionChange);
    setVideoOutput(new AVFVideoRendererControl(this));
}

AVFMediaPlayer::~AVFMediaPlayer()
{
#ifdef QT_DEBUG_AVF
    qDebug() << Q_FUNC_INFO;
#endif
    //Detatch the session from the sessionObserver (which could still be alive trying to communicate with this session).
    [m_observer detatchSession];
    [static_cast<AVFMediaPlayerObserver*>(m_observer) release];
}

void AVFMediaPlayer::setVideoSink(QVideoSink *sink)
{
    m_videoSink = sink ? static_cast<AVFVideoSink *>(sink->platformVideoSink()): nullptr;
    m_videoOutput->setVideoSink(m_videoSink);
}

void AVFMediaPlayer::setVideoOutput(AVFVideoRendererControl *output)
{
#ifdef QT_DEBUG_AVF
    qDebug() << Q_FUNC_INFO << output;
#endif

    if (m_videoOutput == output)
        return;

    //Set the current output layer to null to stop rendering
    if (m_videoOutput) {
        m_videoOutput->setLayer(nullptr);
    }

    m_videoOutput = output;

    if (m_videoOutput && m_state != QMediaPlayer::StoppedState)
        m_videoOutput->setLayer([static_cast<AVFMediaPlayerObserver*>(m_observer) playerLayer]);
}

AVAsset *AVFMediaPlayer::currentAssetHandle()
{
#ifdef QT_DEBUG_AVF
    qDebug() << Q_FUNC_INFO;
#endif
    AVAsset *currentAsset = [[static_cast<AVFMediaPlayerObserver*>(m_observer) playerItem] asset];
    return currentAsset;
}

QMediaPlayer::PlaybackState AVFMediaPlayer::state() const
{
    return m_state;
}

QMediaPlayer::MediaStatus AVFMediaPlayer::mediaStatus() const
{
    return m_mediaStatus;
}

QUrl AVFMediaPlayer::media() const
{
    return m_resources;
}

QIODevice *AVFMediaPlayer::mediaStream() const
{
    return m_mediaStream;
}

static void setURL(AVFMediaPlayerObserver *observer, const QByteArray &url, const QString &mimeType = QString())
{
    NSString *urlString = [NSString stringWithUTF8String:url.constData()];
    NSURL *nsurl = [NSURL URLWithString:urlString];
    [observer setURL:nsurl mimeType:[NSString stringWithUTF8String:mimeType.toLatin1().constData()]];
}

static void setStreamURL(AVFMediaPlayerObserver *observer, const QByteArray &url)
{
    setURL(observer, QByteArrayLiteral("iodevice://") + url, QFileInfo(QString::fromUtf8(url)).suffix());
}

void AVFMediaPlayer::setMedia(const QUrl &content, QIODevice *stream)
{
#ifdef QT_DEBUG_AVF
    qDebug() << Q_FUNC_INFO << content.request().url();
#endif

    [static_cast<AVFMediaPlayerObserver*>(m_observer) unloadMedia];

    m_resources = content;
    resetStream(stream);

    setAudioAvailable(false);
    setVideoAvailable(false);
    setSeekable(false);
    m_requestedPosition = -1;
    orientationChanged(QVideoFrame::Rotation0, false);
    Q_EMIT positionChanged(position());
    if (m_duration != 0) {
        m_duration = 0;
        Q_EMIT durationChanged(0);
    }
    if (!m_metaData.isEmpty()) {
        m_metaData.clear();
        metaDataChanged();
    }
    for (int i = 0; i < QPlatformMediaPlayer::NTrackTypes; ++i) {
        tracks[i].clear();
        nativeTracks[i].clear();
    }
    tracksChanged();

    const QMediaPlayer::MediaStatus oldMediaStatus = m_mediaStatus;
    const QMediaPlayer::PlaybackState oldState = m_state;

    if (!m_mediaStream && content.isEmpty()) {
        m_mediaStatus = QMediaPlayer::NoMedia;
        if (m_mediaStatus != oldMediaStatus)
            Q_EMIT mediaStatusChanged(m_mediaStatus);

        m_state = QMediaPlayer::StoppedState;
        if (m_state != oldState)
            Q_EMIT stateChanged(m_state);

        return;
    }

    m_mediaStatus = QMediaPlayer::LoadingMedia;
    if (m_mediaStatus != oldMediaStatus)
        Q_EMIT mediaStatusChanged(m_mediaStatus);

    if (m_mediaStream) {
        // If there is a data, try to load it,
        // otherwise wait for readyRead.
        if (m_mediaStream->size())
            setStreamURL(m_observer, m_resources.toEncoded());
    } else {
        //Load AVURLAsset
        //initialize asset using content's URL
        setURL(m_observer, m_resources.toEncoded());
    }

    m_state = QMediaPlayer::StoppedState;
    if (m_state != oldState)
        Q_EMIT stateChanged(m_state);
}

qint64 AVFMediaPlayer::position() const
{
    AVPlayerItem *playerItem = [static_cast<AVFMediaPlayerObserver*>(m_observer) playerItem];

    if (m_requestedPosition != -1)
        return m_requestedPosition;

    if (!playerItem)
        return 0;

    CMTime time = [playerItem currentTime];
    return static_cast<quint64>(float(time.value) / float(time.timescale) * 1000.0f);
}

qint64 AVFMediaPlayer::duration() const
{
#ifdef QT_DEBUG_AVF
    qDebug() << Q_FUNC_INFO;
#endif
    return m_duration;
}

float AVFMediaPlayer::bufferProgress() const
{
#ifdef QT_DEBUG_AVF
    qDebug() << Q_FUNC_INFO;
#endif
    return m_bufferProgress/100.;
}

void AVFMediaPlayer::setAudioAvailable(bool available)
{
    if (m_audioAvailable == available)
        return;

    m_audioAvailable = available;
    Q_EMIT audioAvailableChanged(available);
}

bool AVFMediaPlayer::isAudioAvailable() const
{
    return m_audioAvailable;
}

void AVFMediaPlayer::setVideoAvailable(bool available)
{
    if (m_videoAvailable == available)
        return;

    m_videoAvailable = available;
    Q_EMIT videoAvailableChanged(available);
}

bool AVFMediaPlayer::isVideoAvailable() const
{
    return m_videoAvailable;
}

bool AVFMediaPlayer::isSeekable() const
{
    return m_seekable;
}

void AVFMediaPlayer::setSeekable(bool seekable)
{
    if (m_seekable == seekable)
        return;

    m_seekable = seekable;
    Q_EMIT seekableChanged(seekable);
}

QMediaTimeRange AVFMediaPlayer::availablePlaybackRanges() const
{
    AVPlayerItem *playerItem = [static_cast<AVFMediaPlayerObserver*>(m_observer) playerItem];

    if (playerItem) {
        QMediaTimeRange timeRanges;

        NSArray *ranges = [playerItem loadedTimeRanges];
        for (NSValue *timeRange in ranges) {
            CMTimeRange currentTimeRange = [timeRange CMTimeRangeValue];
            qint64 startTime = qint64(float(currentTimeRange.start.value) / currentTimeRange.start.timescale * 1000.0);
            timeRanges.addInterval(startTime, startTime + qint64(float(currentTimeRange.duration.value) / currentTimeRange.duration.timescale * 1000.0));
        }
        if (!timeRanges.isEmpty())
            return timeRanges;
    }
    return QMediaTimeRange(0, duration());
}

qreal AVFMediaPlayer::playbackRate() const
{
    return m_rate;
}

void AVFMediaPlayer::setAudioOutput(QPlatformAudioOutput *output)
{
    if (m_audioOutput == output)
        return;
    if (m_audioOutput)
        m_audioOutput->q->disconnect(this);
    m_audioOutput = output;
    if (m_audioOutput) {
        connect(m_audioOutput->q, &QAudioOutput::deviceChanged, this, &AVFMediaPlayer::audioOutputChanged);
        connect(m_audioOutput->q, &QAudioOutput::volumeChanged, this, &AVFMediaPlayer::setVolume);
        connect(m_audioOutput->q, &QAudioOutput::mutedChanged, this, &AVFMediaPlayer::setMuted);
        //connect(m_audioOutput->q, &QAudioOutput::audioRoleChanged, this, &AVFMediaPlayer::setAudioRole);
    }
    audioOutputChanged();
    setMuted(m_audioOutput ? m_audioOutput->muted : true);
    setVolume(m_audioOutput ? m_audioOutput->volume : 1.);
}

QMediaMetaData AVFMediaPlayer::metaData() const
{
    return m_metaData;
}

void AVFMediaPlayer::setPlaybackRate(qreal rate)
{
#ifdef QT_DEBUG_AVF
    qDebug() << Q_FUNC_INFO << rate;
#endif

    if (qFuzzyCompare(m_rate, rate))
        return;

    m_rate = rate;

    AVPlayer *player = [static_cast<AVFMediaPlayerObserver*>(m_observer) player];
    if (player && m_state == QMediaPlayer::PlayingState)
        [player setRate:m_rate];

    Q_EMIT playbackRateChanged(m_rate);
}

void AVFMediaPlayer::setPosition(qint64 pos)
{
#ifdef QT_DEBUG_AVF
    qDebug() << Q_FUNC_INFO << pos;
#endif

    if (pos == position())
        return;

    AVPlayerItem *playerItem = [static_cast<AVFMediaPlayerObserver*>(m_observer) playerItem];
    if (!playerItem) {
        m_requestedPosition = pos;
        Q_EMIT positionChanged(m_requestedPosition);
        return;
    }

    if (!isSeekable()) {
        if (m_requestedPosition != -1) {
            m_requestedPosition = -1;
            Q_EMIT positionChanged(position());
        }
        return;
    }

    pos = qMax(qint64(0), pos);
    if (duration() > 0)
        pos = qMin(pos, duration());
    m_requestedPosition = pos;

    CMTime newTime = [playerItem currentTime];
    newTime.value = (pos / 1000.0f) * newTime.timescale;
    [playerItem seekToTime:newTime toleranceBefore:kCMTimeZero toleranceAfter:kCMTimeZero
                           completionHandler:^(BOOL finished) {
                                if (finished)
                                    m_requestedPosition = -1;
                           }];

    Q_EMIT positionChanged(pos);

    // Reset media status if the current status is EndOfMedia
    if (m_mediaStatus == QMediaPlayer::EndOfMedia) {
        QMediaPlayer::MediaStatus newMediaStatus = (m_state == QMediaPlayer::PausedState) ? QMediaPlayer::BufferedMedia
                                                                                          : QMediaPlayer::LoadedMedia;
        Q_EMIT mediaStatusChanged((m_mediaStatus = newMediaStatus));
    }
}

void AVFMediaPlayer::play()
{
#ifdef QT_DEBUG_AVF
    qDebug() << Q_FUNC_INFO << "currently: " << m_state;
#endif

    if (m_mediaStatus == QMediaPlayer::NoMedia || m_mediaStatus == QMediaPlayer::InvalidMedia)
        return;

    if (m_state == QMediaPlayer::PlayingState)
        return;

    resetCurrentLoop();

    if (m_videoOutput && m_videoSink)
        m_videoOutput->setLayer([static_cast<AVFMediaPlayerObserver*>(m_observer) playerLayer]);

    // Reset media status if the current status is EndOfMedia
    if (m_mediaStatus == QMediaPlayer::EndOfMedia)
        setPosition(0);

    if (m_mediaStatus == QMediaPlayer::LoadedMedia || m_mediaStatus == QMediaPlayer::BufferedMedia) {
        // Setting the rate starts playback
        [[static_cast<AVFMediaPlayerObserver*>(m_observer) player] setRate:m_rate];
    }

    m_state = QMediaPlayer::PlayingState;
    processLoadStateChange();

    Q_EMIT stateChanged(m_state);
    m_playbackTimer.start(100);
}

void AVFMediaPlayer::pause()
{
#ifdef QT_DEBUG_AVF
    qDebug() << Q_FUNC_INFO << "currently: " << m_state;
#endif

    if (m_mediaStatus == QMediaPlayer::NoMedia)
        return;

    if (m_state == QMediaPlayer::PausedState)
        return;

    m_state = QMediaPlayer::PausedState;

    if (m_videoOutput && m_videoSink)
        m_videoOutput->setLayer([static_cast<AVFMediaPlayerObserver*>(m_observer) playerLayer]);

    [[static_cast<AVFMediaPlayerObserver*>(m_observer) player] pause];

    // Reset media status if the current status is EndOfMedia
    if (m_mediaStatus == QMediaPlayer::EndOfMedia)
        setPosition(0);

    Q_EMIT positionChanged(position());
    Q_EMIT stateChanged(m_state);
    m_playbackTimer.stop();
}

void AVFMediaPlayer::stop()
{
#ifdef QT_DEBUG_AVF
    qDebug() << Q_FUNC_INFO << "currently: " << m_state;
#endif

    if (m_state == QMediaPlayer::StoppedState)
        return;

    // AVPlayer doesn't have stop(), only pause() and play().
    [[static_cast<AVFMediaPlayerObserver*>(m_observer) player] pause];
    setPosition(0);

    if (m_videoOutput)
        m_videoOutput->setLayer(nullptr);

    if (m_mediaStatus == QMediaPlayer::BufferedMedia)
        Q_EMIT mediaStatusChanged((m_mediaStatus = QMediaPlayer::LoadedMedia));

    Q_EMIT stateChanged((m_state = QMediaPlayer::StoppedState));
    m_playbackTimer.stop();
}

void AVFMediaPlayer::setVolume(float volume)
{
#ifdef QT_DEBUG_AVF
    qDebug() << Q_FUNC_INFO << volume;
#endif

    AVPlayer *player = [static_cast<AVFMediaPlayerObserver*>(m_observer) player];
    if (player)
        player.volume = volume;
}

void AVFMediaPlayer::setMuted(bool muted)
{
#ifdef QT_DEBUG_AVF
    qDebug() << Q_FUNC_INFO << muted;
#endif

    AVPlayer *player = [static_cast<AVFMediaPlayerObserver*>(m_observer) player];
    if (player)
        player.muted = muted;
}

void AVFMediaPlayer::audioOutputChanged()
{
#ifdef Q_OS_MACOS
    AVPlayer *player = [static_cast<AVFMediaPlayerObserver*>(m_observer) player];
    if (!m_audioOutput || m_audioOutput->device.id().isEmpty()) {
        player.audioOutputDeviceUniqueID = nil;
        if (!m_audioOutput)
            player.muted = true;
    } else {
        NSString *str = QString::fromUtf8(m_audioOutput->device.id()).toNSString();
        player.audioOutputDeviceUniqueID = str;
    }
#endif
}

void AVFMediaPlayer::processEOS()
{
    if (doLoop()) {
        setPosition(0);
        [[static_cast<AVFMediaPlayerObserver*>(m_observer) player] setRate:m_rate];
        return;
    }

    //AVPlayerItem has reached end of track/stream
#ifdef QT_DEBUG_AVF
    qDebug() << Q_FUNC_INFO;
#endif
    Q_EMIT positionChanged(position());
    m_mediaStatus = QMediaPlayer::EndOfMedia;
    m_state = QMediaPlayer::StoppedState;

    if (m_videoOutput)
        m_videoOutput->setLayer(nullptr);

    Q_EMIT mediaStatusChanged(m_mediaStatus);
    Q_EMIT stateChanged(m_state);
}

void AVFMediaPlayer::processLoadStateChange(QMediaPlayer::PlaybackState newState)
{
    AVPlayerStatus currentStatus = [[static_cast<AVFMediaPlayerObserver*>(m_observer) player] status];

#ifdef QT_DEBUG_AVF
    qDebug() << Q_FUNC_INFO << currentStatus << ", " << m_mediaStatus << ", " << newState;
#endif

    if (m_mediaStatus == QMediaPlayer::NoMedia)
        return;

    if (currentStatus == AVPlayerStatusReadyToPlay) {

        QMediaPlayer::MediaStatus newStatus = m_mediaStatus;

        AVPlayerItem *playerItem = [m_observer playerItem];

        // get the meta data
        m_metaData = AVFMetaData::fromAsset(playerItem.asset);
        Q_EMIT metaDataChanged();
        updateTracks();

        if (playerItem) {
            setSeekable([[playerItem seekableTimeRanges] count] > 0);

            // Get the native size of the video, and reset the bounds of the player layer
            AVPlayerLayer *playerLayer = [m_observer playerLayer];
            if (m_observer.videoTrack && playerLayer) {
                if (!playerLayer.bounds.size.width || !playerLayer.bounds.size.height) {
                    playerLayer.bounds = CGRectMake(0.0f, 0.0f,
                                                    m_observer.videoTrack.assetTrack.naturalSize.width,
                                                    m_observer.videoTrack.assetTrack.naturalSize.height);
                }
            }

            if (m_requestedPosition != -1) {
                setPosition(m_requestedPosition);
                m_requestedPosition = -1;
            }
        }

        newStatus = (newState != QMediaPlayer::StoppedState) ? QMediaPlayer::BufferedMedia
                                                             : QMediaPlayer::LoadedMedia;

        if (newStatus != m_mediaStatus)
            Q_EMIT mediaStatusChanged((m_mediaStatus = newStatus));

    }

    if (newState == QMediaPlayer::PlayingState && [static_cast<AVFMediaPlayerObserver*>(m_observer) player]) {
        // Setting the rate is enough to start playback, no need to call play()
        [[static_cast<AVFMediaPlayerObserver*>(m_observer) player] setRate:m_rate];
        m_playbackTimer.start();
    }
}


void AVFMediaPlayer::processLoadStateChange()
{
    processLoadStateChange(m_state);
}


void AVFMediaPlayer::processLoadStateFailure()
{
    Q_EMIT stateChanged((m_state = QMediaPlayer::StoppedState));
}

void AVFMediaPlayer::processBufferStateChange(int bufferProgress)
{
    if (bufferProgress == m_bufferProgress)
        return;

    auto status = m_mediaStatus;
    // Buffered -> unbuffered.
    if (!bufferProgress) {
        status = QMediaPlayer::StalledMedia;
    } else if (status == QMediaPlayer::StalledMedia) {
        status = QMediaPlayer::BufferedMedia;
        // Resume playback.
        if (m_state == QMediaPlayer::PlayingState) {
            [[static_cast<AVFMediaPlayerObserver*>(m_observer) player] setRate:m_rate];
            m_playbackTimer.start();
        }
    }

    if (m_mediaStatus != status)
        Q_EMIT mediaStatusChanged(m_mediaStatus = status);

    m_bufferProgress = bufferProgress;
    Q_EMIT bufferProgressChanged(bufferProgress/100.);
}

void AVFMediaPlayer::processDurationChange(qint64 duration)
{
    if (duration == m_duration)
        return;

    m_duration = duration;
    Q_EMIT durationChanged(duration);
}

void AVFMediaPlayer::processPositionChange()
{
    if (m_state == QMediaPlayer::StoppedState)
        return;

    Q_EMIT positionChanged(position());
}

void AVFMediaPlayer::processMediaLoadError()
{
    if (m_requestedPosition != -1) {
        m_requestedPosition = -1;
        Q_EMIT positionChanged(position());
    }

    Q_EMIT mediaStatusChanged((m_mediaStatus = QMediaPlayer::InvalidMedia));

    Q_EMIT error(QMediaPlayer::FormatError, tr("Failed to load media"));
}

void AVFMediaPlayer::streamReady()
{
    setStreamURL(m_observer, m_resources.toEncoded());
}

void AVFMediaPlayer::streamDestroyed()
{
    resetStream(nullptr);
}

void AVFMediaPlayer::updateTracks()
{
    bool firstLoad = true;
    for (int i = 0; i < QPlatformMediaPlayer::NTrackTypes; ++i) {
        if (tracks[i].count())
            firstLoad = false;
        tracks[i].clear();
        nativeTracks[i].clear();
    }
    AVPlayerItem *playerItem = [m_observer playerItem];
    if (playerItem) {
        // Check each track for audio and video content
        NSArray *tracks =  playerItem.tracks;
        for (AVPlayerItemTrack *track in tracks) {
            AVAssetTrack *assetTrack = track.assetTrack;
            if (assetTrack) {
                int qtTrack = -1;
                if ([assetTrack.mediaType isEqualToString:AVMediaTypeAudio]) {
                    qtTrack = QPlatformMediaPlayer::AudioStream;
                    setAudioAvailable(true);
                } else if ([assetTrack.mediaType isEqualToString:AVMediaTypeVideo]) {
                    qtTrack = QPlatformMediaPlayer::VideoStream;
                    setVideoAvailable(true);
                    if (m_observer.videoTrack != track) {
                        m_observer.videoTrack = track;
                        bool isMirrored = false;
                        QVideoFrame::RotationAngle orientation = QVideoFrame::Rotation0;
                        videoOrientationForAssetTrack(assetTrack, orientation, isMirrored);
                        orientationChanged(orientation, isMirrored);
                    }
                }
                else if ([assetTrack.mediaType isEqualToString:AVMediaTypeSubtitle]) {
                    qtTrack = QPlatformMediaPlayer::SubtitleStream;
                }
                if (qtTrack != -1) {
                    QMediaMetaData metaData = AVFMetaData::fromAssetTrack(assetTrack);
                    this->tracks[qtTrack].append(metaData);
                    nativeTracks[qtTrack].append(track);
                }
            }
        }
        // subtitles are disabled by default
        if (firstLoad)
            setActiveTrack(SubtitleStream, -1);
    }
    Q_EMIT tracksChanged();
}

void AVFMediaPlayer::setActiveTrack(QPlatformMediaPlayer::TrackType type, int index)
{
    const auto &t = nativeTracks[type];
    if (type == QPlatformMediaPlayer::SubtitleStream) {
        // subtitle streams are not always automatically enabled on macOS/iOS.
        // this hack ensures they get enables and we actually get the text
        AVPlayerItem *playerItem = m_observer.m_playerItem;
        if (playerItem) {
            AVAsset *asset = playerItem.asset;
            if (!asset)
                return;
            AVMediaSelectionGroup *group = [asset mediaSelectionGroupForMediaCharacteristic:AVMediaCharacteristicLegible];
            if (!group)
                return;
            auto *options = group.options;
            if (options.count)
                [playerItem selectMediaOption:options.firstObject inMediaSelectionGroup:group];
        }
    }
    for (int i = 0; i < t.count(); ++i)
        t.at(i).enabled = (i == index);
    emit activeTracksChanged();
}

int AVFMediaPlayer::activeTrack(QPlatformMediaPlayer::TrackType type)
{
    const auto &t = nativeTracks[type];
    for (int i = 0; i < t.count(); ++i)
        if (t.at(i).enabled)
            return i;
    return -1;
}

int AVFMediaPlayer::trackCount(QPlatformMediaPlayer::TrackType type)
{
    return nativeTracks[type].count();
}

QMediaMetaData AVFMediaPlayer::trackMetaData(QPlatformMediaPlayer::TrackType type, int trackNumber)
{
    const auto &t = tracks[type];
    if (trackNumber < 0 || trackNumber >= t.count())
        return QMediaMetaData();
    return t.at(trackNumber);
}

void AVFMediaPlayer::resetStream(QIODevice *stream)
{
    if (m_mediaStream) {
        disconnect(m_mediaStream, &QIODevice::readyRead, this, &AVFMediaPlayer::streamReady);
        disconnect(m_mediaStream, &QIODevice::destroyed, this, &AVFMediaPlayer::streamDestroyed);
    }

    m_mediaStream = stream;

    if (m_mediaStream) {
        connect(m_mediaStream, &QIODevice::readyRead, this, &AVFMediaPlayer::streamReady);
        connect(m_mediaStream, &QIODevice::destroyed, this, &AVFMediaPlayer::streamDestroyed);
    }
}

void AVFMediaPlayer::nativeSizeChanged(QSize size)
{
    if (!m_videoSink)
        return;
    m_videoSink->setNativeSize(size);
}

void AVFMediaPlayer::orientationChanged(QVideoFrame::RotationAngle rotation, bool mirrored)
{
    if (!m_videoOutput)
        return;

    m_videoOutput->setVideoRotation(rotation);
    m_videoOutput->setVideoMirrored(mirrored);
}

void AVFMediaPlayer::videoOrientationForAssetTrack(AVAssetTrack *videoTrack,
                                                   QVideoFrame::RotationAngle &angle,
                                                   bool &mirrored)
{
    angle = QVideoFrame::Rotation0;
    if (videoTrack) {
        CGAffineTransform transform = videoTrack.preferredTransform;
        if (CGAffineTransformIsIdentity(transform))
            return;
        qreal delta = transform.a * transform.d - transform.b * transform.c;
        qreal radians = qAtan2(transform.b, transform.a);
        qreal degrees = qRadiansToDegrees(radians);
        qreal scaleX = (transform.a/qAbs(transform.a)) * qSqrt(qPow(transform.a, 2) + qPow(transform.c, 2));
        qreal scaleY = (transform.d/abs(transform.d)) * qSqrt(qPow(transform.b, 2) + qPow(transform.d, 2));

        if (delta < 0.0) { // flipped
            if (scaleX < 0.0) {
                // vertical flip
                degrees = -degrees;
            } else if (scaleY < 0.0) {
                // horizontal flip
                degrees = (180 + (int)degrees) % 360;
            }
            mirrored = true;
        }

        if (qFuzzyCompare(degrees, qreal(90)) || qFuzzyCompare(degrees, qreal(-270))) {
            angle = QVideoFrame::Rotation90;
        } else if (qFuzzyCompare(degrees, qreal(-90)) || qFuzzyCompare(degrees, qreal(270))) {
            angle = QVideoFrame::Rotation270;
        } else if (qFuzzyCompare(degrees, qreal(180)) || qFuzzyCompare(degrees, qreal(-180))) {
            angle = QVideoFrame::Rotation180;
        }
    }
}

#include "moc_avfmediaplayer_p.cpp"
