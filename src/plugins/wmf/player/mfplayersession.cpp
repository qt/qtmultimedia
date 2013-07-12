/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Mobility Components.
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

#include "qmediacontent.h"
#include "qmediaplayercontrol.h"

#include <QtCore/qcoreapplication.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qthread.h>
#include <QtCore/qvarlengtharray.h>
#include <QtCore/qdebug.h>
#include <QtCore/qfile.h>
#include <QtCore/qbuffer.h>

#include "mfplayercontrol.h"
#ifndef Q_WS_SIMULATOR
#include "evr9videowindowcontrol.h"
#endif
#include "mfvideorenderercontrol.h"
#include "mfaudioendpointcontrol.h"

#include "mfplayersession.h"
#include "mfplayerservice.h"
#include "mfmetadatacontrol.h"
#include <Mferror.h>
#include <nserror.h>
#include "sourceresolver.h"
#include "samplegrabber.h"
#include "mftvideo.h"
#include <wmcodecdsp.h>

//#define DEBUG_MEDIAFOUNDATION
//#define TEST_STREAMING

namespace
{
    //MFStream is added for supporting QIODevice type of media source.
    //It is used to delegate invocations from media foundation(through IMFByteStream) to QIODevice.
    class MFStream : public QObject, public IMFByteStream
    {
        Q_OBJECT
    public:
        MFStream(QIODevice *stream, bool ownStream)
            : m_cRef(1)
            , m_stream(stream)
            , m_ownStream(ownStream)
            , m_currentReadResult(0)
        {
            //Move to the thread of the stream object
            //to make sure invocations on stream
            //are happened in the same thread of stream object
            this->moveToThread(stream->thread());
            connect(stream, SIGNAL(readyRead()), this, SLOT(handleReadyRead()));
        }

        ~MFStream()
        {
            if (m_currentReadResult)
                m_currentReadResult->Release();
            if (m_ownStream)
                m_stream->deleteLater();
        }

        //from IUnknown
        STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppvObject)
        {
            if (!ppvObject)
                return E_POINTER;
            if (riid == IID_IMFByteStream) {
                *ppvObject = static_cast<IMFByteStream*>(this);
            } else if (riid == IID_IUnknown) {
                *ppvObject = static_cast<IUnknown*>(this);
            } else {
                *ppvObject =  NULL;
                return E_NOINTERFACE;
            }
            AddRef();
            return S_OK;
        }

        STDMETHODIMP_(ULONG) AddRef(void)
        {
            return InterlockedIncrement(&m_cRef);
        }

        STDMETHODIMP_(ULONG) Release(void)
        {
            LONG cRef = InterlockedDecrement(&m_cRef);
            if (cRef == 0) {
                this->deleteLater();
            }
            return cRef;
        }


        //from IMFByteStream
        STDMETHODIMP GetCapabilities(DWORD *pdwCapabilities)
        {
            if (!pdwCapabilities)
                return E_INVALIDARG;
            *pdwCapabilities = MFBYTESTREAM_IS_READABLE;
            if (!m_stream->isSequential())
                *pdwCapabilities |= MFBYTESTREAM_IS_SEEKABLE;
            return S_OK;
        }

        STDMETHODIMP GetLength(QWORD *pqwLength)
        {
            if (!pqwLength)
                return E_INVALIDARG;
            QMutexLocker locker(&m_mutex);
            *pqwLength = QWORD(m_stream->size());
            return S_OK;
        }

        STDMETHODIMP SetLength(QWORD)
        {
            return E_NOTIMPL;
        }

        STDMETHODIMP GetCurrentPosition(QWORD *pqwPosition)
        {
            if (!pqwPosition)
                return E_INVALIDARG;
            QMutexLocker locker(&m_mutex);
            *pqwPosition = m_stream->pos();
            return S_OK;
        }

        STDMETHODIMP SetCurrentPosition(QWORD qwPosition)
        {
            QMutexLocker locker(&m_mutex);
            //SetCurrentPosition may happend during the BeginRead/EndRead pair,
            //refusing to execute SetCurrentPosition during that time seems to be
            //the simplest workable solution
            if (m_currentReadResult)
                return S_FALSE;

            bool seekOK = m_stream->seek(qint64(qwPosition));
            if (seekOK)
                return S_OK;
            else
                return S_FALSE;
        }

        STDMETHODIMP IsEndOfStream(BOOL *pfEndOfStream)
        {
            if (!pfEndOfStream)
                return E_INVALIDARG;
            QMutexLocker locker(&m_mutex);
            *pfEndOfStream = m_stream->atEnd() ? TRUE : FALSE;
            return S_OK;
        }

        STDMETHODIMP Read(BYTE *pb, ULONG cb, ULONG *pcbRead)
        {
            QMutexLocker locker(&m_mutex);
            qint64 read = m_stream->read((char*)(pb), qint64(cb));
            if (pcbRead)
                *pcbRead = ULONG(read);
            return S_OK;
        }

        STDMETHODIMP BeginRead(BYTE *pb, ULONG cb, IMFAsyncCallback *pCallback,
                               IUnknown *punkState)
        {
            if (!pCallback || !pb)
                return E_INVALIDARG;

            Q_ASSERT(m_currentReadResult == NULL);

            AsyncReadState *state = new (std::nothrow) AsyncReadState(pb, cb);
            if (state == NULL)
                return E_OUTOFMEMORY;

            HRESULT hr = MFCreateAsyncResult(state, pCallback, punkState, &m_currentReadResult);
            state->Release();
            if (FAILED(hr))
                return hr;

            QCoreApplication::postEvent(this, new QEvent(QEvent::User));
            return hr;
        }

        STDMETHODIMP EndRead(IMFAsyncResult* pResult, ULONG *pcbRead)
        {
            if (!pcbRead)
                return E_INVALIDARG;
            IUnknown *pUnk;
            pResult->GetObject(&pUnk);
            AsyncReadState *state = static_cast<AsyncReadState*>(pUnk);
            *pcbRead = state->bytesRead();
            pUnk->Release();

            m_currentReadResult->Release();
            m_currentReadResult = NULL;

            return S_OK;
        }

        STDMETHODIMP Write(const BYTE *, ULONG, ULONG *)
        {
            return E_NOTIMPL;
        }

        STDMETHODIMP BeginWrite(const BYTE *, ULONG ,
                                IMFAsyncCallback *,
                                IUnknown *)
        {
            return E_NOTIMPL;
        }

        STDMETHODIMP EndWrite(IMFAsyncResult *,
                              ULONG *)
        {
            return E_NOTIMPL;
        }

        STDMETHODIMP Seek(
            MFBYTESTREAM_SEEK_ORIGIN SeekOrigin,
            LONGLONG llSeekOffset,
            DWORD,
            QWORD *pqwCurrentPosition)
        {
            QMutexLocker locker(&m_mutex);
            if (m_currentReadResult)
                return S_FALSE;

            qint64 pos = qint64(llSeekOffset);
            switch (SeekOrigin) {
            case msoCurrent:
                pos += m_stream->pos();
                break;
            }
            bool seekOK = m_stream->seek(pos);
            if (*pqwCurrentPosition)
                *pqwCurrentPosition = pos;
            if (seekOK)
                return S_OK;
            else
                return S_FALSE;
        }

        STDMETHODIMP Flush()
        {
            return E_NOTIMPL;
        }

        STDMETHODIMP Close()
        {
            QMutexLocker locker(&m_mutex);
            if (m_ownStream)
                m_stream->close();
            return S_OK;
        }

    private:
        //AsyncReadState is a helper class used in BeginRead for asynchronous operation
        //to record some BeginRead parameters, so these parameters could be
        //used later when actually executing the read operation in another thread.
        class AsyncReadState : public IUnknown
        {
        public:
            AsyncReadState(BYTE *pb, ULONG cb)
                : m_cRef(1)
                , m_pb(pb)
                , m_cb(cb)
                , m_cbRead(0)
            {
            }

