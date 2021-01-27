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


#include <QtCore/qt_windows.h>
#include <QtCore/QDataStream>
#include <QtCore/QIODevice>
#include <mmsystem.h>
#include "qwindowsaudiodeviceinfo_p.h"
#include "qwindowsaudioutils_p.h"


QWindowsAudioDeviceInfo::QWindowsAudioDeviceInfo(QByteArray dev, int waveID, const QString &description, QAudio::Mode mode)
    : QAudioDeviceInfoPrivate(dev, mode),
      m_description(description),
      devId(waveID)
{
    updateLists();
}

QWindowsAudioDeviceInfo::~QWindowsAudioDeviceInfo()
{
    close();
}

bool QWindowsAudioDeviceInfo::isFormatSupported(const QAudioFormat& format) const
{
    return testSettings(format);
}

QAudioFormat QWindowsAudioDeviceInfo::preferredFormat() const
{
    QAudioFormat nearest;
    if (mode == QAudio::AudioOutput) {
        nearest.setSampleRate(44100);
        nearest.setChannelCount(2);
        nearest.setByteOrder(QAudioFormat::LittleEndian);
        nearest.setSampleType(QAudioFormat::SignedInt);
        nearest.setSampleSize(16);
        nearest.setCodec(QLatin1String("audio/x-raw"));
    } else {
        nearest.setSampleRate(11025);
        nearest.setChannelCount(1);
        nearest.setByteOrder(QAudioFormat::LittleEndian);
        nearest.setSampleType(QAudioFormat::SignedInt);
        nearest.setSampleSize(8);
        nearest.setCodec(QLatin1String("audio/x-raw"));
    }
    return nearest;
}

QList<int> QWindowsAudioDeviceInfo::supportedSampleRates() const
{
    return sampleRatez;
}

QList<int> QWindowsAudioDeviceInfo::supportedChannelCounts() const
{
    return channelz;
}

QList<int> QWindowsAudioDeviceInfo::supportedSampleSizes() const
{
    return sizez;
}

QList<QAudioFormat::Endian> QWindowsAudioDeviceInfo::supportedByteOrders() const
{
    return QList<QAudioFormat::Endian>() << QAudioFormat::LittleEndian;
}

QList<QAudioFormat::SampleType> QWindowsAudioDeviceInfo::supportedSampleTypes() const
{
    return typez;
}


bool QWindowsAudioDeviceInfo::open()
{
    return true;
}

void QWindowsAudioDeviceInfo::close()
{
}

bool QWindowsAudioDeviceInfo::testSettings(const QAudioFormat& format) const
{
    WAVEFORMATEXTENSIBLE wfx;
    if (qt_convertFormat(format, &wfx)) {
        // query only, do not open device
        if (mode == QAudio::AudioOutput) {
            return (waveOutOpen(NULL, UINT_PTR(devId), &wfx.Format, 0, 0,
                                WAVE_FORMAT_QUERY) == MMSYSERR_NOERROR);
        } else { // AudioInput
            return (waveInOpen(NULL, UINT_PTR(devId), &wfx.Format, 0, 0,
                                WAVE_FORMAT_QUERY) == MMSYSERR_NOERROR);
        }
    }

    return false;
}

