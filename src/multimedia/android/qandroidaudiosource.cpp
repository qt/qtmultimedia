// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qandroidaudiosource_p.h"

#include "qopenslesengine_p.h"
#include <private/qaudiohelpers_p.h>
#include <qbuffer.h>
#include <qdebug.h>
#include <qloggingcategory.h>

#ifdef ANDROID
#include <SLES/OpenSLES_AndroidConfiguration.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qpermissions.h>
#endif

static Q_LOGGING_CATEGORY(qLcAndroidAudioSource, "qt.multimedia.android.audiosource")

QT_BEGIN_NAMESPACE

#define NUM_BUFFERS 2
#define DEFAULT_PERIOD_TIME_MS 50
#define MINIMUM_PERIOD_TIME_MS 5

#ifdef ANDROID
static bool hasRecordingPermission()
{
    QMicrophonePermission permission;

    const bool permitted = qApp->checkPermission(permission) == Qt::PermissionStatus::Granted;
    if (!permitted)
        qCWarning(qLcAndroidAudioSource, "Missing microphone permission!");

    return permitted;
}

static void bufferQueueCallback(SLAndroidSimpleBufferQueueItf, void *context)
#else
static void bufferQueueCallback(SLBufferQueueItf, void *context)
#endif
{
    // Process buffer in main thread
    QMetaObject::invokeMethod(reinterpret_cast<QAndroidAudioSource*>(context), "processBuffer");
}

QAndroidAudioSource::QAndroidAudioSource(const QByteArray &device, QObject *parent)
    : QPlatformAudioSource(parent)
    , m_device(device)
    , m_engine(QOpenSLESEngine::instance())
    , m_recorderObject(0)
    , m_recorder(0)
    , m_bufferQueue(0)
    , m_pullMode(true)
    , m_processedBytes(0)
    , m_audioSource(0)
    , m_bufferIODevice(0)
    , m_errorState(QAudio::NoError)
    , m_deviceState(QAudio::StoppedState)
    , m_lastNotifyTime(0)
    , m_volume(1.0)
    , m_bufferSize(0)
    , m_buffers(new QByteArray[NUM_BUFFERS])
    , m_currentBuffer(0)
{
#ifdef ANDROID
    if (qstrcmp(device, QT_ANDROID_PRESET_CAMCORDER) == 0)
        m_recorderPreset = SL_ANDROID_RECORDING_PRESET_CAMCORDER;
    else if (qstrcmp(device, QT_ANDROID_PRESET_VOICE_RECOGNITION) == 0)
        m_recorderPreset = SL_ANDROID_RECORDING_PRESET_VOICE_RECOGNITION;
    else if (qstrcmp(device, QT_ANDROID_PRESET_VOICE_COMMUNICATION) == 0)
        m_recorderPreset = SL_ANDROID_RECORDING_PRESET_VOICE_COMMUNICATION;
    else
        m_recorderPreset = SL_ANDROID_RECORDING_PRESET_GENERIC;
#endif
}

QAndroidAudioSource::~QAndroidAudioSource()
{
    if (m_recorderObject)
        (*m_recorderObject)->Destroy(m_recorderObject);
    delete[] m_buffers;
}

QAudio::Error QAndroidAudioSource::error() const
{
    return m_errorState;
}

QAudio::State QAndroidAudioSource::state() const
{
    return m_deviceState;
}

void QAndroidAudioSource::setFormat(const QAudioFormat &format)
{
    if (m_deviceState == QAudio::StoppedState)
        m_format = format;
}

QAudioFormat QAndroidAudioSource::format() const
{
    return m_format;
}

void QAndroidAudioSource::start(QIODevice *device)
{
    if (m_deviceState != QAudio::StoppedState)
        stopRecording();

    if (!m_pullMode && m_bufferIODevice) {
        m_bufferIODevice->close();
        delete m_bufferIODevice;
        m_bufferIODevice = 0;
    }

    m_pullMode = true;
    m_audioSource = device;

    if (startRecording()) {
        m_deviceState = QAudio::ActiveState;
    } else {
        m_deviceState = QAudio::StoppedState;
        Q_EMIT errorChanged(m_errorState);
    }

    Q_EMIT stateChanged(m_deviceState);
}

