// Copyright (C) 2016 The Qt Company Ltd and/or its subsidiary(-ies).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "avfmediaplayer_p.h"
#include "avfvideorenderercontrol_p.h"
#include <avfvideosink_p.h>
#include <avfmetadata_p.h>

#include "qaudiooutput.h"
#include "private/qplatformaudiooutput_p.h"

#include <QtCore/qdir.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qmimedatabase.h>
#include <QtCore/qpointer.h>
#include <QtCore/qmath.h>
#include <QtCore/qmutex.h>
#include <QtCore/qthread.h>
#include <QtCore/private/qexpected_p.h>

#include <mutex>

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

static void *AVFMediaPlayerObserverRateObservationContext =
        &AVFMediaPlayerObserverRateObservationContext;
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
@property (retain) AVPlayerItemTrack *videoTrack;

- (AVFMediaPlayerObserver *) initWithMediaPlayerSession:(AVFMediaPlayer *)session;
- (void) setURL:(NSURL *)url mimeType:(NSString *)mimeType;
- (void) unloadMedia;
- (void) prepareToPlayAsset:(AVURLAsset *)asset withKeys:(NSArray *)requestedKeys;
- (void) assetFailedToPrepareForPlayback:(NSError *)error;
- (void) playerItemDidReachEnd:(NSNotification *)notification;
- (void) observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object
                         change:(NSDictionary *)change context:(void *)context;
- (void) clearSession;
- (void) notifySeekComplete;
- (void) dealloc;
- (BOOL) resourceLoader:(AVAssetResourceLoader *)resourceLoader shouldWaitForLoadingOfRequestedResource:(AVAssetResourceLoadingRequest *)loadingRequest;
@end

#ifdef Q_OS_IOS
// Alas, no such thing as 'class variable', hence globals:
static unsigned sessionActivationCount;
static QMutex sessionMutex;
#endif // Q_OS_IOS

namespace {

struct GuardedPlatformPlayer
{
    mutable QMutex mutex;
    AVFMediaPlayer *player{};

    explicit operator bool() const
    {
        std::lock_guard guard(mutex);
        return player;
    }

    struct not_a_platform_player_t
    {
    };

    template <typename Functor>
    auto withPlatformPlayer(Functor &&f)
            -> q23::expected<std::invoke_result_t<Functor, AVFMediaPlayer *>,
                             not_a_platform_player_t>
    {
        std::unique_lock guard(mutex);
        if (!player)
            return q23::unexpected{ not_a_platform_player_t{} };
        if constexpr (std::is_void_v<std::invoke_result_t<Functor, AVFMediaPlayer *>>) {
            f(player);
            return {};
        } else {
            return f(player);
        }
    }

    template <typename Functor>
    void invokeWithPlatformPlayer(Functor f)
    {
        std::unique_lock guard(mutex);
        if (!player)
            return;

        if (player->thread()->isCurrentThread()) {
            guard.unlock();
            f(player);
        } else {
            QMetaObject::invokeMethod(player, [f = std::move(f), player = player]() {
                f(player);
            });
        }
    }

    void clear()
    {
        std::lock_guard<QMutex> guard(mutex);
        player = nullptr;
    }
};
} // namespace

@implementation AVFMediaPlayerObserver {
@private
    GuardedPlatformPlayer m_platformPlayer;
    AVPlayer *m_player;
    AVPlayerItem *m_playerItem;
    AVPlayerLayer *m_playerLayer;
    NSURL *m_URL;
    BOOL m_bufferIsLikelyToKeepUp;
    NSData *m_data;
    NSString *m_mimeType;
#ifdef Q_OS_IOS
    BOOL m_activated;
#endif
}

@synthesize m_player, m_playerItem, m_playerLayer;

#ifdef Q_OS_IOS
- (void)setSessionActive:(BOOL)active
{
    const QMutexLocker lock(&sessionMutex);
    if (active) {
        // Don't count the same player twice if already activated,
        // unless it tried to deactivate first:
        if (m_activated)
            return;
        if (!sessionActivationCount)
            [AVAudioSession.sharedInstance setActive:YES error:nil];
        ++sessionActivationCount;
        m_activated = YES;
    } else {
        if (!sessionActivationCount || !m_activated) {
            qWarning("Unbalanced audio session deactivation, ignoring.");
            return;
        }
        --sessionActivationCount;
        m_activated = NO;
        if (!sessionActivationCount)
            [AVAudioSession.sharedInstance setActive:NO error:nil];
    }
}
#endif // Q_OS_IOS

