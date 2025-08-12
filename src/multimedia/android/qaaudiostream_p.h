// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QAAUDIOSTREAM_P_H
#define QAAUDIOSTREAM_P_H

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

#include <QtMultimedia/qaudioformat.h>

#include <aaudio/AAudio.h>

QT_BEGIN_NAMESPACE

namespace QtAAudio {

Q_DECLARE_LOGGING_CATEGORY(qLcAAudioStream)

struct StreamBuilder;

struct Stream
{
    explicit Stream(const StreamBuilder &builder);
    ~Stream();

    Q_DISABLE_COPY_MOVE(Stream)

    bool start();
    void stop();
    void pause();
    void flush();

    bool isOpen() const;
    bool areStreamParametersRespected() const;

private:
    void close();

    aaudio_result_t waitForTargetState(aaudio_stream_state_t targetState);

    template <typename Functor>
    aaudio_result_t requestWithExpectedState(Functor &&request, aaudio_stream_state_t expected);

    // stream members
    AAudioStream *m_stream{ nullptr };
    bool m_areStreamParametersRespected{ false };
};

struct StreamParameterSet
{
    aaudio_sharing_mode_t sharingMode = AAUDIO_SHARING_MODE_SHARED;
    aaudio_direction_t direction = AAUDIO_DIRECTION_OUTPUT;
    aaudio_usage_t outputUsage = AAUDIO_USAGE_MEDIA;
    aaudio_content_type_t outputContentType = AAUDIO_CONTENT_TYPE_MUSIC;
    aaudio_input_preset_t inputPreset = AAUDIO_INPUT_PRESET_VOICE_RECOGNITION;
};

struct StreamBuilder
{
    friend Stream;

    StreamBuilder(QAudioFormat format);
    ~StreamBuilder();

    Q_DISABLE_COPY_MOVE(StreamBuilder)

    QAudioFormat format;
    AAudioStream_dataCallback callback;
    AAudioStream_errorCallback errorCallback;
    void *userData{ nullptr };
    StreamParameterSet params;
    int32_t deviceId{ AAUDIO_UNSPECIFIED };
    int32_t bufferCapacity{ AAUDIO_UNSPECIFIED };

    void setupBuilder();

private:
    AAudioStreamBuilder *m_builder{ nullptr };
};

} // namespace QtAAudio

QT_END_NAMESPACE

#endif