QIODevice *QAndroidAudioSource::start()
{
    if (m_deviceState != QAudio::StoppedState)
        stopRecording();

    m_audioSource = 0;

    if (!m_pullMode && m_bufferIODevice) {
        m_bufferIODevice->close();
        delete m_bufferIODevice;
    }

    m_pullMode = false;
    m_pushBuffer.clear();
    m_bufferIODevice = new QBuffer(&m_pushBuffer, this);
    m_bufferIODevice->open(QIODevice::ReadOnly);

    if (startRecording()) {
        m_deviceState = QAudio::IdleState;
    } else {
        m_deviceState = QAudio::StoppedState;
        Q_EMIT errorChanged(m_errorState);
        m_bufferIODevice->close();
        delete m_bufferIODevice;
        m_bufferIODevice = 0;
    }

    Q_EMIT stateChanged(m_deviceState);
    return m_bufferIODevice;
}

bool QAndroidAudioSource::startRecording()
{
    if (!hasRecordingPermission())
        return false;

    m_processedBytes = 0;
    m_lastNotifyTime = 0;

    SLresult result;

    // configure audio source
    SLDataLocator_IODevice loc_dev = { SL_DATALOCATOR_IODEVICE, SL_IODEVICE_AUDIOINPUT,
                                       SL_DEFAULTDEVICEID_AUDIOINPUT, NULL };
    SLDataSource audioSrc = { &loc_dev, NULL };

    // configure audio sink
#ifdef ANDROID
    SLDataLocator_AndroidSimpleBufferQueue loc_bq = { SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
                                                      NUM_BUFFERS };
#else
    SLDataLocator_BufferQueue loc_bq = { SL_DATALOCATOR_BUFFERQUEUE, NUM_BUFFERS };
#endif

    SLAndroidDataFormat_PCM_EX format_pcm = QOpenSLESEngine::audioFormatToSLFormatPCM(m_format);
    SLDataSink audioSnk = { &loc_bq, &format_pcm };

    // create audio recorder
    // (requires the RECORD_AUDIO permission)
#ifdef ANDROID
    const SLInterfaceID id[2] = { SL_IID_ANDROIDSIMPLEBUFFERQUEUE, SL_IID_ANDROIDCONFIGURATION };
    const SLboolean req[2] = { SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE };
#else
    const SLInterfaceID id[1] = { SL_IID_BUFFERQUEUE };
    const SLboolean req[1] = { SL_BOOLEAN_TRUE };
#endif

    result = (*m_engine->slEngine())->CreateAudioRecorder(m_engine->slEngine(), &m_recorderObject,
                                                          &audioSrc, &audioSnk,
                                                          sizeof(req) / sizeof(SLboolean), id, req);
    if (result != SL_RESULT_SUCCESS) {
        m_errorState = QAudio::OpenError;
        return false;
    }

#ifdef ANDROID
    // configure recorder source
    SLAndroidConfigurationItf configItf;
    result = (*m_recorderObject)->GetInterface(m_recorderObject, SL_IID_ANDROIDCONFIGURATION,
                                               &configItf);
    if (result != SL_RESULT_SUCCESS) {
        m_errorState = QAudio::OpenError;
        return false;
    }

    result = (*configItf)->SetConfiguration(configItf, SL_ANDROID_KEY_RECORDING_PRESET,
                                            &m_recorderPreset, sizeof(SLuint32));

    SLuint32 presetValue = SL_ANDROID_RECORDING_PRESET_NONE;
    SLuint32 presetSize = 2*sizeof(SLuint32); // intentionally too big
    result = (*configItf)->GetConfiguration(configItf, SL_ANDROID_KEY_RECORDING_PRESET,
                                            &presetSize, (void*)&presetValue);

    if (result != SL_RESULT_SUCCESS || presetValue == SL_ANDROID_RECORDING_PRESET_NONE) {
        m_errorState = QAudio::OpenError;
        return false;
    }
#endif

    // realize the audio recorder
    result = (*m_recorderObject)->Realize(m_recorderObject, SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS) {
        m_errorState = QAudio::OpenError;
        return false;
    }

    // get the record interface
    result = (*m_recorderObject)->GetInterface(m_recorderObject, SL_IID_RECORD, &m_recorder);
    if (result != SL_RESULT_SUCCESS) {
        m_errorState = QAudio::FatalError;
        return false;
    }

    // get the buffer queue interface
#ifdef ANDROID
    SLInterfaceID bufferqueueItfID = SL_IID_ANDROIDSIMPLEBUFFERQUEUE;
#else
    SLInterfaceID bufferqueueItfID = SL_IID_BUFFERQUEUE;
#endif
    result = (*m_recorderObject)->GetInterface(m_recorderObject, bufferqueueItfID, &m_bufferQueue);
    if (result != SL_RESULT_SUCCESS) {
        m_errorState = QAudio::FatalError;
        return false;
    }

    // register callback on the buffer queue
    result = (*m_bufferQueue)->RegisterCallback(m_bufferQueue, bufferQueueCallback, this);
    if (result != SL_RESULT_SUCCESS) {
        m_errorState = QAudio::FatalError;
        return false;
    }

    if (m_bufferSize <= 0) {
        m_bufferSize = m_format.bytesForDuration(DEFAULT_PERIOD_TIME_MS * 1000);
    } else {
        int minimumBufSize = m_format.bytesForDuration(MINIMUM_PERIOD_TIME_MS * 1000);
        if (m_bufferSize < minimumBufSize)
            m_bufferSize = minimumBufSize;
    }

    // enqueue empty buffers to be filled by the recorder
    for (int i = 0; i < NUM_BUFFERS; ++i) {
        m_buffers[i].resize(m_bufferSize);

        result = (*m_bufferQueue)->Enqueue(m_bufferQueue, m_buffers[i].data(), m_bufferSize);
        if (result != SL_RESULT_SUCCESS) {
            m_errorState = QAudio::FatalError;
            return false;
        }
    }

    // start recording
    result = (*m_recorder)->SetRecordState(m_recorder, SL_RECORDSTATE_RECORDING);
    if (result != SL_RESULT_SUCCESS) {
        m_errorState = QAudio::FatalError;
        return false;
    }

    m_errorState = QAudio::NoError;

    return true;
}

