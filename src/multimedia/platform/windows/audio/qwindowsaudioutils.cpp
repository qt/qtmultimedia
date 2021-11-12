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

#include "qwindowsaudioutils_p.h"

QT_BEGIN_NAMESPACE

bool QWindowsAudioUtils::formatToWaveFormatExtensible(const QAudioFormat &format, WAVEFORMATEXTENSIBLE &wfx)
{
    if (!format.isValid())
        return false;

    wfx.Format.nSamplesPerSec = format.sampleRate();
    wfx.Format.wBitsPerSample = wfx.Samples.wValidBitsPerSample = format.bytesPerSample()*8;
    wfx.Format.nChannels = format.channelCount();
    wfx.Format.nBlockAlign = (wfx.Format.wBitsPerSample / 8) * wfx.Format.nChannels;
    wfx.Format.nAvgBytesPerSec = wfx.Format.nBlockAlign * wfx.Format.nSamplesPerSec;
    wfx.Format.cbSize = 0;

    if (format.sampleFormat() == QAudioFormat::Float) {
        wfx.Format.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
        wfx.SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
    } else {
        wfx.Format.wFormatTag = WAVE_FORMAT_PCM;
        wfx.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
    }

    if (format.channelCount() > 2) {
        wfx.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
        wfx.Format.cbSize = 22;
        wfx.dwChannelMask = 0xFFFFFFFF >> (32 - format.channelCount());
    }

    return true;
}

QAudioFormat QWindowsAudioUtils::waveFormatExToFormat(const WAVEFORMATEX &in)
{
    QAudioFormat out;
    out.setSampleRate(in.nSamplesPerSec);
    out.setChannelCount(in.nChannels);
    if (in.wFormatTag == WAVE_FORMAT_PCM) {
        if (in.wBitsPerSample == 8)
            out.setSampleFormat(QAudioFormat::UInt8);
        else if (in.wBitsPerSample == 16)
            out.setSampleFormat(QAudioFormat::Int16);
        else if (in.wBitsPerSample == 32)
            out.setSampleFormat(QAudioFormat::Int32);
    } else if (in.wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
        if (in.cbSize >= 22) {
            auto wfe = reinterpret_cast<const WAVEFORMATEXTENSIBLE &>(in);
            if (wfe.SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)
                out.setSampleFormat(QAudioFormat::Float);
        }
    } else if (in.wFormatTag == WAVE_FORMAT_IEEE_FLOAT) {
        out.setSampleFormat(QAudioFormat::Float);
    }

    return out;
}

QAudioFormat QWindowsAudioUtils::mediaTypeToFormat(IMFMediaType *mediaType)
{
    QAudioFormat format;
    if (!mediaType)
        return format;

    UINT32 val = 0;
    if (SUCCEEDED(mediaType->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &val))) {
        format.setChannelCount(int(val));
    }
    if (SUCCEEDED(mediaType->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &val))) {
        format.setSampleRate(int(val));
    }
    UINT32 bitsPerSample = 0;
    mediaType->GetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, &bitsPerSample);

    GUID subType;
    if (SUCCEEDED(mediaType->GetGUID(MF_MT_SUBTYPE, &subType))) {
        if (subType == MFAudioFormat_Float) {
            format.setSampleFormat(QAudioFormat::Float);
        } else if (bitsPerSample == 8) {
            format.setSampleFormat(QAudioFormat::UInt8);
        } else if (bitsPerSample == 16) {
            format.setSampleFormat(QAudioFormat::Int16);
        } else if (bitsPerSample == 32){
            format.setSampleFormat(QAudioFormat::Int32);
        }
    }
    return format;
}

QWindowsIUPointer<IMFMediaType> QWindowsAudioUtils::formatToMediaType(const QAudioFormat &format)
{
    QWindowsIUPointer<IMFMediaType> mediaType;

    if (!format.isValid())
        return mediaType;

    MFCreateMediaType(mediaType.address());

    mediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
    if (format.sampleFormat() == QAudioFormat::Float) {
        mediaType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_Float);
    } else {
        mediaType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
    }

    mediaType->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, UINT32(format.channelCount()));
    mediaType->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, UINT32(format.sampleRate()));
    auto alignmentBlock = UINT32(format.bytesPerFrame());
    mediaType->SetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, alignmentBlock);
    auto avgBytesPerSec = UINT32(format.sampleRate() * format.bytesPerFrame());
    mediaType->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, avgBytesPerSec);
    mediaType->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, UINT32(format.bytesPerSample()*8));
    mediaType->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE);

    return mediaType;
}

QT_END_NAMESPACE
