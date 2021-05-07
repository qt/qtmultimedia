/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd and/or its subsidiary(-ies).
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "qcoreaudiooutput_p.h"
#include "qcoreaudiosessionmanager_p.h"
#include "qcoreaudiodeviceinfo_p.h"
#include "qcoreaudioutils_p.h"
#include <private/qdarwinmediadevices_p.h>
#include <qmediadevices.h>

#include <QtCore/QDataStream>
#include <QtCore/QTimer>
#include <QtCore/QDebug>

#include <AudioUnit/AudioUnit.h>
#include <AudioToolbox/AudioToolbox.h>
#if defined(Q_OS_OSX)
# include <AudioUnit/AudioComponent.h>
#endif

#if defined(Q_OS_IOS) || defined(Q_OS_TVOS)
# include <QtMultimedia/private/qaudiohelpers_p.h>
#endif

QT_BEGIN_NAMESPACE

static const int DEFAULT_BUFFER_SIZE = 8 * 1024;

QCoreAudioOutputBuffer::QCoreAudioOutputBuffer(int bufferSize, int maxPeriodSize, const QAudioFormat &audioFormat)
    : m_deviceError(false)
    , m_maxPeriodSize(maxPeriodSize)
    , m_device(0)
{
    m_buffer = new CoreAudioRingBuffer(bufferSize + (bufferSize % maxPeriodSize == 0 ? 0 : maxPeriodSize - (bufferSize % maxPeriodSize)));
    m_bytesPerFrame = audioFormat.bytesPerFrame();
    m_periodTime = m_maxPeriodSize / m_bytesPerFrame * 1000 / audioFormat.sampleRate();

    m_fillTimer = new QTimer(this);
    connect(m_fillTimer, SIGNAL(timeout()), SLOT(fillBuffer()));
}

QCoreAudioOutputBuffer::~QCoreAudioOutputBuffer()
{
    delete m_buffer;
}

qint64 QCoreAudioOutputBuffer::readFrames(char *data, qint64 maxFrames)
{
    bool    wecan = true;
    qint64  framesRead = 0;

    while (wecan && framesRead < maxFrames) {
        CoreAudioRingBuffer::Region region = m_buffer->acquireReadRegion((maxFrames - framesRead) * m_bytesPerFrame);

        if (region.second > 0) {
            // Ensure that we only read whole frames.
            region.second -= region.second % m_bytesPerFrame;

            if (region.second > 0) {
                memcpy(data + (framesRead * m_bytesPerFrame), region.first, region.second);
                framesRead += region.second / m_bytesPerFrame;
            } else
                wecan = false; // If there is only a partial frame left we should exit.
        }
        else
            wecan = false;

        m_buffer->releaseReadRegion(region);
    }

    if (framesRead == 0 && m_deviceError)
        framesRead = -1;

    return framesRead;
}

qint64 QCoreAudioOutputBuffer::writeBytes(const char *data, qint64 maxSize)
{
    bool    wecan = true;
    qint64  bytesWritten = 0;

    maxSize -= maxSize % m_bytesPerFrame;
    while (wecan && bytesWritten < maxSize) {
        CoreAudioRingBuffer::Region region = m_buffer->acquireWriteRegion(maxSize - bytesWritten);

        if (region.second > 0) {
            memcpy(region.first, data + bytesWritten, region.second);
            bytesWritten += region.second;
        }
        else
            wecan = false;

        m_buffer->releaseWriteRegion(region);
    }

    if (bytesWritten > 0)
        emit readyRead();

    return bytesWritten;
}

int QCoreAudioOutputBuffer::available() const
{
    return m_buffer->free();
}

void QCoreAudioOutputBuffer::reset()
{
    m_buffer->reset();
    m_device = 0;
    m_deviceError = false;
}

void QCoreAudioOutputBuffer::setPrefetchDevice(QIODevice *device)
{
    if (m_device != device) {
        m_device = device;
        if (m_device != 0)
            fillBuffer();
    }
}

void QCoreAudioOutputBuffer::startFillTimer()
{
    if (m_device != 0)
        m_fillTimer->start(m_buffer->size() / 2 / m_maxPeriodSize * m_periodTime);
}

void QCoreAudioOutputBuffer::stopFillTimer()
{
    m_fillTimer->stop();
}

