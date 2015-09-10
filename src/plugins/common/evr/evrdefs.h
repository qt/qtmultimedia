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

#ifndef EVRDEFS_H
#define EVRDEFS_H

#include <d3d9.h>
#include <Evr9.h>
#include <dxva2api.h>

// The following is required to compile with MinGW

#ifdef __GNUC__
typedef struct MFVideoNormalizedRect {
    float left;
    float top;
    float right;
    float bottom;
} MFVideoNormalizedRect;
#endif

#ifndef __IMFGetService_INTERFACE_DEFINED__
#define __IMFGetService_INTERFACE_DEFINED__
DEFINE_GUID(IID_IMFGetService, 0xfa993888, 0x4383, 0x415a, 0xa9,0x30, 0xdd,0x47,0x2a,0x8c,0xf6,0xf7);
MIDL_INTERFACE("fa993888-4383-415a-a930-dd472a8cf6f7")
IMFGetService : public IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE GetService(REFGUID, REFIID, LPVOID *) = 0;
};
#ifdef __CRT_UUID_DECL
__CRT_UUID_DECL(IMFGetService, 0xfa993888, 0x4383, 0x415a, 0xa9,0x30, 0xdd,0x47,0x2a,0x8c,0xf6,0xf7)
#endif
#endif // __IMFGetService_INTERFACE_DEFINED__

#ifndef __IMFVideoDisplayControl_INTERFACE_DEFINED__
#define __IMFVideoDisplayControl_INTERFACE_DEFINED__
typedef enum MFVideoAspectRatioMode
{
    MFVideoARMode_None = 0,
    MFVideoARMode_PreservePicture = 0x1,
    MFVideoARMode_PreservePixel = 0x2,
    MFVideoARMode_NonLinearStretch = 0x4,
    MFVideoARMode_Mask = 0x7
} MFVideoAspectRatioMode;

DEFINE_GUID(IID_IMFVideoDisplayControl, 0xa490b1e4, 0xab84, 0x4d31, 0xa1,0xb2, 0x18,0x1e,0x03,0xb1,0x07,0x7a);
MIDL_INTERFACE("a490b1e4-ab84-4d31-a1b2-181e03b1077a")
IMFVideoDisplayControl : public IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE GetNativeVideoSize(SIZE *, SIZE *) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetIdealVideoSize(SIZE *, SIZE *) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetVideoPosition(const MFVideoNormalizedRect *, const LPRECT) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetVideoPosition(MFVideoNormalizedRect *, LPRECT) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetAspectRatioMode(DWORD) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetAspectRatioMode(DWORD *) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetVideoWindow(HWND) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetVideoWindow(HWND *) = 0;
    virtual HRESULT STDMETHODCALLTYPE RepaintVideo(void) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetCurrentImage(BITMAPINFOHEADER *, BYTE **, DWORD *, LONGLONG *) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetBorderColor(COLORREF) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetBorderColor(COLORREF *) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetRenderingPrefs(DWORD) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetRenderingPrefs(DWORD *) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetFullscreen(BOOL) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetFullscreen(BOOL *) = 0;
};
#ifdef __CRT_UUID_DECL
__CRT_UUID_DECL(IMFVideoDisplayControl, 0xa490b1e4, 0xab84, 0x4d31, 0xa1,0xb2, 0x18,0x1e,0x03,0xb1,0x07,0x7a)
#endif
#endif // __IMFVideoDisplayControl_INTERFACE_DEFINED__

#ifndef __IMFVideoProcessor_INTERFACE_DEFINED__
#define __IMFVideoProcessor_INTERFACE_DEFINED__
DEFINE_GUID(IID_IMFVideoProcessor, 0x6AB0000C, 0xFECE, 0x4d1f, 0xA2,0xAC, 0xA9,0x57,0x35,0x30,0x65,0x6E);
MIDL_INTERFACE("6AB0000C-FECE-4d1f-A2AC-A9573530656E")
IMFVideoProcessor : public IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE GetAvailableVideoProcessorModes(UINT *, GUID **) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetVideoProcessorCaps(LPGUID, DXVA2_VideoProcessorCaps *) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetVideoProcessorMode(LPGUID) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetVideoProcessorMode(LPGUID) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetProcAmpRange(DWORD, DXVA2_ValueRange *) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetProcAmpValues(DWORD, DXVA2_ProcAmpValues *) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetProcAmpValues(DWORD, DXVA2_ProcAmpValues *) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetFilteringRange(DWORD, DXVA2_ValueRange *) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetFilteringValue(DWORD, DXVA2_Fixed32 *) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetFilteringValue(DWORD, DXVA2_Fixed32 *) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetBackgroundColor(COLORREF *) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetBackgroundColor(COLORREF) = 0;
};
#ifdef __CRT_UUID_DECL
__CRT_UUID_DECL(IMFVideoProcessor, 0x6AB0000C, 0xFECE, 0x4d1f, 0xA2,0xAC, 0xA9,0x57,0x35,0x30,0x65,0x6E)
#endif
#endif // __IMFVideoProcessor_INTERFACE_DEFINED__

#endif // EVRDEFS_H

