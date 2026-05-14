// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwasmwebaudiosource_p.h"

#include <emscripten.h>
#include <emscripten/val.h>
#include <QDebug>
#include <QtMath>
#include <QIODevice>

using emscripten::EM_VAL;

QT_BEGIN_NAMESPACE

constexpr unsigned int DEFAULT_BUFFER_DURATION = 250'000; // µs

int QWasmAudioSource::s_nextId = 0;

static QHash<int, QWasmAudioSource *> s_registry;

void QWasmAudioSource::workletReadyCallback(int callbackId)
{
    auto *src = s_registry.value(callbackId);
    if (!src)
        return;
    src->m_workletReady = true;
    src->connectMediaStreamIfReady();
}

void QWasmAudioSource::audioDataCallback(int callbackId)
{
    if (auto *src = s_registry.value(callbackId))
        src->deliverBufferedData();
}

extern "C" {
EMSCRIPTEN_KEEPALIVE void qt_onWorkletReady(int id) { QWasmAudioSource::workletReadyCallback(id); }
EMSCRIPTEN_KEEPALIVE void qt_onAudioFrameReady(int id) { QWasmAudioSource::audioDataCallback(id); }
}

#if QT_CONFIG(thread)

constexpr int RING_BUFFER_DURATION = 100'000; // µs

// Load the AudioWorklet processor via Blob URL.
// The processor writes PCM directly into the WASM heap (SharedArrayBuffer)
// ring buffer.
// Pointers are byte offsets into the WASM linear memory (which is a
// SharedArrayBuffer in threaded builds). The write position is updated with
// Atomics.store so the C++ acquire-load on m_writePos sees coherent data.

EM_JS(void, qt_loadWorkletModule,
      (EM_VAL ctxHandle,
       int ringPtr, int ringSize,
       int wposPtr, int volPtr,
       int channels, int fmt, int bps,
       int callbackId), {
    if (!Module._qtWorkletParams) Module._qtWorkletParams = {};
    Module._qtWorkletParams[callbackId] = {
        heap: HEAP8.buffer, // SharedArrayBuffer: the WASM linear memory, shared with the C++ thread
        ringPtr: ringPtr,
        ringSize: ringSize,
        wposPtr: wposPtr,
        volPtr: volPtr,
        channels: channels,
        fmt: fmt,
        bps: bps
    };
    var code = [
        'class QtCapture extends AudioWorkletProcessor {',
        '  constructor(opts) {',
        '    super(opts);',
        '    var options = opts.processorOptions;',
        '    this._heap8 = new Int8Array(options.heap);',   // typed views over the SharedArrayBuffer
        '    this._heap16 = new Int16Array(options.heap);',  // same buffer, different element width
        '    this._heap32 = new Int32Array(options.heap);',
        '    this._volumeConvInt = new Int32Array(1);',
        '    this._volumeConvFloat = new Float32Array(this._volumeConvInt.buffer);',
        '    this._sampleConvFloat = new Float32Array(1);',
        '    this._sampleConvInt = new Int32Array(this._sampleConvFloat.buffer);',
        '    this._ringBufferPtr = options.ringPtr | 0;',
        '    this._ringBufferSize = options.ringSize | 0;',
        '    this._writePositionIndex = (options.wposPtr >> 2) | 0;',
        '    this._volumeIndex = (options.volPtr >> 2) | 0;',
        '    this._numChannels = options.channels | 0;',
        '    this._format = options.fmt | 0;',
        '    this._bytesPerSample = options.bps | 0;',
        '  }',
        '  process(inputs) {',
        '    var input = inputs[0];',
        '    if (!input || !input.length || !input[0] || !input[0].length) return true;',
        '    var numChannels = Math.min(input.length, this._numChannels);',
        '    var samplesPerChannel = input[0].length;',
        '    var bytesPerSample = this._bytesPerSample, ringSize = this._ringBufferSize, format = this._format;',
        '    var ringPtr = this._ringBufferPtr;',
        '    this._volumeConvInt[0] = Atomics.load(this._heap32, this._volumeIndex);',
        '    var vol = this._volumeConvFloat[0];',
        '    var writePos = Atomics.load(this._heap32, this._writePositionIndex);',
        '    for (var i = 0; i < samplesPerChannel; i++) {',
        '      for (var c = 0; c < numChannels; c++) {',
        '        var sample = input[c][i] * vol;',
        '        sample = sample < -1 ? -1 : sample > 1 ? 1 : sample;',
        '        var offset = ringPtr + writePos;',
        '        if (format === 1) { this._heap8 [offset] = ((sample + 1.0) * 127.5) | 0; }',
        '        else if (format === 2) { this._heap16[offset>>1] = (sample * 32767) | 0; }',
        '        else if (format === 3) { this._heap32[offset>>2] = (sample * 2147483647) | 0; }',
        '        else { this._sampleConvFloat[0] = sample; this._heap32[offset>>2] = this._sampleConvInt[0]; }',
        '        writePos = (writePos + bytesPerSample) % ringSize;',
        '      }',
        '    }',
        '    Atomics.store(this._heap32, this._writePositionIndex, writePos);',
        '    this.port.postMessage(null);',
        '    return true;',
        '  }',
        '}',
        'registerProcessor("qt-audio-capture", QtCapture);'
    ].join('\n');
    var blob = new Blob([code], {type: 'application/javascript'});
    var url = URL.createObjectURL(blob);
    Emval.toValue(ctxHandle).audioWorklet.addModule(url).then(function() {
        URL.revokeObjectURL(url);
        Module._qt_onWorkletReady(callbackId);
    });
});