            //from IUnknown
            STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppvObject)
            {
                if (!ppvObject)
                    return E_POINTER;

                if (riid == IID_IUnknown) {
                    *ppvObject = static_cast<IUnknown*>(this);
                } else {
                    *ppvObject =  NULL;
                    return E_NOINTERFACE;
                }
                AddRef();
                return S_OK;
            }

            STDMETHODIMP_(ULONG) AddRef(void)
            {
                return InterlockedIncrement(&m_cRef);
            }

            STDMETHODIMP_(ULONG) Release(void)
            {
                LONG cRef = InterlockedDecrement(&m_cRef);
                if (cRef == 0)
                    delete this;
                // For thread safety, return a temporary variable.
                return cRef;
            }

            BYTE* pb() const { return m_pb; }
            ULONG cb() const { return m_cb; }
            ULONG bytesRead() const { return m_cbRead; }

            void setBytesRead(ULONG cbRead) { m_cbRead = cbRead; }

        private:
            long m_cRef;
            BYTE *m_pb;
            ULONG m_cb;
            ULONG m_cbRead;
        };

        long m_cRef;
        QIODevice *m_stream;
        bool m_ownStream;
        DWORD m_workQueueId;
        QMutex m_mutex;

        void doRead()
        {
            bool readDone = true;
            IUnknown *pUnk = NULL;
            HRESULT    hr = m_currentReadResult->GetObject(&pUnk);
            if (SUCCEEDED(hr)) {
                //do actual read
                AsyncReadState *state =  static_cast<AsyncReadState*>(pUnk);
                ULONG cbRead;
                Read(state->pb(), state->cb() - state->bytesRead(), &cbRead);
                pUnk->Release();

                state->setBytesRead(cbRead + state->bytesRead());
                if (state->cb() > state->bytesRead() && !m_stream->atEnd()) {
                    readDone = false;
                }
            }

            if (readDone) {
                //now inform the original caller
                m_currentReadResult->SetStatus(hr);
                MFInvokeCallback(m_currentReadResult);
            }
        }


    private Q_SLOTS:
        void handleReadyRead()
        {
            doRead();
        }

    protected:
        void customEvent(QEvent *event)
        {
            if (event->type() != QEvent::User) {
                QObject::customEvent(event);
                return;
            }
            doRead();
        }
        IMFAsyncResult *m_currentReadResult;
    };
}


MFPlayerSession::MFPlayerSession(MFPlayerService *playerService)
    : m_playerService(playerService)
    , m_cRef(1)
    , m_session(0)
    , m_presentationClock(0)
    , m_rateControl(0)
    , m_rateSupport(0)
    , m_volumeControl(0)
    , m_netsourceStatistics(0)
    , m_hCloseEvent(0)
    , m_closing(false)
    , m_pendingRate(1)
    , m_volume(100)
    , m_muted(false)
    , m_status(QMediaPlayer::NoMedia)
    , m_scrubbing(false)
    , m_restoreRate(1)
    , m_mediaTypes(0)
    , m_audioSampleGrabber(0)
    , m_audioSampleGrabberNode(0)
    , m_videoProbeMFT(0)
{
    QObject::connect(this, SIGNAL(sessionEvent(IMFMediaEvent *)), this, SLOT(handleSessionEvent(IMFMediaEvent *)));

    m_pendingState = NoPending;
    ZeroMemory(&m_state, sizeof(m_state));
    m_state.command = CmdStop;
    m_state.prevCmd = CmdNone;
    m_state.rate = 1.0f;
    ZeroMemory(&m_request, sizeof(m_request));
    m_request.command = CmdNone;
    m_request.prevCmd = CmdNone;
    m_request.rate = 1.0f;

    m_audioSampleGrabber = new AudioSampleGrabberCallback;
}

void MFPlayerSession::close()
{
#ifdef DEBUG_MEDIAFOUNDATION
    qDebug() << "close";
#endif

    clear();
    if (!m_session)
        return;

    HRESULT hr = S_OK;
    if (m_session) {
        m_closing = true;
        hr = m_session->Close();
        if (SUCCEEDED(hr)) {
            DWORD dwWaitResult = WaitForSingleObject(m_hCloseEvent, 100);
            if (dwWaitResult == WAIT_TIMEOUT) {
                qWarning() << "session close time out!";
            }
        }
         m_closing = false;
    }

    if (SUCCEEDED(hr)) {
        if (m_session)
            m_session->Shutdown();
        if (m_sourceResolver)
            m_sourceResolver->shutdown();
    }
    if (m_sourceResolver) {
        m_sourceResolver->Release();
        m_sourceResolver = 0;
    }
    if (m_videoProbeMFT) {
        m_videoProbeMFT->Release();
        m_videoProbeMFT = 0;
    }

    if (m_playerService->videoRendererControl()) {
        m_playerService->videoRendererControl()->releaseActivate();
#ifndef Q_WS_SIMULATOR
    } else if (m_playerService->videoWindowControl()) {
        m_playerService->videoWindowControl()->releaseActivate();
#endif
    }

    if (m_session)
        m_session->Release();
    m_session = 0;
    if (m_hCloseEvent)
        CloseHandle(m_hCloseEvent);
    m_hCloseEvent = 0;
}

void MFPlayerSession::addProbe(MFAudioProbeControl *probe)
{
    m_audioSampleGrabber->addProbe(probe);
}

void MFPlayerSession::removeProbe(MFAudioProbeControl *probe)
{
    m_audioSampleGrabber->removeProbe(probe);
}

void MFPlayerSession::addProbe(MFVideoProbeControl* probe)
{
    if (m_videoProbes.contains(probe))
        return;

    m_videoProbes.append(probe);

    if (m_videoProbeMFT)
        m_videoProbeMFT->addProbe(probe);
}

void MFPlayerSession::removeProbe(MFVideoProbeControl* probe)
{
    m_videoProbes.removeOne(probe);

    if (m_videoProbeMFT)
        m_videoProbeMFT->removeProbe(probe);
}

MFPlayerSession::~MFPlayerSession()
{
    m_audioSampleGrabber->Release();
}


void MFPlayerSession::load(const QMediaContent &media, QIODevice *stream)
{
#ifdef DEBUG_MEDIAFOUNDATION
    qDebug() << "load";
#endif
    clear();
    QMediaResourceList resources = media.resources();

    if (m_status == QMediaPlayer::LoadingMedia)
        m_sourceResolver->cancel();

    if (resources.isEmpty() && !stream) {
        changeStatus(QMediaPlayer::NoMedia);
    } else if (stream && (!stream->isReadable())) {
        changeStatus(QMediaPlayer::InvalidMedia);
        emit error(QMediaPlayer::ResourceError, tr("Invalid stream source."), true);
    } else {
        createSession();
        changeStatus(QMediaPlayer::LoadingMedia);
        m_sourceResolver->load(resources, stream);
    }
    emit positionChanged(position());
}

void MFPlayerSession::handleSourceError(long hr)
{
    QString errorString;
    QMediaPlayer::Error errorCode = QMediaPlayer::ResourceError;
    switch (hr) {
    case QMediaPlayer::FormatError:
        errorCode = QMediaPlayer::FormatError;
        errorString = tr("Attempting to play invalid Qt resource.");
        break;
    case NS_E_FILE_NOT_FOUND:
        errorString = tr("The system cannot find the file specified.");
        break;
    case NS_E_SERVER_NOT_FOUND:
        errorString = tr("The specified server could not be found.");
        break;
    case MF_E_UNSUPPORTED_BYTESTREAM_TYPE:
        errorCode = QMediaPlayer::FormatError;
        errorString = tr("Unsupported media type.");
        break;
    default:
        errorString = tr("Failed to load source.");
        break;
    }
    changeStatus(QMediaPlayer::InvalidMedia);
    emit error(errorCode, errorString, true);
}

