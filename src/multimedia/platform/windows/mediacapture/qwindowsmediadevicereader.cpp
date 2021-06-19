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

#include "qwindowsmediadevicereader_p.h"

#include "qwindowsmultimediautils_p.h"
#include <qvideosink.h>
#include <qmediadevices.h>
#include <qaudiodevice.h>
#include <private/qmemoryvideobuffer_p.h>
#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

enum { MEDIA_TYPE_INDEX_DEFAULT = 0xffffffff };

QWindowsMediaDeviceReader::QWindowsMediaDeviceReader(QObject *parent)
    : QObject(parent),
      m_finalizeSemaphore(1)
{
    m_durationTimer.setInterval(100);
    connect(&m_durationTimer, SIGNAL(timeout()), this, SLOT(updateDuration()));
}

QWindowsMediaDeviceReader::~QWindowsMediaDeviceReader()
{
    stopRecording();
    deactivate();
}

// Creates a video or audio media source specified by deviceId (symbolic link)
HRESULT QWindowsMediaDeviceReader::createSource(const QString &deviceId, bool video, IMFMediaSource **source)
{
    if (!source)
        return E_INVALIDARG;

    *source = nullptr;
    IMFAttributes *sourceAttributes = nullptr;

    HRESULT hr = MFCreateAttributes(&sourceAttributes, 2);
    if (SUCCEEDED(hr)) {

        hr = sourceAttributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
                                       video ? MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID
                                             : MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_AUDCAP_GUID);
        if (SUCCEEDED(hr)) {

            hr = sourceAttributes->SetString(video ? MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK
                                                   : MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_AUDCAP_ENDPOINT_ID,
                                             reinterpret_cast<LPCWSTR>(deviceId.utf16()));
            if (SUCCEEDED(hr)) {

                hr = MFCreateDeviceSource(sourceAttributes, source);
            }
        }
        sourceAttributes->Release();
    }

    return hr;
}

// Creates a source/reader aggregating two other sources (video/audio).
// If one of the sources is null the result will be video-only or audio-only.
HRESULT QWindowsMediaDeviceReader::createAggregateReader(IMFMediaSource *firstSource,
                                                         IMFMediaSource *secondSource,
                                                         IMFMediaSource **aggregateSource,
                                                         IMFSourceReader **sourceReader)
{
    if ((!firstSource && !secondSource) || !aggregateSource || !sourceReader)
        return E_INVALIDARG;

    *aggregateSource = nullptr;
    *sourceReader = nullptr;

    IMFCollection *sourceCollection = nullptr;

    HRESULT hr = MFCreateCollection(&sourceCollection);
    if (SUCCEEDED(hr)) {

        if (firstSource)
            sourceCollection->AddElement(firstSource);

        if (secondSource)
            sourceCollection->AddElement(secondSource);

        hr = MFCreateAggregateSource(sourceCollection, aggregateSource);
        if (SUCCEEDED(hr)) {

            IMFAttributes *readerAttributes = nullptr;

            hr = MFCreateAttributes(&readerAttributes, 1);
            if (SUCCEEDED(hr)) {

                // Set callback so OnReadSample() is called for each new video frame or audio sample.
                hr = readerAttributes->SetUnknown(MF_SOURCE_READER_ASYNC_CALLBACK,
                                                  static_cast<IMFSourceReaderCallback*>(this));
                if (SUCCEEDED(hr)) {

                    hr = MFCreateSourceReaderFromMediaSource(*aggregateSource, readerAttributes, sourceReader);
                }
                readerAttributes->Release();
            }
        }
        sourceCollection->Release();
    }
    return hr;
}