void QWindowsAudioDeviceInfo::updateLists()
{
    if (!sizez.isEmpty())
        return;

    DWORD fmt = 0;

    if(mode == QAudio::AudioOutput) {
        WAVEOUTCAPS woc;
        if (waveOutGetDevCaps(devId, &woc, sizeof(WAVEOUTCAPS)) == MMSYSERR_NOERROR)
            fmt = woc.dwFormats;
    } else {
        WAVEINCAPS woc;
        if (waveInGetDevCaps(devId, &woc, sizeof(WAVEINCAPS)) == MMSYSERR_NOERROR)
            fmt = woc.dwFormats;
    }

    sizez.clear();
    sampleRatez.clear();
    channelz.clear();
    typez.clear();

    if (fmt) {
        // Check sample size
        if ((fmt & WAVE_FORMAT_1M08)
            || (fmt & WAVE_FORMAT_1S08)
            || (fmt & WAVE_FORMAT_2M08)
            || (fmt & WAVE_FORMAT_2S08)
            || (fmt & WAVE_FORMAT_4M08)
            || (fmt & WAVE_FORMAT_4S08)
            || (fmt & WAVE_FORMAT_48M08)
            || (fmt & WAVE_FORMAT_48S08)
            || (fmt & WAVE_FORMAT_96M08)
            || (fmt & WAVE_FORMAT_96S08)) {
            sizez.append(8);
        }
        if ((fmt & WAVE_FORMAT_1M16)
            || (fmt & WAVE_FORMAT_1S16)
            || (fmt & WAVE_FORMAT_2M16)
            || (fmt & WAVE_FORMAT_2S16)
            || (fmt & WAVE_FORMAT_4M16)
            || (fmt & WAVE_FORMAT_4S16)
            || (fmt & WAVE_FORMAT_48M16)
            || (fmt & WAVE_FORMAT_48S16)
            || (fmt & WAVE_FORMAT_96M16)
            || (fmt & WAVE_FORMAT_96S16)) {
            sizez.append(16);
        }

        // Check sample rate
        if ((fmt & WAVE_FORMAT_1M08)
           || (fmt & WAVE_FORMAT_1S08)
           || (fmt & WAVE_FORMAT_1M16)
           || (fmt & WAVE_FORMAT_1S16)) {
            sampleRatez.append(11025);
        }
        if ((fmt & WAVE_FORMAT_2M08)
           || (fmt & WAVE_FORMAT_2S08)
           || (fmt & WAVE_FORMAT_2M16)
           || (fmt & WAVE_FORMAT_2S16)) {
            sampleRatez.append(22050);
        }
        if ((fmt & WAVE_FORMAT_4M08)
           || (fmt & WAVE_FORMAT_4S08)
           || (fmt & WAVE_FORMAT_4M16)
           || (fmt & WAVE_FORMAT_4S16)) {
            sampleRatez.append(44100);
        }
        if ((fmt & WAVE_FORMAT_48M08)
            || (fmt & WAVE_FORMAT_48S08)
            || (fmt & WAVE_FORMAT_48M16)
            || (fmt & WAVE_FORMAT_48S16)) {
            sampleRatez.append(48000);
        }
        if ((fmt & WAVE_FORMAT_96M08)
           || (fmt & WAVE_FORMAT_96S08)
           || (fmt & WAVE_FORMAT_96M16)
           || (fmt & WAVE_FORMAT_96S16)) {
            sampleRatez.append(96000);
        }

        // Check channel count
        if (fmt & WAVE_FORMAT_1M08
                || fmt & WAVE_FORMAT_1M16
                || fmt & WAVE_FORMAT_2M08
                || fmt & WAVE_FORMAT_2M16
                || fmt & WAVE_FORMAT_4M08
                || fmt & WAVE_FORMAT_4M16
                || fmt & WAVE_FORMAT_48M08
                || fmt & WAVE_FORMAT_48M16
                || fmt & WAVE_FORMAT_96M08
                || fmt & WAVE_FORMAT_96M16) {
            channelz.append(1);
        }
        if (fmt & WAVE_FORMAT_1S08
                || fmt & WAVE_FORMAT_1S16
                || fmt & WAVE_FORMAT_2S08
                || fmt & WAVE_FORMAT_2S16
                || fmt & WAVE_FORMAT_4S08
                || fmt & WAVE_FORMAT_4S16
                || fmt & WAVE_FORMAT_48S08
                || fmt & WAVE_FORMAT_48S16
                || fmt & WAVE_FORMAT_96S08
                || fmt & WAVE_FORMAT_96S16) {
            channelz.append(2);
        }

        typez.append(QAudioFormat::SignedInt);
        typez.append(QAudioFormat::UnSignedInt);

        // WAVEOUTCAPS and WAVEINCAPS contains information only for the previously tested parameters.
        // WaveOut and WaveInt might actually support more formats, the only way to know is to try
        // opening the device with it.
        QAudioFormat testFormat;
        testFormat.setCodec(QStringLiteral("audio/x-raw"));
        testFormat.setByteOrder(QAudioFormat::LittleEndian);
        testFormat.setSampleType(QAudioFormat::SignedInt);
        testFormat.setChannelCount(channelz.first());
        testFormat.setSampleRate(sampleRatez.at(sampleRatez.size() / 2));
        testFormat.setSampleSize(sizez.last());
        const QAudioFormat defaultTestFormat(testFormat);

        // Check if float samples are supported
        testFormat.setSampleType(QAudioFormat::Float);
        testFormat.setSampleSize(32);
        if (testSettings(testFormat))
            typez.append(QAudioFormat::Float);

        // Check channel counts > 2
        testFormat = defaultTestFormat;
        for (int i = 3; i < 19; ++i) { // <mmreg.h> defines 18 different channels
            testFormat.setChannelCount(i);
            if (testSettings(testFormat))
                channelz.append(i);
        }

        // Check more sample sizes
        testFormat = defaultTestFormat;
        const QList<int> testSampleSizes = QList<int>() << 24 << 32 << 48 << 64;
        for (int s : testSampleSizes) {
            testFormat.setSampleSize(s);
            if (testSettings(testFormat))
                sizez.append(s);
        }

        // Check more sample rates
        testFormat = defaultTestFormat;
        const QList<int> testSampleRates = QList<int>() << 8000 << 16000 << 32000 << 88200 << 192000;
        for (int r : testSampleRates) {
            testFormat.setSampleRate(r);
            if (testSettings(testFormat))
                sampleRatez.append(r);
        }
        std::sort(sampleRatez.begin(), sampleRatez.end());
    }
}

QT_END_NAMESPACE
