/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
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

#include "qaudiodeviceinfo_alsa_p.h"

#include <alsa/version.h>

QT_BEGIN_NAMESPACE

QAudioDeviceInfoInternal::QAudioDeviceInfoInternal(QByteArray dev, QAudio::Mode mode)
{
    handle = 0;

    device = QLatin1String(dev);
    this->mode = mode;

    checkSurround();
}

QAudioDeviceInfoInternal::~QAudioDeviceInfoInternal()
{
    close();
}

bool QAudioDeviceInfoInternal::isFormatSupported(const QAudioFormat& format) const
{
    return testSettings(format);
}

QAudioFormat QAudioDeviceInfoInternal::preferredFormat() const
{
    QAudioFormat nearest;
    if(mode == QAudio::AudioOutput) {
        nearest.setSampleRate(44100);
        nearest.setChannelCount(2);
        nearest.setByteOrder(QAudioFormat::LittleEndian);
        nearest.setSampleType(QAudioFormat::SignedInt);
        nearest.setSampleSize(16);
        nearest.setCodec(QLatin1String("audio/pcm"));
    } else {
        nearest.setSampleRate(8000);
        nearest.setChannelCount(1);
        nearest.setSampleType(QAudioFormat::UnSignedInt);
        nearest.setSampleSize(8);
        nearest.setCodec(QLatin1String("audio/pcm"));
        if(!testSettings(nearest)) {
            nearest.setChannelCount(2);
            nearest.setSampleSize(16);
            nearest.setSampleType(QAudioFormat::SignedInt);
        }
    }
    return nearest;
}

QString QAudioDeviceInfoInternal::deviceName() const
{
    return device;
}

QStringList QAudioDeviceInfoInternal::supportedCodecs()
{
    updateLists();
    return codecz;
}

QList<int> QAudioDeviceInfoInternal::supportedSampleRates()
{
    updateLists();
    return sampleRatez;
}

QList<int> QAudioDeviceInfoInternal::supportedChannelCounts()
{
    updateLists();
    return channelz;
}

QList<int> QAudioDeviceInfoInternal::supportedSampleSizes()
{
    updateLists();
    return sizez;
}

QList<QAudioFormat::Endian> QAudioDeviceInfoInternal::supportedByteOrders()
{
    updateLists();
    return byteOrderz;
}

QList<QAudioFormat::SampleType> QAudioDeviceInfoInternal::supportedSampleTypes()
{
    updateLists();
    return typez;
}

bool QAudioDeviceInfoInternal::open()
{
    int err = 0;
    QString dev = device;
    QList<QByteArray> devices = availableDevices(mode);

    if(dev.compare(QLatin1String("default")) == 0) {
#if(SND_LIB_MAJOR == 1 && SND_LIB_MINOR == 0 && SND_LIB_SUBMINOR >= 14)
        if (devices.size() > 0)
            dev = QLatin1String(devices.first().constData());
        else
            return false;
#else
        dev = QLatin1String("hw:0,0");
#endif
    } else {
#if(SND_LIB_MAJOR == 1 && SND_LIB_MINOR == 0 && SND_LIB_SUBMINOR >= 14)
        dev = device;
#else
        int idx = 0;
        char *name;

        QString shortName = device.mid(device.indexOf(QLatin1String("="),0)+1);

	while(snd_card_get_name(idx,&name) == 0) {
            if(dev.contains(QLatin1String(name)))
                break;
            idx++;
	}
        dev = QString(QLatin1String("hw:%1,0")).arg(idx);
#endif
    }
    if(mode == QAudio::AudioOutput) {
        err=snd_pcm_open( &handle,dev.toLocal8Bit().constData(),SND_PCM_STREAM_PLAYBACK,0);
    } else {
        err=snd_pcm_open( &handle,dev.toLocal8Bit().constData(),SND_PCM_STREAM_CAPTURE,0);
    }
    if(err < 0) {
        handle = 0;
        return false;
    }
    return true;
}

void QAudioDeviceInfoInternal::close()
{
    if(handle)
        snd_pcm_close(handle);
    handle = 0;
}

