// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindowsmediadevicereader_p.h"

#include <QtMultimedia/qaudiodevice.h>
#include <QtMultimedia/qmediadevices.h>
#include <QtMultimedia/qvideosink.h>
#include <QtMultimedia/private/qmemoryvideobuffer_p.h>
#include <QtMultimedia/private/qvideoframe_p.h>
#include <QtMultimedia/private/qwindowsmultimediautils_p.h>
#include <QtCore/qdebug.h>
#include <QtCore/private/qcomptr_p.h>

#include <cguid.h>
#include <mfidl.h>
#include <mmdeviceapi.h>

QT_BEGIN_NAMESPACE

enum { MEDIA_TYPE_INDEX_DEFAULT = 0xffffffff };

QWindowsMediaDeviceReader::QWindowsMediaDeviceReader(QObject *parent)
    : QObject(parent)
{
    m_durationTimer.setInterval(100);
    connect(&m_durationTimer, &QTimer::timeout, this, &QWindowsMediaDeviceReader::updateDuration);
}

QWindowsMediaDeviceReader::~QWindowsMediaDeviceReader()
{
    stopRecording();
    deactivate();
}

// Creates a video or audio media source specified by deviceId (symbolic link)
HRESULT QWindowsMediaDeviceReader::createSource(const QString &deviceId, bool video, ComPtr<IMFMediaSource> &source)
{
    source = nullptr;
    ComPtr<IMFAttributes> sourceAttributes;

    HRESULT hr = MFCreateAttributes(sourceAttributes.GetAddressOf(), 2);
    if (SUCCEEDED(hr)) {

        hr = sourceAttributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
                                       video ? MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID
                                             : MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_AUDCAP_GUID);
        if (SUCCEEDED(hr)) {

            hr = sourceAttributes->SetString(video ? MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK
                                                   : MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_AUDCAP_ENDPOINT_ID,
                                             reinterpret_cast<LPCWSTR>(deviceId.utf16()));
            if (SUCCEEDED(hr)) {

                hr = MFCreateDeviceSource(sourceAttributes.Get(), &source);
            }
        }
    }

    return hr;
}

