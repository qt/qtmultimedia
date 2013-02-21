/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#import <QTKit/QTDataReference.h>
#import <QTKit/QTMovie.h>

#include "qt7backend.h"

#include "qt7playersession.h"
#include "qt7playercontrol.h"
#include "qt7videooutput.h"

#include <QtNetwork/qnetworkcookie.h>
#include <private/qmediaplaylistnavigator_p.h>

#include <CoreFoundation/CoreFoundation.h>
#include <Foundation/Foundation.h>

#include <QtCore/qdatetime.h>
#include <QtCore/qurl.h>

#include <QtCore/qdebug.h>

QT_USE_NAMESPACE

//#define QT_DEBUG_QT7

@interface QTMovieObserver : NSObject
{
@private
    QT7PlayerSession *m_session;
    QTMovie *m_movie;
}

- (QTMovieObserver *) initWithPlayerSession:(QT7PlayerSession*)session;
- (void) setMovie:(QTMovie *)movie;
- (void) processEOS:(NSNotification *)notification;
- (void) processLoadStateChange:(NSNotification *)notification;
- (void) processVolumeChange:(NSNotification *)notification;
- (void) processNaturalSizeChange :(NSNotification *)notification;
- (void) processPositionChange :(NSNotification *)notification;
@end

@implementation QTMovieObserver

- (QTMovieObserver *) initWithPlayerSession:(QT7PlayerSession*)session
{
    if (!(self = [super init]))
        return nil;

    self->m_session = session;
    return self;
}

- (void) setMovie:(QTMovie *)movie
{
    if (m_movie == movie)
        return;

    if (m_movie) {
        [[NSNotificationCenter defaultCenter] removeObserver:self];
        [m_movie release];
    }

    m_movie = movie;

    if (movie) {
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(processEOS:)
                                                     name:QTMovieDidEndNotification
                                                   object:m_movie];

        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(processLoadStateChange:)
                                                     name:QTMovieLoadStateDidChangeNotification
                                                   object:m_movie];

        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(processVolumeChange:)
                                                     name:QTMovieVolumeDidChangeNotification
                                                   object:m_movie];

        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(processPositionChange:)
                                                     name:QTMovieTimeDidChangeNotification
                                                   object:m_movie];

        if (QSysInfo::MacintoshVersion >= QSysInfo::MV_10_6) {
            [[NSNotificationCenter defaultCenter] addObserver:self
                                                     selector:@selector(processNaturalSizeChange:)
                                                         name:@"QTMovieNaturalSizeDidChangeNotification"
                                                       object:m_movie];

        }
        else {
            [[NSNotificationCenter defaultCenter] addObserver:self
                                                     selector:@selector(processNaturalSizeChange:)
                                                         name:QTMovieEditedNotification
                                                       object:m_movie];
        }

        [movie retain];
    }
}

- (void) processEOS:(NSNotification *)notification
{
    Q_UNUSED(notification);
    QMetaObject::invokeMethod(m_session, "processEOS", Qt::AutoConnection);
}

- (void) processLoadStateChange:(NSNotification *)notification
{
    Q_UNUSED(notification);
    QMetaObject::invokeMethod(m_session, "processLoadStateChange", Qt::AutoConnection);
}

- (void) processVolumeChange:(NSNotification *)notification
{
    Q_UNUSED(notification);
    QMetaObject::invokeMethod(m_session, "processVolumeChange", Qt::AutoConnection);
}

- (void) processNaturalSizeChange :(NSNotification *)notification
{
    Q_UNUSED(notification);
    QMetaObject::invokeMethod(m_session, "processNaturalSizeChange", Qt::AutoConnection);
}

- (void) processPositionChange :(NSNotification *)notification
{
    Q_UNUSED(notification);
    QMetaObject::invokeMethod(m_session, "processPositionChange", Qt::AutoConnection);
}

@end

static inline NSString *qString2CFStringRef(const QString &string)
{
    return [NSString stringWithCharacters:reinterpret_cast<const UniChar *>(string.unicode()) length:string.length()];
}

