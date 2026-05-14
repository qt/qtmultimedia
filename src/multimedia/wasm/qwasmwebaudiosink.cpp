// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwasmwebaudiosink_p.h"

#include <emscripten.h>
#include <emscripten/val.h>

#include <QDebug>
#include <QIODevice>
#include <QtMath>

using emscripten::EM_VAL;

QT_BEGIN_NAMESPACE

constexpr unsigned int DEFAULT_BUFFER_DURATION = 250'000; // µs
constexpr int RING_BUFFER_DURATION = 250'000; // µs

int QWasmAudioSink::s_nextId = 0;

static QHash<int, QWasmAudioSink *> s_sinkRegistry;

void QWasmAudioSink::workletReadyCallback(int callbackId)
{
    if (auto *sink = s_sinkRegistry.value(callbackId))
        sink->connectWorklet();
}

void QWasmAudioSink::deliverDataCallback(int callbackId)
{
    if (auto *sink = s_sinkRegistry.value(callbackId))
        sink->deliverData();
}

extern "C" {
EMSCRIPTEN_KEEPALIVE void qt_sinkWorkletReady(int id) { QWasmAudioSink::workletReadyCallback(id); }
EMSCRIPTEN_KEEPALIVE void qt_sinkDeliverData(int id) { QWasmAudioSink::deliverDataCallback(id); }
}

// ---------------------------------------------------------------------------
// Shared EM_JS helpers (both paths)
// ---------------------------------------------------------------------------

EM_JS(void, qt_sink_clearWorklet, (int callbackId), {
    if (Module._qtSinkWorkletParams) Module._qtSinkWorkletParams[callbackId] = undefined;
});

// ---------------------------------------------------------------------------
// Threaded-path EM_JS helpers (SharedArrayBuffer/Atomics ring buffer)
// ---------------------------------------------------------------------------

#if QT_CONFIG(thread)

// Load the AudioWorklet processor via Blob URL.
// The worklet reads PCM from the WASM heap ring buffer (SharedArrayBuffer),
// converts sample-by-sample to planar Float32, and writes to outputs[0].
// rposPtr is updated by the worklet with Atomics.store after each render quantum;
// the C++ side reads it with m_readPos.load(std::memory_order_acquire).