// Selects the requested resolution/frame rate (if specified),
// or chooses a high quality configuration otherwise.
DWORD QWindowsMediaDeviceReader::findMediaTypeIndex(const QCameraFormat &reqFormat)
{
    DWORD mediaIndex = MEDIA_TYPE_INDEX_DEFAULT;

    if (m_sourceReader && m_videoSource) {

        DWORD index = 0;
        IMFMediaType *mediaType = nullptr;

        UINT32 currArea = 0;
        float currFrameRate = 0.0f;

        while (SUCCEEDED(m_sourceReader->GetNativeMediaType(DWORD(MF_SOURCE_READER_FIRST_VIDEO_STREAM),
                                                            index, &mediaType))) {

            GUID subtype = GUID_NULL;
            if (SUCCEEDED(mediaType->GetGUID(MF_MT_SUBTYPE, &subtype))) {

                auto pixelFormat = QWindowsMultimediaUtils::pixelFormatFromMediaSubtype(subtype);
                if (pixelFormat != QVideoFrameFormat::Format_Invalid) {

                    UINT32 width, height;
                    if (SUCCEEDED(MFGetAttributeSize(mediaType, MF_MT_FRAME_SIZE, &width, &height))) {

                        UINT32 num, den;
                        if (SUCCEEDED(MFGetAttributeRatio(mediaType, MF_MT_FRAME_RATE, &num, &den))) {

                            UINT32 area = width * height;
                            float frameRate = float(num) / den;

                            if (!reqFormat.isNull()
                                    && reqFormat.resolution().width() == width
                                    && reqFormat.resolution().height() == height
                                    && qFuzzyCompare(reqFormat.maxFrameRate(), frameRate)
                                    && reqFormat.pixelFormat() == pixelFormat) {
                                mediaType->Release();
                                return index;
                            }

                            if ((currFrameRate < 29.9 && currFrameRate < frameRate) ||
                                    (currFrameRate == frameRate && currArea < area)) {
                                currArea = area;
                                currFrameRate = frameRate;
                                mediaIndex = index;
                            }
                        }
                    }
                }
            }
            mediaType->Release();
            ++index;
        }
    }

    return mediaIndex;
}


// Prepares the source video stream and gets some metadata.
HRESULT QWindowsMediaDeviceReader::prepareVideoStream(DWORD mediaTypeIndex)
{
    if (!m_sourceReader)
        return E_FAIL;

    if (!m_videoSource)
        return S_OK; // It may be audio-only

    HRESULT hr;

    if (mediaTypeIndex == MEDIA_TYPE_INDEX_DEFAULT) {
        hr = m_sourceReader->GetCurrentMediaType(DWORD(MF_SOURCE_READER_FIRST_VIDEO_STREAM),
                                                 &m_videoMediaType);
    } else {
        hr = m_sourceReader->GetNativeMediaType(DWORD(MF_SOURCE_READER_FIRST_VIDEO_STREAM),
                                                mediaTypeIndex, &m_videoMediaType);
        if (SUCCEEDED(hr))
            hr = m_sourceReader->SetCurrentMediaType(DWORD(MF_SOURCE_READER_FIRST_VIDEO_STREAM),
                                                     nullptr, m_videoMediaType);
    }

    if (SUCCEEDED(hr)) {

        GUID subtype = GUID_NULL;
        hr = m_videoMediaType->GetGUID(MF_MT_SUBTYPE, &subtype);
        if (SUCCEEDED(hr)) {

            m_pixelFormat = QWindowsMultimediaUtils::pixelFormatFromMediaSubtype(subtype);

            if (m_pixelFormat == QVideoFrameFormat::Format_Invalid) {
                hr = E_FAIL;
            } else {

                // get the frame dimensions
                hr = MFGetAttributeSize(m_videoMediaType, MF_MT_FRAME_SIZE, &m_frameWidth, &m_frameHeight);
                if (SUCCEEDED(hr)) {

                    // and the stride, which we need to convert the frame later
                    hr = MFGetStrideForBitmapInfoHeader(subtype.Data1, m_frameWidth, &m_stride);
                    if (SUCCEEDED(hr)) {

                        UINT32 frameRateNum, frameRateDen;
                        hr = MFGetAttributeRatio(m_videoMediaType, MF_MT_FRAME_RATE, &frameRateNum, &frameRateDen);
                        if (SUCCEEDED(hr)) {

                            m_frameRate = qreal(frameRateNum) / frameRateDen;

                            hr = m_sourceReader->SetStreamSelection(DWORD(MF_SOURCE_READER_FIRST_VIDEO_STREAM), TRUE);
                        }
                    }
                }
            }
        }
    }

    return hr;
}