bool QAudioDeviceInfoInternal::testSettings(const QAudioFormat& format) const
{
    // Set nearest to closest settings that do work.
    // See if what is in settings will work (return value).
    int err = 0;
    snd_pcm_t* handle;
    snd_pcm_hw_params_t *params;
    QString dev = device;

    QList<QByteArray> devices = QAudioDeviceInfoInternal::availableDevices(QAudio::AudioOutput);

    if(dev.compare(QLatin1String("default")) == 0) {
#if(SND_LIB_MAJOR == 1 && SND_LIB_MINOR == 0 && SND_LIB_SUBMINOR >= 14)
        dev = QLatin1String(devices.first().constData());
#else
        dev = QLatin1String("hw:0,0");
#endif
    } else {
#if(SND_LIB_MAJOR == 1 && SND_LIB_MINOR == 0 && SND_LIB_SUBMINOR >= 14)
        dev = device;
#else
        int idx = 0;
        char *name;

        QString shortName = device.mid(device.indexOf(QLatin1String("="),0)+1);

        while(snd_card_get_name(idx,&name) == 0) {
            if(shortName.compare(QLatin1String(name)) == 0)
                break;
            idx++;
        }
        dev = QString(QLatin1String("hw:%1,0")).arg(idx);
#endif
    }
    if(mode == QAudio::AudioOutput) {
        err=snd_pcm_open( &handle,dev.toLocal8Bit().constData(),SND_PCM_STREAM_PLAYBACK,0);
    } else {
        err=snd_pcm_open( &handle,dev.toLocal8Bit().constData(),SND_PCM_STREAM_CAPTURE,0);
    }
    if(err < 0) {
        handle = 0;
        return false;
    }

    bool testChannel = false;
    bool testCodec = false;
    bool testSampleRate = false;
    bool testType = false;
    bool testSize = false;

    int  dir = 0;

    snd_pcm_nonblock( handle, 0 );
    snd_pcm_hw_params_alloca( &params );
    snd_pcm_hw_params_any( handle, params );

    // set the values!
    snd_pcm_hw_params_set_channels(handle,params,format.channelCount());
    snd_pcm_hw_params_set_rate(handle,params,format.sampleRate(),dir);

    err = -1;

    switch(format.sampleSize()) {
        case 8:
            if(format.sampleType() == QAudioFormat::SignedInt)
                err = snd_pcm_hw_params_set_format(handle,params,SND_PCM_FORMAT_S8);
            else if(format.sampleType() == QAudioFormat::UnSignedInt)
                err = snd_pcm_hw_params_set_format(handle,params,SND_PCM_FORMAT_U8);
            break;
        case 16:
            if(format.sampleType() == QAudioFormat::SignedInt) {
                if(format.byteOrder() == QAudioFormat::LittleEndian)
                    err = snd_pcm_hw_params_set_format(handle,params,SND_PCM_FORMAT_S16_LE);
                else if(format.byteOrder() == QAudioFormat::BigEndian)
                    err = snd_pcm_hw_params_set_format(handle,params,SND_PCM_FORMAT_S16_BE);
            } else if(format.sampleType() == QAudioFormat::UnSignedInt) {
                if(format.byteOrder() == QAudioFormat::LittleEndian)
                    err = snd_pcm_hw_params_set_format(handle,params,SND_PCM_FORMAT_U16_LE);
                else if(format.byteOrder() == QAudioFormat::BigEndian)
                    err = snd_pcm_hw_params_set_format(handle,params,SND_PCM_FORMAT_U16_BE);
            }
            break;
        case 32:
            if(format.sampleType() == QAudioFormat::SignedInt) {
                if(format.byteOrder() == QAudioFormat::LittleEndian)
                    err = snd_pcm_hw_params_set_format(handle,params,SND_PCM_FORMAT_S32_LE);
                else if(format.byteOrder() == QAudioFormat::BigEndian)
                    err = snd_pcm_hw_params_set_format(handle,params,SND_PCM_FORMAT_S32_BE);
            } else if(format.sampleType() == QAudioFormat::UnSignedInt) {
                if(format.byteOrder() == QAudioFormat::LittleEndian)
                    err = snd_pcm_hw_params_set_format(handle,params,SND_PCM_FORMAT_U32_LE);
                else if(format.byteOrder() == QAudioFormat::BigEndian)
                    err = snd_pcm_hw_params_set_format(handle,params,SND_PCM_FORMAT_U32_BE);
            } else if (format.sampleType() == QAudioFormat::Float) {
                if (format.byteOrder() == QAudioFormat::LittleEndian)
                    err = snd_pcm_hw_params_set_format(handle,params,SND_PCM_FORMAT_FLOAT_LE);
                else if (format.byteOrder() == QAudioFormat::BigEndian)
                    err = snd_pcm_hw_params_set_format(handle,params,SND_PCM_FORMAT_FLOAT_BE);
            }
    }

    // For now, just accept only audio/pcm codec
    if(!format.codec().startsWith(QLatin1String("audio/pcm"))) {
        err=-1;
    } else
        testCodec = true;

    if (err>=0 && format.channelCount() != -1) {
        err = snd_pcm_hw_params_test_channels(handle,params,format.channelCount());
        if(err>=0)
            err = snd_pcm_hw_params_set_channels(handle,params,format.channelCount());
        if(err>=0)
            testChannel = true;
    }

    if (err>=0 && format.sampleRate() != -1) {
        err = snd_pcm_hw_params_test_rate(handle,params,format.sampleRate(),0);
        if(err>=0)
            err = snd_pcm_hw_params_set_rate(handle,params,format.sampleRate(),dir);
        if(err>=0)
            testSampleRate = true;
    }

    if((err>=0 && format.sampleSize() != -1) &&
            (format.sampleType() != QAudioFormat::Unknown)) {
        switch(format.sampleSize()) {
            case 8:
                if(format.sampleType() == QAudioFormat::SignedInt)
                    err = snd_pcm_hw_params_set_format(handle,params,SND_PCM_FORMAT_S8);
                else if(format.sampleType() == QAudioFormat::UnSignedInt)
                    err = snd_pcm_hw_params_set_format(handle,params,SND_PCM_FORMAT_U8);
                break;
            case 16:
                if(format.sampleType() == QAudioFormat::SignedInt) {
                    if(format.byteOrder() == QAudioFormat::LittleEndian)
                        err = snd_pcm_hw_params_set_format(handle,params,SND_PCM_FORMAT_S16_LE);
                    else if(format.byteOrder() == QAudioFormat::BigEndian)
                        err = snd_pcm_hw_params_set_format(handle,params,SND_PCM_FORMAT_S16_BE);
                } else if(format.sampleType() == QAudioFormat::UnSignedInt) {
                    if(format.byteOrder() == QAudioFormat::LittleEndian)
                        err = snd_pcm_hw_params_set_format(handle,params,SND_PCM_FORMAT_U16_LE);
                    else if(format.byteOrder() == QAudioFormat::BigEndian)
                        err = snd_pcm_hw_params_set_format(handle,params,SND_PCM_FORMAT_U16_BE);
                }
                break;
            case 32:
                if(format.sampleType() == QAudioFormat::SignedInt) {
                    if(format.byteOrder() == QAudioFormat::LittleEndian)
                        err = snd_pcm_hw_params_set_format(handle,params,SND_PCM_FORMAT_S32_LE);
                    else if(format.byteOrder() == QAudioFormat::BigEndian)
                        err = snd_pcm_hw_params_set_format(handle,params,SND_PCM_FORMAT_S32_BE);
                } else if(format.sampleType() == QAudioFormat::UnSignedInt) {
                    if(format.byteOrder() == QAudioFormat::LittleEndian)
                        err = snd_pcm_hw_params_set_format(handle,params,SND_PCM_FORMAT_U32_LE);
                    else if(format.byteOrder() == QAudioFormat::BigEndian)
                        err = snd_pcm_hw_params_set_format(handle,params,SND_PCM_FORMAT_U32_BE);
                } else if (format.sampleType() == QAudioFormat::Float) {
                    if (format.byteOrder() == QAudioFormat::LittleEndian)
                        err = snd_pcm_hw_params_set_format(handle,params,SND_PCM_FORMAT_FLOAT_LE);
                    else if (format.byteOrder() == QAudioFormat::BigEndian)
                        err = snd_pcm_hw_params_set_format(handle,params,SND_PCM_FORMAT_FLOAT_BE);
                }
        }
        if(err>=0) {
            testSize = true;
            testType = true;
        }
    }
    if(err>=0)
        err = snd_pcm_hw_params(handle, params);

    if(err == 0) {
        // settings work
        // close()
        if(handle)
            snd_pcm_close(handle);
        return true;
    }
    if(handle)
        snd_pcm_close(handle);

    return false;
}