EM_JS(void, qt_sink_loadWorkletModule,
      (EM_VAL ctxHandle,
       int ringBufferPtr, int ringBufferSize,
       int readPositionPtr, int writePositionPtr, int volumePtr,
       int activeIdPtr,
       int channels, int sampleFormat, int bytesPerSample,
       int callbackId), {
    if (!Module._qtSinkWorkletParams) Module._qtSinkWorkletParams = {};
    Module._qtSinkWorkletParams[callbackId] = {
        heap: HEAP8.buffer,
        ringPtr: ringBufferPtr,
        ringSize: ringBufferSize,
        rposPtr: readPositionPtr,
        wposPtr: writePositionPtr,
        volPtr: volumePtr,
        activeIdPtr: activeIdPtr,
        callbackId: callbackId,
        channels: channels,
        fmt: sampleFormat,
        bps: bytesPerSample
    };
    var code = [
        'class QtSink extends AudioWorkletProcessor {',
        '  constructor(opts) {',
        '    super(opts);',
        '    var processorOptions = opts.processorOptions;',
        '    this._heap8 = new Int8Array(processorOptions.heap);',
        '    this._heap16 = new Int16Array(processorOptions.heap);',
        '    this._heap32 = new Int32Array(processorOptions.heap);',
        '    this._readPositionIdx = (processorOptions.rposPtr >> 2) | 0;',
        '    this._writePositionIdx = (processorOptions.wposPtr >> 2) | 0;',
        '    this._volumeIdx = (processorOptions.volPtr >> 2) | 0;',
        '    this._activeIdIdx = (processorOptions.activeIdPtr >> 2) | 0;',
        '    this._myId = processorOptions.callbackId | 0;',
        '    this._ringBufferPtr = processorOptions.ringPtr | 0;',
        '    this._ringBufferSize = processorOptions.ringSize | 0;',
        '    this._numChannels = processorOptions.channels | 0;',
        '    this._sampleFormat = processorOptions.fmt | 0;',
        '    this._bytesPerSample = processorOptions.bps | 0;',
        '    this._volConvI = new Int32Array(1);',
        '    this._volConvF = new Float32Array(this._volConvI.buffer);',
        '    this._fltConvI = new Int32Array(1);',
        '    this._fltConvF = new Float32Array(this._fltConvI.buffer);',
        '  }',
        '  process(inputs, outputs) {',
        '    if (Atomics.load(this._heap32, this._activeIdIdx) !== this._myId) return false;',
        '    var out = outputs[0];',
        '    if (!out || !out.length) return true;',
        '    var numChannels = out.length;',
        '    var samplesPerChannel = out[0].length;',
        '    this._volConvI[0] = Atomics.load(this._heap32, this._volumeIdx);',
        '    var volume = this._volConvF[0];',
        '    var writePosition = Atomics.load(this._heap32, this._writePositionIdx);',
        '    var readPosition = Atomics.load(this._heap32, this._readPositionIdx);',
        '    var ringBufferSize = this._ringBufferSize;',
        '    var availableBytes = (writePosition - readPosition + ringBufferSize) % ringBufferSize;',
        '    var needed = samplesPerChannel * this._numChannels * this._bytesPerSample;',
        '    var canRead = availableBytes < needed ? availableBytes : needed;',
        '    var currentReadPosition = readPosition;',
        '    var ringBufferPtr = this._ringBufferPtr, sampleFormat = this._sampleFormat, bytesPerSample = this._bytesPerSample;',
        '    for (var i = 0; i < samplesPerChannel; i++) {',
        '      for (var channel = 0; channel < this._numChannels; channel++) {',
        '        var sample = 0.0;',
        '        if (canRead > 0) {',
        '          var offset = ringBufferPtr + currentReadPosition;',
        '          if (sampleFormat === 1) { sample = (this._heap8[offset] / 127.5) - 1.0; }',
        '          else if (sampleFormat === 2) { sample = this._heap16[offset>>1] / 32767.0; }',
        '          else if (sampleFormat === 3) { sample = this._heap32[offset>>2] / 2147483647.0; }',
        '          else { this._fltConvI[0] = this._heap32[offset>>2]; sample = this._fltConvF[0]; }',
        '          currentReadPosition = (currentReadPosition + bytesPerSample) % ringBufferSize;',
        '          canRead -= bytesPerSample;',
        '        }',
        '        if (channel < numChannels) {',
        '          sample = sample < -1.0 ? -1.0 : sample > 1.0 ? 1.0 : sample;',
        '          out[channel][i] = volume < 1.0 ? sample * volume : sample;',
        '        }',
        '      }',
        '    }',
        '    Atomics.store(this._heap32, this._readPositionIdx, currentReadPosition);',
        '    this.port.postMessage(null);',
        '    return true;',
        '  }',
        '}',
        'registerProcessor("qt-audio-sink", QtSink);'
    ].join('\n');
    var blob = new Blob([code], { type: 'application/javascript' });
    var url = URL.createObjectURL(blob);
    Emval.toValue(ctxHandle).audioWorklet.addModule(url).then(function() {
        URL.revokeObjectURL(url);
        Module._qt_sinkWorkletReady(callbackId);
    });
});

EM_JS(void, qt_mt_sink_setupWorkletPort, (EM_VAL nodeHandle, int callbackId), {
    Emval.toValue(nodeHandle).port.onmessage = function() {
        Module._qt_sinkDeliverData(callbackId);
    };
});

EM_JS(EM_VAL, qt_sink_createWorkletNode,
      (EM_VAL ctxHandle, int callbackId, int channels), {
    var ctx = Emval.toValue(ctxHandle);
    var params = Module._qtSinkWorkletParams[callbackId];
    var node = new AudioWorkletNode(ctx, 'qt-audio-sink', {
        numberOfInputs: 0,
        numberOfOutputs: 1,
        outputChannelCounts: [channels],
        processorOptions: params
    });
    return Emval.toHandle(node);
});