// Prepares the source audio stream.
HRESULT QWindowsMediaDeviceReader::prepareAudioStream()
{
    if (!m_sourceReader)
        return E_FAIL;

    if (!m_audioSource)
        return S_OK; // It may be video-only

    HRESULT hr = m_sourceReader->GetCurrentMediaType(DWORD(MF_SOURCE_READER_FIRST_AUDIO_STREAM),
                                                     &m_audioMediaType);
    if (SUCCEEDED(hr)) {
        // Request audio samples in float format.
        hr = m_audioMediaType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_Float);
        if (SUCCEEDED(hr)) {
            hr = m_sourceReader->SetCurrentMediaType(DWORD(MF_SOURCE_READER_FIRST_AUDIO_STREAM),
                                                     nullptr, m_audioMediaType);
            if (SUCCEEDED(hr)) {
                hr = m_sourceReader->SetStreamSelection(DWORD(MF_SOURCE_READER_FIRST_AUDIO_STREAM), TRUE);
            }
        }
    }
    return hr;
}

// Retrieves the indexes for selected video/audio streams.
HRESULT QWindowsMediaDeviceReader::initSourceIndexes()
{
    if (!m_sourceReader)
        return E_FAIL;

    m_sourceVideoStreamIndex = MF_SOURCE_READER_INVALID_STREAM_INDEX;
    m_sourceAudioStreamIndex = MF_SOURCE_READER_INVALID_STREAM_INDEX;

    DWORD index = 0;
    BOOL selected = FALSE;

    while (m_sourceReader->GetStreamSelection(index, &selected) == S_OK) {
        if (selected) {
            IMFMediaType *mediaType = nullptr;
            if (SUCCEEDED(m_sourceReader->GetCurrentMediaType(index, &mediaType))) {
                GUID majorType = GUID_NULL;
                if (SUCCEEDED(mediaType->GetGUID(MF_MT_MAJOR_TYPE, &majorType))) {
                    if (majorType == MFMediaType_Video)
                        m_sourceVideoStreamIndex = index;
                    else if (majorType == MFMediaType_Audio)
                        m_sourceAudioStreamIndex = index;
                }
                mediaType->Release();
            }
        }
        ++index;
    }
    if ((m_videoSource && m_sourceVideoStreamIndex == MF_SOURCE_READER_INVALID_STREAM_INDEX) ||
            (m_audioSource && m_sourceAudioStreamIndex == MF_SOURCE_READER_INVALID_STREAM_INDEX))
        return E_FAIL;
    return S_OK;
}

// Activates the requested camera/microphone for streaming.
// One of the IDs may be empty for video-only/audio-only.
bool QWindowsMediaDeviceReader::activate(const QString &cameraId,
                                         const QCameraFormat &cameraFormat,
                                         const QString &microphoneId)
{
    QMutexLocker locker(&m_mutex);

    if (cameraId.isEmpty() && microphoneId.isEmpty())
        return false;

    releaseResources();

    m_active = false;
    m_streaming = false;

    if (!cameraId.isEmpty()) {
        if (!SUCCEEDED(createSource(cameraId, true, &m_videoSource))) {
            releaseResources();
            return false;
        }
    }

    if (!microphoneId.isEmpty()) {
        if (!SUCCEEDED(createSource(microphoneId, false, &m_audioSource))) {
            releaseResources();
            return false;
        }
    }

    if (!SUCCEEDED(createAggregateReader(m_videoSource, m_audioSource, &m_aggregateSource, &m_sourceReader))) {
        releaseResources();
        return false;
    }

    DWORD mediaTypeIndex = findMediaTypeIndex(cameraFormat);

    if (!SUCCEEDED(prepareVideoStream(mediaTypeIndex))) {
        releaseResources();
        return false;
    }

    if (!SUCCEEDED(prepareAudioStream())) {
        releaseResources();
        return false;
    }

    if (!SUCCEEDED(initSourceIndexes())) {
        releaseResources();
        return false;
    }

    updateSinkInputMediaTypes();

    // Request the first frame or audio sample.
    if (!SUCCEEDED(m_sourceReader->ReadSample(MF_SOURCE_READER_ANY_STREAM, 0, nullptr, nullptr, nullptr, nullptr))) {
        releaseResources();
        return false;
    }

    m_active = true;
    return true;
}

