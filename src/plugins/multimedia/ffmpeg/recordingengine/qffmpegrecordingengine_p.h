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

#include <queue>

QT_BEGIN_NAMESPACE

class QFFmpegAudioInput;
class QVideoFrame;
class QPlatformVideoSource;

namespace QFFmpeg
{

class RecordingEngine;
class Muxer;
class AudioEncoder;
class VideoEncoder;
class VideoFrameEncoder;
class EncodingInitializer;

template <typename T>
T dequeueIfPossible(std::queue<T> &queue)
{
    if (queue.empty())
        return T{};

    auto result = std::move(queue.front());
    queue.pop();
    return result;
}

class RecordingEngine : public QObject
{
    Q_OBJECT
public:
    RecordingEngine(const QMediaEncoderSettings &settings, std::unique_ptr<EncodingFormatContext> context);
    ~RecordingEngine();

    void initialize(QFFmpegAudioInput *audioInput,
                    const std::vector<QPlatformVideoSource *> &videoSources);
    void finalize();

    void setPaused(bool p);

    void setMetaData(const QMediaMetaData &metaData);
    AVFormatContext *avFormatContext() { return m_formatContext->avFormatContext(); }
    Muxer *getMuxer() { return m_muxer; }

public Q_SLOTS:
    void newTimeStamp(qint64 time);

Q_SIGNALS:
    void durationChanged(qint64 duration);
    void sessionError(QMediaRecorder::Error code, const QString &description);
    void streamInitializationError(QMediaRecorder::Error code, const QString &description);
    void finalizationDone();

private:
    template<typename... Args>
    void addMediaFrameHandler(Args &&...args);

    class EncodingFinalizer : public QThread
    {
    public:
        EncodingFinalizer(RecordingEngine &recordingEngine);

        void run() override;

    private:
        RecordingEngine &m_recordingEngine;
    };

    friend class EncodingInitializer;
    void addAudioInput(QFFmpegAudioInput *input);
    void addVideoSource(QPlatformVideoSource *source, const QVideoFrame &firstFrame);

    void start();

private:
    QMediaEncoderSettings m_settings;
    QMediaMetaData m_metaData;
    std::unique_ptr<EncodingFormatContext> m_formatContext;
    Muxer *m_muxer = nullptr;

    AudioEncoder *m_audioEncoder = nullptr;
    QList<VideoEncoder *> m_videoEncoders;
    QList<QMetaObject::Connection> m_connections;
    std::unique_ptr<EncodingInitializer> m_initializer;

    QMutex m_timeMutex;
    qint64 m_timeRecorded = 0;

    bool m_isHeaderWritten = false;
};

}

QT_END_NAMESPACE

#endif