void MFPlayerSession::handleMediaSourceReady()
{
    if (QMediaPlayer::LoadingMedia != m_status)
        return;
#ifdef DEBUG_MEDIAFOUNDATION
    qDebug() << "handleMediaSourceReady";
#endif
    HRESULT hr = S_OK;
    IMFPresentationDescriptor* sourcePD;
    IMFMediaSource* mediaSource = m_sourceResolver->mediaSource();
    hr = mediaSource->CreatePresentationDescriptor(&sourcePD);
    if (SUCCEEDED(hr)) {
        m_duration = 0;
        m_playerService->metaDataControl()->updateSource(sourcePD, mediaSource);
        sourcePD->GetUINT64(MF_PD_DURATION, &m_duration);
        //convert from 100 nanosecond to milisecond
        emit durationUpdate(qint64(m_duration / 10000));
        setupPlaybackTopology(mediaSource, sourcePD);
    } else {
        changeStatus(QMediaPlayer::InvalidMedia);
        emit error(QMediaPlayer::ResourceError, tr("Cannot create presentation descriptor."), true);
    }
}

void MFPlayerSession::setupPlaybackTopology(IMFMediaSource *source, IMFPresentationDescriptor *sourcePD)
{
    HRESULT hr = S_OK;
    // Get the number of streams in the media source.
    DWORD cSourceStreams = 0;
    hr = sourcePD->GetStreamDescriptorCount(&cSourceStreams);
    if (FAILED(hr)) {
        changeStatus(QMediaPlayer::UnknownMediaStatus);
        emit error(QMediaPlayer::ResourceError, tr("Failed to get stream count."), true);
        return;
    }

    IMFTopology *topology;
    hr = MFCreateTopology(&topology);
    if (FAILED(hr)) {
        changeStatus(QMediaPlayer::UnknownMediaStatus);
        emit error(QMediaPlayer::ResourceError, tr("Failed to create topology."), true);
        return;
    }

    // Remember output node id for a first video stream
    TOPOID outputNodeId = -1;

    // For each stream, create the topology nodes and add them to the topology.
    DWORD succeededCount = 0;
    for (DWORD i = 0; i < cSourceStreams; i++)
    {
        BOOL fSelected = FALSE;
        IMFStreamDescriptor *streamDesc = NULL;

        HRESULT hr = sourcePD->GetStreamDescriptorByIndex(i, &fSelected, &streamDesc);
        if (SUCCEEDED(hr)) {
            MediaType mediaType = Unknown;
            IMFTopologyNode *sourceNode = addSourceNode(topology, source, sourcePD, streamDesc);
            if (sourceNode) {
                IMFTopologyNode *outputNode = addOutputNode(streamDesc, mediaType, topology, 0);
                if (outputNode) {
                    bool connected = false;
                    if (mediaType == Audio) {
                        if (!m_audioSampleGrabberNode)
                            connected = setupAudioSampleGrabber(topology, sourceNode, outputNode);
                    } else if (mediaType == Video && outputNodeId == -1) {
                        // Remember video output node ID.
                        outputNode->GetTopoNodeID(&outputNodeId);
                    }

                    if (!connected)
                        hr = sourceNode->ConnectOutput(0, outputNode, 0);
                    if (FAILED(hr)) {
                        emit error(QMediaPlayer::FormatError, tr("Unable to play any stream."), false);
                    }
                    else {
                        succeededCount++;
                        m_mediaTypes |= mediaType;
                        switch (mediaType) {
                        case Audio:
                            emit audioAvailable();
                            break;
                        case Video:
                            emit videoAvailable();
                            break;
                        }
                    }
                    outputNode->Release();
                }
                sourceNode->Release();
            }
            streamDesc->Release();
        }
    }

    if (succeededCount == 0) {
        changeStatus(QMediaPlayer::InvalidMedia);
        emit error(QMediaPlayer::ResourceError, tr("Unable to play."), true);
    } else {
        if (outputNodeId != -1) {
            topology = insertMFT(topology, outputNodeId);
        }

        hr = m_session->SetTopology(MFSESSION_SETTOPOLOGY_IMMEDIATE, topology);
        if (FAILED(hr)) {
            changeStatus(QMediaPlayer::UnknownMediaStatus);
            emit error(QMediaPlayer::ResourceError, tr("Failed to set topology."), true);
        }
    }
    topology->Release();
}

IMFTopologyNode* MFPlayerSession::addSourceNode(IMFTopology* topology, IMFMediaSource* source,
                                                IMFPresentationDescriptor* presentationDesc, IMFStreamDescriptor *streamDesc)
{
    IMFTopologyNode *node = NULL;
    HRESULT hr = MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &node);
    if (SUCCEEDED(hr)) {
        hr = node->SetUnknown(MF_TOPONODE_SOURCE, source);
        if (SUCCEEDED(hr)) {
            hr = node->SetUnknown(MF_TOPONODE_PRESENTATION_DESCRIPTOR, presentationDesc);
            if (SUCCEEDED(hr)) {
                hr = node->SetUnknown(MF_TOPONODE_STREAM_DESCRIPTOR, streamDesc);
                if (SUCCEEDED(hr)) {
                    hr = topology->AddNode(node);
                    if (SUCCEEDED(hr))
                        return node;
                }
            }
        }
        node->Release();
    }
    return NULL;
}

IMFTopologyNode* MFPlayerSession::addOutputNode(IMFStreamDescriptor *streamDesc, MediaType& mediaType, IMFTopology* topology, DWORD sinkID)
{
    IMFTopologyNode *node = NULL;
    HRESULT hr = MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &node);
    if (FAILED(hr))
        return NULL;
    node->SetUINT32(MF_TOPONODE_NOSHUTDOWN_ON_REMOVE, FALSE);

    mediaType = Unknown;
    IMFMediaTypeHandler *handler = NULL;
    hr = streamDesc->GetMediaTypeHandler(&handler);
    if (SUCCEEDED(hr)) {
        GUID guidMajorType;
        hr = handler->GetMajorType(&guidMajorType);
        if (SUCCEEDED(hr)) {
            IMFActivate *activate = NULL;
            if (MFMediaType_Audio == guidMajorType) {
                mediaType = Audio;
                activate = m_playerService->audioEndpointControl()->createActivate();
            } else if (MFMediaType_Video == guidMajorType) {
                mediaType = Video;
                if (m_playerService->videoRendererControl()) {
                    activate = m_playerService->videoRendererControl()->createActivate();
#ifndef Q_WS_SIMULATOR
                } else if (m_playerService->videoWindowControl()) {
                    activate = m_playerService->videoWindowControl()->createActivate();
#endif
                } else {
                    qWarning() << "no videoWindowControl or videoRendererControl, unable to add output node for video data";
                }
            } else {
                // Unknown stream type.
                emit error(QMediaPlayer::FormatError, tr("Unknown stream type."), false);
            }

            if (activate) {
                hr = node->SetObject(activate);
                if (SUCCEEDED(hr)) {
                    hr = node->SetUINT32(MF_TOPONODE_STREAMID, sinkID);
                    if (SUCCEEDED(hr)) {
                        if (SUCCEEDED(topology->AddNode(node)))
                            return node;
                    }
                }
            }
        }
    }
    node->Release();
    return NULL;
}

bool MFPlayerSession::addAudioSampleGrabberNode(IMFTopology *topology)
{
    HRESULT hr = S_OK;
    IMFMediaType *pType = 0;
    IMFActivate *sinkActivate = 0;
    do {
        hr = MFCreateMediaType(&pType);
        if (FAILED(hr))
            break;

        hr = pType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
        if (FAILED(hr))
            break;

        hr = pType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
        if (FAILED(hr))
            break;

        hr = MFCreateSampleGrabberSinkActivate(pType, m_audioSampleGrabber, &sinkActivate);
        if (FAILED(hr))
            break;

        // Note: Data is distorted if this attribute is enabled
        hr = sinkActivate->SetUINT32(MF_SAMPLEGRABBERSINK_IGNORE_CLOCK, FALSE);
        if (FAILED(hr))
            break;

        hr = MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &m_audioSampleGrabberNode);
        if (FAILED(hr))
            break;

        hr = m_audioSampleGrabberNode->SetObject(sinkActivate);
        if (FAILED(hr))
            break;

        hr = m_audioSampleGrabberNode->SetUINT32(MF_TOPONODE_STREAMID, 0); // Identifier of the stream sink.
        if (FAILED(hr))
            break;

        hr = m_audioSampleGrabberNode->SetUINT32(MF_TOPONODE_NOSHUTDOWN_ON_REMOVE, FALSE);
        if (FAILED(hr))
            break;

        hr = topology->AddNode(m_audioSampleGrabberNode);
        if (FAILED(hr))
            break;

        pType->Release();
        sinkActivate->Release();
        return true;
    } while (false);

    if (pType)
        pType->Release();
    if (sinkActivate)
        sinkActivate->Release();
    if (m_audioSampleGrabberNode) {
        m_audioSampleGrabberNode->Release();
        m_audioSampleGrabberNode = NULL;
    }
    return false;
}

