/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// INTERNAL USE ONLY: Do NOT use for any other purpose.
//

#include "qwindowsaudiosink_p.h"
#include "qwindowsaudiodevice_p.h"
#include "qwindowsaudioutils_p.h"
#include <QtEndian>
#include <QtCore/QDataStream>
#include <QtCore/qtimer.h>
#include <private/qaudiohelpers_p.h>

#include <qloggingcategory.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(qLcAudioOutput, "qt.multimedia.audiooutput")

QWindowsAudioSink::QWindowsAudioSink(int deviceId)
{
    bytesAvailable = 0;
    buffer_size = 0;
    period_size = 0;
    m_deviceId = deviceId;
    totalTimeValue = 0;
    errorState = QAudio::NoError;
    deviceState = QAudio::StoppedState;
    audioSource = 0;
    pullMode = true;
    volumeCache = qreal(1.0);
    blocks_count = 5;
}

QWindowsAudioSink::~QWindowsAudioSink()
{
    close();
}

void CALLBACK QWindowsAudioSink::waveOutProc( HWAVEOUT hWaveOut, UINT uMsg,
        DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2 )
{
    Q_UNUSED(dwParam1);
    Q_UNUSED(dwParam2);
    Q_UNUSED(hWaveOut);

    QWindowsAudioSink* qAudio;
    qAudio = (QWindowsAudioSink*)(dwInstance);
    if(!qAudio)
        return;

    QMutexLocker locker(&qAudio->mutex);

    switch(uMsg) {
        case WOM_OPEN:
            qAudio->feedback();
            break;
        case WOM_CLOSE:
            return;
        case WOM_DONE:
            if(qAudio->buffer_size == 0 || qAudio->period_size == 0) {
                return;
            }
            qAudio->waveFreeBlockCount++;
            if (qAudio->waveFreeBlockCount >= qAudio->blocks_count)
                qAudio->waveFreeBlockCount = qAudio->blocks_count;

            qAudio->feedback();
            break;
        default:
            return;
    }
}

WAVEHDR* QWindowsAudioSink::allocateBlocks(int size, int count)
{
    int i;
    unsigned char* buffer;
    WAVEHDR* blocks;
    DWORD totalBufferSize = (size + sizeof(WAVEHDR))*count;

    if((buffer=(unsigned char*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,
                    totalBufferSize)) == 0) {
        qWarning("QAudioSink: Memory allocation error");
        return 0;
    }
    blocks = (WAVEHDR*)buffer;
    buffer += sizeof(WAVEHDR)*count;
    for(i = 0; i < count; i++) {
        blocks[i].dwBufferLength = size;
        blocks[i].lpData = (LPSTR)buffer;
        buffer += size;
    }
    return blocks;
}

void QWindowsAudioSink::freeBlocks(WAVEHDR* blockArray)
{
    WAVEHDR* blocks = blockArray;
    for (int i = 0; i < blocks_count; ++i) {
        waveOutUnprepareHeader(hWaveOut,blocks, sizeof(WAVEHDR));
        blocks++;
    }
    HeapFree(GetProcessHeap(), 0, blockArray);
}

QAudioFormat QWindowsAudioSink::format() const
{
    return settings;
}

void QWindowsAudioSink::setFormat(const QAudioFormat& fmt)
{
    if (deviceState == QAudio::StoppedState)
        settings = fmt;
}

void QWindowsAudioSink::start(QIODevice* device)
{
    qCDebug(qLcAudioOutput) << "start(ioDevice)";
    if(deviceState != QAudio::StoppedState)
        close();

    if(!pullMode && audioSource)
        delete audioSource;

    pullMode = true;
    audioSource = device;

    deviceState = QAudio::ActiveState;

    if(!open())
        return;

    emit stateChanged(deviceState);
}