QT7PlayerSession::QT7PlayerSession(QObject *parent)
   : QObject(parent)
   , m_QTMovie(0)
   , m_state(QMediaPlayer::StoppedState)
   , m_mediaStatus(QMediaPlayer::NoMedia)
   , m_mediaStream(0)
   , m_videoOutput(0)
   , m_muted(false)
   , m_tryingAsync(false)
   , m_volume(100)
   , m_rate(1.0)
   , m_duration(0)
   , m_videoAvailable(false)
   , m_audioAvailable(false)
{
    m_movieObserver = [[QTMovieObserver alloc] initWithPlayerSession:this];
}

QT7PlayerSession::~QT7PlayerSession()
{
    if (m_videoOutput)
        m_videoOutput->setMovie(0);

    [(QTMovieObserver*)m_movieObserver setMovie:nil];
    [(QTMovieObserver*)m_movieObserver release];
    [(QTMovie*)m_QTMovie release];
}

void *QT7PlayerSession::movie() const
{
    return m_QTMovie;
}

void QT7PlayerSession::setVideoOutput(QT7VideoOutput *output)
{
    if (m_videoOutput == output)
        return;

    if (m_videoOutput)
        m_videoOutput->setMovie(0);

    m_videoOutput = output;

    if (m_videoOutput && m_state != QMediaPlayer::StoppedState)
        m_videoOutput->setMovie(m_QTMovie);
}

qint64 QT7PlayerSession::position() const
{
    if (!m_QTMovie)
        return 0;

    QTTime qtTime = [(QTMovie*)m_QTMovie currentTime];

    return static_cast<quint64>(float(qtTime.timeValue) / float(qtTime.timeScale) * 1000.0f);
}

qint64 QT7PlayerSession::duration() const
{
    if (!m_QTMovie)
        return 0;

    QTTime qtTime = [(QTMovie*)m_QTMovie duration];

    return static_cast<quint64>(float(qtTime.timeValue) / float(qtTime.timeScale) * 1000.0f);
}

QMediaPlayer::State QT7PlayerSession::state() const
{
    return m_state;
}

QMediaPlayer::MediaStatus QT7PlayerSession::mediaStatus() const
{
    return m_mediaStatus;
}

int QT7PlayerSession::bufferStatus() const
{
    return 100;
}

int QT7PlayerSession::volume() const
{
    return m_volume;
}

bool QT7PlayerSession::isMuted() const
{
    return m_muted;
}

bool QT7PlayerSession::isSeekable() const
{
    return true;
}

#ifndef QUICKTIME_C_API_AVAILABLE
@interface QTMovie(QtExtensions)
- (NSArray*)loadedRanges;
- (QTTime)maxTimeLoaded;
@end
#endif

QMediaTimeRange QT7PlayerSession::availablePlaybackRanges() const
{
    QTMovie *movie = (QTMovie*)m_QTMovie;
#ifndef QUICKTIME_C_API_AVAILABLE
    AutoReleasePool pool;
    if ([movie respondsToSelector:@selector(loadedRanges)]) {
        QMediaTimeRange rc;
        NSArray *r = [movie loadedRanges];
        for (NSValue *tr in r) {
            QTTimeRange timeRange = [tr QTTimeRangeValue];
            qint64 startTime = qint64(float(timeRange.time.timeValue) / timeRange.time.timeScale * 1000.0);
            rc.addInterval(startTime, startTime + qint64(float(timeRange.duration.timeValue) / timeRange.duration.timeScale * 1000.0));
        }
        return rc;
    }
    else if ([movie respondsToSelector:@selector(maxTimeLoaded)]) {
        QTTime loadedTime = [movie maxTimeLoaded];
        return QMediaTimeRange(0, qint64(float(loadedTime.timeValue) / loadedTime.timeScale * 1000.0));
    }
#else
    TimeValue loadedTime;
    TimeScale scale;
    Movie m = [movie quickTimeMovie];
    if (GetMaxLoadedTimeInMovie(m, &loadedTime) == noErr) {
        scale = GetMovieTimeScale(m);
        return QMediaTimeRange(0, qint64(float(loadedTime) / scale * 1000.0f));
    }
#endif
    return QMediaTimeRange(0, duration());
}

