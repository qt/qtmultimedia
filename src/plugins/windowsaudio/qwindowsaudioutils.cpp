/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qwindowsaudioutils.h"

#ifndef SPEAKER_FRONT_LEFT
    #define SPEAKER_FRONT_LEFT            0x00000001
    #define SPEAKER_FRONT_RIGHT           0x00000002
    #define SPEAKER_FRONT_CENTER          0x00000004
    #define SPEAKER_LOW_FREQUENCY         0x00000008
    #define SPEAKER_BACK_LEFT             0x00000010
    #define SPEAKER_BACK_RIGHT            0x00000020
    #define SPEAKER_FRONT_LEFT_OF_CENTER  0x00000040
    #define SPEAKER_FRONT_RIGHT_OF_CENTER 0x00000080
    #define SPEAKER_BACK_CENTER           0x00000100
    #define SPEAKER_SIDE_LEFT             0x00000200
    #define SPEAKER_SIDE_RIGHT            0x00000400
    #define SPEAKER_TOP_CENTER            0x00000800
    #define SPEAKER_TOP_FRONT_LEFT        0x00001000
    #define SPEAKER_TOP_FRONT_CENTER      0x00002000
    #define SPEAKER_TOP_FRONT_RIGHT       0x00004000
    #define SPEAKER_TOP_BACK_LEFT         0x00008000
    #define SPEAKER_TOP_BACK_CENTER       0x00010000
    #define SPEAKER_TOP_BACK_RIGHT        0x00020000
    #define SPEAKER_RESERVED              0x7FFC0000
    #define SPEAKER_ALL                   0x80000000
#endif

#ifndef WAVE_FORMAT_EXTENSIBLE
    #define WAVE_FORMAT_EXTENSIBLE 0xFFFE
#endif

#ifndef WAVE_FORMAT_IEEE_FLOAT
    #define WAVE_FORMAT_IEEE_FLOAT 0x0003
#endif

static const GUID _KSDATAFORMAT_SUBTYPE_PCM = {
     0x00000001, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};

static const GUID _KSDATAFORMAT_SUBTYPE_IEEE_FLOAT = {
     0x00000003, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};

QT_BEGIN_NAMESPACE

bool qt_convertFormat(const QAudioFormat &format, WAVEFORMATEXTENSIBLE *wfx)
{
    if (!wfx
            || !format.isValid()
            || format.codec() != QStringLiteral("audio/pcm")
            || format.sampleRate() <= 0
            || format.channelCount() <= 0
            || format.sampleSize() <= 0
            || format.byteOrder() != QAudioFormat::LittleEndian) {
        return false;
    }

    wfx->Format.nSamplesPerSec = format.sampleRate();
    wfx->Format.wBitsPerSample = wfx->Samples.wValidBitsPerSample = format.sampleSize();
    wfx->Format.nChannels = format.channelCount();
    wfx->Format.nBlockAlign = (wfx->Format.wBitsPerSample / 8) * wfx->Format.nChannels;
    wfx->Format.nAvgBytesPerSec = wfx->Format.nBlockAlign * wfx->Format.nSamplesPerSec;
    wfx->Format.cbSize = 0;

    if (format.sampleType() == QAudioFormat::Float) {
        wfx->Format.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
        wfx->SubFormat = _KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
    } else {
        wfx->Format.wFormatTag = WAVE_FORMAT_PCM;
        wfx->SubFormat = _KSDATAFORMAT_SUBTYPE_PCM;
    }

    if (format.channelCount() > 2) {
        wfx->Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
        wfx->Format.cbSize = 22;
        wfx->dwChannelMask = 0xFFFFFFFF >> (32 - format.channelCount());
    }

    return true;
}

QT_END_NAMESPACE