// Store updated ring buffer params and signal ready without reloading the module.
// Used on subsequent start() calls when the processor is already registered.

EM_JS(void, qt_sink_storeWorkletParams,
      (int ringBufferPtr, int ringBufferSize,
       int readPositionPtr, int writePositionPtr, int volumePtr,
       int activeIdPtr,
       int channels, int sampleFormat, int bytesPerSample,
       int callbackId), {
    if (!Module._qtSinkWorkletParams) Module._qtSinkWorkletParams = {};
    Module._qtSinkWorkletParams[callbackId] = {
        heap: HEAP8.buffer,
        ringPtr: ringBufferPtr,
        ringSize: ringBufferSize,
        rposPtr: readPositionPtr,
        wposPtr: writePositionPtr,
        volPtr: volumePtr,
        activeIdPtr: activeIdPtr,
        callbackId: callbackId,
        channels: channels,
        fmt: sampleFormat,
        bps: bytesPerSample
    };
});

#else // QT_CONFIG(thread)

// Single-threaded path: load a JS worklet that dequeues planar Float32 frames
// posted from the main thread via MessagePort.

EM_JS(void, qt_st_sink_loadWorkletModule,
      (EM_VAL ctxHandle, int callbackId, int channels), {
    var ctx = Emval.toValue(ctxHandle);
    var code = [
        'class QtSink extends AudioWorkletProcessor {',
        '  constructor(opts) {',
        '    super(opts);',
        '    this._numChannels = opts.processorOptions.channels | 0;',
        '    this._queue = [];',
        '    this._pos = 0;',
        '    this.port.onmessage = (e) => { this._queue.push(e.data); };',
        '  }',
        '  process(inputs, outputs) {',
        '    var out = outputs[0];',
        '    if (!out || !out.length) return true;',
        '    var samplesPerChannel = out[0].length;',
        '    for (var i = 0; i < samplesPerChannel; i++) {',
        '      while (this._queue.length > 0 && this._pos >= this._queue[0].samplesPerChannel) {',
        '        this._queue.shift();',
        '        this._pos = 0;',
        '      }',
        '      if (this._queue.length === 0) break;',
        '      var frame = this._queue[0];',
        '      for (var channel = 0; channel < out.length && channel < frame.numChannels; channel++)',
        '        out[channel][i] = frame.data[channel * frame.samplesPerChannel + this._pos];',
        '      this._pos++;',
        '    }',
        '    this.port.postMessage(null);',
        '    return true;',
        '  }',
        '}',
        'registerProcessor("qt-audio-sink", QtSink);'
    ].join('\n');
    var blob = new Blob([code], { type: 'application/javascript' });
    var url = URL.createObjectURL(blob);
    ctx.audioWorklet.addModule(url).then(function() {
        URL.revokeObjectURL(url);
        Module._qt_sinkWorkletReady(callbackId);
    });
});

EM_JS(EM_VAL, qt_st_sink_createWorkletNode,
      (EM_VAL ctxHandle, int callbackId, int channels), {
    var node = new AudioWorkletNode(Emval.toValue(ctxHandle), 'qt-audio-sink', {
        numberOfInputs: 0,
        numberOfOutputs: 1,
        outputChannelCounts: [channels],
        processorOptions: { channels: channels }
    });
    node.port.onmessage = function() { Module._qt_sinkDeliverData(callbackId); };
    return Emval.toHandle(node);
});

// Post one render quantum (planar Float32, ch × spch floats) to the worklet.
// WASM heap is not a SharedArrayBuffer in single-threaded builds, so we copy
// the data before transferring ownership of the buffer to the worklet thread.

EM_JS(void, qt_st_sink_postFrame,
      (EM_VAL nodeHandle, int channels, int samplesPerChannel, const float *dataPtr), {
    var copy = HEAPF32.slice(dataPtr >> 2, (dataPtr >> 2) + channels * samplesPerChannel);
    Emval.toValue(nodeHandle).port.postMessage(
        { numChannels: channels, samplesPerChannel: samplesPerChannel, data: copy }, [copy.buffer]);
});

#endif // QT_CONFIG(thread)