void QWindowsMediaDeviceReader::deactivate()
{
    stopStreaming();
    m_active = false;
    m_streaming = false;
}

void QWindowsMediaDeviceReader::stopStreaming()
{
    QMutexLocker locker(&m_mutex);
    releaseResources();
}

// Releases allocated streaming stuff.
void QWindowsMediaDeviceReader::releaseResources()
{
    if (m_videoMediaType) {
        m_videoMediaType->Release();
        m_videoMediaType = nullptr;
    }
    if (m_audioMediaType) {
        m_audioMediaType->Release();
        m_audioMediaType = nullptr;
    }
    if (m_sourceReader) {
        m_sourceReader->Release();
        m_sourceReader = nullptr;
    }
    if (m_aggregateSource) {
        m_aggregateSource->Release();
        m_aggregateSource = nullptr;
    }
    if (m_videoSource) {
        m_videoSource->Release();
        m_videoSource = nullptr;
    }
    if (m_audioSource) {
        m_audioSource->Release();
        m_audioSource = nullptr;
    }
}

HRESULT QWindowsMediaDeviceReader::createVideoMediaType(const GUID &format, UINT32 bitRate, UINT32 width,
                                                        UINT32 height, qreal frameRate, IMFMediaType **mediaType)
{
    if (!mediaType)
        return E_INVALIDARG;

    *mediaType = nullptr;
    IMFMediaType *targetMediaType = nullptr;

    if (SUCCEEDED(MFCreateMediaType(&targetMediaType))) {

        if (SUCCEEDED(targetMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video))) {

            if (SUCCEEDED(targetMediaType->SetGUID(MF_MT_SUBTYPE, format))) {

                if (SUCCEEDED(targetMediaType->SetUINT32(MF_MT_AVG_BITRATE, bitRate))) {

                    if (SUCCEEDED(MFSetAttributeSize(targetMediaType, MF_MT_FRAME_SIZE, width, height))) {

                        if (SUCCEEDED(MFSetAttributeRatio(targetMediaType, MF_MT_FRAME_RATE,
                                                          UINT32(frameRate * 1000), 1000))) {
                            UINT32 t1, t2;
                            if (SUCCEEDED(MFGetAttributeRatio(m_videoMediaType, MF_MT_PIXEL_ASPECT_RATIO, &t1, &t2))) {

                                if (SUCCEEDED(MFSetAttributeRatio(targetMediaType, MF_MT_PIXEL_ASPECT_RATIO, t1, t2))) {

                                    if (SUCCEEDED(m_videoMediaType->GetUINT32(MF_MT_INTERLACE_MODE, &t1))) {

                                        if (SUCCEEDED(targetMediaType->SetUINT32(MF_MT_INTERLACE_MODE, t1))) {

                                            *mediaType =  targetMediaType;
                                            return S_OK;
                                        }
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
    return E_FAIL;
}

HRESULT QWindowsMediaDeviceReader::createAudioMediaType(const GUID &format, UINT32 bitRate, IMFMediaType **mediaType)
{
    if (!mediaType)
        return E_INVALIDARG;

    *mediaType = nullptr;
    IMFMediaType *targetMediaType = nullptr;

    if (SUCCEEDED(MFCreateMediaType(&targetMediaType))) {

        if (SUCCEEDED(targetMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio))) {

            if (SUCCEEDED(targetMediaType->SetGUID(MF_MT_SUBTYPE, format))) {

                if (bitRate == 0 || SUCCEEDED(targetMediaType->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, bitRate / 8))) {

                    *mediaType =  targetMediaType;
                    return S_OK;
                }
            }
        }
        targetMediaType->Release();
    }
    return E_FAIL;
}

HRESULT QWindowsMediaDeviceReader::updateSinkInputMediaTypes()
{
    HRESULT hr = S_OK;
    if (m_sinkWriter) {
        if (m_videoSource && m_videoMediaType && m_sinkVideoStreamIndex != MF_SINK_WRITER_INVALID_STREAM_INDEX) {
            hr = m_sinkWriter->SetInputMediaType(m_sinkVideoStreamIndex, m_videoMediaType, nullptr);
        }
        if (SUCCEEDED(hr)) {
            if (m_audioSource && m_audioMediaType && m_sinkAudioStreamIndex != MF_SINK_WRITER_INVALID_STREAM_INDEX) {
                hr = m_sinkWriter->SetInputMediaType(m_sinkAudioStreamIndex, m_audioMediaType, nullptr);
            }
        }
    }
    return hr;
}

bool QWindowsMediaDeviceReader::startRecording(const QString &fileName, const GUID &container,
                                               const GUID &videoFormat, UINT32 videoBitRate, UINT32 width,
                                               UINT32 height, qreal frameRate, const GUID &audioFormat,
                                               UINT32 audioBitRate)
{
    QMutexLocker locker(&m_mutex);

    if (!m_active || m_recording || (videoFormat == GUID_NULL && audioFormat == GUID_NULL))
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

                    m_sinkVideoStreamIndex = MF_SINK_WRITER_INVALID_STREAM_INDEX;
                    m_sinkAudioStreamIndex = MF_SINK_WRITER_INVALID_STREAM_INDEX;

                    if (m_videoSource && videoFormat != GUID_NULL) {
                        IMFMediaType *targetMediaType = nullptr;

                        hr = createVideoMediaType(videoFormat, videoBitRate, width, height,
                                                  frameRate, &targetMediaType);
                        if (SUCCEEDED(hr)) {

                            hr = m_sinkWriter->AddStream(targetMediaType, &m_sinkVideoStreamIndex);
                            if (SUCCEEDED(hr)) {

                                hr = m_sinkWriter->SetInputMediaType(m_sinkVideoStreamIndex,
                                                                     m_videoMediaType, nullptr);
                            }
                            targetMediaType->Release();
                        }
                    }

                    if (SUCCEEDED(hr)) {

                        if (m_audioSource && audioFormat != GUID_NULL) {
                            IMFMediaType *targetMediaType = nullptr;

                            hr = createAudioMediaType(audioFormat, audioBitRate, &targetMediaType);
                            if (SUCCEEDED(hr)) {

                                hr = m_sinkWriter->AddStream(targetMediaType, &m_sinkAudioStreamIndex);
                                if (SUCCEEDED(hr)) {

                                    hr = m_sinkWriter->SetInputMediaType(m_sinkAudioStreamIndex,
                                                                         m_audioMediaType, nullptr);
                                }
                                targetMediaType->Release();
                            }
                        }

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
        writerAttributes->Release();
    }
    if (m_sinkWriter && !SUCCEEDED(hr)) {
        m_sinkWriter->Release();
        m_sinkWriter = nullptr;
    }
    return SUCCEEDED(hr);
}

void QWindowsMediaDeviceReader::stopRecording()
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

bool QWindowsMediaDeviceReader::pauseRecording()
{
    if (!m_recording || m_paused)
        return false;
    m_pauseTime = m_lastTimestamp;
    m_paused = true;
    m_pauseChanging = true;
    return true;
}

bool QWindowsMediaDeviceReader::resumeRecording()
{
    if (!m_recording || !m_paused)
        return false;
    m_paused = false;
    m_pauseChanging = true;
    return true;
}

//from IUnknown
STDMETHODIMP QWindowsMediaDeviceReader::QueryInterface(REFIID riid, LPVOID *ppvObject)
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

STDMETHODIMP_(ULONG) QWindowsMediaDeviceReader::AddRef(void)
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) QWindowsMediaDeviceReader::Release(void)
{
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (cRef == 0) {
        this->deleteLater();
    }
    return cRef;
}

UINT32 QWindowsMediaDeviceReader::frameWidth() const
{
    return m_frameWidth;
}

UINT32 QWindowsMediaDeviceReader::frameHeight() const
{
    return m_frameHeight;
}

qreal QWindowsMediaDeviceReader::frameRate() const
{
    return m_frameRate;
}

bool QWindowsMediaDeviceReader::isMuted() const
{
    return m_muted;
}

void QWindowsMediaDeviceReader::setMuted(bool muted)
{
    m_muted = muted;
}

qreal QWindowsMediaDeviceReader::volume() const
{
    return m_volume;
}

void QWindowsMediaDeviceReader::setVolume(qreal volume)
{
    m_volume = volume;
}

void QWindowsMediaDeviceReader::updateDuration()
{
    if (m_currentDuration >= 0 && m_lastDuration != m_currentDuration) {
        m_lastDuration = m_currentDuration;
        emit durationChanged(m_currentDuration);
    }
}

//from IMFSourceReaderCallback
STDMETHODIMP QWindowsMediaDeviceReader::OnReadSample(HRESULT hrStatus, DWORD dwStreamIndex,
                                                     DWORD dwStreamFlags, LONGLONG llTimestamp,
                                                     IMFSample *pSample)
{
    QMutexLocker locker(&m_mutex);

    if (FAILED(hrStatus)) {
        emit streamingError(int(hrStatus));
        return hrStatus;
    }

    m_lastTimestamp = llTimestamp;

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

                // Send the video frame or audio sample to be encoded.
                if (m_sinkWriter && !m_paused) {

                    pSample->SetSampleTime(llTimestamp - m_timeOffset);

                    if (dwStreamIndex == m_sourceVideoStreamIndex) {

                        m_sinkWriter->WriteSample(m_sinkVideoStreamIndex, pSample);

                    } else if (dwStreamIndex == m_sourceAudioStreamIndex) {

                        float volume = m_muted ? 0.0f : float(m_volume);

                        // Change the volume of the audio sample, if needed.
                        if (volume != 1.0f) {
                            IMFMediaBuffer *mediaBuffer = nullptr;
                            if (SUCCEEDED(pSample->ConvertToContiguousBuffer(&mediaBuffer))) {

                                DWORD bufLen = 0;
                                BYTE *buffer = nullptr;

                                if (SUCCEEDED(mediaBuffer->Lock(&buffer, nullptr, &bufLen))) {

                                    float *floatBuffer = reinterpret_cast<float*>(buffer);

                                    for (DWORD i = 0; i < bufLen/4; ++i)
                                        floatBuffer[i] *= volume;

                                    mediaBuffer->Unlock();
                                }
                                mediaBuffer->Release();
                            }
                        }

                        m_sinkWriter->WriteSample(m_sinkAudioStreamIndex, pSample);
                    }
                    m_currentDuration = (llTimestamp - m_timeOffset) / 10000;
                }
            }

            // Generate a new QVideoFrame from IMFSample.
            if (dwStreamIndex == m_sourceVideoStreamIndex) {
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

                        emit newVideoFrame(frame);

                        mediaBuffer->Unlock();
                    }
                    mediaBuffer->Release();
                }
            }
        }
        // request the next video frame or sound sample
        if (m_sourceReader)
            m_sourceReader->ReadSample(MF_SOURCE_READER_ANY_STREAM,
                                       0, nullptr, nullptr, nullptr, nullptr);
    }

    return S_OK;
}

STDMETHODIMP QWindowsMediaDeviceReader::OnFlush(DWORD)
{
    return S_OK;
}

STDMETHODIMP QWindowsMediaDeviceReader::OnEvent(DWORD, IMFMediaEvent*)
{
    return S_OK;
}

//from IMFSinkWriterCallback
STDMETHODIMP QWindowsMediaDeviceReader::OnFinalize(HRESULT)
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

STDMETHODIMP QWindowsMediaDeviceReader::OnMarker(DWORD, LPVOID)
{
    return S_OK;
}

QT_END_NAMESPACE