EM_JS(void, qt_mt_setupWorkletPort, (EM_VAL nodeHandle, int callbackId), {
    Emval.toValue(nodeHandle).port.onmessage = function() {
        Module._qt_onAudioFrameReady(callbackId);
    };
});


#else // QT_CONFIG(thread)

// Single-threaded: load a JS worklet that sends frames via MessagePort.

    // qWarning() << "single threaded";

// qWarning() << "Single-threaded";

EM_JS(void, qt_st_loadWorkletModule, (EM_VAL ctxHandle, int instanceId), {
    var ctx = Emval.toValue(ctxHandle);
    if (!Module._qtAudioData) Module._qtAudioData = {};
    Module._qtAudioData[instanceId] = [];
    var code = [
        'class QtCapture extends AudioWorkletProcessor {',
        '  process(inputs) {',
        '    var input = inputs[0];',
        '    if (input && input.length && input[0] && input[0].length) {',
        '      var numChannels = input.length, samplesPerChannel = input[0].length;',
        '      var buffer = new Float32Array(numChannels * samplesPerChannel);',
        '      for (var c = 0; c < numChannels; c++) buffer.set(input[c], c * samplesPerChannel);',
        '      this.port.postMessage({ch:numChannels,spch:samplesPerChannel,buf:buffer.buffer},[buffer.buffer]);',
        '    }',
        '    return true;',
        '  }',
        '}',
        'registerProcessor("qt-audio-capture", QtCapture);'
    ].join('\n');
    var blob = new Blob([code], {type: 'application/javascript'});
    var url = URL.createObjectURL(blob);
    ctx.audioWorklet.addModule(url).then(function() {
        URL.revokeObjectURL(url);
        Module._qt_onWorkletReady(instanceId);
    });
});

EM_JS(EM_VAL, qt_st_createWorkletNode, (EM_VAL ctxHandle, int instanceId, int channelCount), {
    var node = new AudioWorkletNode(Emval.toValue(ctxHandle), 'qt-audio-capture', {
        numberOfInputs: 1,
        numberOfOutputs: 0,
        channelCount: channelCount,
        channelCountMode: 'explicit'
    });
    node.port.onmessage = function(e) {
        Module._qtAudioData[instanceId].push(e.data);
        Module._qt_onAudioFrameReady(instanceId);
    };
    return Emval.toHandle(node);
});


// Dequeue the oldest frame into a C heap buffer (planar Float32, ch*spch floats).
EM_JS(int, qt_st_readFrame, (int instanceId, float *heapPtr, int *outCh, int *outSpch), {
    var q = Module._qtAudioData && Module._qtAudioData[instanceId];
    if (!q || !q.length) return 0;
    var frame = q.shift();
    var data = new Float32Array(frame.buf);
    HEAPF32.set(data, heapPtr >> 2);
    HEAP32[outCh >> 2] = frame.ch;
    HEAP32[outSpch >> 2] = frame.spch;
    return data.length;
});

#endif // QT_CONFIG(thread)

class QWasmAudioSourceDevice : public QIODevice
{
    QWasmAudioSource *m_source;
public:
    explicit QWasmAudioSourceDevice(QWasmAudioSource *src) : QIODevice(src), m_source(src) {}
    bool isSequential() const override { return true; }
protected:
    qint64 readData(char *data, qint64 maxlen) override { return m_source->readFromBuffer(data, maxlen); }
    qint64 writeData(const char *, qint64) override { Q_UNREACHABLE(); return 0; }
};

