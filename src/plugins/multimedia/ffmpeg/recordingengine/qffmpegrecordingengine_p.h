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

#include "qffmpegthread_p.h"
#include "qffmpegencodingformatcontext_p.h"

#include <private/qplatformmediarecorder_p.h>
#include <qmediarecorder.h>

QT_BEGIN_NAMESPACE

class QFFmpegAudioInput;
class QPlatformAudioBufferInput;
class QPlatformAudioBufferInputBase;
class QVideoFrame;
class QAudioBuffer;
class QPlatformVideoSource;

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
    ~RecordingEngine();

    void initialize(const std::vector<QPlatformAudioBufferInputBase *> &audioSources,
                    const std::vector<QPlatformVideoSource *> &videoSources);
    void finalize();

    void setPaused(bool p);

    void setAutoStop(bool autoStop);

    bool autoStop() const { return m_autoStop; }

    void setMetaData(const QMediaMetaData &metaData);
    AVFormatContext *avFormatContext() { return m_formatContext->avFormatContext(); }
    Muxer *getMuxer() { return m_muxer; }

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
    enum class State {
        None,
        FormatsInitialization,
        EncodersInitialization,
        Encoding, // header written
        Finalization
    };

    class EncodingFinalizer : public QThread
    {
    public:
        EncodingFinalizer(RecordingEngine &recordingEngine);

        void run() override;

    private:
        RecordingEngine &m_recordingEngine;
        State m_state;
    };

    friend class EncodingInitializer;
    void addAudioInput(QFFmpegAudioInput *input);
    void addAudioBufferInput(QPlatformAudioBufferInput *input, const QAudioBuffer &firstBuffer);
    AudioEncoder *createAudioEncoder(const QAudioFormat &format);

    void addVideoSource(QPlatformVideoSource *source, const QVideoFrame &firstFrame);
    void handleSourceEndOfStream();
    void handleEncoderInitialization();

    void handleFormatsInitialization();

    qsizetype encodersCount() const { return m_audioEncoders.size() + m_videoEncoders.size(); }

    template <typename F, typename... Args>
    void forEachEncoder(F &&f, Args &&...args);

    template <typename F>
    bool allOfEncoders(F &&f) const;

private:
    QMediaEncoderSettings m_settings;
    QMediaMetaData m_metaData;
    std::unique_ptr<EncodingFormatContext> m_formatContext;
    Muxer *m_muxer = nullptr;

    QList<AudioEncoder *> m_audioEncoders;
    QList<VideoEncoder *> m_videoEncoders;
    std::unique_ptr<EncodingInitializer> m_formatsInitializer;

    QMutex m_timeMutex;
    qint64 m_timeRecorded = 0;

    bool m_autoStop = false;
    qsizetype m_initializedEncodersCount = 0;
    State m_state = State::None;
};

}

QT_END_NAMESPACE

#endif
