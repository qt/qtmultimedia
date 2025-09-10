// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/qcoreapplication.h>
#include <QtCore/qvarlengtharray.h>
#include <QtMultimedia/private/qaudiohelpers_p.h>
#include "qalsaaudiosource_p.h"

QT_BEGIN_NAMESPACE

//#define DEBUG_AUDIO 1

QAlsaAudioSource::QAlsaAudioSource(QAudioDevice device, const QAudioFormat &fmt, QObject *parent)
    : QPlatformAudioSource(std::move(device), fmt, parent)
{
    bytesAvailable = 0;
    handle = 0;
    access = SND_PCM_ACCESS_RW_INTERLEAVED;
    pcmformat = SND_PCM_FORMAT_S16;
    buffer_size = 0;
    period_size = 0;
    buffer_time = 100000;
    period_time = 20000;
    totalTimeValue = 0;
    deviceState = QAudio::StoppedState;
    audioSource = 0;
    pullMode = true;
    resuming = false;

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &QAlsaAudioSource::userFeed);
}

QAlsaAudioSource::~QAlsaAudioSource()
{
    close();
    disconnect(timer, &QTimer::timeout, this, &QAlsaAudioSource::userFeed);
    delete timer;
}

QAudio::State QAlsaAudioSource::state() const
{
    return deviceState;
}

int QAlsaAudioSource::xrun_recovery(int err)
{
    int  count = 0;
    bool reset = false;

    // ESTRPIPE is not available in all OSes where ALSA is available
    int estrpipe = EIO;
#ifdef ESTRPIPE
    estrpipe = ESTRPIPE;
#endif

    if(err == -EPIPE) {
        setError(QAudio::UnderrunError);
        err = snd_pcm_prepare(handle);
        if(err < 0)
            reset = true;
        else {
            bytesAvailable = checkBytesReady();
            if (bytesAvailable <= 0)
                reset = true;
        }
    } else if ((err == -estrpipe)||(err == -EIO)) {
        setError(QAudio::IOError);
        while((err = snd_pcm_resume(handle)) == -EAGAIN){
            usleep(100);
            count++;
            if(count > 5) {
                reset = true;
                break;
            }
        }
        if(err < 0) {
            err = snd_pcm_prepare(handle);
            if(err < 0)
                reset = true;
        }
    }
    if(reset) {
        close();
        open();
        snd_pcm_prepare(handle);
        return 0;
    }
    return err;
}

int QAlsaAudioSource::setFormat()
{
    snd_pcm_format_t pcmformat = SND_PCM_FORMAT_UNKNOWN;

    switch (m_format.sampleFormat()) {
    case QAudioFormat::UInt8:
        pcmformat = SND_PCM_FORMAT_U8;
        break;
    case QAudioFormat::Int16:
        if constexpr (QSysInfo::ByteOrder == QSysInfo::BigEndian)
            pcmformat = SND_PCM_FORMAT_S16_BE;
        else
            pcmformat = SND_PCM_FORMAT_S16_LE;
        break;
    case QAudioFormat::Int32:
        if constexpr (QSysInfo::ByteOrder == QSysInfo::BigEndian)
            pcmformat = SND_PCM_FORMAT_S32_BE;
        else
            pcmformat = SND_PCM_FORMAT_S32_LE;
        break;
    case QAudioFormat::Float:
        if constexpr (QSysInfo::ByteOrder == QSysInfo::BigEndian)
            pcmformat = SND_PCM_FORMAT_FLOAT_BE;
        else
            pcmformat = SND_PCM_FORMAT_FLOAT_LE;
        break;
    default:
        break;
    }

    return pcmformat != SND_PCM_FORMAT_UNKNOWN
            ? snd_pcm_hw_params_set_format( handle, hwparams, pcmformat)
            : -1;
}

void QAlsaAudioSource::start(QIODevice* device)
{
    if(deviceState != QAudio::StoppedState)
        close();

    if(!pullMode && audioSource)
        delete audioSource;

    pullMode = true;
    audioSource = device;

    deviceState = QAudio::ActiveState;

    if( !open() )
        return;

    emit stateChanged(deviceState);
}