- (AVFMediaPlayerObserver *) initWithMediaPlayerSession:(AVFMediaPlayer *)session
{
    if (!(self = [super init]))
        return nil;
    m_platformPlayer.player = session;
    m_bufferIsLikelyToKeepUp = FALSE;

    m_playerLayer = [AVPlayerLayer playerLayerWithPlayer:nil];
    [m_playerLayer retain];
    m_playerLayer.videoGravity = AVLayerVideoGravityResizeAspectFill;
    m_playerLayer.anchorPoint = CGPointMake(0.0f, 0.0f);
    return self;
}

- (void) setURL:(NSURL *)url mimeType:(NSString *)mimeType
{
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
        m_playerItem = nullptr;
    }
    if (m_player) {
        [m_player setRate:0.0];
        [m_player removeObserver:self forKeyPath:AVF_CURRENT_ITEM_DURATION_KEY];
        [m_player removeObserver:self forKeyPath:AVF_CURRENT_ITEM_KEY];
        [m_player removeObserver:self forKeyPath:AVF_RATE_KEY];
        [m_player replaceCurrentItemWithPlayerItem:nil];

        // Defer the release of AVPlayer to allow CoreMedia/VideoToolbox
        // dispatch queues to finish pending operations. Releasing the
        // player synchronously can cause sporadic crashes on macOS 14
        // when background threads still reference internal resources.
        AVPlayer *player = m_player;
        m_player = nullptr;
        dispatch_async(dispatch_get_main_queue(), ^{
            [player release];
        });
    }
    if (m_playerLayer)
        m_playerLayer.player = nil;
#if defined(Q_OS_IOS)
    [self setSessionActive:NO];
#endif
}

