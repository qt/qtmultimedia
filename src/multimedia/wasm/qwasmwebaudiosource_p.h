// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWASMWEBAUDIOSOURCE_P_H
#define QWASMWEBAUDIOSOURCE_P_H

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
#include <QElapsedTimer>
#include <QHash>

#include <emscripten/val.h>

#include "qwasmjs_p.h"

#if QT_CONFIG(thread)
#  include <atomic>
#endif

QT_BEGIN_NAMESPACE

class QWasmAudioSourceDevice;

class QWasmAudioSource : public QPlatformAudioSource
{
    Q_OBJECT

public:
    QWasmAudioSource(QAudioDevice, const QAudioFormat &, QObject *parent);
    ~QWasmAudioSource() override;

    void start(QIODevice *device) override;
    QIODevice *start() override;
    void stop() override;
    void reset() override;
    void suspend() override;
    void resume() override;
    qsizetype bytesReady() const override;
    void setBufferSize(qsizetype value) override;
    qsizetype bufferSize() const override;
    qint64 processedUSecs() const override;
    QAudio::State state() const override;
    void setVolume(float volume) override;

    qint64 readFromBuffer(char *data, qint64 maxlen);

    static void workletReadyCallback(int callbackId);
    static void audioDataCallback(int callbackId);

private:
    void startPipeline(bool pullMode);
    void connectMediaStreamIfReady();
    void deliverBufferedData();
    void teardownPipeline();

#if QT_CONFIG(thread)
    // Threaded path: JS AudioWorklet writes directly into the WASM heap
    // (SharedArrayBuffer) via the SPSC ring buffer.
    emscripten::val m_workletNode = emscripten::val::undefined();

    QByteArray m_ringBuffer;
    std::atomic<int> m_writePos{0};
    std::atomic<int> m_readPos{0};
    std::atomic<bool> m_running{false};
    std::atomic<float> m_volumeAtomic{1.0f};

#else // QT_CONFIG(thread)

    // Single-threaded path: JS AudioWorklet + MessagePort.
    // No SharedArrayBuffer — frames arrive via postMessage into a JS-side queue,
    // drained by postMessage callbacks into m_pendingData (main-thread only).
    emscripten::val m_workletNode = emscripten::val::undefined();
    QByteArray m_pendingData;
    bool m_running = false;

#endif // QT_CONFIG(thread)

    // Common to both paths
    emscripten::val m_audioContext = emscripten::val::undefined();
    static int s_nextId;
    int m_callbackId = 0;

    JsMediaInputStream *m_inputStream = nullptr;
    emscripten::val m_mediaStream = emscripten::val::undefined();
    bool m_streamReady = false;
    bool m_workletReady = false;

    QIODevice *m_device = nullptr;
    bool m_pullMode  = false;
    bool m_suspended = false;
    quint64 m_processed  = 0;
    qsizetype m_bufferSize = 0;
    QElapsedTimer m_elapsedTimer;

    friend class QWasmAudioSourceDevice;
};

QT_END_NAMESPACE

#endif // QWASMWEBAUDIOSOURCE_P_H