// ---------------------------------------------------------------------------
// QWasmAudioSinkDevice — returned to caller in push mode
// ---------------------------------------------------------------------------

class QWasmAudioSinkDevice : public QIODevice
{
    QWasmAudioSink *m_sink;
public:
    explicit QWasmAudioSinkDevice(QWasmAudioSink *sink) : QIODevice(sink), m_sink(sink) {}
    bool isSequential() const override { return true; }
protected:
    qint64 readData(char *, qint64) override { return 0; }
    qint64 writeData(const char *data, qint64 maxLength) override
    {
        const int freeBytes = static_cast<int>(m_sink->bytesFree());
        const int toWrite = static_cast<int>(qMin(maxLength, static_cast<qint64>(freeBytes)));
        if (toWrite == 0)
            return 0;
        m_sink->writeToRingBuffer(data, toWrite);
        return toWrite;
    }
};

// ---------------------------------------------------------------------------
// QWasmAudioSink
// ---------------------------------------------------------------------------

QWasmAudioSink::QWasmAudioSink(QAudioDevice device, const QAudioFormat &format, QObject *parent)
    : QPlatformAudioSink(std::move(device), format, parent)
{
    m_bufferSize = m_format.bytesForDuration(DEFAULT_BUFFER_DURATION);
    m_deviceId = m_audioDevice.id().toStdString();
    // "System audiooutput/audioinput" is a Qt-internal fallback ID used when the
    // browser has not granted device permissions. It is not a valid browser sinkId,
    // so map it to "" to request the default audio output device.
    if (m_deviceId.compare(0, 7, "System ") == 0)
        m_deviceId.clear();
    qWarning() << "QWasmAudioSink: sinkId =" << (m_deviceId.empty() ? "(none/default)" : m_deviceId.c_str());
    emscripten::val options = emscripten::val::object();
    options.set("latencyHint", emscripten::val("interactive"));
    options.set("sampleRate", m_format.sampleRate());
    if (!m_deviceId.empty())
        options.set("sinkId", emscripten::val(m_deviceId));
    m_audioContext = emscripten::val::global("AudioContext").new_(options);
}

QWasmAudioSink::~QWasmAudioSink()
{
    teardownPipeline();
    if (!m_audioContext.isUndefined() && !m_audioContext.isNull()
            && m_audioContext["state"].as<std::string>() != "closed") {
        m_audioContext.call<emscripten::val>("close");
    }
}

void QWasmAudioSink::start(QIODevice *device)
{
    Q_ASSERT(device);
    Q_ASSERT(device->openMode().testFlag(QIODevice::ReadOnly));
    m_device = device;
    start(true); // pullMode
}

QIODevice *QWasmAudioSink::start()
{
    auto *sinkDevice = new QWasmAudioSinkDevice(this);
    sinkDevice->open(QIODevice::WriteOnly);
    m_device = sinkDevice;
    start(false /*pullMode*/);
    return sinkDevice;
}

void QWasmAudioSink::start(AudioCallback &&callback)
{
    m_audioCallback = std::move(callback);
    start(false); // pushMode
}

