// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpegencodinginitializer_p.h"
#include "qffmpegrecordingengine_p.h"
#include "qvideoframe.h"

#include "private/qplatformvideosource_p.h"

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

EncodingInitializer::EncodingInitializer(RecordingEngine &engine) : m_recordingEngine(engine) { }

void EncodingInitializer::start(QFFmpegAudioInput *audioInput,
                                const std::vector<QPlatformVideoSource *> &videoSources)
{
    if (audioInput)
        m_recordingEngine.addAudioInput(audioInput);

    for (auto source : videoSources)
        addVideoSource(source);

    tryStartRecordingEngine();
}

void EncodingInitializer::addVideoSource(QPlatformVideoSource *source)
{
    Q_ASSERT(source);
    Q_ASSERT(source->isActive());

    if (source->frameFormat().isValid())
        m_recordingEngine.addVideoSource(source, {});
    else if (source->hasError())
        emitStreamInitializationError(QStringLiteral("Source error: ") + source->errorString());
    else
        addPendingVideoSource(source);
}

void EncodingInitializer::addPendingVideoSource(QPlatformVideoSource *source)
{
    Q_ASSERT(m_pendingSources.count(source) == 0);

    m_pendingSources.insert(source);

    connect(source, &QPlatformVideoSource::errorChanged, this, [this, source]() {
        if (source->hasError())
            erasePendingSource(source, QStringLiteral("Source error: ") + source->errorString());
    });

    connect(source, &QPlatformVideoSource::destroyed, this,
            [this, source]() { erasePendingSource(source, QStringLiteral("Source deleted")); });

    connect(source, &QPlatformVideoSource::activeChanged, this, [this, source]() {
        if (!source->isActive())
            erasePendingSource(source, QStringLiteral("Source deactivated"));
    });

    connect(source, &QPlatformVideoSource::newVideoFrame, this,
            [this, source](const QVideoFrame &frame) {
                if (frame.isValid())
                    erasePendingSource(source,
                                       [&]() { m_recordingEngine.addVideoSource(source, frame); });
                else
                    erasePendingSource(source, QStringLiteral("Source has sent the end frame"));
            });
}

void EncodingInitializer::tryStartRecordingEngine()
{
    if (m_pendingSources.empty())
        m_recordingEngine.start();
}

void EncodingInitializer::emitStreamInitializationError(QString error)
{
    emit m_recordingEngine.streamInitializationError(
            QMediaRecorder::ResourceError,
            QStringLiteral("Video steam initialization error. ") + error);
}

template <typename F>
void EncodingInitializer::erasePendingSource(QObject *source, F &&functionOrError)
{
    const auto erasedCount = m_pendingSources.erase(source);
    if (erasedCount == 0)
        return; // got a queued event, just ignore it.

    if constexpr (std::is_invocable_v<F>)
        functionOrError();
    else
        emitStreamInitializationError(functionOrError);

    disconnect(source, nullptr, this, nullptr);
    tryStartRecordingEngine();
}

} // namespace QFFmpeg

QT_END_NAMESPACE
