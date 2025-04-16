// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/qcoreapplication.h>
#include <QtCore/qvarlengtharray.h>
#include <QtMultimedia/private/qaudiohelpers_p.h>
#include "qalsaaudiosink_p.h"
#include <QLoggingCategory>

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(lcAlsaOutput, "qt.multimedia.alsa.output");
//#define DEBUG_AUDIO 1

QAlsaAudioSink::QAlsaAudioSink(QAudioDevice device, const QAudioFormat &format, QObject *parent)
    : QPlatformAudioSink(std::move(device), format, parent)
{
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &QAlsaAudioSink::userFeed);
}

QAlsaAudioSink::~QAlsaAudioSink()
{
    close();
    disconnect(timer, &QTimer::timeout, this, &QAlsaAudioSink::userFeed);
    delete timer;
}

QAudio::State QAlsaAudioSink::state() const
{
    return deviceState;
}

int QAlsaAudioSink::xrun_recovery(int err)
{
    int  count = 0;
    bool reset = false;

    // ESTRPIPE is not available in all OSes where ALSA is available
    int estrpipe = EIO;
#ifdef ESTRPIPE
    estrpipe = ESTRPIPE;
#endif

    if(err == -EPIPE) {
        errorState = QAudio::UnderrunError;
        setError(errorState);
        err = snd_pcm_prepare(handle);
        if(err < 0)
            reset = true;

    } else if ((err == -estrpipe)||(err == -EIO)) {
        errorState = QAudio::IOError;
        setError(errorState);
        while ((err = snd_pcm_resume(handle)) == -EAGAIN) {
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

int QAlsaAudioSink::setFormat()
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

void QAlsaAudioSink::start(QIODevice* device)
{
    if(deviceState != QAudio::StoppedState)
        deviceState = QAudio::StoppedState;

    errorState = QAudio::NoError;

    // Handle change of mode
    if(audioSource && !pullMode) {
        delete audioSource;
        audioSource = 0;
    }

    close();

    pullMode = true;
    audioSource = device;

    connect(audioSource, &QIODevice::readyRead, timer, [this] {
        if (!timer->isActive()) {
            timer->start(period_time / 1000);
        }
    });
    deviceState = QAudio::ActiveState;

    open();

    emit stateChanged(deviceState);
}

QIODevice* QAlsaAudioSink::start()
{
    if(deviceState != QAudio::StoppedState)
        deviceState = QAudio::StoppedState;

    errorState = QAudio::NoError;

    // Handle change of mode
    if(audioSource && !pullMode) {
        delete audioSource;
        audioSource = 0;
    }

    close();

    audioSource = new AlsaOutputPrivate(this);
    audioSource->open(QIODevice::WriteOnly|QIODevice::Unbuffered);
    pullMode = false;

    deviceState = QAudio::IdleState;

    open();

    emit stateChanged(deviceState);

    return audioSource;
}

void QAlsaAudioSink::stop()
{
    if(deviceState == QAudio::StoppedState)
        return;
    errorState = QAudio::NoError;
    deviceState = QAudio::StoppedState;
    close();
    emit stateChanged(deviceState);
}

bool QAlsaAudioSink::open()
{
    if(opened)
        return true;

#ifdef DEBUG_AUDIO
    QTime now(QTime::currentTime());
    qDebug()<<now.second()<<"s "<<now.msec()<<"ms :open()";
#endif
    elapsedTimeOffset = 0;

    int dir;
    int err = -1;
    int count=0;
    unsigned int sampleRate = m_format.sampleRate();

    // Step 1: try and open the device
    while((count < 5) && (err < 0)) {
        err = snd_pcm_open(&handle, m_audioDevice.id().constData(), SND_PCM_STREAM_PLAYBACK, 0);
        if(err < 0)
            count++;
    }
    if (( err < 0)||(handle == 0)) {
        errorState = QAudio::OpenError;
        setError(errorState);
        deviceState = QAudio::StoppedState;
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
        errMessage = QString::fromLatin1("QAudioSink: snd_pcm_hw_params_any: err = %1").arg(err);
    }
    if ( !fatal ) {
        err = snd_pcm_hw_params_set_rate_resample( handle, hwparams, 1 );
        if ( err < 0 ) {
            fatal = true;
            errMessage = QString::fromLatin1("QAudioSink: snd_pcm_hw_params_set_rate_resample: err = %1").arg(err);
        }
    }
    if ( !fatal ) {
        err = snd_pcm_hw_params_set_access( handle, hwparams, access );
        if ( err < 0 ) {
            fatal = true;
            errMessage = QString::fromLatin1("QAudioSink: snd_pcm_hw_params_set_access: err = %1").arg(err);
        }
    }
    if ( !fatal ) {
        err = setFormat();
        if ( err < 0 ) {
            fatal = true;
            errMessage = QString::fromLatin1("QAudioSink: snd_pcm_hw_params_set_format: err = %1").arg(err);
        }
    }
    if ( !fatal ) {
        err = snd_pcm_hw_params_set_channels(handle, hwparams,
                                             (unsigned int)m_format.channelCount());
        if ( err < 0 ) {
            fatal = true;
            errMessage = QString::fromLatin1("QAudioSink: snd_pcm_hw_params_set_channels: err = %1").arg(err);
        }
    }
    if ( !fatal ) {
        err = snd_pcm_hw_params_set_rate_near( handle, hwparams, &sampleRate, 0 );
        if ( err < 0 ) {
            fatal = true;
            errMessage = QString::fromLatin1("QAudioSink: snd_pcm_hw_params_set_rate_near: err = %1").arg(err);
        }
    }
    if ( !fatal ) {
        unsigned int maxBufferTime = 0;
        unsigned int minBufferTime = 0;
        unsigned int maxPeriodTime = 0;
        unsigned int minPeriodTime = 0;

        err = snd_pcm_hw_params_get_buffer_time_max(hwparams, &maxBufferTime, &dir);
        if ( err >= 0)
            err = snd_pcm_hw_params_get_buffer_time_min(hwparams, &minBufferTime, &dir);
        if ( err >= 0)
            err = snd_pcm_hw_params_get_period_time_max(hwparams, &maxPeriodTime, &dir);
        if ( err >= 0)
            err = snd_pcm_hw_params_get_period_time_min(hwparams, &minPeriodTime, &dir);

        if ( err < 0 ) {
            fatal = true;
            errMessage = QString::fromLatin1("QAudioSink: buffer/period min and max: err = %1").arg(err);
        } else {
            static unsigned user_buffer_time = qEnvironmentVariableIntValue("QT_ALSA_OUTPUT_BUFFER_TIME");
            static unsigned user_period_time = qEnvironmentVariableIntValue("QT_ALSA_OUTPUT_PERIOD_TIME");
            const bool outOfRange = maxBufferTime < buffer_time || buffer_time < minBufferTime || maxPeriodTime < period_time || minPeriodTime > period_time;
            if (outOfRange || user_period_time || user_buffer_time) {
                period_time = user_period_time ? user_period_time : minPeriodTime;
                if (!user_buffer_time) {
                    chunks = maxBufferTime / period_time;
                    buffer_time = period_time * chunks;
                } else {
                    buffer_time = user_buffer_time;
                    chunks = buffer_time / period_time;
                }
            }
            qCDebug(lcAlsaOutput) << "buffer time: [" << minBufferTime << "-" << maxBufferTime << "] =" << buffer_time;
            qCDebug(lcAlsaOutput) << "period time: [" << minPeriodTime << "-" << maxPeriodTime << "] =" << period_time;
            qCDebug(lcAlsaOutput) << "chunks =" << chunks;
        }
    }
    if ( !fatal ) {
        err = snd_pcm_hw_params_set_buffer_time_near(handle, hwparams, &buffer_time, &dir);
        if ( err < 0 ) {
            fatal = true;
            errMessage = QString::fromLatin1("QAudioSink: snd_pcm_hw_params_set_buffer_time_near: err = %1").arg(err);
        }
    }
    if ( !fatal ) {
        err = snd_pcm_hw_params_set_period_time_near(handle, hwparams, &period_time, &dir);
        if ( err < 0 ) {
            fatal = true;
            errMessage = QString::fromLatin1("QAudioSink: snd_pcm_hw_params_set_period_time_near: err = %1").arg(err);
        }
    }
    if ( !fatal ) {
        err = snd_pcm_hw_params_set_periods_near(handle, hwparams, &chunks, &dir);
        if ( err < 0 ) {
            fatal = true;
            errMessage = QString::fromLatin1("QAudioSink: snd_pcm_hw_params_set_periods_near: err = %1").arg(err);
        }
    }
    if ( !fatal ) {
        err = snd_pcm_hw_params(handle, hwparams);
        if ( err < 0 ) {
            fatal = true;
            errMessage = QString::fromLatin1("QAudioSink: snd_pcm_hw_params: err = %1").arg(err);
        }
    }
    if( err < 0) {
        qWarning()<<errMessage;
        errorState = QAudio::OpenError;
        setError(errorState);
        deviceState = QAudio::StoppedState;
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
    if(audioBuffer == 0)
        audioBuffer = new char[snd_pcm_frames_to_bytes(handle,buffer_frames)];
    snd_pcm_prepare( handle );
    snd_pcm_start(handle);

    // Step 5: Setup timer
    bytesAvailable = bytesFree();

    // Step 6: Start audio processing
    timer->start(period_time/1000);

    elapsedTimeOffset = 0;
    errorState  = QAudio::NoError;
    totalTimeValue = 0;
    opened = true;

    return true;
}

void QAlsaAudioSink::close()
{
    timer->stop();

    if ( handle ) {
        snd_pcm_drain( handle );
        snd_pcm_close( handle );
        handle = 0;
        delete [] audioBuffer;
        audioBuffer=0;
    }
    if(!pullMode && audioSource) {
        delete audioSource;
        audioSource = 0;
    }
    opened = false;
}

qsizetype QAlsaAudioSink::bytesFree() const
{
    if(resuming)
        return period_size;

    if(deviceState != QAudio::ActiveState && deviceState != QAudio::IdleState)
        return 0;

    int frames = snd_pcm_avail_update(handle);
    if (frames == -EPIPE) {
        // Try and handle buffer underrun
        int err = snd_pcm_recover(handle, frames, 0);
        if (err < 0)
            return 0;
        else
            frames = snd_pcm_avail_update(handle);
    } else if (frames < 0) {
        return 0;
    }

    if ((int)frames > (int)buffer_frames)
        frames = buffer_frames;

    return snd_pcm_frames_to_bytes(handle, frames);
}

qint64 QAlsaAudioSink::write( const char *data, qint64 len )
{
    // Write out some audio data
    if ( !handle )
        return 0;
#ifdef DEBUG_AUDIO
    qDebug()<<"frames to write out = "<<
        snd_pcm_bytes_to_frames( handle, (int)len )<<" ("<<len<<") bytes";
#endif
    int frames, err;
    int space = bytesFree();

    if (!space)
        return 0;

    if (len < space)
        space = len;

    frames = snd_pcm_bytes_to_frames(handle, space);

    if (volume() < 1.0f) {
        QVarLengthArray<char, 4096> out(space);
        QAudioHelperInternal::qMultiplySamples(volume(), m_format, data, out.data(), space);
        err = snd_pcm_writei(handle, out.constData(), frames);
    } else {
        err = snd_pcm_writei(handle, data, frames);
    }

    if(err > 0) {
        totalTimeValue += err;
        resuming = false;
        errorState = QAudio::NoError;
        if (deviceState != QAudio::ActiveState) {
            deviceState = QAudio::ActiveState;
            emit stateChanged(deviceState);
        }
        return snd_pcm_frames_to_bytes( handle, err );
    } else
        err = xrun_recovery(err);

    if(err < 0) {
        close();
        errorState = QAudio::FatalError;
        setError(errorState);
        deviceState = QAudio::StoppedState;
        emit stateChanged(deviceState);
    }
    return 0;
}

void QAlsaAudioSink::setBufferSize(qsizetype value)
{
    if(deviceState == QAudio::StoppedState)
        buffer_size = value;
}

qsizetype QAlsaAudioSink::bufferSize() const
{
    return buffer_size;
}

qint64 QAlsaAudioSink::processedUSecs() const
{
    return qint64(1000000) * totalTimeValue / m_format.sampleRate();
}

void QAlsaAudioSink::resume()
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

            bytesAvailable = (int)snd_pcm_frames_to_bytes(handle, buffer_frames);
        }
        resuming = true;

        deviceState = suspendedInState;
        errorState = QAudio::NoError;
        timer->start(period_time/1000);
        emit stateChanged(deviceState);
    }
}

void QAlsaAudioSink::suspend()
{
    if(deviceState == QAudio::ActiveState || deviceState == QAudio::IdleState || resuming) {
        suspendedInState = deviceState;
        snd_pcm_drain(handle);
        timer->stop();
        deviceState = QAudio::SuspendedState;
        errorState = QAudio::NoError;
        emit stateChanged(deviceState);
    }
}

void QAlsaAudioSink::userFeed()
{
    if(deviceState == QAudio::StoppedState || deviceState == QAudio::SuspendedState)
        return;
#ifdef DEBUG_AUDIO
    QTime now(QTime::currentTime());
    qDebug()<<now.second()<<"s "<<now.msec()<<"ms :userFeed() OUT";
#endif
    if(deviceState ==  QAudio::IdleState)
        bytesAvailable = bytesFree();

    deviceReady();
}

bool QAlsaAudioSink::deviceReady()
{
    if(pullMode) {
        int l = 0;
        int chunks = bytesAvailable/period_size;
        if(chunks==0) {
            bytesAvailable = bytesFree();
            return false;
        }
#ifdef DEBUG_AUDIO
        qDebug()<<"deviceReady() avail="<<bytesAvailable<<" bytes, period size="<<period_size<<" bytes";
        qDebug()<<"deviceReady() no. of chunks that can fit ="<<chunks<<", chunks in bytes ="<<period_size*chunks;
#endif
        int input = period_frames*chunks;
        if(input > (int)buffer_frames)
            input = buffer_frames;
        l = audioSource->read(audioBuffer,snd_pcm_frames_to_bytes(handle, input));

        // reading can take a while and stream may have been stopped
        if (!handle)
            return false;

        if(l > 0) {
            // Got some data to output
            if (deviceState != QAudio::ActiveState && deviceState != QAudio::IdleState)
                return true;
            qint64 bytesWritten = write(audioBuffer,l);
            if (bytesWritten != l)
                audioSource->seek(audioSource->pos()-(l-bytesWritten));
            bytesAvailable = bytesFree();

        } else if(l == 0) {
            // Did not get any data to output
            timer->stop();
            snd_pcm_drain(handle);
            bytesAvailable = bytesFree();
            if(bytesAvailable > snd_pcm_frames_to_bytes(handle, buffer_frames-period_frames)) {
                // Underrun
                if (deviceState != QAudio::IdleState) {
                    errorState = audioSource->atEnd() ? QAudio::NoError : QAudio::UnderrunError;
                    setError(errorState);
                    deviceState = QAudio::IdleState;
                    emit stateChanged(deviceState);
                }
            }

        } else if(l < 0) {
            close();
            deviceState = QAudio::StoppedState;
            errorState = QAudio::IOError;
            setError(errorState);
            emit stateChanged(deviceState);
        }
    } else {
        bytesAvailable = bytesFree();
        if(bytesAvailable > snd_pcm_frames_to_bytes(handle, buffer_frames-period_frames)) {
            // Underrun
            if (deviceState != QAudio::IdleState) {
                errorState = QAudio::UnderrunError;
                setError(errorState);
                deviceState = QAudio::IdleState;
                emit stateChanged(deviceState);
            }
        }
    }

    if(deviceState != QAudio::ActiveState)
        return true;

    return true;
}

void QAlsaAudioSink::reset()
{
    if(handle)
        snd_pcm_reset(handle);

    stop();
}

AlsaOutputPrivate::AlsaOutputPrivate(QAlsaAudioSink* audio)
{
    audioDevice = qobject_cast<QAlsaAudioSink*>(audio);
}

AlsaOutputPrivate::~AlsaOutputPrivate() {}

qint64 AlsaOutputPrivate::readData( char* data, qint64 len)
{
    Q_UNUSED(data);
    Q_UNUSED(len);

    return 0;
}

qint64 AlsaOutputPrivate::writeData(const char* data, qint64 len)
{
    int retry = 0;
    qint64 written = 0;
    if((audioDevice->deviceState == QAudio::ActiveState)
            ||(audioDevice->deviceState == QAudio::IdleState)) {
        while(written < len) {
            int chunk = audioDevice->write(data+written,(len-written));
            if(chunk <= 0)
                retry++;
            written+=chunk;
            if(retry > 10)
                return written;
        }
    }
    return written;

}

QT_END_NAMESPACE

#include "moc_qalsaaudiosink_p.cpp"