void QWasmAudioSink::start(bool pullMode)
{
    if (m_format.sampleFormat() == QAudioFormat::Unknown
            || m_format.channelCount() < 1
            || m_format.channelCount() > 8) {
        qWarning() << "QWasmAudioSink: unsupported format" << m_format;
        setError(QAudio::OpenError);
        return;
    }

    m_pullMode = pullMode;
    m_callbackId = ++s_nextId;
    s_sinkRegistry.insert(m_callbackId, this);

    m_ringBuffer.resize(m_format.bytesForDuration(RING_BUFFER_DURATION));
    m_writePos.store(0, std::memory_order_relaxed);
    m_readPos.store(0, std::memory_order_relaxed);

    m_processed.store(0, std::memory_order_relaxed);
#if QT_CONFIG(thread)
    m_activeCallbackId.store(m_callbackId, std::memory_order_relaxed);
    // m_audioContext is created once in the constructor and reused across mode changes.
    if (!m_workletModuleLoaded) {
        qt_sink_loadWorkletModule(
            m_audioContext.as_handle(),
            static_cast<int>(reinterpret_cast<intptr_t>(m_ringBuffer.data())),
            static_cast<int>(m_ringBuffer.size()),
            static_cast<int>(reinterpret_cast<intptr_t>(&m_readPos)),
            static_cast<int>(reinterpret_cast<intptr_t>(&m_writePos)),
            static_cast<int>(reinterpret_cast<intptr_t>(&m_volumeAtomic)),
            static_cast<int>(reinterpret_cast<intptr_t>(&m_activeCallbackId)),
            m_format.channelCount(),
            static_cast<int>(m_format.sampleFormat()),
            m_format.bytesPerSample(),
            m_callbackId);
    } else {
        qt_sink_storeWorkletParams(
            static_cast<int>(reinterpret_cast<intptr_t>(m_ringBuffer.data())),
            static_cast<int>(m_ringBuffer.size()),
            static_cast<int>(reinterpret_cast<intptr_t>(&m_readPos)),
            static_cast<int>(reinterpret_cast<intptr_t>(&m_writePos)),
            static_cast<int>(reinterpret_cast<intptr_t>(&m_volumeAtomic)),
            static_cast<int>(reinterpret_cast<intptr_t>(&m_activeCallbackId)),
            m_format.channelCount(),
            static_cast<int>(m_format.sampleFormat()),
            m_format.bytesPerSample(),
            m_callbackId);
        connectWorklet();
    }
#else
    if (!m_workletModuleLoaded) {
        qt_st_sink_loadWorkletModule(m_audioContext.as_handle(), m_callbackId,
                                     m_format.channelCount());
    } else {
        connectWorklet();
    }
#endif

    m_audioContext.call<emscripten::val>("resume");
}

// ---------------------------------------------------------------------------
// Connect worklet once the module has loaded
// ---------------------------------------------------------------------------

void QWasmAudioSink::connectWorklet()
{
    m_workletModuleLoaded = true;
    m_running = true;
#if QT_CONFIG(thread)
    deliverData();
    m_workletNode = emscripten::val::take_ownership(
        qt_sink_createWorkletNode(m_audioContext.as_handle(), m_callbackId,
                                  m_format.channelCount()));
    qt_mt_sink_setupWorkletPort(m_workletNode.as_handle(), m_callbackId);
    m_workletNode.call<void>("connect", m_audioContext["destination"]);
#else
    m_workletNode = emscripten::val::take_ownership(
        qt_st_sink_createWorkletNode(m_audioContext.as_handle(), m_callbackId,
                                     m_format.channelCount()));
    m_workletNode.call<void>("connect", m_audioContext["destination"]);
    deliverData();
#endif
    emit stateChanged(QAudio::ActiveState);
}

// ---------------------------------------------------------------------------
// Fill ring buffer from device/callback and deliver frames to worklet (ST only)
// ---------------------------------------------------------------------------

void QWasmAudioSink::deliverData()
{
    if (!m_running || m_suspended)
        return;

    if (m_audioContext["state"] == emscripten::val("suspended"))
        m_audioContext.call<emscripten::val>("resume");

    if (m_pullMode && m_device) {
        const int freeBytes = static_cast<int>(bytesFree());
        if (freeBytes > 0) {
            QByteArray pullBuffer(freeBytes, Qt::Uninitialized);
            const qint64 bytesRead = m_device->read(pullBuffer.data(), freeBytes);
            if (bytesRead > 0)
                writeToRingBuffer(pullBuffer.constData(), static_cast<int>(bytesRead));
        }
    } else if (m_audioCallback) {
        int freeBytes = static_cast<int>(bytesFree());
        freeBytes -= freeBytes % m_format.bytesPerFrame(); // keep m_writePos frame-aligned
        if (freeBytes > 0) {
            QByteArray callbackBuffer(freeBytes, Qt::Uninitialized);
            // Pass volume=1.0f here: volume is applied by the worklet (threaded)
            // or by deliverToWorklet() (single-threaded), not pre-applied here.
            QtMultimediaPrivate::runAudioCallback(
                *m_audioCallback,
                QSpan<std::byte>(reinterpret_cast<std::byte *>(callbackBuffer.data()), freeBytes),
                m_format,
                1.0f);
            writeToRingBuffer(callbackBuffer.constData(), freeBytes);
        }
    }

#if !QT_CONFIG(thread)
    deliverToWorklet();
#endif
}