void QCoreAudioOutputBuffer::fillBuffer()
{
    const int free = m_buffer->free();
    const int writeSize = free - (free % m_maxPeriodSize);

    if (writeSize > 0) {
        bool    wecan = true;
        int     filled = 0;

        while (!m_deviceError && wecan && filled < writeSize) {
            CoreAudioRingBuffer::Region region = m_buffer->acquireWriteRegion(writeSize - filled);

            if (region.second > 0) {
                region.second = m_device->read(region.first, region.second);
                if (region.second > 0)
                    filled += region.second;
                else if (region.second == 0)
                    wecan = false;
                else if (region.second < 0) {
                    m_fillTimer->stop();
                    region.second = 0;
                    m_deviceError = true;
                }
            }
            else
                wecan = false;

            m_buffer->releaseWriteRegion(region);
        }

        if (filled > 0)
            emit readyRead();
    }
}

QCoreAudioOutputDevice::QCoreAudioOutputDevice(QCoreAudioOutputBuffer *audioBuffer, QObject *parent)
    : QIODevice(parent)
    , m_audioBuffer(audioBuffer)
{
    open(QIODevice::WriteOnly | QIODevice::Unbuffered);
}

qint64 QCoreAudioOutputDevice::readData(char *data, qint64 len)
{
    Q_UNUSED(data);
    Q_UNUSED(len);

    return 0;
}

qint64 QCoreAudioOutputDevice::writeData(const char *data, qint64 len)
{
    return m_audioBuffer->writeBytes(data, len);
}

QCoreAudioOutput::QCoreAudioOutput(const QAudioDeviceInfo &device)
    : m_audioDeviceInfo(device)
    , m_isOpen(false)
    , m_internalBufferSize(DEFAULT_BUFFER_SIZE)
    , m_totalFrames(0)
    , m_audioIO(0)
    , m_audioUnit(0)
    , m_audioBuffer(0)
    , m_cachedVolume(1.0)
    , m_volume(1.0)
    , m_pullMode(false)
    , m_errorCode(QAudio::NoError)
    , m_stateCode(QAudio::StoppedState)
{
    QAudioDeviceInfo di = device;
    if (di.isNull())
        di = QMediaDevices::defaultAudioOutput();
#if defined(Q_OS_MACOS)
    const QCoreAudioDeviceInfo *info = static_cast<const QCoreAudioDeviceInfo *>(di.handle());
    Q_ASSERT(info);
    m_audioDeviceId = info->deviceID();
#endif
    m_device = di.id();

    m_clockFrequency = CoreAudioUtils::frequency() / 1000;
    m_audioThreadState.storeRelaxed(Stopped);
}

QCoreAudioOutput::~QCoreAudioOutput()
{
    close();
}

void QCoreAudioOutput::start(QIODevice *device)
{
    QIODevice* op = device;

    if (!m_audioDeviceInfo.isFormatSupported(m_audioFormat) || !open()) {
        m_stateCode = QAudio::StoppedState;
        m_errorCode = QAudio::OpenError;
        return;
    }

    reset();
    m_audioBuffer->reset();
    m_audioBuffer->setPrefetchDevice(op);

    if (op == 0) {
        op = m_audioIO;
        m_stateCode = QAudio::IdleState;
    }
    else
        m_stateCode = QAudio::ActiveState;

    // Start
    m_pullMode = true;
    m_errorCode = QAudio::NoError;
    m_totalFrames = 0;

    if (m_stateCode == QAudio::ActiveState)
        audioThreadStart();

    emit stateChanged(m_stateCode);
}

QIODevice *QCoreAudioOutput::start()
{
    if (!m_audioDeviceInfo.isFormatSupported(m_audioFormat) || !open()) {
        m_stateCode = QAudio::StoppedState;
        m_errorCode = QAudio::OpenError;
        return m_audioIO;
    }

    reset();
    m_audioBuffer->reset();
    m_audioBuffer->setPrefetchDevice(0);

    m_stateCode = QAudio::IdleState;

    // Start
    m_pullMode = false;
    m_errorCode = QAudio::NoError;
    m_totalFrames = 0;

    emit stateChanged(m_stateCode);

    return m_audioIO;
}