qreal QT7PlayerSession::playbackRate() const
{
    return m_rate;
}

void QT7PlayerSession::setPlaybackRate(qreal rate)
{
    if (qFuzzyCompare(m_rate, rate))
        return;

    m_rate = rate;

    if (m_QTMovie != 0 && m_state == QMediaPlayer::PlayingState) {
        AutoReleasePool pool;
        float preferredRate = [[(QTMovie*)m_QTMovie attributeForKey:@"QTMoviePreferredRateAttribute"] floatValue];
        [(QTMovie*)m_QTMovie setRate:preferredRate * m_rate];
    }
}

void QT7PlayerSession::setPosition(qint64 pos)
{
    if ( !isSeekable() || pos == position())
        return;

    if (duration() > 0)
        pos = qMin(pos, duration());

    QTTime newQTTime = [(QTMovie*)m_QTMovie currentTime];
    newQTTime.timeValue = (pos / 1000.0f) * newQTTime.timeScale;
    [(QTMovie*)m_QTMovie setCurrentTime:newQTTime];

    //reset the EndOfMedia status position is changed after playback is finished
    if (m_mediaStatus == QMediaPlayer::EndOfMedia)
        processLoadStateChange();
}

void QT7PlayerSession::play()
{
    if (m_state == QMediaPlayer::PlayingState)
        return;

    m_state = QMediaPlayer::PlayingState;

    if (m_videoOutput)
        m_videoOutput->setMovie(m_QTMovie);

    //reset the EndOfMedia status if the same file is played again
    if (m_mediaStatus == QMediaPlayer::EndOfMedia)
        processLoadStateChange();

    AutoReleasePool pool;
    float preferredRate = [[(QTMovie*)m_QTMovie attributeForKey:@"QTMoviePreferredRateAttribute"] floatValue];
    [(QTMovie*)m_QTMovie setRate:preferredRate * m_rate];

    processLoadStateChange();
    Q_EMIT stateChanged(m_state);
}

void QT7PlayerSession::pause()
{
    if (m_state == QMediaPlayer::PausedState)
        return;

    m_state = QMediaPlayer::PausedState;

    if (m_videoOutput)
        m_videoOutput->setMovie(m_QTMovie);

    //reset the EndOfMedia status if the same file is played again
    if (m_mediaStatus == QMediaPlayer::EndOfMedia)
        processLoadStateChange();

    [(QTMovie*)m_QTMovie setRate:0];

    processLoadStateChange();
    Q_EMIT stateChanged(m_state);
}

void QT7PlayerSession::stop()
{
    if (m_state == QMediaPlayer::StoppedState)
        return;

    m_state = QMediaPlayer::StoppedState;

    [(QTMovie*)m_QTMovie setRate:0];
    setPosition(0);

    if (m_videoOutput)
        m_videoOutput->setMovie(0);

    processLoadStateChange();
    Q_EMIT stateChanged(m_state);
    Q_EMIT positionChanged(position());
}

void QT7PlayerSession::setVolume(int volume)
{
    if (m_volume == volume)
        return;

    m_volume = volume;

    if (m_QTMovie != 0)
        [(QTMovie*)m_QTMovie setVolume:m_volume / 100.0f];

    Q_EMIT volumeChanged(m_volume);
}

void QT7PlayerSession::setMuted(bool muted)
{
    if (m_muted == muted)
        return;

    m_muted = muted;

    if (m_QTMovie != 0)
        [(QTMovie*)m_QTMovie setMuted:m_muted];

    Q_EMIT mutedChanged(muted);
}

QMediaContent QT7PlayerSession::media() const
{
    return m_resources;
}

const QIODevice *QT7PlayerSession::mediaStream() const
{
    return m_mediaStream;
}