- (void) prepareToPlayAsset:(AVURLAsset *)asset
                   withKeys:(NSArray *)requestedKeys
{
    if (!m_platformPlayer)
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

    //Set the initial audio ouptut settings on new player object
    {
        m_platformPlayer.withPlatformPlayer([&](AVFMediaPlayer *player) {
            auto *audioOutput = player->m_audioOutput;
            m_player.volume = (audioOutput ? audioOutput->volume : 1.);
            m_player.muted = (audioOutput ? audioOutput->muted : true);
            player->updateAudioOutputDevice();
        });
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
    [self setSessionActive:YES];
#endif
}

-(void) assetFailedToPrepareForPlayback:(NSError *)error
{
    QMediaPlayer::Error errorCode = QMediaPlayer::FormatError;
    if (error) {
        NSError *underlyingError = error.userInfo[NSUnderlyingErrorKey];
        if (underlyingError && ![underlyingError.domain isEqualToString:AVFoundationErrorDomain])
            errorCode = QMediaPlayer::ResourceError;
    }
    m_platformPlayer.invokeWithPlatformPlayer([errorCode](AVFMediaPlayer *platformPlayer) {
        platformPlayer->processMediaLoadError(errorCode);
    });

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

    m_platformPlayer.invokeWithPlatformPlayer([](AVFMediaPlayer *platformPlayer) {
        platformPlayer->processEOS();
    });
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

                m_platformPlayer.invokeWithPlatformPlayer([](AVFMediaPlayer *platformPlayer) {
                    platformPlayer->processLoadStateChange();
                });
            }
            break;

            case AVPlayerStatusFailed:
            {
                AVPlayerItem *playerItem = static_cast<AVPlayerItem*>(object);
                [self assetFailedToPrepareForPlayback:playerItem.error];

                m_platformPlayer.invokeWithPlatformPlayer([](AVFMediaPlayer *platformPlayer) {
                    platformPlayer->processLoadStateChange();
                });
            }
            break;
        }
    } else if (context == AVFMediaPlayerObserverPresentationSizeContext) {
        QSize size(m_playerItem.presentationSize.width, m_playerItem.presentationSize.height);
        m_platformPlayer.invokeWithPlatformPlayer([size](AVFMediaPlayer *platformPlayer) {
            platformPlayer->nativeSizeChanged(size);
        });
    } else if (context == AVFMediaPlayerObserverBufferLikelyToKeepUpContext)
    {
        const bool isPlaybackLikelyToKeepUp = [m_playerItem isPlaybackLikelyToKeepUp];
        if (isPlaybackLikelyToKeepUp != m_bufferIsLikelyToKeepUp) {
            m_bufferIsLikelyToKeepUp = isPlaybackLikelyToKeepUp;
            int bufferProgress = isPlaybackLikelyToKeepUp ? 100 : 0;

            m_platformPlayer.invokeWithPlatformPlayer(
                    [bufferProgress](AVFMediaPlayer *platformPlayer) {
                platformPlayer->processBufferStateChange(bufferProgress);
            });
        }
    }
    else if (context == AVFMediaPlayerObserverTracksContext)
    {
        m_platformPlayer.invokeWithPlatformPlayer([](AVFMediaPlayer *platformPlayer) {
            platformPlayer->updateTracks();
        });
    }
    //AVPlayer "rate" property value observer.
    else if (context == AVFMediaPlayerObserverRateObservationContext) {
        //QMetaObject::invokeMethod(m_session, "setPlaybackRate",  Qt::AutoConnection, Q_ARG(qreal, [m_player rate]));
    }
    //AVPlayer "currentItem" property observer.
    //Called when the AVPlayer replaceCurrentItemWithPlayerItem:
    //replacement will/did occur.
    else if (context == AVFMediaPlayerObserverCurrentItemObservationContext) {
        AVPlayerItem *newPlayerItem = [change objectForKey:NSKeyValueChangeNewKey];
        if (m_playerItem != newPlayerItem)
            m_playerItem = newPlayerItem;
    } else if (context == AVFMediaPlayerObserverCurrentItemDurationObservationContext) {
        const CMTime time = [m_playerItem duration];
        const qint64 dur =  static_cast<qint64>(float(time.value) / float(time.timescale) * 1000.0f);

        m_platformPlayer.invokeWithPlatformPlayer([dur](AVFMediaPlayer *platformPlayer) {
            platformPlayer->processDurationChange(dur);
        });
    } else {
        [super observeValueForKeyPath:path ofObject:object change:change context:context];
    }
}

- (void)clearSession
{
#ifdef QT_DEBUG_AVF
    qDebug() << Q_FUNC_INFO;
#endif
    m_platformPlayer.clear();
}

- (void)notifySeekComplete
{
    m_platformPlayer.withPlatformPlayer([](AVFMediaPlayer *player) {
        player->seekCompleted();
    });
}

- (void) dealloc
{
#ifdef QT_DEBUG_AVF
    qDebug() << Q_FUNC_INFO;
#endif
    [self unloadMedia];

    m_platformPlayer.clear();

    if (m_URL) {
        [m_URL release];
    }

    [m_mimeType release];
    [m_playerLayer release];
    // 'videoTrack' is a 'retain' property, but still needs a
    // manual 'release' (i.e. setting to nil):
    self.videoTrack = nil;
    [super dealloc];
}

- (BOOL) resourceLoader:(AVAssetResourceLoader *)resourceLoader shouldWaitForLoadingOfRequestedResource:(AVAssetResourceLoadingRequest *)loadingRequest
{
    Q_UNUSED(resourceLoader);

    if (![loadingRequest.request.URL.scheme isEqualToString:@"iodevice"])
        return NO;

    auto result = m_platformPlayer.withPlatformPlayer([&](AVFMediaPlayer *platformPlayer) {
        QIODevice *device = platformPlayer->mediaStream();
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
            QByteArray buffer;
            buffer.resize(maxBytes);

            NSInteger submitted = 0;
            while (submitted < requestedLength) {
                qint64 len = device->read(buffer.data(), maxBytes);
                if (len < 1)
                    break;

                [loadingRequest.dataRequest respondWithData:[NSData dataWithBytes:buffer.constData()
                                                                           length:len]];
                submitted += len;
            }

            // Finish loading even if not all bytes submitted.
            [loadingRequest finishLoading];
        }

        return YES;
    });

    return result.value_or(NO);
}
@end

