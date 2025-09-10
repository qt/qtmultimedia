// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QENCODINGINITIALIZER_P_H
#define QENCODINGINITIALIZER_P_H

#include "qobject.h"
#include <QtMultimedia/private/qmediainputencoderinterface_p.h>
#include <unordered_set>
#include <vector>

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

QT_BEGIN_NAMESPACE

class QFFmpegAudioInput;
class QPlatformVideoSource;
class QPlatformAudioBufferInput;
class QAudioBufferSource;
class QMediaInputEncoderInterface;

namespace QFFmpeg {

class RecordingEngine;

// Initializes RecordingEngine with audio and video sources, potentially lazily
// upon first frame arrival if video frame format is not pre-determined.
class EncodingInitializer : public QObject, private QMediaInputEncoderInterface
{
public:
    EncodingInitializer(RecordingEngine &engine);

    ~EncodingInitializer() override;

    bool start(const std::vector<QAudioBufferSource *> &audioSources,
               const std::vector<QPlatformVideoSource *> &videoSources);

private:
    void addAudioBufferInput(QPlatformAudioBufferInput *input);

    void addPendingAudioBufferInput(QPlatformAudioBufferInput *input);

    void addVideoSource(QPlatformVideoSource *source);

    void addPendingVideoSource(QPlatformVideoSource *source);

    void addPendingSource(QObject *source);

    bool tryStartRecordingEngine();

private:
    void emitStreamInitializationError(QString error);

    template <typename F>
    void erasePendingSource(QObject *source, F &&functionOrError, bool destroyed = false);

    bool canPushFrame() const override;

private:
    RecordingEngine &m_recordingEngine;
    std::unordered_set<QObject *> m_pendingSources;
};

} // namespace QFFmpeg

QT_END_NAMESPACE

#endif // QENCODINGINITIALIZER_P_H