// Creates a source/reader aggregating two other sources (video/audio).
// If one of the sources is null the result will be video-only or audio-only.
HRESULT QWindowsMediaDeviceReader::createAggregateReader(const ComPtr<IMFMediaSource> &firstSource,
                                                         const ComPtr<IMFMediaSource> &secondSource,
                                                         ComPtr<IMFMediaSource> &aggregateSource,
                                                         ComPtr<IMFSourceReader> &sourceReader)
{
    if (!firstSource && !secondSource)
        return E_INVALIDARG;

    aggregateSource = nullptr;
    sourceReader = nullptr;

    ComPtr<IMFCollection> sourceCollection;

    HRESULT hr = MFCreateCollection(sourceCollection.GetAddressOf());
    if (SUCCEEDED(hr)) {

        if (firstSource)
            sourceCollection->AddElement(firstSource.Get());

        if (secondSource)
            sourceCollection->AddElement(secondSource.Get());

        hr = MFCreateAggregateSource(sourceCollection.Get(), &aggregateSource);
        if (SUCCEEDED(hr)) {

            ComPtr<IMFAttributes> readerAttributes;

            hr = MFCreateAttributes(readerAttributes.GetAddressOf(), 1);
            if (SUCCEEDED(hr)) {

                // Set callback so OnReadSample() is called for each new video frame or audio sample.
                hr = readerAttributes->SetUnknown(MF_SOURCE_READER_ASYNC_CALLBACK,
                                                  static_cast<IMFSourceReaderCallback*>(this));
                if (SUCCEEDED(hr)) {

                    hr = MFCreateSourceReaderFromMediaSource(aggregateSource.Get(), readerAttributes.Get(), &sourceReader);
                }
            }
        }
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
        ComPtr<IMFMediaType> mediaType;

        UINT32 currArea = 0;
        float currFrameRate = 0.0f;

        while (SUCCEEDED(m_sourceReader->GetNativeMediaType(DWORD(MF_SOURCE_READER_FIRST_VIDEO_STREAM),
                                                            index, mediaType.ReleaseAndGetAddressOf()))) {

            GUID subtype = GUID_NULL;
            if (SUCCEEDED(mediaType->GetGUID(MF_MT_SUBTYPE, &subtype))) {

                auto pixelFormat = QWindowsMultimediaUtils::pixelFormatFromMediaSubtype(subtype);
                if (pixelFormat != QVideoFrameFormat::Format_Invalid) {

                    UINT32 width, height;
                    if (SUCCEEDED(MFGetAttributeSize(mediaType.Get(), MF_MT_FRAME_SIZE, &width, &height))) {

                        UINT32 num, den;
                        if (SUCCEEDED(MFGetAttributeRatio(mediaType.Get(), MF_MT_FRAME_RATE, &num, &den))) {

                            UINT32 area = width * height;
                            float frameRate = float(num) / den;

                            if (!reqFormat.isNull()
                                    && UINT32(reqFormat.resolution().width()) == width
                                    && UINT32(reqFormat.resolution().height()) == height
                                    && QtPrivate::fuzzyCompare(reqFormat.maxFrameRate(), frameRate)
                                    && reqFormat.pixelFormat() == pixelFormat) {
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
                                                     nullptr, m_videoMediaType.Get());
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
                hr = MFGetAttributeSize(m_videoMediaType.Get(), MF_MT_FRAME_SIZE, &m_frameWidth, &m_frameHeight);
                if (SUCCEEDED(hr)) {

                    // and the stride, which we need to convert the frame later
                    hr = MFGetStrideForBitmapInfoHeader(subtype.Data1, m_frameWidth, &m_stride);
                    if (SUCCEEDED(hr)) {
                        m_stride = qAbs(m_stride);
                        UINT32 frameRateNum, frameRateDen;
                        hr = MFGetAttributeRatio(m_videoMediaType.Get(), MF_MT_FRAME_RATE, &frameRateNum, &frameRateDen);
                        if (SUCCEEDED(hr)) {

                            m_frameRate = qreal(frameRateNum) / frameRateDen;

                            hr = m_sourceReader->SetStreamSelection(DWORD(MF_SOURCE_READER_FIRST_VIDEO_STREAM), TRUE);

                            UINT32 nominalRange = 0;

                            if (SUCCEEDED(m_videoMediaType->GetUINT32(MF_MT_VIDEO_NOMINAL_RANGE, &nominalRange)))
                                m_colorRange = QWindowsMultimediaUtils::colorRangeFromNominalRange(nominalRange);

                            UINT32 yuvMatrix = 0;

                            if (SUCCEEDED(m_videoMediaType->GetUINT32(MF_MT_YUV_MATRIX, &yuvMatrix))) {
                                m_colorSpace = QWindowsMultimediaUtils::colorSpaceFromMatrix(yuvMatrix);
                            }
                        }
                    }
                }
            }
        }
    }

    return hr;
}

HRESULT QWindowsMediaDeviceReader::initAudioType(const ComPtr<IMFMediaType> &mediaType, UINT32 channels, UINT32 samplesPerSec, bool flt)
{
    if (!mediaType)
        return E_INVALIDARG;

    HRESULT hr = mediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
    if (SUCCEEDED(hr)) {
        hr = mediaType->SetGUID(MF_MT_SUBTYPE, flt ? MFAudioFormat_Float : MFAudioFormat_PCM);
        if (SUCCEEDED(hr)) {
            hr = mediaType->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, channels);
            if (SUCCEEDED(hr)) {
                hr = mediaType->SetUINT32(MF_MT_AUDIO_CHANNEL_MASK,
                                          (channels == 1) ? SPEAKER_FRONT_CENTER  : (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT ));
                if (SUCCEEDED(hr)) {
                    hr = mediaType->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, samplesPerSec);
                    if (SUCCEEDED(hr)) {
                        UINT32 bitsPerSample = flt ? 32 : 16;
                        UINT32 bytesPerFrame = channels * bitsPerSample/8;
                        hr = mediaType->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, bitsPerSample);
                        if (SUCCEEDED(hr)) {
                            hr = mediaType->SetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, bytesPerFrame);
                            if (SUCCEEDED(hr)) {
                                hr = mediaType->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, bytesPerFrame * samplesPerSec);
                            }
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
        hr = initAudioType(m_audioMediaType, 2, 48000, true);
        if (SUCCEEDED(hr)) {
            hr = m_sourceReader->SetCurrentMediaType(DWORD(MF_SOURCE_READER_FIRST_AUDIO_STREAM),
                                                     nullptr, m_audioMediaType.Get());
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

    ComPtr<IMFMediaType> mediaType;
    while (m_sourceReader->GetStreamSelection(index, &selected) == S_OK) {
        if (selected) {
            if (SUCCEEDED(m_sourceReader->GetCurrentMediaType(index, mediaType.ReleaseAndGetAddressOf()))) {
                GUID majorType = GUID_NULL;
                if (SUCCEEDED(mediaType->GetGUID(MF_MT_MAJOR_TYPE, &majorType))) {
                    if (majorType == MFMediaType_Video)
                        m_sourceVideoStreamIndex = index;
                    else if (majorType == MFMediaType_Audio)
                        m_sourceAudioStreamIndex = index;
                }
            }
        }
        ++index;
    }
    if ((m_videoSource && m_sourceVideoStreamIndex == MF_SOURCE_READER_INVALID_STREAM_INDEX) ||
            (m_audioSource && m_sourceAudioStreamIndex == MF_SOURCE_READER_INVALID_STREAM_INDEX))
        return E_FAIL;
    return S_OK;
}

bool QWindowsMediaDeviceReader::setAudioOutput(const QString &audioOutputId)
{
    QMutexLocker locker(&m_mutex);

    stopMonitoring();

    m_audioOutputId = audioOutputId;

    if (!m_active || m_audioOutputId.isEmpty())
        return true;

    HRESULT hr = startMonitoring();

    return SUCCEEDED(hr);
}

HRESULT QWindowsMediaDeviceReader::startMonitoring()
{
    if (m_audioOutputId.isEmpty())
        return E_FAIL;

    ComPtr<IMFAttributes> sinkAttributes;

    HRESULT hr = MFCreateAttributes(sinkAttributes.GetAddressOf(), 1);
    if (SUCCEEDED(hr)) {

        hr = sinkAttributes->SetString(MF_AUDIO_RENDERER_ATTRIBUTE_ENDPOINT_ID,
                                       reinterpret_cast<LPCWSTR>(m_audioOutputId.utf16()));
        if (SUCCEEDED(hr)) {

            ComPtr<IMFMediaSink> mediaSink;
            hr = MFCreateAudioRenderer(sinkAttributes.Get(), mediaSink.GetAddressOf());
            if (SUCCEEDED(hr)) {

                ComPtr<IMFStreamSink> streamSink;
                hr = mediaSink->GetStreamSinkByIndex(0, streamSink.GetAddressOf());
                if (SUCCEEDED(hr)) {

                    ComPtr<IMFMediaTypeHandler> typeHandler;
                    hr = streamSink->GetMediaTypeHandler(typeHandler.GetAddressOf());
                    if (SUCCEEDED(hr)) {

                        hr = typeHandler->IsMediaTypeSupported(m_audioMediaType.Get(), nullptr);
                        if (SUCCEEDED(hr)) {

                            hr = typeHandler->SetCurrentMediaType(m_audioMediaType.Get());
                            if (SUCCEEDED(hr)) {

                                ComPtr<IMFAttributes> writerAttributes;

                                HRESULT hr = MFCreateAttributes(writerAttributes.GetAddressOf(), 1);
                                if (SUCCEEDED(hr)) {

                                    hr = writerAttributes->SetUINT32(MF_SINK_WRITER_DISABLE_THROTTLING, TRUE);
                                    if (SUCCEEDED(hr)) {

                                        ComPtr<IMFSinkWriter> sinkWriter;
                                        hr = MFCreateSinkWriterFromMediaSink(mediaSink.Get(), writerAttributes.Get(), sinkWriter.GetAddressOf());
                                        if (SUCCEEDED(hr)) {

                                            hr = sinkWriter->SetInputMediaType(0, m_audioMediaType.Get(), nullptr);
                                            if (SUCCEEDED(hr)) {

                                                ComPtr<IMFSimpleAudioVolume> audioVolume;

                                                if (SUCCEEDED(MFGetService(
                                                            mediaSink.Get(), MR_POLICY_VOLUME_SERVICE,
                                                            IID_PPV_ARGS(&audioVolume)))) {
                                                    audioVolume->SetMasterVolume(float(m_outputVolume));
                                                    audioVolume->SetMute(m_outputMuted);
                                                }

                                                hr = sinkWriter->BeginWriting();
                                                if (SUCCEEDED(hr)) {
                                                    m_monitorSink = mediaSink;
                                                    m_monitorWriter = sinkWriter;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return hr;
}

void QWindowsMediaDeviceReader::stopMonitoring()
{
    m_monitorWriter = nullptr;
    if (m_monitorSink) {
        m_monitorSink->Shutdown();
        m_monitorSink = nullptr;
    }
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

    stopMonitoring();
    releaseResources();

    m_active = false;
    m_streaming = false;

    if (!cameraId.isEmpty()) {
        if (!SUCCEEDED(createSource(cameraId, true, m_videoSource))) {
            releaseResources();
            return false;
        }
    }

    if (!microphoneId.isEmpty()) {
        if (!SUCCEEDED(createSource(microphoneId, false, m_audioSource))) {
            releaseResources();
            return false;
        }
    }

    if (!SUCCEEDED(createAggregateReader(m_videoSource, m_audioSource, m_aggregateSource, m_sourceReader))) {
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
    startMonitoring();

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
    stopMonitoring();
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
    m_videoMediaType = nullptr;
    m_audioMediaType = nullptr;
    m_sourceReader = nullptr;
    m_aggregateSource = nullptr;
    m_videoSource = nullptr;
    m_audioSource = nullptr;
}

HRESULT QWindowsMediaDeviceReader::createVideoMediaType(const GUID &format, UINT32 bitRate, UINT32 width,
                                                        UINT32 height, qreal frameRate, ComPtr<IMFMediaType> &mediaType)
{
    mediaType = nullptr;
    ComPtr<IMFMediaType> targetMediaType;

    if (SUCCEEDED(MFCreateMediaType(targetMediaType.GetAddressOf()))) {

        if (SUCCEEDED(targetMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video))) {

            if (SUCCEEDED(targetMediaType->SetGUID(MF_MT_SUBTYPE, format))) {

                if (SUCCEEDED(targetMediaType->SetUINT32(MF_MT_AVG_BITRATE, bitRate))) {

                    if (SUCCEEDED(MFSetAttributeSize(targetMediaType.Get(), MF_MT_FRAME_SIZE, width, height))) {

                        if (SUCCEEDED(MFSetAttributeRatio(targetMediaType.Get(), MF_MT_FRAME_RATE,
                                                          UINT32(frameRate * 1000), 1000))) {
                            UINT32 t1, t2;
                            if (SUCCEEDED(MFGetAttributeRatio(m_videoMediaType.Get(), MF_MT_PIXEL_ASPECT_RATIO, &t1, &t2))) {

                                if (SUCCEEDED(MFSetAttributeRatio(targetMediaType.Get(), MF_MT_PIXEL_ASPECT_RATIO, t1, t2))) {

                                    if (SUCCEEDED(m_videoMediaType->GetUINT32(MF_MT_INTERLACE_MODE, &t1))) {

                                        if (SUCCEEDED(targetMediaType->SetUINT32(MF_MT_INTERLACE_MODE, t1))) {

                                            mediaType = targetMediaType;
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
    }
    return E_FAIL;
}

HRESULT QWindowsMediaDeviceReader::createAudioMediaType(const GUID &format, UINT32 bitRate, ComPtr<IMFMediaType> &mediaType)
{
    mediaType = nullptr;
    ComPtr<IMFMediaType> targetMediaType;

    if (SUCCEEDED(MFCreateMediaType(targetMediaType.GetAddressOf()))) {

        if (SUCCEEDED(targetMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio))) {

            if (SUCCEEDED(targetMediaType->SetGUID(MF_MT_SUBTYPE, format))) {

                if (bitRate == 0 || SUCCEEDED(targetMediaType->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, bitRate / 8))) {

                    mediaType = targetMediaType;
                    return S_OK;
                }
            }
        }
    }
    return E_FAIL;
}

HRESULT QWindowsMediaDeviceReader::updateSinkInputMediaTypes()
{
    HRESULT hr = S_OK;
    if (m_sinkWriter) {
        if (m_videoSource && m_videoMediaType && m_sinkVideoStreamIndex != MF_SINK_WRITER_INVALID_STREAM_INDEX) {
            hr = m_sinkWriter->SetInputMediaType(m_sinkVideoStreamIndex, m_videoMediaType.Get(), nullptr);
        }
        if (SUCCEEDED(hr)) {
            if (m_audioSource && m_audioMediaType && m_sinkAudioStreamIndex != MF_SINK_WRITER_INVALID_STREAM_INDEX) {
                hr = m_sinkWriter->SetInputMediaType(m_sinkAudioStreamIndex, m_audioMediaType.Get(), nullptr);
            }
        }
    }
    return hr;
}

QMediaRecorder::Error QWindowsMediaDeviceReader::startRecording(
        const QString &fileName, const GUID &container, const GUID &videoFormat, UINT32 videoBitRate,
        UINT32 width, UINT32 height, qreal frameRate, const GUID &audioFormat, UINT32 audioBitRate)
{
    QMutexLocker locker(&m_mutex);

    if (!m_active || m_recording || (videoFormat == GUID_NULL && audioFormat == GUID_NULL))
        return QMediaRecorder::ResourceError;

    ComPtr<IMFAttributes> writerAttributes;

    HRESULT hr = MFCreateAttributes(writerAttributes.GetAddressOf(), 2);
    if (FAILED(hr))
        return QMediaRecorder::ResourceError;

    // Set callback so OnFinalize() is called after video is saved.
    hr = writerAttributes->SetUnknown(MF_SINK_WRITER_ASYNC_CALLBACK,
                                      static_cast<IMFSinkWriterCallback*>(this));
    if (FAILED(hr))
        return QMediaRecorder::ResourceError;

    hr = writerAttributes->SetGUID(MF_TRANSCODE_CONTAINERTYPE, container);
    if (FAILED(hr))
        return QMediaRecorder::ResourceError;

    ComPtr<IMFSinkWriter> sinkWriter;
    hr = MFCreateSinkWriterFromURL(reinterpret_cast<LPCWSTR>(fileName.utf16()),
                                   nullptr, writerAttributes.Get(), sinkWriter.GetAddressOf());
    if (FAILED(hr))
        return QMediaRecorder::LocationNotWritable;

    m_sinkVideoStreamIndex = MF_SINK_WRITER_INVALID_STREAM_INDEX;
    m_sinkAudioStreamIndex = MF_SINK_WRITER_INVALID_STREAM_INDEX;

    if (m_videoSource && videoFormat != GUID_NULL) {
        ComPtr<IMFMediaType> targetMediaType;

        hr = createVideoMediaType(videoFormat, videoBitRate, width, height, frameRate, targetMediaType);
        if (SUCCEEDED(hr)) {

            hr = sinkWriter->AddStream(targetMediaType.Get(), &m_sinkVideoStreamIndex);
            if (SUCCEEDED(hr)) {

                hr = sinkWriter->SetInputMediaType(m_sinkVideoStreamIndex, m_videoMediaType.Get(), nullptr);
            }
        }
    }

    if (SUCCEEDED(hr)) {
        if (m_audioSource && audioFormat != GUID_NULL) {
            ComPtr<IMFMediaType> targetMediaType;

            hr = createAudioMediaType(audioFormat, audioBitRate, targetMediaType);
            if (SUCCEEDED(hr)) {

                hr = sinkWriter->AddStream(targetMediaType.Get(), &m_sinkAudioStreamIndex);
                if (SUCCEEDED(hr)) {

                    hr = sinkWriter->SetInputMediaType(m_sinkAudioStreamIndex, m_audioMediaType.Get(), nullptr);
                }
            }
        }
    }

    if (FAILED(hr))
        return QMediaRecorder::FormatError;

    hr = sinkWriter->BeginWriting();
    if (FAILED(hr))
        return QMediaRecorder::ResourceError;

    m_sinkWriter = std::move(sinkWriter);
    m_lastDuration = -1;
    m_currentDuration = 0;
    updateDuration();
    m_durationTimer.start();
    m_recording = true;
    m_firstFrame = true;
    m_paused = false;
    m_pauseChanging = false;

    return QMediaRecorder::NoError;
}

void QWindowsMediaDeviceReader::stopRecording()
{
    QMutexLocker locker(&m_mutex);

    if (m_sinkWriter && m_recording) {

        HRESULT hr = m_sinkWriter->Finalize();

        if (SUCCEEDED(hr)) {
            m_hasFinalized.wait(&m_mutex);
        } else {
            m_sinkWriter = nullptr;

            QMetaObject::invokeMethod(this, [this, hr] {
                emit recordingError(hr);
            }, Qt::QueuedConnection);
        }
    }

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

void QWindowsMediaDeviceReader::setInputMuted(bool muted)
{
    m_inputMuted = muted;
}

void QWindowsMediaDeviceReader::setInputVolume(qreal volume)
{
    m_inputVolume = qBound(0.0, volume, 1.0);
}

void QWindowsMediaDeviceReader::setOutputMuted(bool muted)
{
    QMutexLocker locker(&m_mutex);

    m_outputMuted = muted;

    if (m_active && m_monitorSink) {
        ComPtr<IMFSimpleAudioVolume> audioVolume;
        if (SUCCEEDED(MFGetService(m_monitorSink.Get(), MR_POLICY_VOLUME_SERVICE,
                                   IID_PPV_ARGS(&audioVolume)))) {
            audioVolume->SetMute(m_outputMuted);
        }
    }
}

void QWindowsMediaDeviceReader::setOutputVolume(qreal volume)
{
    QMutexLocker locker(&m_mutex);

    m_outputVolume = qBound(0.0, volume, 1.0);

    if (m_active && m_monitorSink) {
        ComPtr<IMFSimpleAudioVolume> audioVolume;
        if (SUCCEEDED(MFGetService(m_monitorSink.Get(), MR_POLICY_VOLUME_SERVICE,
                                   IID_PPV_ARGS(&audioVolume)))) {
            audioVolume->SetMasterVolume(float(m_outputVolume));
        }
    }
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
        emit streamingError(hrStatus);
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

            if (m_monitorWriter && dwStreamIndex == m_sourceAudioStreamIndex)
                m_monitorWriter->WriteSample(0, pSample);

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

                        float volume = m_inputMuted ? 0.0f : float(m_inputVolume);

                        // Change the volume of the audio sample, if needed.
                        if (volume != 1.0f) {
                            ComPtr<IMFMediaBuffer> mediaBuffer;
                            if (SUCCEEDED(pSample->ConvertToContiguousBuffer(mediaBuffer.GetAddressOf()))) {

                                DWORD bufLen = 0;
                                BYTE *buffer = nullptr;

                                if (SUCCEEDED(mediaBuffer->Lock(&buffer, nullptr, &bufLen))) {

                                    float *floatBuffer = reinterpret_cast<float*>(buffer);

                                    for (DWORD i = 0; i < bufLen/4; ++i)
                                        floatBuffer[i] *= volume;

                                    mediaBuffer->Unlock();
                                }
                            }
                        }

                        m_sinkWriter->WriteSample(m_sinkAudioStreamIndex, pSample);
                    }
                    m_currentDuration = (llTimestamp - m_timeOffset) / 10000;
                }
            }

            // Generate a new QVideoFrame from IMFSample.
            if (dwStreamIndex == m_sourceVideoStreamIndex) {
                ComPtr<IMFMediaBuffer> mediaBuffer;
                if (SUCCEEDED(pSample->ConvertToContiguousBuffer(mediaBuffer.GetAddressOf()))) {

                    DWORD bufLen = 0;
                    BYTE *buffer = nullptr;

                    if (SUCCEEDED(mediaBuffer->Lock(&buffer, nullptr, &bufLen))) {
                        auto bytes = QByteArray(reinterpret_cast<char*>(buffer), bufLen);
                        QVideoFrameFormat format(QSize(m_frameWidth, m_frameHeight), m_pixelFormat);
                        format.setColorRange(m_colorRange);
                        format.setColorSpace(m_colorSpace);

                        QVideoFrame frame = QVideoFramePrivate::createFrame(
                                std::make_unique<QMemoryVideoBuffer>(std::move(bytes), m_stride),
                                std::move(format));

                        // WMF uses 100-nanosecond units, Qt uses microseconds
                        frame.setStartTime(llTimestamp * 0.1);

                        LONGLONG duration = -1;
                        if (SUCCEEDED(pSample->GetSampleDuration(&duration)))
                            frame.setEndTime((llTimestamp + duration) * 0.1);

                        emit videoFrameChanged(frame);

                        mediaBuffer->Unlock();
                    }
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
    m_sinkWriter = nullptr;
    emit recordingStopped();
    m_hasFinalized.notify_one();
    return S_OK;
}

STDMETHODIMP QWindowsMediaDeviceReader::OnMarker(DWORD, LPVOID)
{
    return S_OK;
}

QT_END_NAMESPACE

#include "moc_qwindowsmediadevicereader_p.cpp"