AVFMediaPlayer::AVFMediaPlayer(QMediaPlayer *player)
    : QObject(player),
      QPlatformMediaPlayer(player),
      m_mediaStream(nullptr),
      m_rate(1.0),
      m_requestedPosition(-1),
      m_duration(0),
      m_bufferProgress(0)
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

    // Unload media before the C++ side is torn down, so that
    // CoreMedia/VideoToolbox threads don't outlive our objects.
    [m_observer unloadMedia];

    //Detatch the session from the sessionObserver (which could still be alive trying to communicate with this session).
    [m_observer clearSession];
    [m_observer release];
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

    if (m_videoOutput && state() != QMediaPlayer::StoppedState)
        m_videoOutput->setLayer([m_observer playerLayer]);
}

AVAsset *AVFMediaPlayer::currentAssetHandle()
{
#ifdef QT_DEBUG_AVF
    qDebug() << Q_FUNC_INFO;
#endif
    AVAsset *currentAsset = [[m_observer playerItem] asset];
    return currentAsset;
}

QUrl AVFMediaPlayer::media() const
{
    return m_resources;
}

QIODevice *AVFMediaPlayer::mediaStream() const
{
    return m_mediaStream;
}

static void setURL(AVFMediaPlayerObserver *observer, const QUrl &url, const QString &mimeType = QString())
{
    QUrl resolvedUrl = url;
    // AVFoundation cannot handle file URLs with a relative path
    if (url.isLocalFile() && !QDir::isAbsolutePath(url.path()))
        resolvedUrl = QUrl::fromLocalFile(QFileInfo(url.path()).absoluteFilePath());
    NSURL *nsurl = resolvedUrl.toNSURL();
    [observer setURL:nsurl mimeType:[NSString stringWithUTF8String:mimeType.toLatin1().constData()]];
}


void AVFMediaPlayer::setMedia(const QUrl &content, QIODevice *stream)
{
#ifdef QT_DEBUG_AVF
    qDebug() << Q_FUNC_INFO << content.request().url();
#endif

    [m_observer unloadMedia];

    m_resources = content;
    resetStream(stream);

    m_requestedPosition = -1;
    orientationChanged(QtVideo::Rotation::None, false);
    positionChanged(position());
    if (m_duration != 0) {
        m_duration = 0;
        durationChanged(0);
    }
    if (!m_metaData.isEmpty()) {
        m_metaData.clear();
        metaDataChanged();
    }
    resetBufferProgress();
    for (int i = 0; i < QPlatformMediaPlayer::NTrackTypes; ++i) {
        tracks[i].clear();
        nativeTracks[i].clear();
    }
    tracksChanged();

    if (!m_mediaStream && content.isEmpty()) {
        seekableChanged(false);
        audioAvailableChanged(false);
        videoAvailableChanged(false);

        mediaStatusChanged(QMediaPlayer::NoMedia);
        stateChanged(QMediaPlayer::StoppedState);

        return;
    }

    mediaStatusChanged(QMediaPlayer::LoadingMedia);

    if (m_mediaStream) {
        // If there is a data, try to load it,
        // otherwise wait for readyRead.
        if (m_mediaStream->size())
            loadMediaStream();
    } else {
        //Load AVURLAsset
        //initialize asset using content's URL
        setURL(m_observer, m_resources);
    }

    stateChanged(QMediaPlayer::StoppedState);
}

