// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef MFPLAYERSESSION_H
#define MFPLAYERSESSION_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <mfapi.h>
#include <mfidl.h>

#include "qmediaplayer.h"
#include "qmediatimerange.h"

#include <QtCore/qcoreevent.h>
#include <QtCore/qmutex.h>
#include <QtCore/qurl.h>
#include <QtCore/qwaitcondition.h>
#include <QtMultimedia/qaudioformat.h>
#include <QtMultimedia/qvideoframeformat.h>
#include <qaudiodevice.h>
#include <qtimer.h>
#include "mfplayercontrol_p.h"

QT_BEGIN_NAMESPACE

class QUrl;

class SourceResolver;
class MFVideoRendererControl;
class MFPlayerControl;
class MFPlayerService;
class AudioSampleGrabberCallback;
class MFTransform;

class MFPlayerSession : public QObject, public IMFAsyncCallback
{
    Q_OBJECT
    friend class SourceResolver;
public:
    MFPlayerSession(MFPlayerControl *playerControl = 0);
    ~MFPlayerSession();

    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppvObject) override;

    STDMETHODIMP_(ULONG) AddRef(void) override;

    STDMETHODIMP_(ULONG) Release(void) override;

    STDMETHODIMP Invoke(IMFAsyncResult *pResult) override;

    STDMETHODIMP GetParameters(DWORD *pdwFlags, DWORD *pdwQueue) override
    {
        Q_UNUSED(pdwFlags);
        Q_UNUSED(pdwQueue);
        return E_NOTIMPL;
    }

    void load(const QUrl &media, QIODevice *stream);
    void stop(bool immediate = false);
    void start();
    void pause();

    QMediaPlayer::MediaStatus status() const;
    qint64 position();
    void setPosition(qint64 position);
    qreal playbackRate() const;
    void setPlaybackRate(qreal rate);
    float bufferProgress();
    QMediaTimeRange availablePlaybackRanges();

    void changeStatus(QMediaPlayer::MediaStatus newStatus);

    void close();

    void setAudioOutput(QPlatformAudioOutput *device);

    QMediaMetaData metaData() const { return m_metaData; }

    void setVideoSink(QVideoSink *sink);

    void setActiveTrack(QPlatformMediaPlayer::TrackType type, int index);
    int activeTrack(QPlatformMediaPlayer::TrackType type);
    int trackCount(QPlatformMediaPlayer::TrackType);
    QMediaMetaData trackMetaData(QPlatformMediaPlayer::TrackType type, int trackNumber);

    void setPlayerControl(MFPlayerControl *playerControl) { m_playerControl = playerControl; }

    void statusChanged() { if (m_playerControl) m_playerControl->handleStatusChanged(); }
    void tracksChanged() { if (m_playerControl) m_playerControl->handleTracksChanged(); }
    void audioAvailable() { if (m_playerControl) m_playerControl->handleAudioAvailable(); }
    void videoAvailable() { if (m_playerControl) m_playerControl->handleVideoAvailable(); }
    void durationUpdate(qint64 duration) { if (m_playerControl) m_playerControl->handleDurationUpdate(duration); }
    void seekableUpdate(bool seekable) { if (m_playerControl) m_playerControl->handleSeekableUpdate(seekable); }
    void error(QMediaPlayer::Error error, QString errorString, bool isFatal) { if (m_playerControl) m_playerControl->handleError(error, errorString, isFatal); }
    void playbackRateChanged(qreal rate) { if (m_playerControl) m_playerControl->playbackRateChanged(rate); }
    void bufferProgressChanged(float percentFilled) { if (m_playerControl) m_playerControl->bufferProgressChanged(percentFilled); }
    void metaDataChanged() { if (m_playerControl) m_playerControl->metaDataChanged(); }
    void positionChanged(qint64 position) { if (m_playerControl) m_playerControl->positionChanged(position); }

public Q_SLOTS:
    void setVolume(float volume);
    void setMuted(bool muted);
    void updateOutputRouting();

Q_SIGNALS:
    void sessionEvent(IMFMediaEvent  *sessionEvent);

private Q_SLOTS:
    void handleMediaSourceReady();
    void handleSessionEvent(IMFMediaEvent *sessionEvent);
    void handleSourceError(long hr);
    void timeout();