bool MFPlayerSession::setupAudioSampleGrabber(IMFTopology *topology, IMFTopologyNode *sourceNode, IMFTopologyNode *outputNode)
{
    if (!addAudioSampleGrabberNode(topology))
        return false;

    HRESULT hr = S_OK;
    IMFTopologyNode *pTeeNode = NULL;

    IMFMediaTypeHandler *typeHandler = NULL;
    IMFMediaType *mediaType = NULL;
    do {
        hr = MFCreateTopologyNode(MF_TOPOLOGY_TEE_NODE, &pTeeNode);
        if (FAILED(hr))
            break;
        hr = sourceNode->ConnectOutput(0, pTeeNode, 0);
        if (FAILED(hr))
            break;
        hr = pTeeNode->ConnectOutput(0, outputNode, 0);
        if (FAILED(hr))
            break;
        hr = pTeeNode->ConnectOutput(1, m_audioSampleGrabberNode, 0);
        if (FAILED(hr))
            break;
    } while (false);

    if (pTeeNode)
        pTeeNode->Release();
    if (mediaType)
        mediaType->Release();
    if (typeHandler)
        typeHandler->Release();
    return hr == S_OK;
}

QAudioFormat MFPlayerSession::audioFormatForMFMediaType(IMFMediaType *mediaType) const
{
    WAVEFORMATEX *wfx = 0;
    UINT32 size;
    HRESULT hr = MFCreateWaveFormatExFromMFMediaType(mediaType, &wfx, &size, MFWaveFormatExConvertFlag_Normal);
    if (FAILED(hr))
        return QAudioFormat();

    if (size < sizeof(WAVEFORMATEX)) {
        CoTaskMemFree(wfx);
        return QAudioFormat();
    }

    if (wfx->wFormatTag != WAVE_FORMAT_PCM) {
        CoTaskMemFree(wfx);
        return QAudioFormat();
    }

    QAudioFormat format;
    format.setSampleRate(wfx->nSamplesPerSec);
    format.setChannelCount(wfx->nChannels);
    format.setSampleSize(wfx->wBitsPerSample);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);

    CoTaskMemFree(wfx);
    return format;
}

// BindOutputNode
// Sets the IMFStreamSink pointer on an output node.
// IMFActivate pointer in the output node must be converted to an
// IMFStreamSink pointer before the topology loader resolves the topology.
HRESULT BindOutputNode(IMFTopologyNode *pNode)
{
    IUnknown *nodeObject = NULL;
    IMFActivate *activate = NULL;
    IMFStreamSink *stream = NULL;
    IMFMediaSink *sink = NULL;

    HRESULT hr = pNode->GetObject(&nodeObject);
    if (FAILED(hr))
        return hr;

    hr = nodeObject->QueryInterface(IID_PPV_ARGS(&activate));
    if (SUCCEEDED(hr)) {
        DWORD dwStreamID = 0;

        // Try to create the media sink.
        hr = activate->ActivateObject(IID_PPV_ARGS(&sink));
        if (SUCCEEDED(hr))
           dwStreamID = MFGetAttributeUINT32(pNode, MF_TOPONODE_STREAMID, 0);

        if (SUCCEEDED(hr)) {
            // First check if the media sink already has a stream sink with the requested ID.
            hr = sink->GetStreamSinkById(dwStreamID, &stream);
            if (FAILED(hr)) {
                // Create the stream sink.
                hr = sink->AddStreamSink(dwStreamID, NULL, &stream);
            }
        }

        // Replace the node's object pointer with the stream sink.
        if (SUCCEEDED(hr)) {
            hr = pNode->SetObject(stream);
        }
    } else {
        hr = nodeObject->QueryInterface(IID_PPV_ARGS(&stream));
    }

    if (nodeObject)
        nodeObject->Release();
    if (activate)
        activate->Release();
    if (stream)
        stream->Release();
    if (sink)
        sink->Release();
    return hr;
}

// BindOutputNodes
// Sets the IMFStreamSink pointers on all of the output nodes in a topology.
HRESULT BindOutputNodes(IMFTopology *pTopology)
{
    DWORD cNodes = 0;

    IMFCollection *collection = NULL;
    IUnknown *element = NULL;
    IMFTopologyNode *node = NULL;

    // Get the collection of output nodes.
    HRESULT hr = pTopology->GetOutputNodeCollection(&collection);

    // Enumerate all of the nodes in the collection.
    if (SUCCEEDED(hr))
        hr = collection->GetElementCount(&cNodes);

    if (SUCCEEDED(hr)) {
        for (DWORD i = 0; i < cNodes; i++) {
            hr = collection->GetElement(i, &element);
            if (FAILED(hr))
                break;

            hr = element->QueryInterface(IID_IMFTopologyNode, (void**)&node);
            if (FAILED(hr))
                break;

            // Bind this node.
            hr = BindOutputNode(node);
            if (FAILED(hr))
                break;
        }
    }

    if (collection)
        collection->Release();
    if (element)
        element->Release();
    if (node)
        node->Release();
    return hr;
}

// This method binds output nodes to complete the topology,
// then loads the topology and inserts MFT between the output node
// and a filter connected to the output node.
IMFTopology *MFPlayerSession::insertMFT(IMFTopology *topology, TOPOID outputNodeId)
{
    bool isNewTopology = false;

    IMFTopoLoader *topoLoader = 0;
    IMFTopology *resolvedTopology = 0;
    IMFCollection *outputNodes = 0;

    do {
        if (FAILED(BindOutputNodes(topology)))
            break;

        if (FAILED(MFCreateTopoLoader(&topoLoader)))
            break;

        if (FAILED(topoLoader->Load(topology, &resolvedTopology, NULL))) {
            // Topology could not be resolved, adding ourselves a color converter
            // to the topology might solve the problem
            insertColorConverter(topology, outputNodeId);
            if (FAILED(topoLoader->Load(topology, &resolvedTopology, NULL)))
                break;
        }

        if (insertResizer(resolvedTopology))
            isNewTopology = true;

        // Get all output nodes and search for video output node.
        if (FAILED(resolvedTopology->GetOutputNodeCollection(&outputNodes)))
            break;

        DWORD elementCount = 0;
        if (FAILED(outputNodes->GetElementCount(&elementCount)))
            break;

        for (DWORD n = 0; n < elementCount; n++) {
            IUnknown *element = 0;
            IMFTopologyNode *node = 0;
            IUnknown *outputObject = 0;
            IMFMediaTypeHandler *videoSink = 0;
            IMFTopologyNode *inputNode = 0;
            IMFTopologyNode *mftNode = 0;
            bool mftAdded = false;

            do {
                if (FAILED(outputNodes->GetElement(n, &element)))
                    break;

                if (FAILED(element->QueryInterface(IID_IMFTopologyNode, (void**)&node)))
                    break;

                TOPOID id;
                if (FAILED(node->GetTopoNodeID(&id)))
                    break;

                if (id != outputNodeId)
                    break;

                // Use output supported media types for the MFT
                if (FAILED(node->GetObject(&outputObject)))
                    break;

                if (FAILED(outputObject->QueryInterface(IID_IMFMediaTypeHandler, (void**)&videoSink)))
                    break;

                DWORD mtCount;
                if (FAILED(videoSink->GetMediaTypeCount(&mtCount)))
                    break;

                for (DWORD i = 0; i < mtCount; ++i) {
                    IMFMediaType *type = 0;
                    if (SUCCEEDED(videoSink->GetMediaTypeByIndex(i, &type)))
                        m_videoProbeMFT->addSupportedMediaType(type);
                }

                // Insert MFT between the output node and the node connected to it.
                DWORD outputIndex = 0;
                if (FAILED(node->GetInput(0, &inputNode, &outputIndex)))
                    break;

                if (FAILED(MFCreateTopologyNode(MF_TOPOLOGY_TRANSFORM_NODE, &mftNode)))
                    break;

                if (FAILED(mftNode->SetObject(m_videoProbeMFT)))
                    break;

                if (FAILED(resolvedTopology->AddNode(mftNode)))
                    break;

                if (FAILED(inputNode->ConnectOutput(0, mftNode, 0)))
                    break;

                if (FAILED(mftNode->ConnectOutput(0, node, 0)))
                    break;

                mftAdded = true;
                isNewTopology = true;
            } while (false);

            if (mftNode)
                mftNode->Release();
            if (inputNode)
                inputNode->Release();
            if (node)
                node->Release();
            if (element)
                element->Release();
            if (videoSink)
                videoSink->Release();
            if (outputObject)
                outputObject->Release();

            if (mftAdded)
                break;
        }
    } while (false);

    if (outputNodes)
        outputNodes->Release();

    if (topoLoader)
        topoLoader->Release();

    if (isNewTopology) {
        topology->Release();
        return resolvedTopology;
    }

    if (resolvedTopology)
        resolvedTopology->Release();

    return topology;
}