void QAudioDeviceInfoInternal::updateLists()
{
    // redo all lists based on current settings
    sampleRatez.clear();
    channelz.clear();
    sizez.clear();
    byteOrderz.clear();
    typez.clear();
    codecz.clear();

    if(!handle)
        open();

    if(!handle)
        return;

    for(int i=0; i<(int)MAX_SAMPLE_RATES; i++) {
        //if(snd_pcm_hw_params_test_rate(handle, params, SAMPLE_RATES[i], dir) == 0)
        sampleRatez.append(SAMPLE_RATES[i]);
    }
    channelz.append(1);
    channelz.append(2);
    if (surround40) channelz.append(4);
    if (surround51) channelz.append(6);
    if (surround71) channelz.append(8);
    sizez.append(8);
    sizez.append(16);
    sizez.append(32);
    byteOrderz.append(QAudioFormat::LittleEndian);
    byteOrderz.append(QAudioFormat::BigEndian);
    typez.append(QAudioFormat::SignedInt);
    typez.append(QAudioFormat::UnSignedInt);
    typez.append(QAudioFormat::Float);
    codecz.append(QLatin1String("audio/pcm"));
    close();
}

QList<QByteArray> QAudioDeviceInfoInternal::availableDevices(QAudio::Mode mode)
{
    QList<QByteArray> devices;
    QByteArray filter;

#if(SND_LIB_MAJOR == 1 && SND_LIB_MINOR == 0 && SND_LIB_SUBMINOR >= 14)
    // Create a list of all current audio devices that support mode
    void **hints, **n;
    char *name, *descr, *io;

    if(snd_device_name_hint(-1, "pcm", &hints) < 0) {
        qWarning() << "no alsa devices available";
        return devices;
    }
    n = hints;

    if(mode == QAudio::AudioInput) {
        filter = "Input";
    } else {
        filter = "Output";
    }

    while (*n != NULL) {
        name = snd_device_name_get_hint(*n, "NAME");
        if (name != 0 && qstrcmp(name, "null") != 0) {
            descr = snd_device_name_get_hint(*n, "DESC");
            io = snd_device_name_get_hint(*n, "IOID");

            if ((descr != NULL) && ((io == NULL) || (io == filter))) {
                QString deviceName = QLatin1String(name);
                QString deviceDescription = QLatin1String(descr);
                if (deviceDescription.contains(QLatin1String("Default Audio Device")))
                    devices.prepend(deviceName.toLocal8Bit().constData());
                else
                    devices.append(deviceName.toLocal8Bit().constData());
            }

            free(name);
            if (descr != NULL)
                free(descr);
            if (io != NULL)
                free(io);
        }
        ++n;
    }
    snd_device_name_free_hint(hints);
#else
    int idx = 0;
    char* name;

    while(snd_card_get_name(idx,&name) == 0) {
        devices.append(name);
        idx++;
    }
#endif

    if (devices.size() > 0)
        devices.append("default");

    return devices;
}