private:
    long m_cRef;
    MFPlayerControl *m_playerControl = nullptr;
    MFVideoRendererControl *m_videoRendererControl = nullptr;
    IMFMediaSession *m_session;
    IMFPresentationClock *m_presentationClock;
    IMFRateControl *m_rateControl;
    IMFRateSupport *m_rateSupport;
    IMFAudioStreamVolume *m_volumeControl;
    IPropertyStore *m_netsourceStatistics;
    qint64 m_position = 0;
    qint64 m_restorePosition = -1;
    qint64 m_timeCounter = 0;
    UINT64 m_duration = 0;
    bool m_updatingTopology = false;
    bool m_updateRoutingOnStart = false;

    enum Command
    {
        CmdNone = 0,
        CmdStop,
        CmdStart,
        CmdPause,
        CmdSeek,
        CmdSeekResume,
        CmdStartAndSeek
    };

    void clear();
    void setPositionInternal(qint64 position, Command requestCmd);
    void setPlaybackRateInternal(qreal rate);
    void commitRateChange(qreal rate, BOOL isThin);
    bool canScrub() const;
    void scrub(bool enableScrub);
    bool m_scrubbing;
    float m_restoreRate;

    SourceResolver  *m_sourceResolver;
    HANDLE           m_hCloseEvent;
    bool m_closing;

    enum MediaType
    {
        Unknown = 0,
        Audio = 1,
        Video = 2,
    };
    DWORD m_mediaTypes;

    enum PendingState
    {
        NoPending = 0,
        CmdPending,
        SeekPending,
    };

    struct SeekState
    {
        void setCommand(Command cmd) {
            prevCmd = command;
            command = cmd;
        }
        Command command;
        Command prevCmd;
        float   rate;        // Playback rate
        BOOL    isThin;      // Thinned playback?
        qint64  start;       // Start position
    };
    SeekState   m_state;        // Current nominal state.
    SeekState   m_request;      // Pending request.
    PendingState m_pendingState;
    float m_pendingRate;
    void updatePendingCommands(Command command);

    struct TrackInfo
    {
        QList<QMediaMetaData> metaData;
        QList<int> nativeIndexes;
        int currentIndex = -1;
        TOPOID sourceNodeId = -1;
        TOPOID outputNodeId = -1;
        GUID format = GUID_NULL;
    };
    TrackInfo m_trackInfo[QPlatformMediaPlayer::NTrackTypes];

    QMediaPlayer::MediaStatus m_status;
    bool m_canScrub;
    float m_volume = 1.;
    bool m_muted = false;

    QPlatformAudioOutput *m_audioOutput = nullptr;
    QMediaMetaData m_metaData;

    void setVolumeInternal(float volume);

    bool createSession();
    void setupPlaybackTopology(IMFMediaSource *source, IMFPresentationDescriptor *sourcePD);
    bool getStreamInfo(IMFStreamDescriptor *stream, MFPlayerSession::MediaType *type, QString *name, QString *language, GUID *format) const;
    IMFTopologyNode* addSourceNode(IMFTopology* topology, IMFMediaSource* source,
        IMFPresentationDescriptor* presentationDesc, IMFStreamDescriptor *streamDesc);
    IMFTopologyNode* addOutputNode(MediaType mediaType, IMFTopology* topology, DWORD sinkID);

    bool addAudioSampleGrabberNode(IMFTopology* topology);
    bool setupAudioSampleGrabber(IMFTopology *topology, IMFTopologyNode *sourceNode, IMFTopologyNode *outputNode);
    QAudioFormat audioFormatForMFMediaType(IMFMediaType *mediaType) const;
    // ### Below can be used to monitor the audio channel. Currently unused.
    AudioSampleGrabberCallback *m_audioSampleGrabber;
    IMFTopologyNode *m_audioSampleGrabberNode;

    IMFTopology *insertMFT(IMFTopology *topology, TOPOID outputNodeId);
    bool insertResizer(IMFTopology *topology);
    void insertColorConverter(IMFTopology *topology, TOPOID outputNodeId);
    // ### Below can be used to monitor the video channel. Functionality currently unused.
    MFTransform *m_videoProbeMFT;

    QTimer m_signalPositionChangeTimer;
    qint64 m_lastPosition = -1;
};

QT_END_NAMESPACE

#endif
