/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
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

#include "qwindowscamerareader_p.h"

#include "qwindowsmultimediautils_p.h"
#include <qvideosink.h>
#include <private/qmemoryvideobuffer_p.h>
#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

QWindowsCameraReader::QWindowsCameraReader(QObject *parent)
    : QObject(parent),
      m_finalizeSemaphore(1)
{
    m_durationTimer.setInterval(100);
    connect(&m_durationTimer, SIGNAL(timeout()), this, SLOT(updateDuration()));
}

QWindowsCameraReader::~QWindowsCameraReader()
{
    deactivate();
}

bool QWindowsCameraReader::activate(const QString &cameraId)
{
    QMutexLocker locker(&m_mutex);

    if (m_sourceMediaType) {
        m_sourceMediaType->Release();
        m_sourceMediaType = nullptr;
    }
    if (m_sourceReader) {
        m_sourceReader->Release();
        m_sourceReader = nullptr;
    }
    if (m_source) {
        m_source->Release();
        m_source = nullptr;
    }

    m_active = false;
    m_streaming = false;

    IMFAttributes *sourceAttributes = nullptr;

    HRESULT hr = MFCreateAttributes(&sourceAttributes, 2);
    if (SUCCEEDED(hr)) {

        hr = sourceAttributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
                                       MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
        if (SUCCEEDED(hr)) {

            hr = sourceAttributes->SetString(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK,
                                             reinterpret_cast<LPCWSTR>(cameraId.utf16()));
            if (SUCCEEDED(hr)) {

                // get the media source associated with the symbolic link
                hr = MFCreateDeviceSource(sourceAttributes, &m_source);
                if (SUCCEEDED(hr)) {

                    IMFAttributes *readerAttributes = nullptr;

                    hr = MFCreateAttributes(&readerAttributes, 1);
                    if (SUCCEEDED(hr)) {

                        // Set callback so OnReadSample() is called for each new video frame.
                        hr = readerAttributes->SetUnknown(MF_SOURCE_READER_ASYNC_CALLBACK,
                                                          static_cast<IMFSourceReaderCallback*>(this));
                        if (SUCCEEDED(hr)) {

                            hr = MFCreateSourceReaderFromMediaSource(m_source, readerAttributes, &m_sourceReader);
                            if (SUCCEEDED(hr)) {

                                hr = m_sourceReader->GetCurrentMediaType(DWORD(MF_SOURCE_READER_FIRST_VIDEO_STREAM),
                                                                         &m_sourceMediaType);
                                if (SUCCEEDED(hr)) {

                                    GUID subtype = GUID_NULL;
                                    hr = m_sourceMediaType->GetGUID(MF_MT_SUBTYPE, &subtype);
                                    if (SUCCEEDED(hr)) {

                                        m_pixelFormat = QWindowsMultimediaUtils::pixelFormatFromMediaSubtype(subtype);

                                        if (m_pixelFormat == QVideoFrameFormat::Format_Invalid) {
                                            hr = S_FALSE;
                                        } else {

                                            // get the frame dimensions
                                            hr = MFGetAttributeSize(m_sourceMediaType, MF_MT_FRAME_SIZE, &m_frameWidth, &m_frameHeight);
                                            if (SUCCEEDED(hr)) {

                                                // and the stride, which we need to convert the frame later
                                                hr = MFGetStrideForBitmapInfoHeader(subtype.Data1, m_frameWidth, &m_stride);
                                                if (SUCCEEDED(hr)) {

                                                    UINT32 frameRateNum, frameRateDen;
                                                    hr = MFGetAttributeRatio(m_sourceMediaType, MF_MT_FRAME_RATE, &frameRateNum, &frameRateDen);
                                                    if (SUCCEEDED(hr)) {

                                                        m_frameRate = qreal(frameRateNum) / frameRateDen;

                                                        hr = m_sourceReader->SetStreamSelection(DWORD(MF_SOURCE_READER_FIRST_VIDEO_STREAM), TRUE);
                                                        if (SUCCEEDED(hr)) {

                                                            m_active = true;

                                                            // request the first frame
                                                            hr = m_sourceReader->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM,
                                                                                            0, nullptr, nullptr, nullptr, nullptr);
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        readerAttributes->Release();
                    }
                }
            }
        }
        sourceAttributes->Release();
    }

    return SUCCEEDED(hr);
}

void QWindowsCameraReader::deactivate()
{
    stopRecording();
    stopStreaming();
    m_active = false;
    m_streaming = false;
}

void QWindowsCameraReader::stopStreaming()
{
    QMutexLocker locker(&m_mutex);

    if (m_sourceMediaType) {
        m_sourceMediaType->Release();
        m_sourceMediaType = nullptr;
    }
    if (m_sourceReader) {
        m_sourceReader->Release();
        m_sourceReader = nullptr;
    }
    if (m_source) {
        m_source->Release();
        m_source = nullptr;
    }
}

bool QWindowsCameraReader::startRecording(const QString &fileName, const GUID &container,
                                          const GUID &videoFormat, UINT32 bitRate, UINT32 width,
                                          UINT32 height, qreal frameRate)
{
    QMutexLocker locker(&m_mutex);

    if (!m_active || m_recording)
        return false;

    IMFAttributes *writerAttributes = nullptr;

    HRESULT hr = MFCreateAttributes(&writerAttributes, 2);
    if (SUCCEEDED(hr)) {

        // Set callback so OnFinalize() is called after video is saved.
        hr = writerAttributes->SetUnknown(MF_SINK_WRITER_ASYNC_CALLBACK,
                                          static_cast<IMFSinkWriterCallback*>(this));
        if (SUCCEEDED(hr)) {

            hr = writerAttributes->SetGUID(MF_TRANSCODE_CONTAINERTYPE, container);
            if (SUCCEEDED(hr)) {

                hr = MFCreateSinkWriterFromURL(reinterpret_cast<LPCWSTR>(fileName.utf16()),
                                               nullptr, writerAttributes, &m_sinkWriter);
                if (SUCCEEDED(hr)) {

                    IMFMediaType *targetMediaType = nullptr;

                    hr = MFCreateMediaType(&targetMediaType);
                    if (SUCCEEDED(hr)) {

                        hr = targetMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
                        if (SUCCEEDED(hr)) {

                            hr = targetMediaType->SetGUID(MF_MT_SUBTYPE, videoFormat);
                            if (SUCCEEDED(hr)) {

                                hr = targetMediaType->SetUINT32(MF_MT_AVG_BITRATE, bitRate);
                                if (SUCCEEDED(hr)) {

                                    hr = MFSetAttributeSize(targetMediaType, MF_MT_FRAME_SIZE, width, height);
                                    if (SUCCEEDED(hr)) {

                                        hr = MFSetAttributeRatio(targetMediaType, MF_MT_FRAME_RATE, UINT32(frameRate * 1000), 1000);
                                        if (SUCCEEDED(hr)) {

                                            UINT32 t1, t2;
                                            MFGetAttributeRatio(m_sourceMediaType, MF_MT_PIXEL_ASPECT_RATIO, &t1, &t2);
                                            MFSetAttributeRatio(targetMediaType, MF_MT_PIXEL_ASPECT_RATIO, t1, t2);

                                            m_sourceMediaType->GetUINT32(MF_MT_INTERLACE_MODE, &t1);
                                            targetMediaType->SetUINT32(MF_MT_INTERLACE_MODE, t1);

                                            hr = m_sinkWriter->AddStream(targetMediaType, &m_videoStreamIndex);
                                            if (SUCCEEDED(hr)) {

                                                hr = m_sinkWriter->SetInputMediaType(m_videoStreamIndex, m_sourceMediaType, nullptr);
                                                if (SUCCEEDED(hr)) {

                                                    hr = m_sinkWriter->BeginWriting();
                                                    if (SUCCEEDED(hr)) {
                                                        m_lastDuration = -1;
                                                        m_currentDuration = 0;
                                                        updateDuration();
                                                        m_durationTimer.start();
                                                        m_recording = true;
                                                        m_firstFrame = true;
                                                        m_paused = false;
                                                        m_pauseChanging = false;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        targetMediaType->Release();
                    }
                }
            }
        }
        writerAttributes->Release();
    }
    return SUCCEEDED(hr);
}

void QWindowsCameraReader::stopRecording()
{
    // The semaphore is used to ensure the video is properly saved
    // to disk, e.g, before the app exits. Released on OnFinalize.
    // Acquire it BEFORE locking the mutex, to avoid deadlocks.
    m_finalizeSemaphore.acquire();
    QMutexLocker locker(&m_mutex);

    if (m_sinkWriter && m_recording)
        m_sinkWriter->Finalize();
    else
        m_finalizeSemaphore.release();

    m_recording = false;
    m_paused = false;
    m_pauseChanging = false;

    m_durationTimer.stop();
    m_lastDuration = -1;
    m_currentDuration = -1;
}

bool QWindowsCameraReader::pauseRecording()
{
    if (!m_recording || m_paused)
        return false;
    m_paused = true;
    m_pauseChanging = true;
    return true;
}

bool QWindowsCameraReader::resumeRecording()
{
    if (!m_recording || !m_paused)
        return false;
    m_paused = false;
    m_pauseChanging = true;
    return true;
}

//from IUnknown
STDMETHODIMP QWindowsCameraReader::QueryInterface(REFIID riid, LPVOID *ppvObject)
{
    if (!ppvObject)
        return E_POINTER;
    if (riid == IID_IMFSourceReaderCallback) {
        *ppvObject = static_cast<IMFSourceReaderCallback*>(this);
    } else if (riid == IID_IMFSinkWriterCallback) {
        *ppvObject = static_cast<IMFSinkWriterCallback*>(this);
    } else if (riid == IID_IUnknown) {
        *ppvObject = static_cast<IUnknown*>(static_cast<IMFSourceReaderCallback*>(this));
    } else {
        *ppvObject =  nullptr;
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) QWindowsCameraReader::AddRef(void)
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) QWindowsCameraReader::Release(void)
{
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (cRef == 0) {
        this->deleteLater();
    }
    return cRef;
}

void QWindowsCameraReader::setSurface(QVideoSink *surface)
{
    m_surface = surface;
}

UINT32 QWindowsCameraReader::frameWidth() const
{
    return m_frameWidth;
}

UINT32 QWindowsCameraReader::frameHeight() const
{
    return m_frameHeight;
}

qreal QWindowsCameraReader::frameRate() const
{
    return m_frameRate;
}

void QWindowsCameraReader::updateDuration()
{
    if (m_currentDuration >= 0 && m_lastDuration != m_currentDuration) {
        m_lastDuration = m_currentDuration;
        emit durationChanged(m_currentDuration);
    }
}

//from IMFSourceReaderCallback
STDMETHODIMP QWindowsCameraReader::OnReadSample(HRESULT hrStatus, DWORD dwStreamIndex,
                                                DWORD dwStreamFlags, LONGLONG llTimestamp,
                                                IMFSample *pSample)
{
    Q_UNUSED(hrStatus);
    Q_UNUSED(dwStreamIndex);

    QMutexLocker locker(&m_mutex);

    if ((dwStreamFlags & MF_SOURCE_READERF_ENDOFSTREAM) == MF_SOURCE_READERF_ENDOFSTREAM) {
        m_streaming = false;
        emit streamingStopped();
    } else {
        if (!m_streaming) {
            m_streaming = true;
            emit streamingStarted();
        }
        if (pSample) {

            if (m_recording) {

                if (m_firstFrame) {
                    m_timeOffset = llTimestamp;
                    m_firstFrame = false;
                    emit recordingStarted();
                }

                if (m_pauseChanging) {
                    // Recording time should not pass while paused.
                    if (m_paused)
                        m_pauseTime = llTimestamp;
                    else
                        m_timeOffset += llTimestamp - m_pauseTime;
                    m_pauseChanging = false;
                }

                // Send the video frame to be encoded.
                if (m_sinkWriter && !m_paused) {
                    pSample->SetSampleTime(llTimestamp - m_timeOffset);
                    m_sinkWriter->WriteSample(m_videoStreamIndex, pSample);
                    m_currentDuration = (llTimestamp - m_timeOffset) / 10000;
                }
            }

            // Send the video frame as a preview, if we have a surface.
            if (m_surface) {
                IMFMediaBuffer *mediaBuffer = nullptr;
                if (SUCCEEDED(pSample->ConvertToContiguousBuffer(&mediaBuffer))) {

                    DWORD bufLen = 0;
                    BYTE *buffer = nullptr;

                    if (SUCCEEDED(mediaBuffer->Lock(&buffer, nullptr, &bufLen))) {
                        auto bytes = QByteArray(reinterpret_cast<char*>(buffer), bufLen);

                        QVideoFrame frame(new QMemoryVideoBuffer(bytes, m_stride),
                                          QVideoFrameFormat(QSize(m_frameWidth, m_frameHeight), m_pixelFormat));

                        // WMF uses 100-nanosecond units, Qt uses microseconds
                        frame.setStartTime(llTimestamp * 0.1);

                        LONGLONG duration = -1;
                        if (SUCCEEDED(pSample->GetSampleDuration(&duration)))
                            frame.setEndTime((llTimestamp + duration) * 0.1);

                        m_surface->newVideoFrame(frame);
                        mediaBuffer->Unlock();
                    }
                    mediaBuffer->Release();
                }
            }
        }
        // request the next frame
        m_sourceReader->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM,
                                   0, nullptr, nullptr, nullptr, nullptr);
    }

    return S_OK;
}

STDMETHODIMP QWindowsCameraReader::OnFlush(DWORD)
{
    return S_OK;
}

STDMETHODIMP QWindowsCameraReader::OnEvent(DWORD, IMFMediaEvent*)
{
    return S_OK;
}

//from IMFSinkWriterCallback
STDMETHODIMP QWindowsCameraReader::OnFinalize(HRESULT)
{
    QMutexLocker locker(&m_mutex);
    if (m_sinkWriter) {
        m_sinkWriter->Release();
        m_sinkWriter = nullptr;
    }

    m_finalizeSemaphore.release();
    emit recordingStopped();
    return S_OK;
}

STDMETHODIMP QWindowsCameraReader::OnMarker(DWORD, LPVOID)
{
    return S_OK;
}

QT_END_NAMESPACE