qint64 AVFMediaPlayer::position() const
{
    AVPlayerItem *playerItem = [m_observer playerItem];

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

QMediaTimeRange AVFMediaPlayer::availablePlaybackRanges() const
{
    AVPlayerItem *playerItem = [m_observer playerItem];

    if (!playerItem)
        return {};

    if (state() == QMediaPlayer::StoppedState)
        return {};

    QMediaTimeRange timeRanges;

    NSArray *ranges = [playerItem loadedTimeRanges];
    for (NSValue *timeRange in ranges) {
        CMTimeRange currentTimeRange = [timeRange CMTimeRangeValue];
        qint64 startTime = qint64(float(currentTimeRange.start.value) / currentTimeRange.start.timescale * 1000.0);
        timeRanges.addInterval(startTime, startTime + qint64(float(currentTimeRange.duration.value) / currentTimeRange.duration.timescale * 1000.0));
    }
    return timeRanges;
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
        connect(m_audioOutput->q, &QAudioOutput::deviceChanged, this, &AVFMediaPlayer::updateAudioOutputDevice);
        connect(m_audioOutput->q, &QAudioOutput::volumeChanged, this, &AVFMediaPlayer::setVolume);
        connect(m_audioOutput->q, &QAudioOutput::mutedChanged, this, &AVFMediaPlayer::setMuted);
        //connect(m_audioOutput->q, &QAudioOutput::audioRoleChanged, this, &AVFMediaPlayer::setAudioRole);
    }
    updateAudioOutputDevice();
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

    if (QtPrivate::fuzzyCompare(m_rate, rate))
        return;

    m_rate = rate;

    AVPlayer *player = [m_observer player];
    if (player && state() == QMediaPlayer::PlayingState)
        [player setRate:m_rate];

    playbackRateChanged(m_rate);
}

void AVFMediaPlayer::setPosition(qint64 pos)
{
#ifdef QT_DEBUG_AVF
    qDebug() << Q_FUNC_INFO << pos;
#endif

    if (pos == position())
        return;

    AVPlayerItem *playerItem = [m_observer playerItem];
    if (!playerItem) {
        m_requestedPosition = pos;
        positionChanged(m_requestedPosition);
        return;
    }

    if (!isSeekable()) {
        if (m_requestedPosition != -1) {
            m_requestedPosition = -1;
            positionChanged(position());
        }
        return;
    }

    pos = qMax(qint64(0), pos);
    if (duration() > 0)
        pos = qMin(pos, duration());
    m_requestedPosition = pos;

    CMTime newTime = [playerItem currentTime];
    newTime.value = (pos / 1000.0f) * newTime.timescale;
    AVFMediaPlayerObserver *observer = m_observer;
    [playerItem seekToTime:newTime toleranceBefore:kCMTimeZero toleranceAfter:kCMTimeZero
                           completionHandler:^(BOOL finished) {
                                if (finished)
                                    [observer notifySeekComplete];
                           }];

    positionChanged(pos);

    // Reset media status if the current status is EndOfMedia
    if (mediaStatus() == QMediaPlayer::EndOfMedia) {
        QMediaPlayer::MediaStatus newMediaStatus = (state() == QMediaPlayer::PausedState)
                ? QMediaPlayer::BufferedMedia
                : QMediaPlayer::LoadedMedia;
        mediaStatusChanged(newMediaStatus);
    }
}

void AVFMediaPlayer::play()
{
#ifdef QT_DEBUG_AVF
    qDebug() << Q_FUNC_INFO << "currently: " << state();
#endif

    if (mediaStatus() == QMediaPlayer::NoMedia || mediaStatus() == QMediaPlayer::InvalidMedia)
        return;

    if (state() == QMediaPlayer::PlayingState)
        return;

    if (state() != QMediaPlayer::PausedState)
        resetCurrentLoop();

    if (m_videoOutput && m_videoSink)
        m_videoOutput->setLayer([m_observer playerLayer]);

    // Reset media status if the current status is EndOfMedia
    if (mediaStatus() == QMediaPlayer::EndOfMedia)
        setPosition(0);

    if (mediaStatus() == QMediaPlayer::LoadedMedia
        || mediaStatus() == QMediaPlayer::BufferedMedia) {
        // Setting the rate starts playback
        [[m_observer player] setRate:m_rate];
    }

    processLoadStateChange(QMediaPlayer::PlayingState);

    stateChanged(QMediaPlayer::PlayingState);
    m_playbackTimer.start(100);
}

void AVFMediaPlayer::pause()
{
#ifdef QT_DEBUG_AVF
    qDebug() << Q_FUNC_INFO << "currently: " << state();
#endif

    if (mediaStatus() == QMediaPlayer::NoMedia || mediaStatus() == QMediaPlayer::InvalidMedia)
        return;

    if (state() == QMediaPlayer::PausedState)
        return;

    stateChanged(QMediaPlayer::PausedState);

    if (m_videoOutput && m_videoSink)
        m_videoOutput->setLayer([m_observer playerLayer]);

    [[m_observer player] pause];

    // Reset media status if the current status is EndOfMedia
    if (mediaStatus() == QMediaPlayer::EndOfMedia)
        setPosition(0);

    positionChanged(position());
    m_playbackTimer.stop();
}