void QT7PlayerSession::setMedia(const QMediaContent &content, QIODevice *stream)
{
    AutoReleasePool pool;

#ifdef QT_DEBUG_QT7
    qDebug() << Q_FUNC_INFO << content.canonicalUrl();
#endif

    if (m_QTMovie) {
        [(QTMovieObserver*)m_movieObserver setMovie:nil];

        if (m_videoOutput)
            m_videoOutput->setMovie(0);

        [(QTMovie*)m_QTMovie release];
        m_QTMovie = 0;
        m_resourceHandler.clear();
    }

    m_resources = content;
    m_mediaStream = stream;
    QMediaPlayer::MediaStatus oldMediaStatus = m_mediaStatus;

    if (content.isNull()) {
        m_mediaStatus = QMediaPlayer::NoMedia;
        if (m_state != QMediaPlayer::StoppedState)
            Q_EMIT stateChanged(m_state = QMediaPlayer::StoppedState);

        if (m_mediaStatus != oldMediaStatus)
            Q_EMIT mediaStatusChanged(m_mediaStatus);
        Q_EMIT positionChanged(position());
        return;
    }

    m_mediaStatus = QMediaPlayer::LoadingMedia;
    if (m_mediaStatus != oldMediaStatus)
        Q_EMIT mediaStatusChanged(m_mediaStatus);

    QNetworkRequest request = content.canonicalResource().request();

    QVariant cookies = request.header(QNetworkRequest::CookieHeader);
    if (cookies.isValid()) {
        NSHTTPCookieStorage *store = [NSHTTPCookieStorage sharedHTTPCookieStorage];
        QList<QNetworkCookie> cookieList = cookies.value<QList<QNetworkCookie> >();

        Q_FOREACH (const QNetworkCookie &requestCookie, cookieList) {
            NSMutableDictionary *p = [NSMutableDictionary dictionaryWithObjectsAndKeys:
                               qString2CFStringRef(requestCookie.name()), NSHTTPCookieName,
                               qString2CFStringRef(requestCookie.value()), NSHTTPCookieValue,
                               qString2CFStringRef(requestCookie.domain()), NSHTTPCookieDomain,
                               qString2CFStringRef(requestCookie.path()), NSHTTPCookiePath,
                               nil
                               ];
            if (requestCookie.isSessionCookie())
                [p setObject:[NSString stringWithUTF8String:"TRUE"] forKey:NSHTTPCookieDiscard];
            else
                [p setObject:[NSDate dateWithTimeIntervalSince1970:requestCookie.expirationDate().toTime_t()] forKey:NSHTTPCookieExpires];

            [store setCookie:[NSHTTPCookie cookieWithProperties:p]];
        }
    }

    // Attempt multiple times to open the movie.
    // First try - attempt open in async mode
    openMovie(true);

    Q_EMIT positionChanged(position());
}