QIODevice* QAlsaAudioSource::start()
{
    if(deviceState != QAudio::StoppedState)
        close();

    if(!pullMode && audioSource)
        delete audioSource;

    pullMode = false;
    audioSource = new AlsaInputPrivate(this);
    audioSource->open(QIODevice::ReadOnly | QIODevice::Unbuffered);

    deviceState = QAudio::IdleState;

    if( !open() )
        return 0;

    emit stateChanged(deviceState);

    return audioSource;
}

void QAlsaAudioSource::stop()
{
    if(deviceState == QAudio::StoppedState)
        return;

    deviceState = QAudio::StoppedState;

    close();
    emit stateChanged(deviceState);
}

bool QAlsaAudioSource::open()
{
#ifdef DEBUG_AUDIO
    QTime now(QTime::currentTime());
    qDebug()<<now.second()<<"s "<<now.msec()<<"ms :open()";
#endif
    elapsedTimeOffset = 0;

    int dir;
    int err = -1;
    int count=0;
    unsigned int sampleRate=m_format.sampleRate();

    // Step 1: try and open the device
    while((count < 5) && (err < 0)) {
        err = snd_pcm_open(&handle, m_audioDevice.id().constData(), SND_PCM_STREAM_CAPTURE, 0);
        if(err < 0)
            count++;
    }
    if (( err < 0)||(handle == 0)) {
        setError(QAudio::OpenError);
        deviceState = QAudio::StoppedState;
        emit stateChanged(deviceState);
        return false;
    }
    snd_pcm_nonblock( handle, 0 );

    // Step 2: Set the desired HW parameters.
    snd_pcm_hw_params_alloca( &hwparams );

    bool fatal = false;
    QString errMessage;
    unsigned int chunks = 8;

    err = snd_pcm_hw_params_any( handle, hwparams );
    if ( err < 0 ) {
        fatal = true;
        errMessage = QString::fromLatin1("QAudioSource: snd_pcm_hw_params_any: err = %1").arg(err);
    }
    if ( !fatal ) {
        err = snd_pcm_hw_params_set_rate_resample( handle, hwparams, 1 );
        if ( err < 0 ) {
            fatal = true;
            errMessage = QString::fromLatin1("QAudioSource: snd_pcm_hw_params_set_rate_resample: err = %1").arg(err);
        }
    }
    if ( !fatal ) {
        err = snd_pcm_hw_params_set_access( handle, hwparams, access );
        if ( err < 0 ) {
            fatal = true;
            errMessage = QString::fromLatin1("QAudioSource: snd_pcm_hw_params_set_access: err = %1").arg(err);
        }
    }
    if ( !fatal ) {
        err = setFormat();
        if ( err < 0 ) {
            fatal = true;
            errMessage = QString::fromLatin1("QAudioSource: snd_pcm_hw_params_set_format: err = %1").arg(err);
        }
    }
    if ( !fatal ) {
        err = snd_pcm_hw_params_set_channels( handle, hwparams, (unsigned int)m_format.channelCount() );
        if ( err < 0 ) {
            fatal = true;
            errMessage = QString::fromLatin1("QAudioSource: snd_pcm_hw_params_set_channels: err = %1").arg(err);
        }
    }
    if ( !fatal ) {
        err = snd_pcm_hw_params_set_rate_near( handle, hwparams, &sampleRate, 0 );
        if ( err < 0 ) {
            fatal = true;
            errMessage = QString::fromLatin1("QAudioSource: snd_pcm_hw_params_set_rate_near: err = %1").arg(err);
        }
    }
    if ( !fatal ) {
        err = snd_pcm_hw_params_set_buffer_time_near(handle, hwparams, &buffer_time, &dir);
        if ( err < 0 ) {
            fatal = true;
            errMessage = QString::fromLatin1("QAudioSource: snd_pcm_hw_params_set_buffer_time_near: err = %1").arg(err);
        }
    }
    if ( !fatal ) {
        err = snd_pcm_hw_params_set_period_time_near(handle, hwparams, &period_time, &dir);
        if ( err < 0 ) {
            fatal = true;
            errMessage = QString::fromLatin1("QAudioSource: snd_pcm_hw_params_set_period_time_near: err = %1").arg(err);
        }
    }
    if ( !fatal ) {
        err = snd_pcm_hw_params_set_periods_near(handle, hwparams, &chunks, &dir);
        if ( err < 0 ) {
            fatal = true;
            errMessage = QString::fromLatin1("QAudioSource: snd_pcm_hw_params_set_periods_near: err = %1").arg(err);
        }
    }
    if ( !fatal ) {
        err = snd_pcm_hw_params(handle, hwparams);
        if ( err < 0 ) {
            fatal = true;
            errMessage = QString::fromLatin1("QAudioSource: snd_pcm_hw_params: err = %1").arg(err);
        }
    }
    if( err < 0) {
        qWarning()<<errMessage;
        setError(QAudio::OpenError);
        deviceState = QAudio::StoppedState;
        emit stateChanged(deviceState);
        return false;
    }
    snd_pcm_hw_params_get_buffer_size(hwparams,&buffer_frames);
    buffer_size = snd_pcm_frames_to_bytes(handle,buffer_frames);
    snd_pcm_hw_params_get_period_size(hwparams,&period_frames, &dir);
    period_size = snd_pcm_frames_to_bytes(handle,period_frames);
    snd_pcm_hw_params_get_buffer_time(hwparams,&buffer_time, &dir);
    snd_pcm_hw_params_get_period_time(hwparams,&period_time, &dir);

    // Step 3: Set the desired SW parameters.
    snd_pcm_sw_params_t *swparams;
    snd_pcm_sw_params_alloca(&swparams);
    snd_pcm_sw_params_current(handle, swparams);
    snd_pcm_sw_params_set_start_threshold(handle,swparams,period_frames);
    snd_pcm_sw_params_set_stop_threshold(handle,swparams,buffer_frames);
    snd_pcm_sw_params_set_avail_min(handle, swparams,period_frames);
    snd_pcm_sw_params(handle, swparams);

    // Step 4: Prepare audio
    ringBuffer.resize(buffer_size);
    snd_pcm_prepare( handle );
    snd_pcm_start(handle);

    // Step 5: Setup timer
    bytesAvailable = checkBytesReady();

    if(pullMode)
        connect(audioSource, &QIODevice::readyRead, this, &QAlsaAudioSource::userFeed);

    // Step 6: Start audio processing
    chunks = buffer_size/period_size;
    timer->start(period_time*chunks/2000);

    setError(QAudio::NoError);

    totalTimeValue = 0;

    return true;
}

