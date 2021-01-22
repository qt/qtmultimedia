/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QWINDOWSAUDIOUTILS_H
#define QWINDOWSAUDIOUTILS_H

#include <qaudioformat.h>
#include <QtCore/qt_windows.h>
#include <mmsystem.h>

#ifndef _WAVEFORMATEXTENSIBLE_

    #define _WAVEFORMATEXTENSIBLE_
    typedef struct
    {
        WAVEFORMATEX Format;          // Base WAVEFORMATEX data
        union
        {
            WORD wValidBitsPerSample; // Valid bits in each sample container
            WORD wSamplesPerBlock;    // Samples per block of audio data; valid
                                      // if wBitsPerSample=0 (but rarely used).
            WORD wReserved;           // Zero if neither case above applies.
        } Samples;
        DWORD dwChannelMask;          // Positions of the audio channels
        GUID SubFormat;               // Format identifier GUID
    } WAVEFORMATEXTENSIBLE, *PWAVEFORMATEXTENSIBLE, *LPPWAVEFORMATEXTENSIBLE;
    typedef const WAVEFORMATEXTENSIBLE* LPCWAVEFORMATEXTENSIBLE;

#endif

QT_BEGIN_NAMESPACE

bool qt_convertFormat(const QAudioFormat &format, WAVEFORMATEXTENSIBLE *wfx);

QT_END_NAMESPACE

#endif // QWINDOWSAUDIOUTILS_H
