// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "evrhelpers_p.h"

#include <cguid.h>

#ifndef D3DFMT_YV12
#define D3DFMT_YV12 (D3DFORMAT)MAKEFOURCC ('Y', 'V', '1', '2')
#endif
#ifndef D3DFMT_NV12
#define D3DFMT_NV12 (D3DFORMAT)MAKEFOURCC ('N', 'V', '1', '2')
#endif

QT_BEGIN_NAMESPACE

HRESULT qt_evr_getFourCC(IMFMediaType *type, DWORD *fourCC)
{
    if (!fourCC)
        return E_POINTER;

    HRESULT hr = S_OK;
    GUID guidSubType = GUID_NULL;

    if (SUCCEEDED(hr))
        hr = type->GetGUID(MF_MT_SUBTYPE, &guidSubType);

    if (SUCCEEDED(hr))
        *fourCC = guidSubType.Data1;

    return hr;
}

bool qt_evr_areMediaTypesEqual(IMFMediaType *type1, IMFMediaType *type2)
{
    if (!type1 && !type2)
        return true;
    if (!type1 || !type2)
        return false;

    DWORD dwFlags = 0;
    HRESULT hr = type1->IsEqual(type2, &dwFlags);

    return (hr == S_OK);
}

HRESULT qt_evr_validateVideoArea(const MFVideoArea& area, UINT32 width, UINT32 height)
{
    float fOffsetX = qt_evr_MFOffsetToFloat(area.OffsetX);
    float fOffsetY = qt_evr_MFOffsetToFloat(area.OffsetY);

    if ( ((LONG)fOffsetX + area.Area.cx > (LONG)width) ||
         ((LONG)fOffsetY + area.Area.cy > (LONG)height) ) {
        return MF_E_INVALIDMEDIATYPE;
    }
    return S_OK;
}

bool qt_evr_isSampleTimePassed(IMFClock *clock, IMFSample *sample)
{
    if (!sample || !clock)
        return false;

    HRESULT hr = S_OK;
    MFTIME hnsTimeNow = 0;
    MFTIME hnsSystemTime = 0;
    MFTIME hnsSampleStart = 0;
    MFTIME hnsSampleDuration = 0;

    hr = clock->GetCorrelatedTime(0, &hnsTimeNow, &hnsSystemTime);

    if (SUCCEEDED(hr))
        hr = sample->GetSampleTime(&hnsSampleStart);

    if (SUCCEEDED(hr))
        hr = sample->GetSampleDuration(&hnsSampleDuration);

    if (SUCCEEDED(hr)) {
        if (hnsSampleStart + hnsSampleDuration < hnsTimeNow)
            return true;
    }

    return false;
}

QVideoFrameFormat::PixelFormat qt_evr_pixelFormatFromD3DFormat(DWORD format)
{
    switch (format) {
    case D3DFMT_A8R8G8B8:
        return QVideoFrameFormat::Format_BGRA8888;
    case D3DFMT_X8R8G8B8:
        return QVideoFrameFormat::Format_BGRX8888;
    case D3DFMT_A8:
        return QVideoFrameFormat::Format_Y8;
    case D3DFMT_A8B8G8R8:
        return QVideoFrameFormat::Format_RGBA8888;
    case D3DFMT_X8B8G8R8:
        return QVideoFrameFormat::Format_RGBX8888;
    case D3DFMT_UYVY:
        return QVideoFrameFormat::Format_UYVY;
    case D3DFMT_YUY2:
        return QVideoFrameFormat::Format_YUYV;
    case D3DFMT_NV12:
        return QVideoFrameFormat::Format_NV12;
    case D3DFMT_YV12:
        return QVideoFrameFormat::Format_YV12;
    case D3DFMT_UNKNOWN:
    default:
        return QVideoFrameFormat::Format_Invalid;
    }
}

D3DFORMAT qt_evr_D3DFormatFromPixelFormat(QVideoFrameFormat::PixelFormat format)
{
    switch (format) {
    case QVideoFrameFormat::Format_ARGB8888:
        return D3DFMT_A8B8G8R8;
    case QVideoFrameFormat::Format_BGRA8888:
        return D3DFMT_A8R8G8B8;
    case QVideoFrameFormat::Format_BGRX8888:
        return D3DFMT_X8R8G8B8;
    case QVideoFrameFormat::Format_Y8:
        return D3DFMT_A8;
    case QVideoFrameFormat::Format_RGBA8888:
        return D3DFMT_A8B8G8R8;
    case QVideoFrameFormat::Format_RGBX8888:
        return D3DFMT_X8B8G8R8;
    case QVideoFrameFormat::Format_UYVY:
        return D3DFMT_UYVY;
    case QVideoFrameFormat::Format_YUYV:
        return D3DFMT_YUY2;
    case QVideoFrameFormat::Format_NV12:
        return D3DFMT_NV12;
    case QVideoFrameFormat::Format_YV12:
        return D3DFMT_YV12;
    case QVideoFrameFormat::Format_Invalid:
    default:
        return D3DFMT_UNKNOWN;
    }
}

QT_END_NAMESPACE