void QAlsaAudioSource::close()
{
    timer->stop();

    if ( handle ) {
        snd_pcm_drop( handle );
        snd_pcm_close( handle );
        handle = 0;
    }
}

int QAlsaAudioSource::checkBytesReady()
{
    if(resuming)
        bytesAvailable = period_size;
    else if(deviceState != QAudio::ActiveState
            && deviceState != QAudio::IdleState)
        bytesAvailable = 0;
    else {
        int frames = snd_pcm_avail_update(handle);
        if (frames < 0) {
            bytesAvailable = frames;
        } else {
            if((int)frames > (int)buffer_frames)
                frames = buffer_frames;
            bytesAvailable = snd_pcm_frames_to_bytes(handle, frames);
        }
    }
    return bytesAvailable;
}

qsizetype QAlsaAudioSource::bytesReady() const
{
    return qMax(bytesAvailable, 0);
}

qint64 QAlsaAudioSource::read(char* data, qint64 len)
{
    // Read in some audio data and write it to QIODevice, pull mode
    if ( !handle )
        return 0;

    int bytesRead = 0;
    int bytesInRingbufferBeforeRead = ringBuffer.bytesOfDataInBuffer();

    if (ringBuffer.bytesOfDataInBuffer() < len) {

        // bytesAvaiable is saved as a side effect of checkBytesReady().
        int bytesToRead = checkBytesReady();

        if (bytesToRead < 0) {
            // bytesAvailable as negative is error code, try to recover from it.
            xrun_recovery(bytesToRead);
            bytesToRead = checkBytesReady();
            if (bytesToRead < 0) {
                // recovery failed must stop and set error.
                close();
                setError(QAudio::IOError);
                deviceState = QAudio::StoppedState;
                emit stateChanged(deviceState);
                return 0;
            }
        }

        bytesToRead = qMin<qint64>(len, bytesToRead);
        bytesToRead = qMin<qint64>(ringBuffer.freeBytes(), bytesToRead);
        bytesToRead -= bytesToRead % period_size;

        int count=0;
        int err = 0;
        QVarLengthArray<char, 4096> buffer(bytesToRead);
        while(count < 5 && bytesToRead > 0) {
            int chunks = bytesToRead / period_size;
            int frames = chunks * period_frames;
            if (frames > (int)buffer_frames)
                frames = buffer_frames;

            int readFrames = snd_pcm_readi(handle, buffer.data(), frames);
            bytesRead = snd_pcm_frames_to_bytes(handle, readFrames);
            if (volume() < 1.0f)
                QAudioHelperInternal::qMultiplySamples(volume(), m_format, buffer.constData(),
                                                       buffer.data(), bytesRead);

            if (readFrames >= 0) {
                ringBuffer.write(buffer.data(), bytesRead);
#ifdef DEBUG_AUDIO
                qDebug() << QString::fromLatin1("read in bytes = %1 (frames=%2)").arg(bytesRead).arg(readFrames).toLatin1().constData();
#endif
                break;
            } else if((readFrames == -EAGAIN) || (readFrames == -EINTR)) {
                setError(QAudio::IOError);
                err = 0;
                break;
            } else {
                if(readFrames == -EPIPE) {
                    setError(QAudio::UnderrunError);
                    err = snd_pcm_prepare(handle);
#ifdef ESTRPIPE
                } else if(readFrames == -ESTRPIPE) {
                    err = snd_pcm_prepare(handle);
#endif
                }
                if(err != 0) break;
            }
            count++;
        }

    }

    bytesRead += bytesInRingbufferBeforeRead;

    if (bytesRead > 0) {
        // got some send it onward
#ifdef DEBUG_AUDIO
        qDebug() << "frames to write to QIODevice = " <<
            snd_pcm_bytes_to_frames( handle, (int)bytesRead ) << " (" << bytesRead << ") bytes";
#endif
        if (deviceState != QAudio::ActiveState && deviceState != QAudio::IdleState)
            return 0;

        if (pullMode) {
            qint64 l = 0;
            qint64 bytesWritten = 0;
            while (ringBuffer.bytesOfDataInBuffer() > 0) {
                l = audioSource->write(ringBuffer.availableData(), ringBuffer.availableDataBlockSize());
                if (l > 0) {
                    ringBuffer.readBytes(l);
                    bytesWritten += l;
                } else {
                    break;
                }
            }

            if (l < 0) {
                close();
                setError(QAudio::IOError);
                deviceState = QAudio::StoppedState;
                emit stateChanged(deviceState);
            } else if (l == 0 && bytesWritten == 0) {
                if (deviceState != QAudio::IdleState) {
                    setError(QAudio::NoError);
                    deviceState = QAudio::IdleState;
                    emit stateChanged(deviceState);
                }
            } else {
                bytesAvailable -= bytesWritten;
                totalTimeValue += bytesWritten;
                resuming = false;
                if (deviceState != QAudio::ActiveState) {
                    setError(QAudio::NoError);
                    deviceState = QAudio::ActiveState;
                    emit stateChanged(deviceState);
                }
            }

            return bytesWritten;
        } else {
            while (ringBuffer.bytesOfDataInBuffer() > 0) {
                int size = ringBuffer.availableDataBlockSize();
                memcpy(data, ringBuffer.availableData(), size);
                data += size;
                ringBuffer.readBytes(size);
            }

            bytesAvailable -= bytesRead;
            totalTimeValue += bytesRead;
            resuming = false;
            if (deviceState != QAudio::ActiveState) {
                setError(QAudio::NoError);
                deviceState = QAudio::ActiveState;
                emit stateChanged(deviceState);
            }

            return bytesRead;
        }
    }

    return 0;
}

