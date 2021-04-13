/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Mobility Components.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
#include <qaudiodeviceinfo.h>
#include "mfplayercontrol_p.h"

QT_BEGIN_NAMESPACE
class QUrl;
QT_END_NAMESPACE

QT_USE_NAMESPACE

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

    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppvObject);

    STDMETHODIMP_(ULONG) AddRef(void);

    STDMETHODIMP_(ULONG) Release(void);

    STDMETHODIMP Invoke(IMFAsyncResult *pResult);

    STDMETHODIMP GetParameters(DWORD *pdwFlags, DWORD *pdwQueue)
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
    int volume() const;
    void setVolume(int volume);
    bool isMuted() const;
    void setMuted(bool muted);
    float bufferProgress();
    QMediaTimeRange availablePlaybackRanges();

    void changeStatus(QMediaPlayer::MediaStatus newStatus);

    void close();

    bool setAudioOutput(const QAudioDeviceInfo &device);
    QAudioDeviceInfo audioOutput() const { return m_audioOutput; }

    QMediaMetaData metaData() const { return m_metaData; }

    void setVideoSink(QVideoSink *sink);

    void statusChanged() { m_playerControl->handleStatusChanged(); }
    void audioAvailable() { m_playerControl->handleAudioAvailable(); }
    void videoAvailable() { m_playerControl->handleVideoAvailable(); }
    void durationUpdate(qint64 duration) { m_playerControl->handleDurationUpdate(duration); }
    void seekableUpdate(bool seekable) { m_playerControl->handleSeekableUpdate(seekable); }
    void error(QMediaPlayer::Error error, QString errorString, bool isFatal) { m_playerControl->handleError(error, errorString, isFatal); }
    void playbackRateChanged(qreal rate) { m_playerControl->playbackRateChanged(rate); }
    void volumeChanged(int volume) { m_playerControl->volumeChanged(volume); }
    void mutedChanged(bool muted) { m_playerControl->mutedChanged(muted); }
    void bufferProgressChanged(float percentFilled) { m_playerControl->bufferProgressChanged(percentFilled); }
    void metaDataChanged() { m_playerControl->metaDataChanged(); }
    void positionChanged(qint64 position) { m_playerControl->positionChanged(position); }

Q_SIGNALS:
    void sessionEvent(IMFMediaEvent  *sessionEvent);

private Q_SLOTS:
    void handleMediaSourceReady();
    void handleSessionEvent(IMFMediaEvent *sessionEvent);
    void handleSourceError(long hr);

private:
    long m_cRef;
    MFPlayerControl *m_playerControl;
    MFVideoRendererControl *m_videoRendererControl = nullptr;
    IMFMediaSession *m_session;
    IMFPresentationClock *m_presentationClock;
    IMFRateControl *m_rateControl;
    IMFRateSupport *m_rateSupport;
    IMFAudioStreamVolume *m_volumeControl;
    IPropertyStore *m_netsourceStatistics;
    PROPVARIANT m_varStart;
    UINT64 m_duration;

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

    QMediaPlayer::MediaStatus m_status;
    bool m_canScrub;
    int m_volume;
    bool m_muted;

    QAudioDeviceInfo m_audioOutput;
    QMediaMetaData m_metaData;
    IMFActivate *m_currentAudioActivate = nullptr;


    void setVolumeInternal(int volume);

    void createSession();
    void setupPlaybackTopology(IMFMediaSource *source, IMFPresentationDescriptor *sourcePD);
    MediaType getStreamType(IMFStreamDescriptor *stream) const;
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
};


#endif
