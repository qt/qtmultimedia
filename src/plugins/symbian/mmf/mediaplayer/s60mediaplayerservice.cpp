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

#include "DebugMacros.h"

#include <QtCore/qvariant.h>
#include <QtCore/qdebug.h>
#include <QtGui/qwidget.h>

#include "s60mediaplayerservice.h"
#include "s60mediaplayercontrol.h"
#include "s60videoplayersession.h"
#include "s60audioplayersession.h"
#include "s60mediametadataprovider.h"
#include "s60mediarecognizer.h"
#include "s60videowidgetcontrol.h"
#include "s60videowindowcontrol.h"
#ifdef HAS_VIDEORENDERERCONTROL_IN_VIDEOPLAYER
#include "s60videorenderer.h"
#endif
#include "s60mediaplayeraudioendpointselector.h"
#include "s60medianetworkaccesscontrol.h"
#include "s60mediastreamcontrol.h"

#include <qmediaplaylistnavigator.h>
#include <qmediaplaylist.h>

/*!
    Construct a media service with the given \a parent.
*/

S60MediaPlayerService::S60MediaPlayerService(QObject *parent)
    : QMediaService(parent)
    , m_control(NULL)
    , m_videoPlayerSession(NULL)
    , m_audioPlayerSession(NULL)
    , m_metaData(NULL)
    , m_audioEndpointSelector(NULL)
    , m_streamControl(NULL)
    , m_networkAccessControl(NULL)
    , m_videoOutput(NULL)
{
    DP0("S60MediaPlayerService::S60MediaPlayerService +++");

    m_control = new S60MediaPlayerControl(*this, this);
    m_metaData = new S60MediaMetaDataProvider(m_control, this);
    m_audioEndpointSelector = new S60MediaPlayerAudioEndpointSelector(m_control, this);
    m_streamControl = new S60MediaStreamControl(m_control, this);
    m_networkAccessControl =  new S60MediaNetworkAccessControl(this);

    DP0("S60MediaPlayerService::S60MediaPlayerService ---");
}

/*!
    Destroys a media service.
*/

S60MediaPlayerService::~S60MediaPlayerService()
{
    DP0("S60MediaPlayerService::~S60MediaPlayerService +++");
    DP0("S60MediaPlayerService::~S60MediaPlayerService ---");
}

/*!
    \return a pointer to the media control, which matches the controller \a name.

    If the service does not implement the control, or if it is unavailable a
    null pointer is returned instead.

    Controls must be returned to the service when no longer needed using the
    releaseControl() function.
*/

QMediaControl *S60MediaPlayerService::requestControl(const char *name)
{
    DP0("S60MediaPlayerService::requestControl");

    if (qstrcmp(name, QMediaPlayerControl_iid) == 0)
        return m_control;

    if (qstrcmp(name, QMediaNetworkAccessControl_iid) == 0)
        return m_networkAccessControl;

    if (qstrcmp(name, QMetaDataReaderControl_iid) == 0)
        return m_metaData;

    if (qstrcmp(name, QAudioEndpointSelector_iid) == 0)
        return m_audioEndpointSelector;

    if (qstrcmp(name, QMediaStreamsControl_iid) == 0)
        return m_streamControl;

    if (!m_videoOutput) {
        if (qstrcmp(name, QVideoWidgetControl_iid) == 0) {
            m_videoOutput = new S60VideoWidgetControl(this);
        }
#ifdef HAS_VIDEORENDERERCONTROL_IN_VIDEOPLAYER
        else if (qstrcmp(name, QVideoRendererControl_iid) == 0) {
            m_videoOutput = new S60VideoRenderer(this);
        }
#endif /* HAS_VIDEORENDERERCONTROL_IN_VIDEOPLAYER */
        else if (qstrcmp(name, QVideoWindowControl_iid) == 0) {
            m_videoOutput = new S60VideoWindowControl(this);
        }

        if (m_videoOutput) {
            m_control->setVideoOutput(m_videoOutput);
            return m_videoOutput;
        }
    }else {
        if (qstrcmp(name, QVideoWidgetControl_iid) == 0 ||
#ifdef HAS_VIDEORENDERERCONTROL_IN_VIDEOPLAYER
            qstrcmp(name, QVideoRendererControl_iid) == 0 ||
#endif /* HAS_VIDEORENDERERCONTROL_IN_VIDEOPLAYER */
            qstrcmp(name, QVideoWindowControl_iid) == 0){
            return m_videoOutput;
        }
    }
    return 0;
}