void QAlsaAudioSource::resume()
{
    if(deviceState == QAudio::SuspendedState) {
        int err = 0;

        if(handle) {
            err = snd_pcm_prepare( handle );
            if(err < 0)
                xrun_recovery(err);

            err = snd_pcm_start(handle);
            if(err < 0)
                xrun_recovery(err);

            bytesAvailable = buffer_size;
        }
        resuming = true;
        deviceState = QAudio::ActiveState;
        int chunks = buffer_size/period_size;
        timer->start(period_time*chunks/2000);
        emit stateChanged(deviceState);
    }
}

void QAlsaAudioSource::setBufferSize(qsizetype value)
{
    buffer_size = value;
}

qsizetype QAlsaAudioSource::bufferSize() const
{
    return buffer_size;
}

qint64 QAlsaAudioSource::processedUSecs() const
{
    qint64 result = qint64(1000000) * totalTimeValue /
        m_format.bytesPerFrame() /
        m_format.sampleRate();

    return result;
}

void QAlsaAudioSource::suspend()
{
    if(deviceState == QAudio::ActiveState||resuming) {
        snd_pcm_drain(handle);
        timer->stop();
        deviceState = QAudio::SuspendedState;
        emit stateChanged(deviceState);
    }
}

void QAlsaAudioSource::userFeed()
{
    if(deviceState == QAudio::StoppedState || deviceState == QAudio::SuspendedState)
        return;
#ifdef DEBUG_AUDIO
    QTime now(QTime::currentTime());
    qDebug()<<now.second()<<"s "<<now.msec()<<"ms :userFeed() IN";
#endif
    deviceReady();
}