// ---------------------------------------------------------------------------
// Single-threaded: drain ring buffer → convert PCM → post quanta to worklet
// ---------------------------------------------------------------------------

#if !QT_CONFIG(thread)
void QWasmAudioSink::deliverToWorklet()
{
    const int numChannels = m_format.channelCount();
    const int bytesPerSample = m_format.bytesPerSample();
    const int renderBlockFrames = 128;
    const int bytesPerFrame = renderBlockFrames * numChannels * bytesPerSample;
    const int ringBufferSize = m_ringBuffer.size();
    const float volume = m_volumeAtomic.load(std::memory_order_relaxed);
    const QAudioFormat::SampleFormat sampleFormat = m_format.sampleFormat();

    while (true) {
        const int writePosition = m_writePos.load(std::memory_order_acquire);
        const int readPosition = m_readPos.load(std::memory_order_relaxed);
        const int availableBytes = (writePosition - readPosition + ringBufferSize) % ringBufferSize;
        if (availableBytes < bytesPerFrame)
            break;

        // Copy one quantum from the ring buffer into a local stack buffer.
        char quantumBuffer[128 * 8 * sizeof(float)];
        const int tailBytes = ringBufferSize - readPosition;
        if (bytesPerFrame <= tailBytes) {
            memcpy(quantumBuffer, m_ringBuffer.constData() + readPosition, bytesPerFrame);
        } else {
            memcpy(quantumBuffer, m_ringBuffer.constData() + readPosition, tailBytes);
            memcpy(quantumBuffer + tailBytes, m_ringBuffer.constData(), bytesPerFrame - tailBytes);
        }
        m_readPos.store((readPosition + bytesPerFrame) % ringBufferSize, std::memory_order_release);
        m_processed += bytesPerFrame;

        // Convert interleaved PCM to planar Float32 with volume applied.
        float planar[128 * 8];
        for (int i = 0; i < renderBlockFrames; ++i) {
            for (int channel = 0; channel < numChannels; ++channel) {
                float sample = 0.0f;
                const int sampleIndex = i * numChannels + channel;
                switch (sampleFormat) {
                case QAudioFormat::UInt8:
                    sample = (reinterpret_cast<const quint8 *>(quantumBuffer)[sampleIndex] / 127.5f) - 1.0f;
                    break;
                case QAudioFormat::Int16:
                    sample = reinterpret_cast<const qint16 *>(quantumBuffer)[sampleIndex] / 32767.0f;
                    break;
                case QAudioFormat::Int32:
                    sample = reinterpret_cast<const qint32 *>(quantumBuffer)[sampleIndex] / 2147483647.0f;
                    break;
                case QAudioFormat::Float:
                    sample = reinterpret_cast<const float *>(quantumBuffer)[sampleIndex];
                    break;
                default:
                    break;
                }
                planar[channel * renderBlockFrames + i] = qBound(-1.0f, sample * volume, 1.0f);
            }
        }
        qt_st_sink_postFrame(m_workletNode.as_handle(), numChannels, renderBlockFrames, planar);
    }
}
#endif

// ---------------------------------------------------------------------------
// Ring buffer write helper (shared by push device, pull mode, callback mode)
// ---------------------------------------------------------------------------

void QWasmAudioSink::writeToRingBuffer(const char *data, int bytes)
{
    const int ringBufferSize = m_ringBuffer.size();
    const int writePosition = m_writePos.load(std::memory_order_relaxed);
    const int tailBytes = ringBufferSize - writePosition;
    if (bytes <= tailBytes) {
        memcpy(m_ringBuffer.data() + writePosition, data, bytes);
    } else {
        memcpy(m_ringBuffer.data() + writePosition, data, tailBytes);
        memcpy(m_ringBuffer.data(), data + tailBytes, bytes - tailBytes);
    }
    m_writePos.store((writePosition + bytes) % ringBufferSize, std::memory_order_release);
#if QT_CONFIG(thread)
    // Threaded: track bytes sent to ring buffer as a proxy for bytes played.
    // The error is bounded by the ring buffer occupancy (~100ms).
    m_processed.fetch_add(static_cast<quint64>(bytes), std::memory_order_relaxed);
#endif
}