// This method checks if the topology contains a color converter transform (CColorConvertDMO),
// if it does it inserts a resizer transform (CResizerDMO) to handle dynamic frame size change
// of the video stream.
// Returns true if it inserted a resizer
bool MFPlayerSession::insertResizer(IMFTopology *topology)
{
    bool inserted = false;
    WORD elementCount = 0;
    IMFTopologyNode *node = 0;
    IUnknown *object = 0;
    IWMColorConvProps *colorConv = 0;
    IMFTransform *resizer = 0;
    IMFTopologyNode *resizerNode = 0;
    IMFTopologyNode *inputNode = 0;

    HRESULT hr = topology->GetNodeCount(&elementCount);
    if (FAILED(hr))
        return false;

    for (WORD i = 0; i < elementCount; ++i) {
        if (node) {
            node->Release();
            node = 0;
        }
        if (object) {
            object->Release();
            object = 0;
        }

        if (FAILED(topology->GetNode(i, &node)))
            break;

        MF_TOPOLOGY_TYPE nodeType;
        if (FAILED(node->GetNodeType(&nodeType)))
            break;

        if (nodeType != MF_TOPOLOGY_TRANSFORM_NODE)
            continue;

        if (FAILED(node->GetObject(&object)))
            break;

        if (FAILED(object->QueryInterface(&colorConv)))
            continue;

        if (FAILED(CoCreateInstance(CLSID_CResizerDMO, NULL, CLSCTX_INPROC_SERVER, IID_IMFTransform, (void**)&resizer)))
            break;

        if (FAILED(MFCreateTopologyNode(MF_TOPOLOGY_TRANSFORM_NODE, &resizerNode)))
            break;

        if (FAILED(resizerNode->SetObject(resizer)))
            break;

        if (FAILED(topology->AddNode(resizerNode)))
            break;

        DWORD outputIndex = 0;
        if (FAILED(node->GetInput(0, &inputNode, &outputIndex))) {
            topology->RemoveNode(resizerNode);
            break;
        }

        if (FAILED(inputNode->ConnectOutput(0, resizerNode, 0))) {
            topology->RemoveNode(resizerNode);
            break;
        }

        if (FAILED(resizerNode->ConnectOutput(0, node, 0))) {
            inputNode->ConnectOutput(0, node, 0);
            topology->RemoveNode(resizerNode);
            break;
        }

        inserted = true;
        break;
    }

    if (node)
        node->Release();
    if (object)
        object->Release();
    if (colorConv)
        colorConv->Release();
    if (resizer)
        resizer->Release();
    if (resizerNode)
        resizerNode->Release();
    if (inputNode)
        inputNode->Release();

    return inserted;
}

// This method inserts a color converter (CColorConvertDMO) in the topology,
// typically to convert to RGB format.
// Usually this converter is automatically inserted when the topology is resolved but
// for some reason it fails to do so in some cases, we then do it ourselves.
void MFPlayerSession::insertColorConverter(IMFTopology *topology, TOPOID outputNodeId)
{
    IMFCollection *outputNodes = 0;

    if (FAILED(topology->GetOutputNodeCollection(&outputNodes)))
        return;

    DWORD elementCount = 0;
    if (FAILED(outputNodes->GetElementCount(&elementCount)))
        goto done;

    for (DWORD n = 0; n < elementCount; n++) {
        IUnknown *element = 0;
        IMFTopologyNode *node = 0;
        IMFTopologyNode *inputNode = 0;
        IMFTopologyNode *mftNode = 0;
        IMFTransform *converter = 0;

        do {
            if (FAILED(outputNodes->GetElement(n, &element)))
                break;

            if (FAILED(element->QueryInterface(IID_IMFTopologyNode, (void**)&node)))
                break;

            TOPOID id;
            if (FAILED(node->GetTopoNodeID(&id)))
                break;

            if (id != outputNodeId)
                break;

            DWORD outputIndex = 0;
            if (FAILED(node->GetInput(0, &inputNode, &outputIndex)))
                break;

            if (FAILED(MFCreateTopologyNode(MF_TOPOLOGY_TRANSFORM_NODE, &mftNode)))
                break;

            if (FAILED(CoCreateInstance(CLSID_CColorConvertDMO, NULL, CLSCTX_INPROC_SERVER, IID_IMFTransform, (void**)&converter)))
                break;

            if (FAILED(mftNode->SetObject(converter)))
                break;

            if (FAILED(topology->AddNode(mftNode)))
                break;

            if (FAILED(inputNode->ConnectOutput(0, mftNode, 0)))
                break;

            if (FAILED(mftNode->ConnectOutput(0, node, 0)))
                break;

        } while (false);

        if (mftNode)
            mftNode->Release();
        if (inputNode)
            inputNode->Release();
        if (node)
            node->Release();
        if (element)
            element->Release();
        if (converter)
            converter->Release();
    }

done:
    if (outputNodes)
        outputNodes->Release();
}

void MFPlayerSession::stop(bool immediate)
{
#ifdef DEBUG_MEDIAFOUNDATION
    qDebug() << "stop";
#endif
    if (!immediate && m_pendingState != NoPending) {
        m_request.setCommand(CmdStop);
    } else {
        if (m_state.command == CmdStop)
            return;

        if (m_scrubbing)
            scrub(false);

        if (SUCCEEDED(m_session->Stop())) {
            m_state.setCommand(CmdStop);
            m_pendingState = CmdPending;
            if (m_status != QMediaPlayer::EndOfMedia) {
                m_varStart.vt = VT_I8;
                m_varStart.hVal.QuadPart = 0;
            }
        } else {
            emit error(QMediaPlayer::ResourceError, tr("Failed to stop."), true);
        }
    }
}

void MFPlayerSession::start()
{
    switch (m_status) {
    case QMediaPlayer::EndOfMedia:
        m_varStart.hVal.QuadPart = 0;
        changeStatus(QMediaPlayer::BufferedMedia);
        return;
    }

#ifdef DEBUG_MEDIAFOUNDATION
    qDebug() << "start";
#endif

    if (m_pendingState != NoPending) {
        m_request.setCommand(CmdStart);
    } else {
        if (m_state.command == CmdStart)
            return;

        if (m_scrubbing)
            scrub(false);

        if (SUCCEEDED(m_session->Start(&GUID_NULL, &m_varStart))) {
            m_state.setCommand(CmdStart);
            m_pendingState = CmdPending;
            PropVariantInit(&m_varStart);
        } else {
            emit error(QMediaPlayer::ResourceError, tr("failed to start playback"), true);
        }
    }
}