QIODevice* QWindowsAudioSink::start()
{
    qCDebug(qLcAudioOutput) << "start()";
    if(deviceState != QAudio::StoppedState)
        close();

    if(!pullMode && audioSource)
        delete audioSource;

    pullMode = false;
    audioSource = new OutputPrivate(this);
    audioSource->open(QIODevice::WriteOnly|QIODevice::Unbuffered);

    deviceState = QAudio::IdleState;

    if(!open())
        return 0;

    emit stateChanged(deviceState);

    return audioSource;
}

void QWindowsAudioSink::stop()
{
    qCDebug(qLcAudioOutput) << "stop()";
    if(deviceState == QAudio::StoppedState)
        return;
    close();
    if(!pullMode && audioSource) {
        delete audioSource;
        audioSource = 0;
    }
    emit stateChanged(deviceState);
}

bool QWindowsAudioSink::open()
{
    qCDebug(qLcAudioOutput) << "open()";

    period_size = 0;

    if (!QWindowsAudioUtils::formatToWaveFormatExtensible(settings, wfx)) {
        qWarning("QAudioSink: open error, invalid format.");
    } else if (buffer_size == 0) {
        // Default buffer size, 200ms, default period size is 40ms
        buffer_size
                = (settings.sampleRate()
                * settings.bytesPerFrame()
                + 39) / 5;
        period_size = buffer_size / 5;
    } else {
        period_size = buffer_size / 5;
    }

    // Make even size of wave block to prevent crackling
    // due to waveOutWrite() does not like odd buffer length
    period_size &= ~1;

    if (period_size == 0) {
        errorState = QAudio::OpenError;
        deviceState = QAudio::StoppedState;
        emit stateChanged(deviceState);
        return false;
    }

    const int periods = buffer_size / period_size;
    bool ok = false;
    static int wave_buffers = qEnvironmentVariableIntValue("QT_WAVE_BUFFERS", &ok);
    if (wave_buffers < periods) {
        if (ok)
            qWarning("Number of WAVE buffers (QT_WAVE_BUFFERS=%d) cannot be less than %d.", wave_buffers, periods);
        wave_buffers = periods;
    }

    blocks_count = wave_buffers;
    waveBlocks = allocateBlocks(period_size, blocks_count);

    mutex.lock();
    waveFreeBlockCount = blocks_count;
    mutex.unlock();

    waveCurrentBlock = 0;

    if (audioBuffer == nullptr)
        audioBuffer = new char[blocks_count * period_size];

    elapsedTimeOffset = 0;

    if (waveOutOpen(&hWaveOut, UINT_PTR(m_deviceId), &wfx.Format,
                    (DWORD_PTR)&waveOutProc,
                    (DWORD_PTR) this,
                    CALLBACK_FUNCTION) != MMSYSERR_NOERROR) {
        errorState = QAudio::OpenError;
        deviceState = QAudio::StoppedState;
        emit stateChanged(deviceState);
        qWarning("QAudioSink: open error");
        return false;
    }

    totalTimeValue = 0;
    elapsedTimeOffset = 0;

    errorState = QAudio::NoError;
    if(pullMode) {
        deviceState = QAudio::ActiveState;
        QTimer::singleShot(10, this, SLOT(feedback()));
    } else
        deviceState = QAudio::IdleState;

    return true;
}

void QWindowsAudioSink::pauseAndSleep()
{
    waveOutPause(hWaveOut);
    int bitrate = settings.sampleRate() * settings.bytesPerFrame();
    // Time of written data.
    int delay = (buffer_size - bytesFree()) * 1000 / bitrate;
    Sleep(delay + 10);
}

void QWindowsAudioSink::close()
{
    qCDebug(qLcAudioOutput) << "close()";
    if(deviceState == QAudio::StoppedState)
        return;

    // Pause playback before reset to avoid uneeded crackling at the end.
    pauseAndSleep();
    QMutexLocker locker(&mutex);
    deviceState = QAudio::StoppedState;
    errorState = QAudio::NoError;
    locker.unlock();
    waveOutReset(hWaveOut);

    freeBlocks(waveBlocks);
    waveOutClose(hWaveOut);
    locker.relock();
    delete [] audioBuffer;
    audioBuffer = nullptr;
    buffer_size = 0;
    qCDebug(qLcAudioOutput) << "end close()";
}