// ---------------------------------------------------------------------------
// Control
// ---------------------------------------------------------------------------

void QWasmAudioSink::stop()
{
    if (!m_running)
        return;
    teardownPipeline();
    if (!m_pullMode && m_device)
        delete m_device;
    m_device = nullptr;
    emit stateChanged(QAudio::StoppedState);
}

void QWasmAudioSink::reset()
{
    teardownPipeline();
    m_processed.store(0, std::memory_order_relaxed);
    setError(QAudio::NoError);
    emit stateChanged(QAudio::StoppedState);
}

void QWasmAudioSink::suspend()
{
    if (!m_running || m_suspended)
        return;
    m_suspended = true;
    m_audioContext.call<emscripten::val>("suspend");
    emit stateChanged(QAudio::SuspendedState);
}

void QWasmAudioSink::resume()
{
    if (!m_running || !m_suspended)
        return;
    m_suspended = false;
    m_audioContext.call<emscripten::val>("resume");
    emit stateChanged(QAudio::ActiveState);
}

qsizetype QWasmAudioSink::bytesFree() const
{
    const int ringBufferSize = m_ringBuffer.size();
    if (ringBufferSize == 0)
        return 0;
    const int writePosition = m_writePos.load(std::memory_order_relaxed);
    const int readPosition = m_readPos.load(std::memory_order_acquire);
    const int usedBytes = (writePosition - readPosition + ringBufferSize) % ringBufferSize;
    return static_cast<qsizetype>(ringBufferSize - usedBytes - 1); // -1 distinguishes full from empty
}

void QWasmAudioSink::setBufferSize(qsizetype value)
{
    if (!m_running)
        m_bufferSize = value;
}

qsizetype QWasmAudioSink::bufferSize() const
{
    return m_bufferSize;
}

qint64 QWasmAudioSink::processedUSecs() const
{
    return m_format.durationForBytes(
        static_cast<qint64>(m_processed.load(std::memory_order_relaxed)));
}

QAudio::State QWasmAudioSink::state() const
{
    if (!m_running)
        return QAudio::StoppedState;
    if (m_suspended)
        return QAudio::SuspendedState;
    return QAudio::ActiveState;
}

void QWasmAudioSink::setVolume(float volume)
{
    QPlatformAudioEndpointBase::setVolume(volume);
    m_volumeAtomic.store(volume, std::memory_order_relaxed);
}

void QWasmAudioSink::setError(QAudio::Error error)
{
    QPlatformAudioEndpointBase::setError(error);
}

// ---------------------------------------------------------------------------
// Teardown
// ---------------------------------------------------------------------------

void QWasmAudioSink::teardownPipeline()
{
    if (m_callbackId) {
        s_sinkRegistry.remove(m_callbackId);
        qt_sink_clearWorklet(m_callbackId);
        m_callbackId = 0;
    }

    m_running = false;
    m_suspended = false;
    m_audioCallback.reset();

#if QT_CONFIG(thread)
    // Invalidate the active ID so the current worklet sees a generation mismatch
    // on its next process() call and returns false, deactivating itself.
    m_activeCallbackId.store(0, std::memory_order_release);
#endif

    // Disconnect the node so audio stops immediately.
    // The AudioContext stays alive and is reused on the next start().
    // Do not suspend the threaded AudioContext here — the worklet deactivates
    // itself via the activeCallbackId mismatch, and suspending causes an audible
    // gap when teardown is immediately followed by a restart (e.g. channel change).
    if (!m_workletNode.isUndefined() && !m_workletNode.isNull())
        m_workletNode.call<void>("disconnect");
#if !QT_CONFIG(thread)
    if (!m_audioContext.isUndefined() && !m_audioContext.isNull())
        m_audioContext.call<emscripten::val>("suspend");
#endif
    m_workletNode = emscripten::val::undefined();
}

QT_END_NAMESPACE