// Interleave planar float and convert to PCM — used by the single-threaded path.
// multithread does this in worklet processor, so no need to share this.
static void convertFloatToPcm(const float *planarData, int numChannels, int samplesPerChannel,
                               float volume, QAudioFormat::SampleFormat fmt, int bytesPerSample,
                               char *out)
{
    switch (fmt) {
    case QAudioFormat::UInt8:
        for (int i = 0; i < samplesPerChannel; ++i)
            for (int ch = 0; ch < numChannels; ++ch, out += bytesPerSample) {
                const float s = qBound(-1.0f, planarData[ch * samplesPerChannel + i] * volume, 1.0f);
                *reinterpret_cast<quint8 *>(out) = static_cast<quint8>((s + 1.0f) * 127.5f);
            }
        break;
    case QAudioFormat::Int16:
        for (int i = 0; i < samplesPerChannel; ++i)
            for (int ch = 0; ch < numChannels; ++ch, out += bytesPerSample) {
                const float s = qBound(-1.0f, planarData[ch * samplesPerChannel + i] * volume, 1.0f);
                *reinterpret_cast<qint16 *>(out) = static_cast<qint16>(s * 32767.0f);
            }
        break;
    case QAudioFormat::Int32:
        for (int i = 0; i < samplesPerChannel; ++i)
            for (int ch = 0; ch < numChannels; ++ch, out += bytesPerSample) {
                const float s = qBound(-1.0f, planarData[ch * samplesPerChannel + i] * volume, 1.0f);
                *reinterpret_cast<qint32 *>(out) = static_cast<qint32>(s * 2147483647.0f);
            }
        break;
    case QAudioFormat::Float:
        for (int i = 0; i < samplesPerChannel; ++i)
            for (int ch = 0; ch < numChannels; ++ch, out += bytesPerSample) {
                const float s = qBound(-1.0f, planarData[ch * samplesPerChannel + i] * volume, 1.0f);
                *reinterpret_cast<float *>(out) = s;
            }
        break;
    default:
        break;
    }
}

// ===========================================================================
// QWasmAudioSource implementation
// ===========================================================================

QWasmAudioSource::QWasmAudioSource(QAudioDevice device,
                                   const QAudioFormat &fmt,
                                   QObject *parent)
    : QPlatformAudioSource(std::move(device), fmt, parent)
{
    m_bufferSize = m_format.bytesForDuration(DEFAULT_BUFFER_DURATION);
}

QWasmAudioSource::~QWasmAudioSource()
{
    teardownPipeline();
}

void QWasmAudioSource::start(QIODevice *device)
{
    m_device = device;
    start(true);
}

QIODevice *QWasmAudioSource::start()
{
    auto *dev = new QWasmAudioSourceDevice(this);
    dev->open(QIODevice::ReadOnly);
    m_device = dev;
    start(false);
    return dev;
}

void QWasmAudioSource::start(bool pullMode)
{
    if (m_running || m_inputStream)
        return;

    if (m_format.sampleFormat() == QAudioFormat::Unknown
            || m_format.channelCount() < 1
            || m_format.channelCount() > 8) {
        qWarning() << "QWasmAudioSource: unsupported format" << m_format;
        setError(QAudio::OpenError);
        return;
    }

    m_pullMode = pullMode;
    m_processed = 0;
    m_streamReady = false;
    m_workletReady = false;
    m_callbackId = ++s_nextId;
    s_registry.insert(m_callbackId, this);

#if QT_CONFIG(thread)
    m_ringBuffer.resize(m_format.bytesForDuration(RING_BUFFER_DURATION));
    m_writePos.store(0, std::memory_order_relaxed);
    m_readPos.store(0, std::memory_order_relaxed);
#endif

    m_inputStream = new JsMediaInputStream(this);
    m_inputStream->setUseAudio(true);
    m_inputStream->setUseVideo(false);
    connect(m_inputStream, &JsMediaInputStream::mediaAudioStreamReady, this, [this]() {
        m_mediaStream = m_inputStream->getMediaStream();
        m_streamReady = true;
        connectMediaStreamIfReady();
    });
    m_inputStream->setStreamDevice(m_audioDevice.id().toStdString());

#if QT_CONFIG(thread)
    {
        auto attrs = emscripten::val::object();
        attrs.set("latencyHint", emscripten::val("interactive"));
        attrs.set("sampleRate", m_format.sampleRate());
        auto sinkId = emscripten::val::object();
        sinkId.set("type", emscripten::val("none")); // do not send to output device
        attrs.set("sinkId", sinkId);
        m_audioContext = emscripten::val::global("AudioContext").new_(attrs);
    }
    // m_ringBuffer lives in the WASM heap (a SharedArrayBuffer in threaded builds);
    // the worklet accesses it directly via HEAP8.buffer without any copy.
    qt_loadWorkletModule(m_audioContext.as_handle(),
        static_cast<int>(reinterpret_cast<intptr_t>(m_ringBuffer.data())),
        static_cast<int>(m_ringBuffer.size()),
        static_cast<int>(reinterpret_cast<intptr_t>(&m_writePos)),
        static_cast<int>(reinterpret_cast<intptr_t>(&m_volumeAtomic)),
        m_format.channelCount(),
        static_cast<int>(m_format.sampleFormat()),
        m_format.bytesPerSample(),
        m_callbackId);
#else
    auto attrs = emscripten::val::object();
    attrs.set("latencyHint", emscripten::val("interactive"));
    attrs.set("sampleRate", m_format.sampleRate());
    auto sinkId = emscripten::val::object();
    sinkId.set("type", emscripten::val("none")); // do not send to output device
    attrs.set("sinkId", sinkId);
    m_audioContext = emscripten::val::global("AudioContext").new_(attrs);
    qt_st_loadWorkletModule(m_audioContext.as_handle(), m_callbackId);
#endif

    m_elapsedTimer.start();
}

