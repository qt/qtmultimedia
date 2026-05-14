// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWASMWEBAUDIOSINK_P_H
#define QWASMWEBAUDIOSINK_P_H

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

#include <private/qaudiosystem_p.h>
#include <QHash>

#include <emscripten/val.h>

#include <atomic>
#include <optional>
#include <string>

QT_BEGIN_NAMESPACE

class QWasmAudioSinkDevice;

class QWasmAudioSink : public QPlatformAudioSink
{
    Q_OBJECT

public:
    QWasmAudioSink(QAudioDevice, const QAudioFormat &, QObject *parent);
    ~QWasmAudioSink() override;

    void start(QIODevice *device) override;
    QIODevice *start() override;
    void start(AudioCallback &&callback) override;
    bool hasCallbackAPI() override { return true; }
    void stop() override;
    void reset() override;
    void suspend() override;
    void resume() override;
    qsizetype bytesFree() const override;
    void setBufferSize(qsizetype value) override;
    qsizetype bufferSize() const override;
    qint64 processedUSecs() const override;
    QAudio::State state() const override;
    void setVolume(float volume) override;
    void setError(QAudio::Error) override;

    static void workletReadyCallback(int callbackId);
    static void deliverDataCallback(int callbackId);

private:
    void start(bool pullMode);
    void deliverData();
    void connectWorklet();
    void writeToRingBuffer(const char *data, int bytes);
    void teardownPipeline();

#if !QT_CONFIG(thread)
    void deliverToWorklet();
#endif

    static int s_nextId;
    int m_callbackId = 0;

    emscripten::val m_audioContext = emscripten::val::undefined();
#if QT_CONFIG(thread)
    std::atomic<int> m_activeCallbackId{0};
#endif

    std::atomic<quint64> m_processed{0};
    bool m_workletModuleLoaded = false;

    emscripten::val m_workletNode = emscripten::val::undefined();

    // Ring buffer: main thread writes, worklet reads (threaded) or
    // deliverToWorklet() reads and posts (single-threaded).
    QByteArray m_ringBuffer;
    std::atomic<int> m_writePos{0};
    std::atomic<int> m_readPos{0};
    std::atomic<float> m_volumeAtomic{1.0f};

    QIODevice *m_device = nullptr;
    bool m_pullMode = false;
    bool m_running = false;
    bool m_suspended = false;
    std::optional<AudioCallback> m_audioCallback;
    qsizetype m_bufferSize = 0;
    std::string m_deviceId;

    friend class QWasmAudioSinkDevice;
};

QT_END_NAMESPACE

#endif // QWASMWEBAUDIOSINK_P_H