void QCoreAudioOutput::stop()
{
    QMutexLocker lock(&m_mutex);
    if (m_stateCode != QAudio::StoppedState) {
        audioThreadDrain();

        m_stateCode = QAudio::StoppedState;
        m_errorCode = QAudio::NoError;
        emit stateChanged(m_stateCode);
    }
}

void QCoreAudioOutput::reset()
{
    QMutexLocker lock(&m_mutex);
    if (m_stateCode != QAudio::StoppedState) {
        audioThreadStop();

        m_stateCode = QAudio::StoppedState;
        m_errorCode = QAudio::NoError;
        emit stateChanged(m_stateCode);
    }
}

void QCoreAudioOutput::suspend()
{
    QMutexLocker lock(&m_mutex);
    if (m_stateCode == QAudio::ActiveState || m_stateCode == QAudio::IdleState) {
        audioThreadStop();

        m_stateCode = QAudio::SuspendedState;
        m_errorCode = QAudio::NoError;
        emit stateChanged(m_stateCode);
    }
}

void QCoreAudioOutput::resume()
{
    QMutexLocker lock(&m_mutex);
    if (m_stateCode == QAudio::SuspendedState) {
        audioThreadStart();

        m_stateCode = m_pullMode ? QAudio::ActiveState : QAudio::IdleState;
        m_errorCode = QAudio::NoError;
        emit stateChanged(m_stateCode);
    }
}

qsizetype QCoreAudioOutput::bytesFree() const
{
    return m_audioBuffer->available();
}

int QCoreAudioOutput::periodSize() const
{
    return m_periodSizeBytes;
}

void QCoreAudioOutput::setBufferSize(qsizetype value)
{
    if (m_stateCode == QAudio::StoppedState)
        m_internalBufferSize = value;
}

qsizetype QCoreAudioOutput::bufferSize() const
{
    return m_internalBufferSize;
}

qint64 QCoreAudioOutput::processedUSecs() const
{
    return m_totalFrames * 1000000 / m_audioFormat.sampleRate();
}

QAudio::Error QCoreAudioOutput::error() const
{
    return m_errorCode;
}

QAudio::State QCoreAudioOutput::state() const
{
    return m_stateCode;
}

void QCoreAudioOutput::setFormat(const QAudioFormat &format)
{
    if (m_stateCode == QAudio::StoppedState)
        m_audioFormat = format;
}

QAudioFormat QCoreAudioOutput::format() const
{
    return m_audioFormat;
}

void QCoreAudioOutput::setVolume(qreal volume)
{
    m_cachedVolume = qBound(qreal(0.0), volume, qreal(1.0));
    if (!m_isOpen)
        return;

#if defined(Q_OS_OSX)
    //on OS X the volume can be set directly on the AudioUnit
    if (AudioUnitSetParameter(m_audioUnit,
                              kHALOutputParam_Volume,
                              kAudioUnitScope_Global,
                              0 /* bus */,
                              m_cachedVolume,
                              0) == noErr)
        m_volume = m_cachedVolume;
#endif
}

qreal QCoreAudioOutput::volume() const
{
    return m_cachedVolume;
}

void QCoreAudioOutput::setCategory(const QString &category)
{
    Q_UNUSED(category);
}

QString QCoreAudioOutput::category() const
{
    return QString();
}

void QCoreAudioOutput::deviceStopped()
{
    emit stateChanged(m_stateCode);
}

void QCoreAudioOutput::inputReady()
{
    QMutexLocker lock(&m_mutex);
    if (m_stateCode == QAudio::IdleState) {
        audioThreadStart();

        m_stateCode = QAudio::ActiveState;
        m_errorCode = QAudio::NoError;

        emit stateChanged(m_stateCode);
    }
}

