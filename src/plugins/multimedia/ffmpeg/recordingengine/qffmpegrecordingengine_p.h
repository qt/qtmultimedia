// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QFFMPEGENCODER_P_H
#define QFFMPEGENCODER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtFFmpegMediaPluginImpl/private/qffmpegthread_p.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpegencodingformatcontext_p.h>

#include <QtMultimedia/private/qplatformmediarecorder_p.h>
#include <qmediarecorder.h>

QT_BEGIN_NAMESPACE

class QAudioBuffer;
class QAudioFormat;
class QFFmpegAudioInput;
class QPlatformAudioBufferInput;
class QAudioBufferSource;
class QPlatformVideoSource;
class QVideoFrame;

namespace QFFmpeg
{

class RecordingEngine;
class Muxer;
class AudioEncoder;
class VideoEncoder;
class VideoFrameEncoder;
class EncodingInitializer;

class RecordingEngine : public QObject
{
    Q_OBJECT
public:
    RecordingEngine(const QMediaEncoderSettings &settings, std::unique_ptr<EncodingFormatContext> context);
    ~RecordingEngine() override;

    /** Initializes the recording engine immediately or
     *  postpones it if no source formats provided.
     *  Returns true if no session errors have occurred during the immediate run or
     *  the engine is to be initialized postponly.
     *  If any session error has occurred, it emits the signal sessionError and returns false.
     */
    bool initialize(const std::vector<QAudioBufferSource *> &audioSources,
                    const std::vector<QPlatformVideoSource *> &videoSources);
    void finalize();

    void setPaused(bool p);

    void setAutoStop(bool autoStop);

    bool autoStop() const { return m_autoStop; }

    void setMetaData(const QMediaMetaData &metaData);
    AVFormatContext *avFormatContext() { return m_formatContext->avFormatContext(); }
    Muxer *getMuxer() { return m_muxer.get(); }

    bool isEndOfSourceStreams() const;

public Q_SLOTS:
    void newTimeStamp(qint64 time);

Q_SIGNALS:
    void durationChanged(qint64 duration);
    void sessionError(QMediaRecorder::Error code, const QString &description);
    void streamInitializationError(QMediaRecorder::Error code, const QString &description);
    void finalizationDone();
    void autoStopped();

private:
    // Normal states transition, Stop is called upon Encoding,
    // header, content, and trailer are written:
    // None -> FormatsInitializing -> EncodersInitializing -> Encoding -> Finalizing
    //
    // Stop is called upon FormatsInitializing, nothing is written to the output:
    // None -> FormatsInitializing -> Finalizing
    //
    // Stop is called upon EncodersInitializing, nothing is written to the output:
    // None -> FormatsInitializing -> EncodersInitializing -> Finalizing
    enum class State {
        None,
        FormatsInitializing,
        EncodersInitializing,
        Encoding, // header written
        Finalizing
    };

    class EncodingFinalizer : public QThread
    {
    public:
        EncodingFinalizer(RecordingEngine &recordingEngine, bool writeTrailer);

        void run() override;

    private:
        RecordingEngine &m_recordingEngine;
        bool m_writeTrailer;
    };

    friend class EncodingInitializer;
    void addAudioInput(QFFmpegAudioInput *input);
    void addAudioBufferInput(QPlatformAudioBufferInput *input, const QAudioBuffer &firstBuffer);
    AudioEncoder *createAudioEncoder(const QAudioFormat &format);

    void addVideoSource(QPlatformVideoSource *source, const QVideoFrame &firstFrame);
    void handleSourceEndOfStream();
    void handleEncoderInitialization();

    bool startEncoders();

    size_t encodersCount() const { return m_audioEncoders.size() + m_videoEncoders.size(); }

    void stopAndDeleteThreads();

    template <typename F, typename... Args>
    void forEachEncoder(F &&f, Args &&...args);

    template <typename F>
    bool allOfEncoders(F &&f) const;

private:
    QMediaEncoderSettings m_settings;
    QMediaMetaData m_metaData;
    std::unique_ptr<EncodingFormatContext> m_formatContext;
    ConsumerThreadUPtr<Muxer> m_muxer;

    std::vector<ConsumerThreadUPtr<AudioEncoder>> m_audioEncoders;
    std::vector<ConsumerThreadUPtr<VideoEncoder>> m_videoEncoders;
    std::unique_ptr<EncodingInitializer> m_formatsInitializer;

    QMutex m_timeMutex;
    qint64 m_timeRecorded = 0;

    bool m_autoStop = false;
    size_t m_initializedEncodersCount = 0;
    State m_state = State::None;
};

} // namespace QFFmpeg

QT_END_NAMESPACE

#endif