void QWasmAudioSource::stop()
{
    if (!m_running)
        return;
    if (m_pullMode)
        deliverBufferedData();
    teardownPipeline();
    if (!m_pullMode)
        m_device->deleteLater();
    m_device = nullptr;
}

void QWasmAudioSource::reset()
{
    teardownPipeline();
    m_processed = 0;
    setError(QAudio::NoError);
    m_device = nullptr;
}

void QWasmAudioSource::setBufferSize(qsizetype value)
{
    m_bufferSize = value;
}

qsizetype QWasmAudioSource::bufferSize() const
{
    return m_bufferSize;
}

qint64 QWasmAudioSource::processedUSecs() const
{
    return m_format.durationForBytes(m_processed);
}

QAudio::State QWasmAudioSource::state() const
{
    if (!m_running) return QAudio::StoppedState;
    if (m_suspended) return QAudio::SuspendedState;
    return QAudio::ActiveState;
}

void QWasmAudioSource::setVolume(float vol)
{
    QPlatformAudioSource::setVolume(vol);
#if QT_CONFIG(thread)
    m_volumeAtomic.store(vol, std::memory_order_relaxed);
#endif
}

void QWasmAudioSource::suspend()
{
    if (!m_running || m_suspended)
        return;
    m_suspended = true;
    m_audioContext.call<void>("suspend");
}

void QWasmAudioSource::resume()
{
    if (!m_running || !m_suspended)
        return;
    m_suspended = false;
    m_audioContext.call<void>("resume");
}

void QWasmAudioSource::connectMediaStreamIfReady()
{
    if (!m_streamReady || !m_workletReady)
        return;
#if QT_CONFIG(thread)
    auto nodeOpts = emscripten::val::object();
    nodeOpts.set("numberOfInputs", 1);
    nodeOpts.set("numberOfOutputs", 0);
    nodeOpts.set("channelCount", m_format.channelCount());
    nodeOpts.set("channelCountMode", emscripten::val("explicit"));
    nodeOpts.set("processorOptions",
                 emscripten::val::module_property("_qtWorkletParams")[m_callbackId]);
    m_workletNode = emscripten::val::global("AudioWorkletNode")
                        .new_(m_audioContext, std::string("qt-audio-capture"), nodeOpts);
    qt_mt_setupWorkletPort(m_workletNode.as_handle(), m_callbackId);
    m_audioContext.call<emscripten::val>("createMediaStreamSource", m_mediaStream)
                     .call<void>("connect", m_workletNode, 0, 0);
    m_audioContext.call<void>("resume");
    m_running.store(true, std::memory_order_release);
#else
    m_workletNode = emscripten::val::take_ownership(
            qt_st_createWorkletNode(m_audioContext.as_handle(), m_callbackId, m_format.channelCount()));
    m_audioContext.call<emscripten::val>("createMediaStreamSource", m_mediaStream)
                  .call<void>("connect", m_workletNode);
    m_audioContext.call<void>("resume");
    m_running = true;
#endif
}