void AVFMediaPlayer::stop()
{
#ifdef QT_DEBUG_AVF
    qDebug() << Q_FUNC_INFO << "currently: " << state();
#endif

    if (state() == QMediaPlayer::StoppedState && mediaStatus() != QMediaPlayer::EndOfMedia)
        return;

    // AVPlayer doesn't have stop(), only pause() and play().
    [[m_observer player] pause];
    setPosition(0);

    if (m_videoOutput)
        m_videoOutput->setLayer(nullptr);

    resetBufferProgress();

    if (mediaStatus() == QMediaPlayer::BufferedMedia || mediaStatus() == QMediaPlayer::EndOfMedia)
        mediaStatusChanged(QMediaPlayer::LoadedMedia);

    stateChanged(QMediaPlayer::StoppedState);
    m_playbackTimer.stop();
}

void AVFMediaPlayer::setVolume(float volume)
{
#ifdef QT_DEBUG_AVF
    qDebug() << Q_FUNC_INFO << volume;
#endif

    AVPlayer *player = [m_observer player];
    if (player)
        player.volume = volume;
}

void AVFMediaPlayer::setMuted(bool muted)
{
#ifdef QT_DEBUG_AVF
    qDebug() << Q_FUNC_INFO << muted;
#endif

    AVPlayer *player = [m_observer player];
    if (player)
        player.muted = muted;
}

void AVFMediaPlayer::updateAudioOutputDevice()
{
#ifdef Q_OS_MACOS
    AVPlayer *player = [m_observer player];
    if (!player)
        return;

    if (!m_audioOutput || m_audioOutput->device.id().isEmpty()) {
        if (!m_audioOutput)
            player.muted = true;
        player.audioOutputDeviceUniqueID = nil;
    } else {
        NSString *str = QString::fromUtf8(m_audioOutput->device.id()).toNSString();
        player.audioOutputDeviceUniqueID = str;
    }
#endif
}

void AVFMediaPlayer::processEOS()
{
    if (doLoop()) {
        positionChanged(duration());
        setPosition(0);
        [[m_observer player] setRate:m_rate];
        return;
    }

    //AVPlayerItem has reached end of track/stream
#ifdef QT_DEBUG_AVF
    qDebug() << Q_FUNC_INFO;
#endif
    positionChanged(position());

    if (m_videoOutput)
        m_videoOutput->setLayer(nullptr);

    resetBufferProgress();

    stateChanged(QMediaPlayer::StoppedState);
    mediaStatusChanged(QMediaPlayer::EndOfMedia);
}

void AVFMediaPlayer::processLoadStateChange(QMediaPlayer::PlaybackState newState)
{
    AVPlayerStatus currentStatus = [[m_observer player] status];

#ifdef QT_DEBUG_AVF
    qDebug() << Q_FUNC_INFO << currentStatus << ", " << mediaStatus() << ", " << newState;
#endif

    if (mediaStatus() == QMediaPlayer::NoMedia)
        return;

    if (currentStatus == AVPlayerStatusReadyToPlay) {

        AVPlayerItem *playerItem = [m_observer playerItem];

        applyPitchCompensation(m_pitchCompensationEnabled);

        // get the meta data
        m_metaData = AVFMetaData::fromAsset(playerItem.asset);
        metaDataChanged();
        updateTracks();

        if (playerItem) {
            seekableChanged([[playerItem seekableTimeRanges] count] > 0);

            // Get the native size of the video, and reset the bounds of the player layer
            AVPlayerLayer *playerLayer = [m_observer playerLayer];
            if (m_observer.videoTrack && playerLayer) {
                if (!playerLayer.bounds.size.width || !playerLayer.bounds.size.height) {
                    playerLayer.bounds = CGRectMake(0.0f, 0.0f,
                                                    m_observer.videoTrack.assetTrack.naturalSize.width,
                                                    m_observer.videoTrack.assetTrack.naturalSize.height);
                }
            }

            if (m_requestedPosition != -1)
                setPosition(m_requestedPosition);
        }

        QMediaPlayer::MediaStatus newStatus = (newState != QMediaPlayer::StoppedState)
                ? QMediaPlayer::BufferedMedia
                : QMediaPlayer::LoadedMedia;

        if (newStatus != mediaStatus()) {
            if (newStatus == QMediaPlayer::BufferedMedia
                && mediaStatus() == QMediaPlayer::LoadingMedia) {
                // Emit intermediate transitions to match expected signal sequence
                mediaStatusChanged(QMediaPlayer::LoadedMedia);
                mediaStatusChanged(QMediaPlayer::BufferingMedia);
            } else if (newStatus == QMediaPlayer::BufferedMedia
                       && mediaStatus() == QMediaPlayer::LoadedMedia) {
                mediaStatusChanged(QMediaPlayer::BufferingMedia);
            }
            mediaStatusChanged(newStatus);
        }
    }

    if (newState == QMediaPlayer::PlayingState && [m_observer player]) {
        // Setting the rate is enough to start playback, no need to call play()
        [[m_observer player] setRate:m_rate];
        m_playbackTimer.start();
    }
}