/*!
    Releases a \a control back to the service.
*/

void S60MediaPlayerService::releaseControl(QMediaControl *control)
{
    DP0("S60MediaPlayerService::releaseControl ++");

    if (control == m_videoOutput) {
        m_videoOutput = 0;
        m_control->setVideoOutput(m_videoOutput);
    }

    DP0("S60MediaPlayerService::releaseControl --");
}

/*!
 * \return media player session(audio playersesion/video playersession)
 *  by recognizing whether media is audio or video and sets it on media type.
*/
S60MediaPlayerSession* S60MediaPlayerService::PlayerSession()
{
    DP0("S60MediaPlayerService::PlayerSession");

    QUrl url = m_control->media().canonicalUrl();

    if (url.isEmpty() == true) {
        return NULL;
    }

    QScopedPointer<S60MediaRecognizer> mediaRecognizer(new S60MediaRecognizer);
    S60MediaRecognizer::MediaType mediaType = mediaRecognizer->mediaType(url);
    mediaRecognizer.reset();

    switch (mediaType) {
    case S60MediaRecognizer::Video:
    case S60MediaRecognizer::Url: {
        m_control->setMediaType(S60MediaSettings::Video);
        return VideoPlayerSession();
        }
    case S60MediaRecognizer::Audio: {
        m_control->setMediaType(S60MediaSettings::Audio);
        return AudioPlayerSession();
        }
    default:
        m_control->setMediaType(S60MediaSettings::Unknown);
        break;
    }

    return NULL;
}

/*!
 *  \return media playersession (videoplayersession).
 *  constructs the videoplayersession object and connects all the respective signals and slots.
 *  and initialises all the media settings.
*/

S60MediaPlayerSession* S60MediaPlayerService::VideoPlayerSession()
{
    DP0("S60MediaPlayerService::VideoPlayerSession +++");

    if (!m_videoPlayerSession) {
        m_videoPlayerSession = new S60VideoPlayerSession(this, m_networkAccessControl);

        connect(m_videoPlayerSession, SIGNAL(positionChanged(qint64)),
                m_control, SIGNAL(positionChanged(qint64)));
        connect(m_videoPlayerSession, SIGNAL(playbackRateChanged(qreal)),
                m_control, SIGNAL(playbackRateChanged(qreal)));
        connect(m_videoPlayerSession, SIGNAL(volumeChanged(int)),
                m_control, SIGNAL(volumeChanged(int)));
        connect(m_videoPlayerSession, SIGNAL(mutedChanged(bool)),
                m_control, SIGNAL(mutedChanged(bool)));
        connect(m_videoPlayerSession, SIGNAL(durationChanged(qint64)),
                m_control, SIGNAL(durationChanged(qint64)));
        connect(m_videoPlayerSession, SIGNAL(stateChanged(QMediaPlayer::State)),
                m_control, SIGNAL(stateChanged(QMediaPlayer::State)));
        connect(m_videoPlayerSession, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)),
                m_control, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)));
        connect(m_videoPlayerSession,SIGNAL(bufferStatusChanged(int)),
                m_control, SIGNAL(bufferStatusChanged(int)));
        connect(m_videoPlayerSession, SIGNAL(videoAvailableChanged(bool)),
                m_control, SIGNAL(videoAvailableChanged(bool)));
        connect(m_videoPlayerSession, SIGNAL(audioAvailableChanged(bool)),
                m_control, SIGNAL(audioAvailableChanged(bool)));
        connect(m_videoPlayerSession, SIGNAL(seekableChanged(bool)),
                m_control, SIGNAL(seekableChanged(bool)));
        connect(m_videoPlayerSession, SIGNAL(availablePlaybackRangesChanged(const QMediaTimeRange&)),
                m_control, SIGNAL(availablePlaybackRangesChanged(const QMediaTimeRange&)));
        connect(m_videoPlayerSession, SIGNAL(error(int, const QString &)),
                m_control, SIGNAL(error(int, const QString &)));
        connect(m_videoPlayerSession, SIGNAL(metaDataChanged()),
                m_metaData, SIGNAL(metaDataChanged()));
        connect(m_videoPlayerSession, SIGNAL(activeEndpointChanged(const QString&)),
                m_audioEndpointSelector, SIGNAL(activeEndpointChanged(const QString&)));
        connect(m_videoPlayerSession, SIGNAL(mediaChanged()),
                m_streamControl, SLOT(handleStreamsChanged()));
        connect(m_videoPlayerSession, SIGNAL(accessPointChanged(int)),
                m_networkAccessControl, SLOT(accessPointChanged(int)));

    }

    m_videoPlayerSession->setVolume(m_control->mediaControlSettings().volume());
    m_videoPlayerSession->setMuted(m_control->mediaControlSettings().isMuted());
    m_videoPlayerSession->setAudioEndpoint(m_control->mediaControlSettings().audioEndpoint());

    DP0("S60MediaPlayerService::VideoPlayerSession ---");

    return m_videoPlayerSession;
}