void QT7PlayerSession::openMovie(bool tryAsync)
{
    QUrl requestUrl = m_resources.canonicalResource().request().url();
    if (requestUrl.scheme().isEmpty())
        requestUrl.setScheme(QLatin1String("file"));

#ifdef QT_DEBUG_QT7
    qDebug() << Q_FUNC_INFO << requestUrl;
#endif

    NSError *err = 0;
    NSString *urlString = [NSString stringWithUTF8String:requestUrl.toEncoded().constData()];

    NSMutableDictionary *attr = [NSMutableDictionary dictionaryWithObjectsAndKeys:
                [NSNumber numberWithBool:YES], QTMovieOpenAsyncOKAttribute,
                [NSNumber numberWithBool:YES], QTMovieIsActiveAttribute,
                [NSNumber numberWithBool:YES], QTMovieResolveDataRefsAttribute,
                [NSNumber numberWithBool:YES], QTMovieDontInteractWithUserAttribute,
                nil];


    if (requestUrl.scheme() == QLatin1String("qrc")) {
        // Load from Qt resource
        m_resourceHandler.setResourceFile(QLatin1Char(':') + requestUrl.path());
        if (!m_resourceHandler.isValid()) {
            Q_EMIT error(QMediaPlayer::FormatError, tr("Attempting to play invalid Qt resource"));
            return;
        }

        CFDataRef resourceData =
                CFDataCreateWithBytesNoCopy(0, m_resourceHandler.data(), m_resourceHandler.size(), kCFAllocatorNull);

        QTDataReference *dataReference =
                [QTDataReference dataReferenceWithReferenceToData:(NSData*)resourceData
                                                             name:qString2CFStringRef(requestUrl.path())
                                                         MIMEType:nil];

        [attr setObject:dataReference forKey:QTMovieDataReferenceAttribute];

        CFRelease(resourceData);
    } else {
        [attr setObject:[NSURL URLWithString:urlString] forKey:QTMovieURLAttribute];
    }

    if (tryAsync && QSysInfo::MacintoshVersion >= QSysInfo::MV_10_6) {
        [attr setObject:[NSNumber numberWithBool:YES] forKey:@"QTMovieOpenAsyncRequiredAttribute"];
// XXX: This is disabled for now. causes some problems with video playback for some formats
//        [attr setObject:[NSNumber numberWithBool:YES] forKey:@"QTMovieOpenForPlaybackAttribute"];
        m_tryingAsync = true;
    }
    else
        m_tryingAsync = false;

    m_QTMovie = [QTMovie movieWithAttributes:attr error:&err];
    if (err != nil) {
        // First attempt to test for inability to perform async
//        if ([err code] == QTErrorMovieOpeningCannotBeAsynchronous) { XXX: error code unknown!
        if (m_tryingAsync) {
            m_tryingAsync = false;
            err = nil;
            [attr removeObjectForKey:@"QTMovieOpenAsyncRequiredAttribute"];
            m_QTMovie = [QTMovie movieWithAttributes:attr error:&err];
        }
    }

    if (err != nil) {
        m_QTMovie = 0;
        QString description = QString::fromUtf8([[err localizedDescription] UTF8String]);
        Q_EMIT error(QMediaPlayer::FormatError, description);

#ifdef QT_DEBUG_QT7
        qDebug() << Q_FUNC_INFO << description;
#endif
    }
    else {
        [(QTMovie*)m_QTMovie retain];

        [(QTMovieObserver*)m_movieObserver setMovie:(QTMovie*)m_QTMovie];

        if (m_state != QMediaPlayer::StoppedState && m_videoOutput)
            m_videoOutput->setMovie(m_QTMovie);

        processLoadStateChange();

        [(QTMovie*)m_QTMovie setMuted:m_muted];
        [(QTMovie*)m_QTMovie setVolume:m_volume / 100.0f];
    }
}

bool QT7PlayerSession::isAudioAvailable() const
{
    if (!m_QTMovie)
        return false;

    AutoReleasePool pool;
    return [[(QTMovie*)m_QTMovie attributeForKey:@"QTMovieHasAudioAttribute"] boolValue] == YES;
}

bool QT7PlayerSession::isVideoAvailable() const
{
    if (!m_QTMovie)
        return false;

    AutoReleasePool pool;
    return [[(QTMovie*)m_QTMovie attributeForKey:@"QTMovieHasVideoAttribute"] boolValue] == YES;
}

void QT7PlayerSession::processEOS()
{
#ifdef QT_DEBUG_QT7
    qDebug() << Q_FUNC_INFO;
#endif
    Q_EMIT positionChanged(position());
    m_mediaStatus = QMediaPlayer::EndOfMedia;
    if (m_videoOutput)
        m_videoOutput->setMovie(0);
    Q_EMIT stateChanged(m_state = QMediaPlayer::StoppedState);
    Q_EMIT mediaStatusChanged(m_mediaStatus);
}