bool QAlsaAudioSource::deviceReady()
{
    if(pullMode) {
        // reads some audio data and writes it to QIODevice
        read(0, buffer_size);
    } else {
        // emits readyRead() so user will call read() on QIODevice to get some audio data
        AlsaInputPrivate* a = qobject_cast<AlsaInputPrivate*>(audioSource);
        a->trigger();
    }
    bytesAvailable = checkBytesReady();

    if(deviceState != QAudio::ActiveState)
        return true;

    if (bytesAvailable < 0) {
        // bytesAvailable as negative is error code, try to recover from it.
        xrun_recovery(bytesAvailable);
        bytesAvailable = checkBytesReady();
        if (bytesAvailable < 0) {
            // recovery failed must stop and set error.
            close();
            setError(QAudio::IOError);
            deviceState = QAudio::StoppedState;
            emit stateChanged(deviceState);
            return 0;
        }
    }

    return true;
}

void QAlsaAudioSource::reset()
{
    if(handle)
        snd_pcm_reset(handle);
    stop();
    bytesAvailable = 0;
}

void QAlsaAudioSource::drain()
{
    if(handle)
        snd_pcm_drain(handle);
}

AlsaInputPrivate::AlsaInputPrivate(QAlsaAudioSource* audio)
{
    audioDevice = qobject_cast<QAlsaAudioSource*>(audio);
}

AlsaInputPrivate::~AlsaInputPrivate()
{
}

qint64 AlsaInputPrivate::readData( char* data, qint64 len)
{
    return audioDevice->read(data,len);
}

qint64 AlsaInputPrivate::writeData(const char* data, qint64 len)
{
    Q_UNUSED(data);
    Q_UNUSED(len);
    return 0;
}

void AlsaInputPrivate::trigger()
{
    emit readyRead();
}

RingBuffer::RingBuffer() :
        m_head(0),
        m_tail(0)
{
}

void RingBuffer::resize(int size)
{
    m_data.resize(size);
}

int RingBuffer::bytesOfDataInBuffer() const
{
    if (m_head < m_tail)
        return m_tail - m_head;
    else if (m_tail < m_head)
        return m_data.size() + m_tail - m_head;
    else
        return 0;
}

int RingBuffer::freeBytes() const
{
    if (m_head > m_tail)
        return m_head - m_tail - 1;
    else if (m_tail > m_head)
        return m_data.size() - m_tail + m_head - 1;
    else
        return m_data.size() - 1;
}

const char *RingBuffer::availableData() const
{
    return (m_data.constData() + m_head);
}

int RingBuffer::availableDataBlockSize() const
{
    if (m_head > m_tail)
        return m_data.size() - m_head;
    else if (m_tail > m_head)
        return m_tail - m_head;
    else
        return 0;
}

void RingBuffer::readBytes(int bytes)
{
    m_head = (m_head + bytes) % m_data.size();
}

void RingBuffer::write(char *data, int len)
{
    if (m_tail + len < m_data.size()) {
        memcpy(m_data.data() + m_tail, data, len);
        m_tail += len;
    } else {
        int bytesUntilEnd = m_data.size() - m_tail;
        memcpy(m_data.data() + m_tail, data, bytesUntilEnd);
        if (len - bytesUntilEnd > 0)
            memcpy(m_data.data(), data + bytesUntilEnd, len - bytesUntilEnd);
        m_tail = len - bytesUntilEnd;
    }
}

QT_END_NAMESPACE

#include "moc_qalsaaudiosource_p.cpp"