QByteArray QAudioDeviceInfoInternal::defaultInputDevice()
{
    QList<QByteArray> devices = availableDevices(QAudio::AudioInput);
    if(devices.size() == 0)
        return QByteArray();

    return devices.first();
}

QByteArray QAudioDeviceInfoInternal::defaultOutputDevice()
{
    QList<QByteArray> devices = availableDevices(QAudio::AudioOutput);
    if(devices.size() == 0)
        return QByteArray();

    return devices.first();
}

void QAudioDeviceInfoInternal::checkSurround()
{
    surround40 = false;
    surround51 = false;
    surround71 = false;

    void **hints, **n;
    char *name, *descr, *io;

    if(snd_device_name_hint(-1, "pcm", &hints) < 0)
        return;

    n = hints;

    while (*n != NULL) {
        name = snd_device_name_get_hint(*n, "NAME");
        descr = snd_device_name_get_hint(*n, "DESC");
        io = snd_device_name_get_hint(*n, "IOID");
        if((name != NULL) && (descr != NULL)) {
            QString deviceName = QLatin1String(name);
            if (mode == QAudio::AudioOutput) {
                if(deviceName.contains(QLatin1String("surround40")))
                    surround40 = true;
                if(deviceName.contains(QLatin1String("surround51")))
                    surround51 = true;
                if(deviceName.contains(QLatin1String("surround71")))
                    surround71 = true;
            }
        }
        if(name != NULL)
            free(name);
        if(descr != NULL)
            free(descr);
        if(io != NULL)
            free(io);
        ++n;
    }
    snd_device_name_free_hint(hints);
}

QT_END_NAMESPACE