OSStatus QCoreAudioOutput::renderCallback(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData)
{
    Q_UNUSED(ioActionFlags);
    Q_UNUSED(inTimeStamp);
    Q_UNUSED(inBusNumber);
    Q_UNUSED(inNumberFrames);

    QCoreAudioOutput* d = static_cast<QCoreAudioOutput*>(inRefCon);

    const int threadState = d->m_audioThreadState.fetchAndAddAcquire(0);
    if (threadState == Stopped) {
        ioData->mBuffers[0].mDataByteSize = 0;
        d->audioDeviceStop();
    }
    else {
        const UInt32 bytesPerFrame = d->m_streamFormat.mBytesPerFrame;
        qint64 framesRead;

        framesRead = d->m_audioBuffer->readFrames((char*)ioData->mBuffers[0].mData,
                                                 ioData->mBuffers[0].mDataByteSize / bytesPerFrame);

        if (framesRead > 0) {
            ioData->mBuffers[0].mDataByteSize = framesRead * bytesPerFrame;
            d->m_totalFrames += framesRead;

#if defined(Q_OS_MACOS)
            // If playback is already stopped.
            if (threadState != Running) {
                qreal oldVolume = d->m_cachedVolume;
                // Decrease volume smoothly.
                d->setVolume(d->m_volume / 2);
                d->m_cachedVolume = oldVolume;
            }
#elif defined(Q_OS_IOS) || defined(Q_OS_TVOS)
            // on iOS we have to adjust the sound volume ourselves
            if (!qFuzzyCompare(d->m_cachedVolume, qreal(1.0f))) {
                QAudioHelperInternal::qMultiplySamples(d->m_cachedVolume,
                                                       d->m_audioFormat,
                                                       ioData->mBuffers[0].mData, /* input */
                                                       ioData->mBuffers[0].mData, /* output */
                                                       ioData->mBuffers[0].mDataByteSize);
            }
#endif

        }
        else {
            ioData->mBuffers[0].mDataByteSize = 0;
            if (framesRead == 0) {
                if (threadState == Draining)
                    d->audioDeviceStop();
                else
                    d->audioDeviceIdle();
            }
            else
                d->audioDeviceError();
        }
    }

    return noErr;
}

bool QCoreAudioOutput::open()
{
#if defined(Q_OS_IOS)
    // Set default category to Ambient (implies MixWithOthers). This makes sure audio stops playing
    // if the screen is locked or if the Silent switch is toggled.
    CoreAudioSessionManager::instance().setCategory(CoreAudioSessionManager::Ambient, CoreAudioSessionManager::None);
    CoreAudioSessionManager::instance().setActive(true);
#endif

    if (m_errorCode != QAudio::NoError)
        return false;

    if (m_isOpen) {
        setVolume(m_cachedVolume);
        return true;
    }

    AudioComponentDescription componentDescription;
    componentDescription.componentType = kAudioUnitType_Output;
#if defined(Q_OS_OSX)
    componentDescription.componentSubType = kAudioUnitSubType_HALOutput;
#else
    componentDescription.componentSubType = kAudioUnitSubType_RemoteIO;
#endif
    componentDescription.componentManufacturer = kAudioUnitManufacturer_Apple;
    componentDescription.componentFlags = 0;
    componentDescription.componentFlagsMask = 0;

    AudioComponent component = AudioComponentFindNext(0, &componentDescription);
    if (component == 0) {
        qWarning() << "QAudioOutput: Failed to find Output component";
        return false;
    }

    if (AudioComponentInstanceNew(component, &m_audioUnit) != noErr) {
        qWarning() << "QAudioOutput: Unable to Open Output Component";
        return false;
    }

    // register callback
    AURenderCallbackStruct callback;
    callback.inputProc = renderCallback;
    callback.inputProcRefCon = this;

    if (AudioUnitSetProperty(m_audioUnit,
                             kAudioUnitProperty_SetRenderCallback,
                             kAudioUnitScope_Global,
                             0,
                             &callback,
                             sizeof(callback)) != noErr) {
        qWarning() << "QAudioOutput: Failed to set AudioUnit callback";
        return false;
    }

#if defined(Q_OS_OSX)
    //Set Audio Device
    if (AudioUnitSetProperty(m_audioUnit,
                             kAudioOutputUnitProperty_CurrentDevice,
                             kAudioUnitScope_Global,
                             0,
                             &m_audioDeviceId,
                             sizeof(m_audioDeviceId)) != noErr) {
        qWarning() << "QAudioOutput: Unable to use configured device";
        return false;
    }
#endif

    // Set stream format
    m_streamFormat = CoreAudioUtils::toAudioStreamBasicDescription(m_audioFormat);

    UInt32 size = sizeof(m_streamFormat);
    if (AudioUnitSetProperty(m_audioUnit,
                                kAudioUnitProperty_StreamFormat,
                                kAudioUnitScope_Input,
                                0,
                                &m_streamFormat,
                                size) != noErr) {
        qWarning() << "QAudioOutput: Unable to Set Stream information";
        return false;
    }

    // Allocate buffer
    UInt32 numberOfFrames = 0;
#if defined(Q_OS_OSX)
    size = sizeof(UInt32);
    if (AudioUnitGetProperty(m_audioUnit,
                             kAudioDevicePropertyBufferFrameSize,
                             kAudioUnitScope_Global,
                             0,
                             &numberOfFrames,
                             &size) != noErr) {
        qWarning() << "QAudioInput: Failed to get audio period size";
        return false;
    }
#else //iOS
    Float32 bufferSize = CoreAudioSessionManager::instance().currentIOBufferDuration();
    bufferSize *= m_streamFormat.mSampleRate;
    numberOfFrames = bufferSize;
#endif

    m_periodSizeBytes = numberOfFrames * m_streamFormat.mBytesPerFrame;
    if (m_internalBufferSize < m_periodSizeBytes * 2)
        m_internalBufferSize = m_periodSizeBytes * 2;
    else
        m_internalBufferSize -= m_internalBufferSize % m_streamFormat.mBytesPerFrame;

    m_audioBuffer = new QCoreAudioOutputBuffer(m_internalBufferSize, m_periodSizeBytes, m_audioFormat);
    connect(m_audioBuffer, SIGNAL(readyRead()), SLOT(inputReady())); //Pull

    m_audioIO = new QCoreAudioOutputDevice(m_audioBuffer, this);

    //Init
    if (AudioUnitInitialize(m_audioUnit)) {
        qWarning() << "QAudioOutput: Failed to initialize AudioUnit";
        return false;
    }

    m_isOpen = true;

    setVolume(m_cachedVolume);

    return true;
}