qsizetype QWindowsAudioSink::bytesFree() const
{
    int buf;
    QMutexLocker locker(&mutex);
    buf = waveFreeBlockCount*period_size;

    return buf;
}

void QWindowsAudioSink::setBufferSize(qsizetype value)
{
    if(deviceState == QAudio::StoppedState)
        buffer_size = value;
}

qsizetype QWindowsAudioSink::bufferSize() const
{
    return buffer_size;
}

qint64 QWindowsAudioSink::processedUSecs() const
{
    if (deviceState == QAudio::StoppedState)
        return 0;
    qint64 result = qint64(1000000) * totalTimeValue /
        settings.bytesPerFrame() / settings.sampleRate();

    return result;
}

qint64 QWindowsAudioSink::write(const char *data, qint64 len)
{
    qCDebug(qLcAudioOutput) << "write()" << len << deviceState << period_size;

    // Write out some audio data
    if (deviceState != QAudio::ActiveState && deviceState != QAudio::IdleState)
        return 0;

    WAVEHDR* current;
    qint64 written = 0;
    current = &waveBlocks[waveCurrentBlock];
    while(len > 0) {
        mutex.lock();
        if(waveFreeBlockCount==0) {
            mutex.unlock();
            break;
        }
        mutex.unlock();

        if(current->dwFlags & WHDR_PREPARED)
            waveOutUnprepareHeader(hWaveOut, current, sizeof(WAVEHDR));

        qint64 remain;
        if(len < period_size)
            remain = len;
        else
            remain = period_size;

        if (volumeCache < qreal(1.0))
            QAudioHelperInternal::qMultiplySamples(volumeCache, settings, data, current->lpData, remain);
        else
            memcpy(current->lpData, data, remain);

        len -= remain;
        data += remain;
        current->dwBufferLength = remain;
        written += remain;
        waveOutPrepareHeader(hWaveOut, current, sizeof(WAVEHDR));
        waveOutWrite(hWaveOut, current, sizeof(WAVEHDR));

        mutex.lock();
        waveFreeBlockCount--;
        qCDebug(qLcAudioOutput) << "write out" << current->dwBufferLength << "waveFreeBlockCount" << waveFreeBlockCount;
        mutex.unlock();

        totalTimeValue += current->dwBufferLength;
        waveCurrentBlock++;
        waveCurrentBlock %= blocks_count;
        current = &waveBlocks[waveCurrentBlock];
        current->dwUser = 0;
        errorState = QAudio::NoError;
        if (deviceState != QAudio::ActiveState) {
            deviceState = QAudio::ActiveState;
            emit stateChanged(deviceState);
        }
    }
    return written;
}

void QWindowsAudioSink::resume()
{
    qCDebug(qLcAudioOutput) << "resume()";
    if(deviceState == QAudio::SuspendedState) {
        deviceState = pullMode ? QAudio::ActiveState : QAudio::IdleState;
        errorState = QAudio::NoError;
        waveOutRestart(hWaveOut);
        QTimer::singleShot(10, this, SLOT(feedback()));
        emit stateChanged(deviceState);
    }
}

void QWindowsAudioSink::suspend()
{
    qCDebug(qLcAudioOutput) << "suspend()";
    if(deviceState == QAudio::ActiveState || deviceState == QAudio::IdleState) {
        pauseAndSleep();
        deviceState = QAudio::SuspendedState;
        errorState = QAudio::NoError;
        emit stateChanged(deviceState);
    }
}

void QWindowsAudioSink::feedback()
{
    qCDebug(qLcAudioOutput) << "feedback()";
    bytesAvailable = waveFreeBlockCount * period_size;

    if (deviceState != QAudio::StoppedState && deviceState != QAudio::SuspendedState) {
        if (bytesAvailable >= period_size) {
            qCDebug(qLcAudioOutput) << "   ->invoke()";
            QMetaObject::invokeMethod(this, "deviceReady", Qt::QueuedConnection);
        }
    }
}

