// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpegencodinginitializer_p.h"
#include "qffmpegrecordingengineutils_p.h"
#include "qffmpegrecordingengine_p.h"
#include "qffmpegaudioinput_p.h"
#include "qvideoframe.h"

#include "private/qplatformvideoframeinput_p.h"
#include "private/qplatformaudiobufferinput_p.h"
#include "private/qplatformaudiobufferinput_p.h"

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

EncodingInitializer::EncodingInitializer(RecordingEngine &engine) : m_recordingEngine(engine) { }

EncodingInitializer::~EncodingInitializer()
{
    for (QObject *source : m_pendingSources)
        setEncoderInterface(source, nullptr);
}

bool EncodingInitializer::start(const std::vector<QAudioBufferSource *> &audioSources,
                                const std::vector<QPlatformVideoSource *> &videoSources)
{
    for (auto source : audioSources) {
        if (auto audioInput = qobject_cast<QFFmpegAudioInput *>(source))
            m_recordingEngine.addAudioInput(audioInput);
        else if (auto audioBufferInput = qobject_cast<QPlatformAudioBufferInput *>(source))
            addAudioBufferInput(audioBufferInput);
        else
            Q_ASSERT(!"Undefined source type");
    }

    for (auto source : videoSources)
        addVideoSource(source);

    return tryStartRecordingEngine();
}

void EncodingInitializer::addAudioBufferInput(QPlatformAudioBufferInput *input)
{
    Q_ASSERT(input);

    if (input->audioFormat().isValid())
        m_recordingEngine.addAudioBufferInput(input, {});
    else
        addPendingAudioBufferInput(input);
}

void EncodingInitializer::addPendingAudioBufferInput(QPlatformAudioBufferInput *input)
{
    addPendingSource(input);

    connect(input, &QPlatformAudioBufferInput::destroyed, this, [this, input]() {
        erasePendingSource(input, QStringLiteral("Audio source deleted"), true);
    });

    connect(input, &QPlatformAudioBufferInput::newAudioBuffer, this,
            [this, input](const QAudioBuffer &buffer) {
                if (buffer.isValid())
                    erasePendingSource(
                            input, [&]() { m_recordingEngine.addAudioBufferInput(input, buffer); });
                else
                    erasePendingSource(input,
                                       QStringLiteral("Audio source has sent the end frame"));
            });
}

void EncodingInitializer::addVideoSource(QPlatformVideoSource *source)
{
    Q_ASSERT(source);
    Q_ASSERT(source->isActive());

    if (source->frameFormat().isValid())
        m_recordingEngine.addVideoSource(source, {});
    else if (source->hasError())
        emitStreamInitializationError(QStringLiteral("Video source error: ")
                                      + source->errorString());
    else
        addPendingVideoSource(source);
}

void EncodingInitializer::addPendingVideoSource(QPlatformVideoSource *source)
{
    addPendingSource(source);

    connect(source, &QPlatformVideoSource::errorChanged, this, [this, source]() {
        if (source->hasError())
            erasePendingSource(source,
                               QStringLiteral("Videio source error: ") + source->errorString());
    });

    connect(source, &QPlatformVideoSource::destroyed, this, [this, source]() {
        erasePendingSource(source, QStringLiteral("Source deleted"), true);
    });

    connect(source, &QPlatformVideoSource::activeChanged, this, [this, source]() {
        if (!source->isActive())
            erasePendingSource(source, QStringLiteral("Video source deactivated"));
    });

    connect(source, &QPlatformVideoSource::newVideoFrame, this,
            [this, source](const QVideoFrame &frame) {
                if (frame.isValid())
                    erasePendingSource(source,
                                       [&]() { m_recordingEngine.addVideoSource(source, frame); });
                else
                    erasePendingSource(source,
                                       QStringLiteral("Video source has sent the end frame"));
            });
}

bool EncodingInitializer::tryStartRecordingEngine()
{
    if (m_pendingSources.empty())
        return m_recordingEngine.startEncoders();

    // return true as no errors found, even though they can occur later on,
    // upon the following encoders initializations.
    return true;
}

void EncodingInitializer::emitStreamInitializationError(QString error)
{
    emit m_recordingEngine.streamInitializationError(
            QMediaRecorder::ResourceError,
            QStringLiteral("Video steam initialization error. ") + error);
}

void EncodingInitializer::addPendingSource(QObject *source)
{
    Q_ASSERT(m_pendingSources.count(source) == 0);

    setEncoderInterface(source, this);
    m_pendingSources.emplace(source);
}

template <typename F>
void EncodingInitializer::erasePendingSource(QObject *source, F &&functionOrError, bool destroyed)
{
    const auto erasedCount = m_pendingSources.erase(source);
    if (erasedCount == 0)
        return; // got a queued event, just ignore it.

    if (!destroyed) {
        setEncoderInterface(source, nullptr);
        disconnect(source, nullptr, this, nullptr);
    }

    if constexpr (std::is_invocable_v<F>)
        functionOrError();
    else
        emitStreamInitializationError(functionOrError);

    tryStartRecordingEngine();
}

bool EncodingInitializer::canPushFrame() const
{
    return true;
}

} // namespace QFFmpeg

QT_END_NAMESPACE