void MFPlayerSession::pause()
{
#ifdef DEBUG_MEDIAFOUNDATION
    qDebug() << "pause";
#endif
    if (m_pendingState != NoPending) {
        m_request.setCommand(CmdPause);
    } else {
        if (m_state.command == CmdPause)
            return;

        if (SUCCEEDED(m_session->Pause())) {
            m_state.setCommand(CmdPause);
            m_pendingState = CmdPending;
        } else {
            emit error(QMediaPlayer::ResourceError, tr("Failed to pause."), false);
        }
    }
}

void MFPlayerSession::changeStatus(QMediaPlayer::MediaStatus newStatus)
{
    if (m_status == newStatus)
        return;
#ifdef DEBUG_MEDIAFOUNDATION
    qDebug() << "MFPlayerSession::changeStatus" << newStatus;
#endif
    m_status = newStatus;
    emit statusChanged();
}

QMediaPlayer::MediaStatus MFPlayerSession::status() const
{
    return m_status;
}

void MFPlayerSession::createSession()
{
    close();

    m_hCloseEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    m_sourceResolver = new SourceResolver();
    QObject::connect(m_sourceResolver, SIGNAL(mediaSourceReady()), this, SLOT(handleMediaSourceReady()));
    QObject::connect(m_sourceResolver, SIGNAL(error(long)), this, SLOT(handleSourceError(long)));

    m_videoProbeMFT = new MFTransform;
    for (int i = 0; i < m_videoProbes.size(); ++i)
        m_videoProbeMFT->addProbe(m_videoProbes.at(i));

    Q_ASSERT(m_session == NULL);
    HRESULT hr = MFCreateMediaSession(NULL, &m_session);
    if (FAILED(hr)) {
        changeStatus(QMediaPlayer::UnknownMediaStatus);
        emit error(QMediaPlayer::ResourceError, tr("Unable to create mediasession."), true);
    }

    hr = m_session->BeginGetEvent(this, m_session);

    if (FAILED(hr)) {
        changeStatus(QMediaPlayer::UnknownMediaStatus);
        emit error(QMediaPlayer::ResourceError, tr("Unable to pull session events."), false);
    }

    PropVariantInit(&m_varStart);
    m_varStart.vt = VT_I8;
    m_varStart.hVal.QuadPart = 0;
}

qint64 MFPlayerSession::position()
{
    if (m_request.command == CmdSeek || m_request.command == CmdSeekResume)
        return m_request.start;

    if (m_pendingState == SeekPending)
        return m_state.start;

    if (m_state.command == CmdStop)
        return qint64(m_varStart.hVal.QuadPart / 10000);

    if (m_presentationClock) {
        MFTIME time, sysTime;
        if (FAILED(m_presentationClock->GetCorrelatedTime(0, &time, &sysTime)))
            return 0;
        return qint64(time / 10000);
    }
    return 0;
}

void MFPlayerSession::setPosition(qint64 position)
{
#ifdef DEBUG_MEDIAFOUNDATION
    qDebug() << "setPosition";
#endif
    if (m_pendingState != NoPending) {
        m_request.setCommand(CmdSeek);
        m_request.start = position;
    } else {
        setPositionInternal(position, CmdNone);
    }
}

void MFPlayerSession::setPositionInternal(qint64 position, Command requestCmd)
{
    if (m_status == QMediaPlayer::EndOfMedia)
        changeStatus(QMediaPlayer::LoadedMedia);
    if (m_state.command == CmdStop && requestCmd != CmdSeekResume) {
        m_varStart.vt = VT_I8;
        m_varStart.hVal.QuadPart = LONGLONG(position * 10000);
        return;
    }

    if (m_state.command == CmdPause)
        scrub(true);

#ifdef DEBUG_MEDIAFOUNDATION
    qDebug() << "setPositionInternal";
#endif

    PROPVARIANT varStart;
    varStart.vt = VT_I8;
    varStart.hVal.QuadPart = LONGLONG(position * 10000);
    if (SUCCEEDED(m_session->Start(NULL, &varStart)))
    {
        PropVariantInit(&m_varStart);
        // Store the pending state.
        m_state.setCommand(CmdStart);
        m_state.start = position;
        m_pendingState = SeekPending;
    } else {
        emit error(QMediaPlayer::ResourceError, tr("Failed to seek."), true);
    }
}

qreal MFPlayerSession::playbackRate() const
{
    if (m_pendingState != NoPending)
        return m_request.rate;
    return m_state.rate;
}

void MFPlayerSession::setPlaybackRate(qreal rate)
{
    if (m_scrubbing) {
        m_restoreRate = rate;
        return;
    }
    setPlaybackRateInternal(rate);
}

void MFPlayerSession::setPlaybackRateInternal(qreal rate)
{
    if (rate == m_request.rate)
        return;

    m_pendingRate = rate;
    if (!m_rateSupport)
        return;

#ifdef DEBUG_MEDIAFOUNDATION
    qDebug() << "setPlaybackRate";
#endif
    BOOL isThin = FALSE;

    //from MSDN http://msdn.microsoft.com/en-us/library/aa965220%28v=vs.85%29.aspx
    //Thinning applies primarily to video streams.
    //In thinned mode, the source drops delta frames and deliver only key frames.
    //At very high playback rates, the source might skip some key frames (for example, deliver every other key frame).

    if (FAILED(m_rateSupport->IsRateSupported(FALSE, rate, NULL))) {
        isThin = TRUE;
        if (FAILED(m_rateSupport->IsRateSupported(isThin, rate, NULL))) {
            qWarning() << "unable to set playbackrate = " << rate;
        }
    }
    if (m_pendingState != NoPending) {
        m_request.rate = rate;
        m_request.isThin = isThin;
        // Remember the current transport state (play, paused, etc), so that we
        // can restore it after the rate change, if necessary. However, if
        // anothercommand is already pending, that one takes precedent.
        if (m_request.command == CmdNone)
            m_request.setCommand(m_state.command);
    } else {
        //No pending operation. Commit the new rate.
        commitRateChange(rate, isThin);
    }
}

void MFPlayerSession::commitRateChange(qreal rate, BOOL isThin)
{
#ifdef DEBUG_MEDIAFOUNDATION
    qDebug() << "commitRateChange";
#endif
    Q_ASSERT(m_pendingState == NoPending);
    MFTIME  hnsSystemTime = 0;
    MFTIME  hnsClockTime = 0;
    Command cmdNow = m_state.command;
    // Allowed rate transitions:
    // Positive <-> negative:   Stopped
    // Negative <-> zero:       Stopped
    // Postive <-> zero:        Paused or stopped
    if ((rate > 0 && m_state.rate <= 0) || (rate < 0 && m_state.rate >= 0)) {
        if (cmdNow == CmdStart) {
            // Get the current clock position. This will be the restart time.
            m_presentationClock->GetCorrelatedTime(0, &hnsClockTime, &hnsSystemTime);
            Q_ASSERT(hnsSystemTime != 0);

            // We need to stop only when dealing with negative rates
            if (rate >= 0 && m_state.rate >= 0)
                pause();
            else
                stop();

            // If we deal with negative rates, we stopped the session and consequently
            // reset the position to zero. We then need to resume to the current position.
            m_request.setCommand(rate < 0 || m_state.rate < 0 ? CmdSeekResume : CmdStart);
            m_request.start = hnsClockTime / 10000;
        } else if (cmdNow == CmdPause) {
            if (rate < 0 || m_state.rate < 0) {
                // The current state is paused.
                // For this rate change, the session must be stopped. However, the
                // session cannot transition back from stopped to paused.
                // Therefore, this rate transition is not supported while paused.
                qWarning() << "Unable to change rate from positive to negative or vice versa in paused state";
                return;
            }

            // This happens when resuming playback after scrubbing in pause mode.
            // This transition requires the session to be paused. Even though our
            // internal state is set to paused, the session might not be so we need
            // to enforce it
            if (rate > 0 && m_state.rate == 0) {
                m_state.setCommand(CmdNone);
                pause();
            }
        }
    } else if (rate == 0 && m_state.rate > 0) {
        if (cmdNow != CmdPause) {
            // Transition to paused.
            // This transisition requires the paused state.
            // Pause and set the rate.
            pause();

            // Request: Switch back to current state.
            m_request.setCommand(cmdNow);
        }
    } else if (rate == 0 && m_state.rate < 0) {
        // Changing rate from negative to zero requires to stop the session
        m_presentationClock->GetCorrelatedTime(0, &hnsClockTime, &hnsSystemTime);

        stop();

        // Resumte to the current position (stop() will reset the position to 0)
        m_request.setCommand(CmdSeekResume);
        m_request.start = hnsClockTime / 10000;
    }

    // Set the rate.
    if (FAILED(m_rateControl->SetRate(isThin, rate))) {
        qWarning() << "failed to set playbackrate = " << rate;
        return;
    }

    // Adjust our current rate and requested rate.
    m_pendingRate = m_request.rate = m_state.rate = rate;

}