void QT7PlayerSession::processLoadStateChange()
{
    if (!m_QTMovie)
        return;

    AutoReleasePool pool;

    long state = [[(QTMovie*)m_QTMovie attributeForKey:QTMovieLoadStateAttribute] longValue];

#ifdef QT_DEBUG_QT7
    qDebug() << Q_FUNC_INFO << state;
#endif

#ifndef QUICKTIME_C_API_AVAILABLE
    enum {
      kMovieLoadStateError          = -1L,
      kMovieLoadStateLoading        = 1000,
      kMovieLoadStateLoaded         = 2000,
      kMovieLoadStatePlayable       = 10000,
      kMovieLoadStatePlaythroughOK  = 20000,
      kMovieLoadStateComplete       = 100000
    };
#endif

    if (state == kMovieLoadStateError) {
        if (m_tryingAsync) {
            NSError *error = [(QTMovie*)m_QTMovie attributeForKey:@"QTMovieLoadStateErrorAttribute"];
            if ([error code] == componentNotThreadSafeErr) {
                // Last Async check, try again with no such flag
                openMovie(false);
            }
        }
        else {
            if (m_videoOutput)
                m_videoOutput->setMovie(0);

            Q_EMIT error(QMediaPlayer::FormatError, tr("Failed to load media"));
            Q_EMIT mediaStatusChanged(m_mediaStatus = QMediaPlayer::InvalidMedia);
            Q_EMIT stateChanged(m_state = QMediaPlayer::StoppedState);
        }

        return;
    }

    QMediaPlayer::MediaStatus newStatus = QMediaPlayer::NoMedia;
    bool isPlaying = (m_state != QMediaPlayer::StoppedState);

    if (state >= kMovieLoadStatePlaythroughOK) {
        newStatus = isPlaying ? QMediaPlayer::BufferedMedia : QMediaPlayer::LoadedMedia;
    } else if (state >= kMovieLoadStatePlayable)
        newStatus = isPlaying ? QMediaPlayer::BufferingMedia : QMediaPlayer::LoadingMedia;
    else if (state >= kMovieLoadStateLoading) {
        if (!isPlaying)
            newStatus = QMediaPlayer::LoadingMedia;
        else if (m_mediaStatus >= QMediaPlayer::LoadedMedia)
            newStatus = QMediaPlayer::StalledMedia;
        else
            newStatus = QMediaPlayer::LoadingMedia;
    }

    if (state >= kMovieLoadStatePlayable &&
        m_state == QMediaPlayer::PlayingState &&
        [(QTMovie*)m_QTMovie rate] == 0) {

        float preferredRate = [[(QTMovie*)m_QTMovie attributeForKey:@"QTMoviePreferredRateAttribute"] floatValue];

        [(QTMovie*)m_QTMovie setRate:preferredRate * m_rate];
    }

    if (state >= kMovieLoadStateLoaded) {
        qint64 currentDuration = duration();
        if (m_duration != currentDuration)
            Q_EMIT durationChanged(m_duration = currentDuration);

        if (m_audioAvailable != isAudioAvailable())
            Q_EMIT audioAvailableChanged(m_audioAvailable = !m_audioAvailable);

        if (m_videoAvailable != isVideoAvailable())
            Q_EMIT videoAvailableChanged(m_videoAvailable = !m_videoAvailable);
    }

    if (newStatus != m_mediaStatus)
        Q_EMIT mediaStatusChanged(m_mediaStatus = newStatus);
}

void QT7PlayerSession::processVolumeChange()
{
    if (!m_QTMovie)
        return;

    int newVolume = qRound(100.0f * [((QTMovie*)m_QTMovie) volume]);

    if (newVolume != m_volume) {
        Q_EMIT volumeChanged(m_volume = newVolume);
    }
}

void QT7PlayerSession::processNaturalSizeChange()
{
    AutoReleasePool pool;
    NSSize size = [[(QTMovie*)m_QTMovie attributeForKey:@"QTMovieNaturalSizeAttribute"] sizeValue];
#ifdef QT_DEBUG_QT7
    qDebug() << Q_FUNC_INFO << QSize(size.width, size.height);
#endif

    if (m_videoOutput)
        m_videoOutput->updateNaturalSize(QSize(size.width, size.height));
}

void QT7PlayerSession::processPositionChange()
{
    Q_EMIT positionChanged(position());
}

#include "moc_qt7playersession.cpp"
