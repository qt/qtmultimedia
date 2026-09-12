// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QMOCKAUDIOSINK_P_H
#define QMOCKAUDIOSINK_P_H

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

#include <private/qaudiosystem_p.h>

QT_BEGIN_NAMESPACE

// Minimal QPlatformAudioSink stub: exists purely so that QAudioSink construction against a mock
// output device succeeds hermetically in unit tests. It never drives its stored callback itself;
// tests pump audio callbacks manually (e.g. via QRtAudioEngine::pumpAudioCallback).
class QMockAudioSink : public QPlatformAudioSink
{
public:
    QMockAudioSink(QAudioDevice device, const QAudioFormat &format, QObject *parent)
        : QPlatformAudioSink(std::move(device), format, parent)
    {
    }

    void start(QIODevice *) override { updateStreamState(QAudio::ActiveState); }
    QIODevice *start() override
    {
        updateStreamState(QAudio::ActiveState);
        return nullptr;
    }
    void start(AudioCallback &&) override { updateStreamState(QAudio::ActiveState); }
    bool hasCallbackAPI() override { return true; }

    void stop() override { updateStreamState(QAudio::StoppedState); }
    void reset() override { }
    void suspend() override { updateStreamState(QAudio::SuspendedState); }
    void resume() override { updateStreamState(QAudio::ActiveState); }

    qsizetype bytesFree() const override { return 0; }
    void setBufferSize(qsizetype) override { }
    qsizetype bufferSize() const override { return 0; }
    qint64 processedUSecs() const override { return 0; }
};

QT_END_NAMESPACE

#endif
