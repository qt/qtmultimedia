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

#ifndef EVRHELPERS_H
#define EVRHELPERS_H

#include "evrdefs.h"
#include <qvideoframe.h>

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

QVideoFrame::PixelFormat qt_evr_pixelFormatFromD3DFormat(DWORD format);
D3DFORMAT qt_evr_D3DFormatFromPixelFormat(QVideoFrame::PixelFormat format);

QT_END_NAMESPACE

#endif // EVRHELPERS_H