void MFPlayerSession::scrub(bool enableScrub)
{
    if (m_scrubbing == enableScrub)
        return;

    m_scrubbing = enableScrub;

    if (!canScrub()) {
        if (!enableScrub)
            m_pendingRate = m_restoreRate;
        return;
    }

    if (enableScrub)  {
        // Enter scrubbing mode. Cache the rate.
        m_restoreRate = m_request.rate;
        setPlaybackRateInternal(0.0f);
    } else {
        // Leaving scrubbing mode. Restore the old rate.
        setPlaybackRateInternal(m_restoreRate);
    }
}

int MFPlayerSession::volume() const
{
    return m_volume;
}

void MFPlayerSession::setVolume(int volume)
{
    if (m_volume == volume)
        return;
    m_volume = volume;
    if (m_volumeControl)
        m_volumeControl->SetMasterVolume(m_volume * 0.01f);
    emit volumeChanged(m_volume);
}

bool MFPlayerSession::isMuted() const
{
    return m_muted;
}

void MFPlayerSession::setMuted(bool muted)
{
    if (m_muted == muted)
        return;
    m_muted = muted;
    if (m_volumeControl)
        m_volumeControl->SetMute(BOOL(m_muted));
    emit mutedChanged(m_muted);
}

int MFPlayerSession::bufferStatus()
{
    if (!m_netsourceStatistics)
        return 0;
    PROPVARIANT var;
    PROPERTYKEY key;
    key.fmtid = MFNETSOURCE_STATISTICS;
    key.pid = MFNETSOURCE_BUFFERPROGRESS_ID;
    int progress = -1;
    if (SUCCEEDED(m_netsourceStatistics->GetValue(key, &var))) {
        progress = var.lVal;
    }
    PropVariantClear(&var);

#ifdef DEBUG_MEDIAFOUNDATION
    qDebug() << "bufferStatus: progress = " << progress;
#endif

    return progress;
}

QMediaTimeRange MFPlayerSession::availablePlaybackRanges()
{
    if (!m_netsourceStatistics)
        return QMediaTimeRange();

    qint64 start = 0, end = 0;
    PROPVARIANT var;
    PROPERTYKEY key;
    key.fmtid = MFNETSOURCE_STATISTICS;
    key.pid = MFNETSOURCE_SEEKRANGESTART_ID;
    if (SUCCEEDED(m_netsourceStatistics->GetValue(key, &var))) {
        start = qint64(var.uhVal.QuadPart / 10000);
        key.pid = MFNETSOURCE_SEEKRANGEEND_ID;
        if (SUCCEEDED(m_netsourceStatistics->GetValue(key, &var))) {
            end = qint64(var.uhVal.QuadPart / 10000);
        }
    }
    PropVariantClear(&var);
    return QMediaTimeRange(start, end);
}

HRESULT MFPlayerSession::QueryInterface(REFIID riid, void** ppvObject)
{
    if (!ppvObject)
        return E_POINTER;
    if (riid == IID_IMFAsyncCallback) {
        *ppvObject = static_cast<IMFAsyncCallback*>(this);
    } else if (riid == IID_IUnknown) {
        *ppvObject = static_cast<IUnknown*>(this);
    } else {
        *ppvObject =  NULL;
        return E_NOINTERFACE;
    }
    return S_OK;
}

ULONG MFPlayerSession::AddRef(void)
{
    return InterlockedIncrement(&m_cRef);
}

ULONG MFPlayerSession::Release(void)
{
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (cRef == 0)
        this->deleteLater();
    return cRef;
}

HRESULT MFPlayerSession::Invoke(IMFAsyncResult *pResult)
{
    if (pResult->GetStateNoAddRef() != m_session)
        return S_OK;

    IMFMediaEvent *pEvent = NULL;
    // Get the event from the event queue.
    HRESULT hr = m_session->EndGetEvent(pResult, &pEvent);
    if (FAILED(hr)) {
        return S_OK;
    }

    MediaEventType meType = MEUnknown;
    hr = pEvent->GetType(&meType);
    if (FAILED(hr)) {
        pEvent->Release();
        return S_OK;
    }

    if (meType == MESessionClosed) {
        SetEvent(m_hCloseEvent);
        pEvent->Release();
        return S_OK;
    } else {
        hr = m_session->BeginGetEvent(this, m_session);
        if (FAILED(hr)) {
            pEvent->Release();
            return S_OK;
        }
    }

    if (!m_closing)
        emit sessionEvent(pEvent);
    return S_OK;
}