void AVFMediaPlayer::processLoadStateChange()
{
    processLoadStateChange(state());
}


void AVFMediaPlayer::processLoadStateFailure()
{
    stateChanged(QMediaPlayer::StoppedState);
}

void AVFMediaPlayer::processBufferStateChange(int bufferProgress)
{
    if (state() == QMediaPlayer::StoppedState)
        return;

    if (bufferProgress == m_bufferProgress)
        return;

    auto status = mediaStatus();
    // Buffered -> unbuffered.
    if (!bufferProgress) {
        status = QMediaPlayer::StalledMedia;
    } else if (status == QMediaPlayer::StalledMedia) {
        status = QMediaPlayer::BufferedMedia;
        // Resume playback.
        if (state() == QMediaPlayer::PlayingState) {
            [[m_observer player] setRate:m_rate];
            m_playbackTimer.start();
        }
    }

    mediaStatusChanged(status);

    m_bufferProgress = bufferProgress;
    bufferProgressChanged(bufferProgress / 100.);
}

void AVFMediaPlayer::processDurationChange(qint64 duration)
{
    if (duration == m_duration)
        return;

    m_duration = duration;
    durationChanged(duration);
}

void AVFMediaPlayer::processPositionChange()
{
    if (state() == QMediaPlayer::StoppedState)
        return;

    positionChanged(position());
}

void AVFMediaPlayer::processMediaLoadError(QMediaPlayer::Error errorCode)
{
    if (m_requestedPosition != -1) {
        m_requestedPosition = -1;
        positionChanged(position());
    }

    setInvalidMediaWithError(errorCode, tr("Failed to load media"));
}

void AVFMediaPlayer::seekCompleted()
{
    m_requestedPosition = -1;
}

void AVFMediaPlayer::streamReady()
{
    loadMediaStream();
}

void AVFMediaPlayer::loadMediaStream()
{
    QString suffix;
    if (!m_resources.isEmpty())
        suffix = QFileInfo(m_resources.path()).suffix();
    if (suffix.isEmpty() && m_mediaStream)
        suffix = QMimeDatabase().mimeTypeForData(m_mediaStream).preferredSuffix();
    const QString url = QStringLiteral("iodevice:///iodevice.") + suffix;
    setURL(m_observer, QUrl(url), suffix);
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
    bool hasAudio = false;
    bool hasVideo = false;
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
                    hasAudio = true;
                } else if ([assetTrack.mediaType isEqualToString:AVMediaTypeVideo]) {
                    qtTrack = QPlatformMediaPlayer::VideoStream;
                    hasVideo = true;
                    if (m_observer.videoTrack != track) {
                        m_observer.videoTrack = track;
                        bool isMirrored = false;
                        QtVideo::Rotation orientation = QtVideo::Rotation::None;
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
    audioAvailableChanged(hasAudio);
    videoAvailableChanged(hasVideo);
    tracksChanged();
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
#if defined(Q_OS_VISIONOS)
            [asset loadMediaSelectionGroupForMediaCharacteristic:AVMediaCharacteristicLegible
                                               completionHandler:[=](AVMediaSelectionGroup *group, NSError *error) {
                                                   // FIXME: handle error
                                                   if (error)
                                                       return;
                                                   auto *options = group.options;
                                                   if (options.count)
                                                       [playerItem selectMediaOption:options.firstObject inMediaSelectionGroup:group];
                                               }];
#else
            AVMediaSelectionGroup *group = [asset mediaSelectionGroupForMediaCharacteristic:AVMediaCharacteristicLegible];
            if (!group)
                return;
            auto *options = group.options;
            if (options.count)
                [playerItem selectMediaOption:options.firstObject inMediaSelectionGroup:group];
#endif
        }
    }
    for (int i = 0; i < t.count(); ++i)
        t.at(i).enabled = (i == index);
    activeTracksChanged();
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