void QAndroidAudioSource::stop()
{
    if (m_deviceState == QAudio::StoppedState)
        return;

    m_deviceState = QAudio::StoppedState;

    stopRecording();

    m_errorState = QAudio::NoError;
    Q_EMIT stateChanged(m_deviceState);
}

void QAndroidAudioSource::stopRecording()
{
    flushBuffers();

    (*m_recorder)->SetRecordState(m_recorder, SL_RECORDSTATE_STOPPED);
    (*m_bufferQueue)->Clear(m_bufferQueue);

    (*m_recorderObject)->Destroy(m_recorderObject);
    m_recorderObject = 0;

    for (int i = 0; i < NUM_BUFFERS; ++i)
        m_buffers[i].clear();
    m_currentBuffer = 0;

    if (!m_pullMode && m_bufferIODevice) {
        m_bufferIODevice->close();
        delete m_bufferIODevice;
        m_bufferIODevice = 0;
        m_pushBuffer.clear();
    }
}

void QAndroidAudioSource::suspend()
{
    if (m_deviceState == QAudio::ActiveState) {
        m_deviceState = QAudio::SuspendedState;
        emit stateChanged(m_deviceState);

        (*m_recorder)->SetRecordState(m_recorder, SL_RECORDSTATE_PAUSED);
    }
}