void QWasmAudioSource::deliverBufferedData()
{
    if (!m_running || !m_device || m_suspended)
        return;

#if QT_CONFIG(thread)
    const int avail = static_cast<int>(bytesReady());
    if (avail == 0)
        return;
    if (m_pullMode) {
        const int ringSize = m_ringBuffer.size();
        int rpos = m_readPos.load(std::memory_order_relaxed);
        const int tail = ringSize - rpos;
        if (avail <= tail) {
            m_device->write(m_ringBuffer.constData() + rpos, avail);
        } else {
            m_device->write(m_ringBuffer.constData() + rpos, tail);
            m_device->write(m_ringBuffer.constData(), avail - tail);
        }
        m_processed += avail;
        m_readPos.store((rpos + avail) % ringSize, std::memory_order_release);
    } else {
        emit m_device->readyRead();
    }
#else
    float frameBuf[128 * 8]; // one Web Audio quantum: 128 frames × 8 ch max
    int numCh = 0, spch = 0;
    const int bytesPerSample = m_format.bytesPerSample();
    const float vol = volume();
    m_pendingData.reserve(m_pendingData.size() + m_bufferSize);
    while (qt_st_readFrame(m_callbackId, frameBuf, &numCh, &spch) > 0) {
        const int prevSize = m_pendingData.size();
        m_pendingData.resize(prevSize + spch * numCh * bytesPerSample);
        convertFloatToPcm(frameBuf, numCh, spch, vol,
                          m_format.sampleFormat(), bytesPerSample,
                          m_pendingData.data() + prevSize);
    }
    if (m_pendingData.isEmpty())
        return;
    if (m_pullMode) {
        m_processed += m_pendingData.size();
        m_device->write(m_pendingData);
        m_pendingData.clear();
    } else {
        emit m_device->readyRead();
    }
#endif
}

qint64 QWasmAudioSource::readFromBuffer(char *data, qint64 maxlen)
{
#if QT_CONFIG(thread)
    const int avail = static_cast<int>(bytesReady());
    const int chunk = static_cast<int>(qMin(maxlen, static_cast<qint64>(avail)));
    if (chunk == 0)
        return 0;
    const int ringSize = m_ringBuffer.size();
    int rpos = m_readPos.load(std::memory_order_relaxed);
    const int tail = ringSize - rpos;
    if (chunk <= tail) {
        memcpy(data, m_ringBuffer.constData() + rpos, chunk);
    } else {
        memcpy(data, m_ringBuffer.constData() + rpos, tail);
        memcpy(data + tail, m_ringBuffer.constData(), chunk - tail);
    }
    m_processed += chunk;
    m_readPos.store((rpos + chunk) % ringSize, std::memory_order_release);
    return chunk;
#else
    const qint64 chunk = qMin(maxlen, static_cast<qint64>(m_pendingData.size()));
    if (chunk == 0)
        return 0;
    memcpy(data, m_pendingData.constData(), chunk);
    m_pendingData.remove(0, chunk);
    m_processed += chunk;
    return chunk;
#endif
}

qsizetype QWasmAudioSource::bytesReady() const
{
    if (!m_running)
        return 0;
#if QT_CONFIG(thread)
    const int w = m_writePos.load(std::memory_order_acquire);
    const int r = m_readPos.load(std::memory_order_relaxed);
    return static_cast<qsizetype>((w - r + m_ringBuffer.size()) % m_ringBuffer.size());
#else
    return static_cast<qsizetype>(m_pendingData.size());
#endif
}

void QWasmAudioSource::teardownPipeline()
{
    if (m_callbackId) {
        s_registry.remove(m_callbackId);
        auto paramsMap = emscripten::val::module_property("_qtWorkletParams");
        if (!paramsMap.isUndefined()) paramsMap.set(m_callbackId, emscripten::val::undefined());
        auto dataMap = emscripten::val::module_property("_qtAudioData");
        if (!dataMap.isUndefined()) dataMap.set(m_callbackId, emscripten::val::array());
        m_callbackId = 0;
    }
    m_workletNode = emscripten::val::undefined();
    if (!m_audioContext.isUndefined()) {
        m_audioContext.call<void>("close");
        m_audioContext = emscripten::val::undefined();
    }
#if QT_CONFIG(thread)
    m_running.store(false, std::memory_order_release);
#else
    m_running = false;
    m_pendingData.clear();
#endif
    m_suspended = m_workletReady = m_streamReady = false;
    delete m_inputStream;
    m_inputStream = nullptr;
    m_mediaStream = emscripten::val::undefined();
    m_elapsedTimer.invalidate();
}

QT_END_NAMESPACE