/*!
 *  \return media playersession (audioplayersession).
 *  constructs the audioplayersession object and connects all the respective signals and slots.
 *  and initialises all the media settings.
*/

S60MediaPlayerSession* S60MediaPlayerService::AudioPlayerSession()
{
    DP0("S60MediaPlayerService::AudioPlayerSession +++");

    if (!m_audioPlayerSession) {
        m_audioPlayerSession = new S60AudioPlayerSession(this);

        connect(m_audioPlayerSession, SIGNAL(positionChanged(qint64)),
                m_control, SIGNAL(positionChanged(qint64)));
        connect(m_audioPlayerSession, SIGNAL(playbackRateChanged(qreal)),
                m_control, SIGNAL(playbackRateChanged(qreal)));
        connect(m_audioPlayerSession, SIGNAL(volumeChanged(int)),
                m_control, SIGNAL(volumeChanged(int)));
        connect(m_audioPlayerSession, SIGNAL(mutedChanged(bool)),
                m_control, SIGNAL(mutedChanged(bool)));
        connect(m_audioPlayerSession, SIGNAL(durationChanged(qint64)),
                m_control, SIGNAL(durationChanged(qint64)));
        connect(m_audioPlayerSession, SIGNAL(stateChanged(QMediaPlayer::State)),
                m_control, SIGNAL(stateChanged(QMediaPlayer::State)));
        connect(m_audioPlayerSession, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)),
                m_control, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)));
        connect(m_audioPlayerSession,SIGNAL(bufferStatusChanged(int)),
                m_control, SIGNAL(bufferStatusChanged(int)));
        connect(m_audioPlayerSession, SIGNAL(videoAvailableChanged(bool)),
                m_control, SIGNAL(videoAvailableChanged(bool)));
        connect(m_audioPlayerSession, SIGNAL(audioAvailableChanged(bool)),
                m_control, SIGNAL(audioAvailableChanged(bool)));
        connect(m_audioPlayerSession, SIGNAL(seekableChanged(bool)),
                m_control, SIGNAL(seekableChanged(bool)));
        connect(m_audioPlayerSession, SIGNAL(availablePlaybackRangesChanged(const QMediaTimeRange&)),
                m_control, SIGNAL(availablePlaybackRangesChanged(const QMediaTimeRange&)));
        connect(m_audioPlayerSession, SIGNAL(error(int, const QString &)),
                m_control, SIGNAL(error(int, const QString &)));
        connect(m_audioPlayerSession, SIGNAL(metaDataChanged()),
                m_metaData, SIGNAL(metaDataChanged()));
        connect(m_audioPlayerSession, SIGNAL(activeEndpointChanged(const QString&)),
                m_audioEndpointSelector, SIGNAL(activeEndpointChanged(const QString&)));
        connect(m_audioPlayerSession, SIGNAL(mediaChanged()),
                m_streamControl, SLOT(handleStreamsChanged()));

    }

    m_audioPlayerSession->setVolume(m_control->mediaControlSettings().volume());
    m_audioPlayerSession->setMuted(m_control->mediaControlSettings().isMuted());
    m_audioPlayerSession->setAudioEndpoint(m_control->mediaControlSettings().audioEndpoint());

    DP0("S60MediaPlayerService::AudioPlayerSession ---");

    return m_audioPlayerSession;
}