void QCoreAudioOutput::close()
{
    if (m_audioUnit != 0) {
        AudioOutputUnitStop(m_audioUnit);
        AudioUnitUninitialize(m_audioUnit);
        AudioComponentInstanceDispose(m_audioUnit);
    }

    delete m_audioBuffer;
}

void QCoreAudioOutput::audioThreadStart()
{
    startTimers();
    m_audioThreadState.storeRelaxed(Running);
    AudioOutputUnitStart(m_audioUnit);
}

void QCoreAudioOutput::audioThreadStop()
{
    stopTimers();
    if (m_audioThreadState.testAndSetAcquire(Running, Stopped))
        m_threadFinished.wait(&m_mutex, 500);
}

void QCoreAudioOutput::audioThreadDrain()
{
    stopTimers();
    if (m_audioThreadState.testAndSetAcquire(Running, Draining))
        m_threadFinished.wait(&m_mutex, 500);
}

void QCoreAudioOutput::audioDeviceStop()
{
    AudioOutputUnitStop(m_audioUnit);
    m_audioThreadState.storeRelaxed(Stopped);
    m_threadFinished.wakeOne();
}

void QCoreAudioOutput::audioDeviceIdle()
{
    if (m_stateCode == QAudio::ActiveState) {
        QMutexLocker lock(&m_mutex);
        audioDeviceStop();

        m_errorCode = QAudio::UnderrunError;
        m_stateCode = QAudio::IdleState;
        QMetaObject::invokeMethod(this, "deviceStopped", Qt::QueuedConnection);
    }
}

void QCoreAudioOutput::audioDeviceError()
{
    if (m_stateCode == QAudio::ActiveState) {
        QMutexLocker lock(&m_mutex);
        audioDeviceStop();

        m_errorCode = QAudio::IOError;
        m_stateCode = QAudio::StoppedState;
        QMetaObject::invokeMethod(this, "deviceStopped", Qt::QueuedConnection);
    }
}

void QCoreAudioOutput::startTimers()
{
    m_audioBuffer->startFillTimer();
}

void QCoreAudioOutput::stopTimers()
{
    m_audioBuffer->stopFillTimer();
}

QT_END_NAMESPACE

#include "moc_qcoreaudiooutput_p.cpp"
