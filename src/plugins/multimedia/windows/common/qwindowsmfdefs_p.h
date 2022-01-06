/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#ifndef QWINDOWSMFDEFS_P_H
#define QWINDOWSMFDEFS_P_H

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

#include <d3d9.h>
#include <dxva2api.h>
#include <mfidl.h>

// Stuff that is missing or incorrecty defined in MinGW.

extern const GUID QMM_MFTranscodeContainerType_ADTS;
extern const GUID QMM_MFTranscodeContainerType_ASF;
extern const GUID QMM_MFTranscodeContainerType_AVI;
extern const GUID QMM_MFTranscodeContainerType_FLAC;
extern const GUID QMM_MFTranscodeContainerType_MP3;
extern const GUID QMM_MFTranscodeContainerType_MPEG4;
extern const GUID QMM_MFTranscodeContainerType_WAVE;

extern const GUID QMM_MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID;
extern const GUID QMM_MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_AUDCAP_GUID;
extern const GUID QMM_MF_TRANSCODE_CONTAINERTYPE;

extern const GUID QMM_MF_SD_STREAM_NAME;
extern const GUID QMM_MF_SD_LANGUAGE;

extern const GUID QMM_KSCATEGORY_VIDEO_CAMERA;

extern const GUID QMM_MR_POLICY_VOLUME_SERVICE;

extern const PROPERTYKEY QMM_PKEY_Device_FriendlyName;

extern "C" HRESULT WINAPI MFCreateDeviceSource(IMFAttributes *pAttributes, IMFMediaSource **ppSource);

#define QMM_MFSESSION_GETFULLTOPOLOGY_CURRENT 1
#define QMM_PRESENTATION_CURRENT_POSITION 0x7fffffffffffffff

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

#ifndef __IMFSimpleAudioVolume_INTERFACE_DEFINED__
#define __IMFSimpleAudioVolume_INTERFACE_DEFINED__
DEFINE_GUID(IID_IMFSimpleAudioVolume, 0x089EDF13, 0xCF71, 0x4338, 0x8D,0x13, 0x9E,0x56,0x9D,0xBD,0xC3,0x19);
MIDL_INTERFACE("089EDF13-CF71-4338-8D13-9E569DBDC319")
IMFSimpleAudioVolume : public IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE SetMasterVolume(float) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetMasterVolume(float *) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetMute(BOOL) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetMute(BOOL *) = 0;
};
#ifdef __CRT_UUID_DECL
__CRT_UUID_DECL(IMFSimpleAudioVolume, 0x089EDF13, 0xCF71, 0x4338, 0x8D,0x13, 0x9E,0x56,0x9D,0xBD,0xC3,0x19)
#endif
#endif // __IMFSimpleAudioVolume_INTERFACE_DEFINED__

#endif // QWINDOWSMFDEFS_P_H