void QAndroidAudioSource::resume()
{
    if (m_deviceState == QAudio::SuspendedState || m_deviceState == QAudio::IdleState) {
        (*m_recorder)->SetRecordState(m_recorder, SL_RECORDSTATE_RECORDING);

        m_deviceState = QAudio::ActiveState;
        emit stateChanged(m_deviceState);
    }
}

void QAndroidAudioSource::processBuffer()
{
    if (m_deviceState == QAudio::StoppedState || m_deviceState == QAudio::SuspendedState)
        return;

    if (m_deviceState != QAudio::ActiveState) {
        m_errorState = QAudio::NoError;
        m_deviceState = QAudio::ActiveState;
        emit stateChanged(m_deviceState);
    }

    QByteArray *processedBuffer = &m_buffers[m_currentBuffer];
    writeDataToDevice(processedBuffer->constData(), processedBuffer->size());

    // Re-enqueue the buffer
    SLresult result = (*m_bufferQueue)->Enqueue(m_bufferQueue,
                                                processedBuffer->data(),
                                                processedBuffer->size());

    m_currentBuffer = (m_currentBuffer + 1) % NUM_BUFFERS;

    // If the buffer queue is empty (shouldn't happen), stop recording.
#ifdef ANDROID
    SLAndroidSimpleBufferQueueState state;
#else
    SLBufferQueueState state;
#endif
    result = (*m_bufferQueue)->GetState(m_bufferQueue, &state);
    if (result != SL_RESULT_SUCCESS || state.count == 0) {
        stop();
        m_errorState = QAudio::FatalError;
        Q_EMIT errorChanged(m_errorState);
    }
}

void QAndroidAudioSource::writeDataToDevice(const char *data, int size)
{
    m_processedBytes += size;

    QByteArray outData;

    // Apply volume
    if (m_volume < 1.0f) {
        outData.resize(size);
        QAudioHelperInternal::qMultiplySamples(m_volume, m_format, data, outData.data(), size);
    } else {
        outData.append(data, size);
    }

    if (m_pullMode) {
        // write buffer to the QIODevice
        if (m_audioSource->write(outData) < 0) {
            stop();
            m_errorState = QAudio::IOError;
            Q_EMIT errorChanged(m_errorState);
        }
    } else {
        // emits readyRead() so user will call read() on QIODevice to get some audio data
        if (m_bufferIODevice != 0) {
            m_pushBuffer.append(outData);
            Q_EMIT m_bufferIODevice->readyRead();
        }
    }
}

void QAndroidAudioSource::flushBuffers()
{
    SLmillisecond recorderPos;
    (*m_recorder)->GetPosition(m_recorder, &recorderPos);
    qint64 devicePos = processedUSecs();

    qint64 delta = recorderPos * 1000 - devicePos;

    if (delta > 0) {
        const int writeSize = qMin(m_buffers[m_currentBuffer].size(),
                                       m_format.bytesForDuration(delta));
        writeDataToDevice(m_buffers[m_currentBuffer].constData(), writeSize);
    }
}

qsizetype QAndroidAudioSource::bytesReady() const
{
    if (m_deviceState == QAudio::ActiveState || m_deviceState == QAudio::SuspendedState)
        return m_bufferIODevice ? m_bufferIODevice->bytesAvailable() : m_bufferSize;

    return 0;
}

void QAndroidAudioSource::setBufferSize(qsizetype value)
{
    m_bufferSize = value;
}

qsizetype QAndroidAudioSource::bufferSize() const
{
    return m_bufferSize;
}

qint64 QAndroidAudioSource::processedUSecs() const
{
    return m_format.durationForBytes(m_processedBytes);
}

void QAndroidAudioSource::setVolume(qreal vol)
{
    m_volume = vol;
}

qreal QAndroidAudioSource::volume() const
{
    return m_volume;
}

void QAndroidAudioSource::reset()
{
    stop();
}

QT_END_NAMESPACE
