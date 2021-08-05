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

#ifndef QWINDOWSAUDIOUTILS_H
#define QWINDOWSAUDIOUTILS_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

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

#if defined(Q_CC_MINGW) && !defined(__MINGW64_VERSION_MAJOR)
struct IBaseFilter; // Needed for strmif.h from stock MinGW.
struct _DDPIXELFORMAT;
typedef struct _DDPIXELFORMAT* LPDDPIXELFORMAT;
#endif

#include <strmif.h>
#if !defined(Q_CC_MINGW) || defined(__MINGW64_VERSION_MAJOR)
#  include <uuids.h>
#else

extern GUID CLSID_AudioInputDeviceCategory;
extern GUID CLSID_AudioRendererCategory;
extern GUID IID_ICreateDevEnum;
extern GUID CLSID_SystemDeviceEnum;

#ifndef __ICreateDevEnum_INTERFACE_DEFINED__
#define __ICreateDevEnum_INTERFACE_DEFINED__

DECLARE_INTERFACE_(ICreateDevEnum, IUnknown)
{
    STDMETHOD(CreateClassEnumerator)(REFCLSID clsidDeviceClass,
                                     IEnumMoniker **ppEnumMoniker,
                                     DWORD dwFlags) PURE;
};

#endif //  __ICreateDevEnum_INTERFACE_DEFINED__

#ifndef __IErrorLog_INTERFACE_DEFINED__
#define __IErrorLog_INTERFACE_DEFINED__

DECLARE_INTERFACE_(IErrorLog, IUnknown)
{
    STDMETHOD(AddError)(THIS_ LPCOLESTR, EXCEPINFO *) PURE;
};

#endif /* __IErrorLog_INTERFACE_DEFINED__ */

#ifndef __IPropertyBag_INTERFACE_DEFINED__
#define __IPropertyBag_INTERFACE_DEFINED__

const GUID IID_IPropertyBag = {0x55272A00, 0x42CB, 0x11CE, {0x81, 0x35, 0x00, 0xAA, 0x00, 0x4B, 0xB8, 0x51}};

DECLARE_INTERFACE_(IPropertyBag, IUnknown)
{
    STDMETHOD(Read)(THIS_ LPCOLESTR, VARIANT *, IErrorLog *) PURE;
    STDMETHOD(Write)(THIS_ LPCOLESTR, VARIANT *) PURE;
};

#endif /* __IPropertyBag_INTERFACE_DEFINED__ */

#endif // defined(Q_CC_MINGW) && !defined(__MINGW64_VERSION_MAJOR)

// For mingw toolchain mmsystem.h only defines half the defines, so add if needed.
#ifndef WAVE_FORMAT_44M08
#define WAVE_FORMAT_44M08 0x00000100
#define WAVE_FORMAT_44S08 0x00000200
#define WAVE_FORMAT_44M16 0x00000400
#define WAVE_FORMAT_44S16 0x00000800
#define WAVE_FORMAT_48M08 0x00001000
#define WAVE_FORMAT_48S08 0x00002000
#define WAVE_FORMAT_48M16 0x00004000
#define WAVE_FORMAT_48S16 0x00008000
#define WAVE_FORMAT_96M08 0x00010000
#define WAVE_FORMAT_96S08 0x00020000
#define WAVE_FORMAT_96M16 0x00040000
#define WAVE_FORMAT_96S16 0x00080000
#endif

QT_BEGIN_NAMESPACE

bool qt_convertFormat(const QAudioFormat &format, WAVEFORMATEXTENSIBLE *wfx);

QT_END_NAMESPACE

#endif // QWINDOWSAUDIOUTILS_H