void MFPlayerSession::handleSessionEvent(IMFMediaEvent *sessionEvent)
{
    HRESULT hrStatus = S_OK;
    HRESULT hr = sessionEvent->GetStatus(&hrStatus);
    if (FAILED(hr)) {
        sessionEvent->Release();
        return;
    }

    MediaEventType meType = MEUnknown;
    hr = sessionEvent->GetType(&meType);

#ifdef DEBUG_MEDIAFOUNDATION
    if (FAILED(hrStatus))
        qDebug() << "handleSessionEvent: MediaEventType = " << meType << "Failed";
    else
        qDebug() << "handleSessionEvent: MediaEventType = " << meType;
#endif

    switch (meType) {
    case MENonFatalError: {
            PROPVARIANT var;
            PropVariantInit(&var);
            sessionEvent->GetValue(&var);
            qWarning() << "handleSessionEvent: non fatal error = " << var.ulVal;
            PropVariantClear(&var);
            emit error(QMediaPlayer::ResourceError, tr("Media session non-fatal error."), false);
        }
        break;
    case MESourceUnknown:
        changeStatus(QMediaPlayer::InvalidMedia);
        break;
    case MEError:
        changeStatus(QMediaPlayer::UnknownMediaStatus);
        qWarning() << "handleSessionEvent: serious error = " << hrStatus;
        emit error(QMediaPlayer::ResourceError, tr("Media session serious error."), true);
        break;
    case MESessionRateChanged:
        // If the rate change succeeded, we've already got the rate
        // cached. If it failed, try to get the actual rate.
        if (FAILED(hrStatus)) {
            PROPVARIANT var;
            PropVariantInit(&var);
            if (SUCCEEDED(sessionEvent->GetValue(&var)) && (var.vt == VT_R4))    {
                m_state.rate = var.fltVal;
            }
            emit playbackRateChanged(playbackRate());
        }
        break;
    case MESessionScrubSampleComplete :
        if (m_scrubbing)
            updatePendingCommands(CmdStart);
        break;
    case MESessionStarted:
        updatePendingCommands(CmdStart);
#ifndef Q_WS_SIMULATOR
        // playback started, we can now set again the procAmpValues if they have been
        // changed previously (these are lost when loading a new media)
        if (m_playerService->videoWindowControl()) {
            m_playerService->videoWindowControl()->setProcAmpValues();
        }
#endif
        break;
    case MESessionStopped:
        if (m_status != QMediaPlayer::EndOfMedia) {
            m_varStart.vt = VT_I8;
            m_varStart.hVal.QuadPart = 0;

            // Reset to Loaded status unless we are loading a new media
            // or if the media is buffered (to avoid restarting the video surface)
            if (m_status != QMediaPlayer::LoadingMedia && m_status != QMediaPlayer::BufferedMedia)
                changeStatus(QMediaPlayer::LoadedMedia);
        }
        updatePendingCommands(CmdStop);
        break;
    case MESessionPaused:
        updatePendingCommands(CmdPause);
        break;
    case MEReconnectStart:
#ifdef DEBUG_MEDIAFOUNDATION
            qDebug() << "MEReconnectStart" << ((hrStatus == S_OK) ? "OK" : "Failed");
#endif
        break;
    case MEReconnectEnd:
#ifdef DEBUG_MEDIAFOUNDATION
            qDebug() << "MEReconnectEnd" << ((hrStatus == S_OK) ? "OK" : "Failed");
#endif
        break;
    case MESessionTopologySet:
        if (FAILED(hrStatus)) {
            changeStatus(QMediaPlayer::InvalidMedia);
            emit error(QMediaPlayer::FormatError, tr("Unsupported media, a codec is missing."), true);
        } else {
            if (m_audioSampleGrabberNode) {
                IMFMediaType *mediaType = 0;
                hr = MFGetTopoNodeCurrentType(m_audioSampleGrabberNode, 0, FALSE, &mediaType);
                if (SUCCEEDED(hr)) {
                    m_audioSampleGrabber->setFormat(audioFormatForMFMediaType(mediaType));
                    mediaType->Release();
                }
            }

            if (SUCCEEDED(MFGetService(m_session, MR_POLICY_VOLUME_SERVICE, IID_PPV_ARGS(&m_volumeControl)))) {
                m_volumeControl->SetMasterVolume(m_volume * 0.01f);
                m_volumeControl->SetMute(m_muted);
            }

            DWORD dwCharacteristics = 0;
            m_sourceResolver->mediaSource()->GetCharacteristics(&dwCharacteristics);
            emit seekableUpdate(MFMEDIASOURCE_CAN_SEEK & dwCharacteristics);

            // Topology is resolved and successfuly set, this happens only after loading a new media.
            // Make sure we always start the media from the beginning
            m_varStart.vt = VT_I8;
            m_varStart.hVal.QuadPart = 0;

            changeStatus(QMediaPlayer::LoadedMedia);
        }
        break;
    }

    if (FAILED(hrStatus)) {
        sessionEvent->Release();
        return;
    }

    switch (meType) {
    case MEBufferingStarted:
        changeStatus(m_status == QMediaPlayer::LoadedMedia ? QMediaPlayer::BufferingMedia : QMediaPlayer::StalledMedia);
        emit bufferStatusChanged(bufferStatus());
        break;
    case MEBufferingStopped:
        if (m_status == QMediaPlayer::BufferingMedia)
            stop(true);
        changeStatus(QMediaPlayer::BufferedMedia);
        emit bufferStatusChanged(bufferStatus());
        break;
    case MEEndOfPresentation:
        stop();
        changeStatus(QMediaPlayer::EndOfMedia);
        m_varStart.vt = VT_I8;
        //keep reporting the final position after end of media
        m_varStart.hVal.QuadPart = m_duration;
        break;
    case MESessionEnded:
        m_pendingState = NoPending;
        m_state.command = CmdStop;
        m_state.prevCmd = CmdNone;
        m_request.command = CmdNone;
        m_request.prevCmd = CmdNone;
        break;
    case MEEndOfPresentationSegment:
        break;
    case MEAudioSessionVolumeChanged:
        if (m_volumeControl) {
            float currentVolume = 1;
            if (SUCCEEDED(m_volumeControl->GetMasterVolume(&currentVolume))) {
                int scaledVolume = currentVolume * 100;
                if (scaledVolume != m_volume) {
                    m_volume = scaledVolume;
                    emit volumeChanged(scaledVolume);
                }
            }
            BOOL currentMuted = FALSE;
            if (SUCCEEDED(m_volumeControl->GetMute(&currentMuted))) {
                if (currentMuted != BOOL(m_muted)) {
                    m_muted = bool(currentMuted);
                    emit mutedChanged(m_muted);
                }
            }
        }
        break;
    case MESessionTopologyStatus: {
            UINT32 status;
            if (SUCCEEDED(sessionEvent->GetUINT32(MF_EVENT_TOPOLOGY_STATUS, &status))) {
                if (status == MF_TOPOSTATUS_READY) {
                    IMFClock* clock;
                    if (SUCCEEDED(m_session->GetClock(&clock))) {
                        clock->QueryInterface(IID_IMFPresentationClock, (void**)(&m_presentationClock));
                        clock->Release();
                    }

                    if (SUCCEEDED(MFGetService(m_session, MF_RATE_CONTROL_SERVICE, IID_PPV_ARGS(&m_rateControl)))) {
                        if (SUCCEEDED(MFGetService(m_session, MF_RATE_CONTROL_SERVICE, IID_PPV_ARGS(&m_rateSupport)))) {
                            if ((m_mediaTypes & Video) == Video
                                && SUCCEEDED(m_rateSupport->IsRateSupported(TRUE, 0, NULL)))
                                m_canScrub = true;
                        }
                        BOOL isThin = FALSE;
                        float rate = 1;
                        if (SUCCEEDED(m_rateControl->GetRate(&isThin, &rate))) {
                            if (m_pendingRate != rate) {
                                m_state.rate = m_request.rate = rate;
                                setPlaybackRate(m_pendingRate);
                            }
                        }
                    }
                    MFGetService(m_session, MFNETSOURCE_STATISTICS_SERVICE, IID_PPV_ARGS(&m_netsourceStatistics));

                    if (!m_netsourceStatistics || bufferStatus() == 100) {
                        // If the source reader doesn't implement the statistics service, just set the status
                        // to buffered, since there is no way to query the buffering progress...
                        changeStatus(QMediaPlayer::BufferedMedia);
                    } else {
                        // Start to trigger buffering. Once enough buffering is done, the session will
                        // be automatically stopped unless the user has explicitly started playback.
                        start();
                    }
                }
            }
        }
        break;
    }

    sessionEvent->Release();
}

void MFPlayerSession::updatePendingCommands(Command command)
{
    emit positionChanged(position());
    if (m_state.command != command || m_pendingState == NoPending)
        return;

    // The current pending command has completed.
    if (m_pendingState == SeekPending && m_state.prevCmd == CmdPause) {
        m_pendingState = NoPending;
        m_state.setCommand(CmdPause);
    }

    m_pendingState = NoPending;

    //First look for rate changes.
    if (m_request.rate != m_state.rate) {
        commitRateChange(m_request.rate, m_request.isThin);
    }

    // Now look for new requests.
    if (m_pendingState == NoPending) {
        switch (m_request.command) {
        case CmdStart:
            start();
            break;
        case CmdPause:
            pause();
            break;
        case CmdStop:
            stop();
            break;
        case CmdSeek:
        case CmdSeekResume:
            setPositionInternal(m_request.start, m_request.command);
        }
        m_request.setCommand(CmdNone);
    }

}

bool MFPlayerSession::canScrub() const
{
    return m_canScrub && m_rateSupport && m_rateControl;
}

void MFPlayerSession::clear()
{
#ifdef DEBUG_MEDIAFOUNDATION
    qDebug() << "MFPlayerSession::clear";
#endif
    m_mediaTypes = 0;
    m_canScrub = false;

    m_pendingState = NoPending;
    m_state.command = CmdStop;
    m_state.prevCmd = CmdNone;
    m_request.command = CmdNone;
    m_request.prevCmd = CmdNone;

    if (m_presentationClock) {
        m_presentationClock->Release();
        m_presentationClock = NULL;
    }
    if (m_rateControl) {
        m_rateControl->Release();
        m_rateControl = NULL;
    }
    if (m_rateSupport) {
        m_rateSupport->Release();
        m_rateSupport = NULL;
    }
    if (m_volumeControl) {
        m_volumeControl->Release();
        m_volumeControl = NULL;
    }
    if (m_netsourceStatistics) {
        m_netsourceStatistics->Release();
        m_netsourceStatistics = NULL;
    }
    if (m_audioSampleGrabberNode) {
        m_audioSampleGrabberNode->Release();
        m_audioSampleGrabberNode = NULL;
    }
}
