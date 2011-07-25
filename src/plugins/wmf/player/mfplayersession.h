/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef MFPLAYERSESSION_H
#define MFPLAYERSESSION_H

#include <mfapi.h>
#include <mfidl.h>

#include "qmediaplayer.h"
#include "qmediaresource.h"
#include "qmediaservice.h"
#include "qmediatimerange.h"

#include <QtCore/qcoreevent.h>
#include <QtCore/qmutex.h>
#include <QtCore/qurl.h>
#include <QtCore/qwaitcondition.h>

QT_BEGIN_NAMESPACE
class QMediaContent;
QT_END_NAMESPACE

QT_USE_NAMESPACE

class SourceResolver;
#ifndef Q_WS_SIMULATOR
class Evr9VideoWindowControl;
#endif
class MFAudioEndpointControl;
class MFVideoRendererControl;
class MFPlayerControl;
class MFMetaDataControl;

class MFPlayerSession : public QObject, public IMFAsyncCallback
{
    Q_OBJECT
    friend class SourceResolver;
public:
    MFPlayerSession(QObject *parent = 0);
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

    void load(const QMediaContent &media, QIODevice *stream);
    void stop();
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
    int bufferStatus();
    QMediaTimeRange availablePlaybackRanges();

    void changeStatus(QMediaPlayer::MediaStatus newStatus);

Q_SIGNALS:
    void error(QMediaPlayer::Error error, QString errorString, bool isFatal);
    void sessionEvent(IMFMediaEvent  *sessionEvent);
    void statusChanged();
    void audioAvailable();
    void videoAvailable();
    void durationUpdate(qint64 duration);
    void seekableUpdate(bool seekable);
    void positionChanged(qint64 position);
    void playbackRateChanged(qreal rate);
    void volumeChanged(int volume);
    void mutedChanged(bool muted);
    void bufferStatusChanged(int percentFilled);

private Q_SLOTS:
    void handleMediaSourceReady();
    void handleSessionEvent(IMFMediaEvent *sessionEvent);
    void handleSourceError(long hr);

private:
    IMFMediaSession *m_session;
    IMFPresentationClock *m_presentationClock;
    IMFRateControl *m_rateControl;
    IMFRateSupport *m_rateSupport;
    IMFSimpleAudioVolume *m_volumeControl;
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

    void createSession();
    void setupPlaybackTopology(IMFMediaSource *source, IMFPresentationDescriptor *sourcePD);
    IMFTopologyNode* addSourceNode(IMFTopology* topology, IMFMediaSource* source,
        IMFPresentationDescriptor* presentationDesc, IMFStreamDescriptor *streamDesc);
    IMFTopologyNode* addOutputNode(IMFStreamDescriptor *streamDesc, MediaType& mediaType, IMFTopology* topology, DWORD sinkID);
};


#endif