void AVFMediaPlayer::applyPitchCompensation(bool enabled)
{
    AVPlayerItem *playerItem = [m_observer playerItem];
    if (playerItem) {
        if (enabled)
            playerItem.audioTimePitchAlgorithm = AVAudioTimePitchAlgorithmSpectral;
        else
            playerItem.audioTimePitchAlgorithm = AVAudioTimePitchAlgorithmVarispeed;
    }
}

void AVFMediaPlayer::resetBufferProgress()
{
    if (m_bufferProgress != 0) {
        m_bufferProgress = 0;
        bufferProgressChanged(0);
    }
}

void AVFMediaPlayer::nativeSizeChanged(QSize size)
{
    if (!m_videoSink)
        return;
    m_videoSink->setNativeSize(size);
}

void AVFMediaPlayer::orientationChanged(QtVideo::Rotation rotation, bool mirrored)
{
    if (!m_videoOutput)
        return;

    m_videoOutput->setVideoRotation(rotation);
    m_videoOutput->setVideoMirrored(mirrored);
}

void AVFMediaPlayer::videoOrientationForAssetTrack(AVAssetTrack *videoTrack,
                                                   QtVideo::Rotation &angle,
                                                   bool &mirrored)
{
    angle = QtVideo::Rotation::None;
    mirrored = false;
    if (videoTrack) {
        CGAffineTransform transform = videoTrack.preferredTransform;
        if (CGAffineTransformIsIdentity(transform))
            return;

        // determinant < 0 means the transform includes a reflection (mirror)
        qreal det = transform.a * transform.d - transform.b * transform.c;
        mirrored = (det < 0.0);

        // Factor out mirror before computing rotation angle.
        // Negating the first column of a mirrored matrix yields a pure rotation.
        qreal ra = mirrored ? -transform.a : transform.a;
        qreal rb = mirrored ? -transform.b : transform.b;

        qreal degrees = qRadiansToDegrees(qAtan2(rb, ra));
        if (degrees < 0)
            degrees += 360.0;

        if (QtPrivate::fuzzyCompare(degrees, qreal(90))
            || QtPrivate::fuzzyCompare(degrees, qreal(-270))) {
            angle = QtVideo::Rotation::Clockwise90;
        } else if (QtPrivate::fuzzyCompare(degrees, qreal(-90))
                   || QtPrivate::fuzzyCompare(degrees, qreal(270))) {
            angle = QtVideo::Rotation::Clockwise270;
        } else if (QtPrivate::fuzzyCompare(degrees, qreal(180))
                   || QtPrivate::fuzzyCompare(degrees, qreal(-180))) {
            angle = QtVideo::Rotation::Clockwise180;
        }
    }
}

void AVFMediaPlayer::setPitchCompensation(bool enabled)
{
    if (m_pitchCompensationEnabled == enabled)
        return;

    applyPitchCompensation(enabled);

    m_pitchCompensationEnabled = enabled;
    pitchCompensationChanged(enabled);
}

bool AVFMediaPlayer::pitchCompensation() const
{
    return m_pitchCompensationEnabled;
}

QPlatformMediaPlayer::PitchCompensationAvailability
AVFMediaPlayer::pitchCompensationAvailability() const
{
    return QPlatformMediaPlayer::PitchCompensationAvailability::Available;
}

#include "moc_avfmediaplayer_p.cpp"
