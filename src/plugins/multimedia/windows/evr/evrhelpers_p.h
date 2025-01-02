// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef EVRHELPERS_H
#define EVRHELPERS_H

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

#include <qvideoframe.h>
#include <d3d9.h>
#include <dxva2api.h>
#include <evr9.h>
#include <evr.h>
#include <mfidl.h>
#include <mfapi.h>
#include <mferror.h>

QT_BEGIN_NAMESPACE

template<class T>
static inline void qt_evr_safe_release(T **unk)
{
    if (*unk) {
        (*unk)->Release();
        *unk = NULL;
    }
}

HRESULT qt_evr_getFourCC(IMFMediaType *type, DWORD *fourCC);

bool qt_evr_areMediaTypesEqual(IMFMediaType *type1, IMFMediaType *type2);

HRESULT qt_evr_validateVideoArea(const MFVideoArea& area, UINT32 width, UINT32 height);

bool qt_evr_isSampleTimePassed(IMFClock *clock, IMFSample *sample);

inline float qt_evr_MFOffsetToFloat(const MFOffset& offset)
{
    return offset.value + (float(offset.fract) / 65536);
}

inline MFOffset qt_evr_makeMFOffset(float v)
{
    MFOffset offset;
    offset.value = short(v);
    offset.fract = WORD(65536 * (v-offset.value));
    return offset;
}

inline MFVideoArea qt_evr_makeMFArea(float x, float y, DWORD width, DWORD height)
{
    MFVideoArea area;
    area.OffsetX = qt_evr_makeMFOffset(x);
    area.OffsetY = qt_evr_makeMFOffset(y);
    area.Area.cx = width;
    area.Area.cy = height;
    return area;
}

inline HRESULT qt_evr_getFrameRate(IMFMediaType *pType, MFRatio *pRatio)
{
    return MFGetAttributeRatio(pType, MF_MT_FRAME_RATE,
                               reinterpret_cast<UINT32*>(&pRatio->Numerator),
                               reinterpret_cast<UINT32*>(&pRatio->Denominator));
}

QVideoFrameFormat::PixelFormat qt_evr_pixelFormatFromD3DFormat(DWORD format);
D3DFORMAT qt_evr_D3DFormatFromPixelFormat(QVideoFrameFormat::PixelFormat format);

QT_END_NAMESPACE

#endif // EVRHELPERS_H

