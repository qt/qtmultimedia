// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSMEDIADEVICEREADER_H
#define QWINDOWSMEDIADEVICEREADER_H

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

#include <QtMultimedia/qcameradevice.h>
#include <QtMultimedia/qmediarecorder.h>
#include <QtMultimedia/qvideoframe.h>
#include <QtCore/qmutex.h>
#include <QtCore/qobject.h>
#include <QtCore/qtimer.h>
#include <QtCore/qwaitcondition.h>

#include <QtCore/private/qcomobject_p.h>
#include <QtCore/private/qcomptr_p.h>

#include <mfapi.h>
#include <mferror.h>
#include <mfidl.h>
#include <mfreadwrite.h>

QT_BEGIN_NAMESPACE

class QVideoSink;

class QWindowsMediaDeviceReader
    : public QObject,
      public QComObjectWithDeleteLater<QWindowsMediaDeviceReader, IMFSourceReaderCallback,
                                       IMFSinkWriterCallback>
{
    Q_OBJECT
public:
    explicit QWindowsMediaDeviceReader(QObject *parent = nullptr);
    ~QWindowsMediaDeviceReader();

    //from IMFSourceReaderCallback
    STDMETHODIMP OnReadSample(HRESULT hrStatus, DWORD dwStreamIndex,
                              DWORD dwStreamFlags, LONGLONG llTimestamp, IMFSample *pSample) override;
    STDMETHODIMP OnFlush(DWORD dwStreamIndex) override;
    STDMETHODIMP OnEvent(DWORD dwStreamIndex, IMFMediaEvent *pEvent) override;

    //from IMFSinkWriterCallback
    STDMETHODIMP OnFinalize(HRESULT hrStatus) override;
    STDMETHODIMP OnMarker(DWORD dwStreamIndex, LPVOID pvContext) override;

    bool activate(const QString &cameraId,
                  const QCameraFormat &cameraFormat,
                  const QString &microphoneId);
    void deactivate();

    QMediaRecorder::Error startRecording(const QString &fileName, const GUID &container,
                           const GUID &videoFormat, UINT32 videoBitRate, UINT32 width,
                           UINT32 height, qreal frameRate, const GUID &audioFormat,
                           UINT32 audioBitRate);
    void stopRecording();
    bool pauseRecording();
    bool resumeRecording();

    UINT32 frameWidth() const;
    UINT32 frameHeight() const;
    qreal frameRate() const;
    void setInputMuted(bool muted);
    void setInputVolume(qreal volume);
    void setOutputMuted(bool muted);
    void setOutputVolume(qreal volume);
    bool setAudioOutput(const QString &audioOutputId);

Q_SIGNALS:
    void streamingStarted();
    void streamingStopped();
    void streamingError(HRESULT errorCode);
    void recordingStarted();
    void recordingStopped();
    void recordingError(HRESULT errorCode);
    void durationChanged(qint64 duration);
    void videoFrameChanged(const QVideoFrame &frame);

private slots:
    void updateDuration();

private:
    HRESULT createSource(const QString &deviceId, bool video, ComPtr<IMFMediaSource> &source);
    HRESULT createAggregateReader(const ComPtr<IMFMediaSource> &firstSource,
                                  const ComPtr<IMFMediaSource> &secondSource,
                                  ComPtr<IMFMediaSource> &aggregateSource,
                                  ComPtr<IMFSourceReader> &sourceReader);
    HRESULT createVideoMediaType(const GUID &format, UINT32 bitRate, UINT32 width, UINT32 height,
                                 qreal frameRate, ComPtr<IMFMediaType> &mediaType);
    HRESULT createAudioMediaType(const GUID &format, UINT32 bitRate, ComPtr<IMFMediaType> &mediaType);
    HRESULT initAudioType(const ComPtr<IMFMediaType> &mediaType, UINT32 channels, UINT32 samplesPerSec, bool flt);
    HRESULT prepareVideoStream(DWORD mediaTypeIndex);
    HRESULT prepareAudioStream();
    HRESULT initSourceIndexes();
    HRESULT updateSinkInputMediaTypes();
    HRESULT startMonitoring();
    void stopMonitoring();
    void releaseResources();
    void stopStreaming();
    DWORD findMediaTypeIndex(const QCameraFormat &reqFormat);

    QMutex             m_mutex;
    QWaitCondition     m_hasFinalized;
    ComPtr<IMFMediaSource>  m_videoSource;
    ComPtr<IMFMediaType>    m_videoMediaType;
    ComPtr<IMFMediaSource>  m_audioSource;
    ComPtr<IMFMediaType>    m_audioMediaType;
    ComPtr<IMFMediaSource>  m_aggregateSource;
    ComPtr<IMFSourceReader> m_sourceReader;
    ComPtr<IMFSinkWriter>   m_sinkWriter;
    ComPtr<IMFMediaSink>    m_monitorSink;
    ComPtr<IMFSinkWriter>   m_monitorWriter;
    QString            m_audioOutputId;
    DWORD              m_sourceVideoStreamIndex = MF_SOURCE_READER_INVALID_STREAM_INDEX;
    DWORD              m_sourceAudioStreamIndex = MF_SOURCE_READER_INVALID_STREAM_INDEX;
    DWORD              m_sinkVideoStreamIndex = MF_SINK_WRITER_INVALID_STREAM_INDEX;
    DWORD              m_sinkAudioStreamIndex = MF_SINK_WRITER_INVALID_STREAM_INDEX;
    UINT32             m_frameWidth = 0;
    UINT32             m_frameHeight = 0;
    qreal              m_frameRate = 0.0;
    LONG               m_stride = 0;
    bool               m_active = false;
    bool               m_streaming = false;
    bool               m_recording = false;
    bool               m_firstFrame = false;
    bool               m_paused = false;
    bool               m_pauseChanging = false;
    bool               m_inputMuted = false;
    bool               m_outputMuted = false;
    qreal              m_inputVolume = 1.0;
    qreal              m_outputVolume = 1.0;
    QVideoFrameFormat::PixelFormat m_pixelFormat = QVideoFrameFormat::Format_Invalid;
    LONGLONG           m_timeOffset = 0;
    LONGLONG           m_pauseTime = 0;
    LONGLONG           m_lastTimestamp = 0;
    QTimer             m_durationTimer;
    qint64             m_currentDuration = -1;
    qint64             m_lastDuration = -1;
    QVideoFrameFormat::ColorRange m_colorRange = QVideoFrameFormat::ColorRange_Unknown;
    QVideoFrameFormat::ColorSpace m_colorSpace = QVideoFrameFormat::ColorSpace_Undefined;
};

QT_END_NAMESPACE

#endif // QWINDOWSMEDIADEVICEREADER_H