bool QWindowsAudioSink::deviceReady()
{
    qCDebug(qLcAudioOutput) << ">>>> deviceReady() state=" << deviceState << pullMode;

    if(deviceState == QAudio::StoppedState || deviceState == QAudio::SuspendedState)
        return false;

    if(pullMode) {
        qCDebug(qLcAudioOutput) << "deviceReady() avail="<<bytesAvailable<<" bytes, period size="<<period_size<<" bytes";
        bool startup = false;
        if(totalTimeValue == 0)
            startup = true;

        bool full=false;

        mutex.lock();
        if (waveFreeBlockCount == 0)
            full = true;
        mutex.unlock();

        if (full) {
            qCDebug(qLcAudioOutput) << "Skipping data as unable to write";
            return true;
        }

        if(startup)
            waveOutPause(hWaveOut);
        int input = bytesAvailable;
        int l = audioSource->read(audioBuffer,input);
        if(l > 0) {
            int out = write(audioBuffer, l);
            if(out > 0) {
                if (deviceState != QAudio::ActiveState) {
                    deviceState = QAudio::ActiveState;
                    emit stateChanged(deviceState);
                }
            }
            if ( out < l) {
                // Didn't write all data
                audioSource->seek(audioSource->pos()-(l-out));
            }
            qCDebug(qLcAudioOutput) << "wrote" << out << "bytes out of" << l << "read";

            if (startup)
                waveOutRestart(hWaveOut);
        } else if(l == 0) {
            bytesAvailable = bytesFree();

            int check = 0;

            mutex.lock();
            check = waveFreeBlockCount;
            mutex.unlock();

            if (check == blocks_count) {
                if (deviceState != QAudio::IdleState) {
                    errorState = QAudio::UnderrunError;
                    deviceState = QAudio::IdleState;
                    emit stateChanged(deviceState);
                }
            }

        } else if(l < 0) {
            bytesAvailable = bytesFree();
            if (errorState != QAudio::IOError)
                errorState = QAudio::IOError;
        }
    } else {
        int buffered;

        mutex.lock();
        buffered = waveFreeBlockCount;
        mutex.unlock();

        if (buffered >= blocks_count && deviceState == QAudio::ActiveState) {
            if (deviceState != QAudio::IdleState) {
                errorState = QAudio::UnderrunError;
                deviceState = QAudio::IdleState;
                emit stateChanged(deviceState);
            }
        }
    }
    qCDebug(qLcAudioOutput) << "<<<< end deviceReady() state=" << deviceState;

    return true;
}

QAudio::Error QWindowsAudioSink::error() const
{
    return errorState;
}

QAudio::State QWindowsAudioSink::state() const
{
    return deviceState;
}

void QWindowsAudioSink::setVolume(qreal v)
{
    if (qFuzzyCompare(volumeCache, v))
        return;

    volumeCache = qBound(qreal(0), v, qreal(1));
}

qreal QWindowsAudioSink::volume() const
{
    return volumeCache;
}

void QWindowsAudioSink::reset()
{
    stop();
}

OutputPrivate::OutputPrivate(QWindowsAudioSink* audio)
{
    audioDevice = qobject_cast<QWindowsAudioSink*>(audio);
}

OutputPrivate::~OutputPrivate() {}

qint64 OutputPrivate::readData( char* data, qint64 len)
{
    Q_UNUSED(data);
    Q_UNUSED(len);

    return 0;
}

qint64 OutputPrivate::writeData(const char* data, qint64 len)
{
    int retry = 0;
    qint64 written = 0;

    if((audioDevice->deviceState == QAudio::ActiveState)
            ||(audioDevice->deviceState == QAudio::IdleState)) {
        qint64 l = len;
        while(written < l) {
            int chunk = audioDevice->write(data+written,(l-written));
            if(chunk <= 0)
                retry++;
            else
                written+=chunk;

            if(retry > 10)
                return written;
        }
        audioDevice->deviceState = QAudio::ActiveState;
    }
    return written;
}

QT_END_NAMESPACE

#include "moc_qwindowsaudiosink_p.cpp"
