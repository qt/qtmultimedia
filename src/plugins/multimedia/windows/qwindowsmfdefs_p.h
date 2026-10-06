// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

#include <mfmediaengine.h>

// Stuff that is missing or incorrectly defined in MinGW.
//
// MinGW's mfmediaengine.h does not cover the timed text (subtitle) part of the Media Foundation
// media engine API. So we provide our own implementation

#ifndef __IMFTimedText_INTERFACE_DEFINED__

// Only ever passed around as opaque out-parameters.
struct IMFTimedTextBinary;
struct IMFTimedTextRegion;
struct IMFTimedTextStyle;
struct IMFTimedTextCueList;

struct IMFTimedTextNotify;
struct IMFTimedTextTrack;
struct IMFTimedTextTrackList;
struct IMFTimedTextCue;
struct IMFTimedTextFormattedText;

typedef enum MF_TIMED_TEXT_TRACK_KIND
{
    MF_TIMED_TEXT_TRACK_KIND_UNKNOWN = 0,
    MF_TIMED_TEXT_TRACK_KIND_SUBTITLES = 1,
    MF_TIMED_TEXT_TRACK_KIND_CAPTIONS = 2,
    MF_TIMED_TEXT_TRACK_KIND_METADATA = 3
} MF_TIMED_TEXT_TRACK_KIND;

typedef enum MF_TIMED_TEXT_TRACK_READY_STATE
{
    MF_TIMED_TEXT_TRACK_READY_STATE_NONE = 0,
    MF_TIMED_TEXT_TRACK_READY_STATE_LOADING = 1,
    MF_TIMED_TEXT_TRACK_READY_STATE_LOADED = 2,
    MF_TIMED_TEXT_TRACK_READY_STATE_ERROR = 3
} MF_TIMED_TEXT_TRACK_READY_STATE;

typedef enum MF_TIMED_TEXT_ERROR_CODE
{
    MF_TIMED_TEXT_ERROR_CODE_NOERROR = 0,
    MF_TIMED_TEXT_ERROR_CODE_FATAL = 1,
    MF_TIMED_TEXT_ERROR_CODE_DATA_FORMAT = 2,
    MF_TIMED_TEXT_ERROR_CODE_NETWORK = 3,
    MF_TIMED_TEXT_ERROR_CODE_INTERNAL = 4
} MF_TIMED_TEXT_ERROR_CODE;

typedef enum MF_TIMED_TEXT_CUE_EVENT
{
    MF_TIMED_TEXT_CUE_EVENT_ACTIVE = 0,
    MF_TIMED_TEXT_CUE_EVENT_INACTIVE = 1,
    MF_TIMED_TEXT_CUE_EVENT_CLEAR = 2
} MF_TIMED_TEXT_CUE_EVENT;

// Service GUID for IMFGetService::GetService(). On MinGW, EXTERN_GUID expands to
// a DECLSPEC_SELECTANY definition, so no INITGUID plumbing is needed.
EXTERN_GUID(MF_MEDIA_ENGINE_TIMEDTEXT,
            0x805ea411, 0x92e0, 0x4e59, 0x9b, 0x6e, 0x5c, 0x7d, 0x79, 0x15, 0xe6, 0x4f);

#define __IMFTimedText_INTERFACE_DEFINED__
MIDL_INTERFACE("1f2a94c9-a3df-430d-9d0f-acd85ddc29af")
IMFTimedText : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE RegisterNotifications(IMFTimedTextNotify *notify) = 0;
    virtual HRESULT STDMETHODCALLTYPE SelectTrack(DWORD trackId, BOOL selected) = 0;
    virtual HRESULT STDMETHODCALLTYPE AddDataSource(IMFByteStream *byteStream, LPCWSTR label,
                                                   LPCWSTR language, MF_TIMED_TEXT_TRACK_KIND kind,
                                                   BOOL isDefault, DWORD *trackId) = 0;
    virtual HRESULT STDMETHODCALLTYPE AddDataSourceFromUrl(LPCWSTR url, LPCWSTR label,
                                                          LPCWSTR language,
                                                          MF_TIMED_TEXT_TRACK_KIND kind,
                                                          BOOL isDefault, DWORD *trackId) = 0;
    virtual HRESULT STDMETHODCALLTYPE AddTrack(LPCWSTR label, LPCWSTR language,
                                              MF_TIMED_TEXT_TRACK_KIND kind,
                                              IMFTimedTextTrack **track) = 0;
    virtual HRESULT STDMETHODCALLTYPE RemoveTrack(IMFTimedTextTrack *track) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetCueTimeOffset(double *offset) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetCueTimeOffset(double offset) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetTracks(IMFTimedTextTrackList **tracks) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetActiveTracks(IMFTimedTextTrackList **activeTracks) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetTextTracks(IMFTimedTextTrackList **textTracks) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetMetadataTracks(IMFTimedTextTrackList **metadataTracks) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetInBandEnabled(BOOL enabled) = 0;
    virtual BOOL STDMETHODCALLTYPE IsInBandEnabled() = 0;
};
__CRT_UUID_DECL(IMFTimedText,
                0x1f2a94c9, 0xa3df, 0x430d, 0x9d, 0x0f, 0xac, 0xd8, 0x5d, 0xdc, 0x29, 0xaf)

#define __IMFTimedTextNotify_INTERFACE_DEFINED__
MIDL_INTERFACE("df6b87b6-ce12-45db-aba7-432fe054e57d")
IMFTimedTextNotify : public IUnknown
{
public:
    // NB: these do not return HRESULT.
    virtual void STDMETHODCALLTYPE TrackAdded(DWORD trackId) = 0;
    virtual void STDMETHODCALLTYPE TrackRemoved(DWORD trackId) = 0;
    virtual void STDMETHODCALLTYPE TrackSelected(DWORD trackId, BOOL selected) = 0;
    virtual void STDMETHODCALLTYPE TrackReadyStateChanged(DWORD trackId) = 0;
    virtual void STDMETHODCALLTYPE Error(MF_TIMED_TEXT_ERROR_CODE errorCode,
                                         HRESULT extendedErrorCode, DWORD sourceTrackId) = 0;
    virtual void STDMETHODCALLTYPE Cue(MF_TIMED_TEXT_CUE_EVENT cueEvent, double currentTime,
                                       IMFTimedTextCue *cue) = 0;
    virtual void STDMETHODCALLTYPE Reset() = 0;
};
__CRT_UUID_DECL(IMFTimedTextNotify,
                0xdf6b87b6, 0xce12, 0x45db, 0xab, 0xa7, 0x43, 0x2f, 0xe0, 0x54, 0xe5, 0x7d)

#define __IMFTimedTextTrack_INTERFACE_DEFINED__
MIDL_INTERFACE("8822c32d-654e-4233-bf21-d7f2e67d30d4")
IMFTimedTextTrack : public IUnknown
{
public:
    virtual DWORD STDMETHODCALLTYPE GetId() = 0;
    virtual HRESULT STDMETHODCALLTYPE GetLabel(LPWSTR *label) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetLabel(LPCWSTR label) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetLanguage(LPWSTR *language) = 0;
    virtual MF_TIMED_TEXT_TRACK_KIND STDMETHODCALLTYPE GetTrackKind() = 0;
    virtual BOOL STDMETHODCALLTYPE IsInBand() = 0;
    virtual HRESULT STDMETHODCALLTYPE GetInBandMetadataTrackDispatchType(LPWSTR *dispatchType) = 0;
    virtual BOOL STDMETHODCALLTYPE IsActive() = 0;
    virtual MF_TIMED_TEXT_ERROR_CODE STDMETHODCALLTYPE GetErrorCode() = 0;
    virtual HRESULT STDMETHODCALLTYPE GetExtendedErrorCode() = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDataFormat(GUID *format) = 0;
    virtual MF_TIMED_TEXT_TRACK_READY_STATE STDMETHODCALLTYPE GetReadyState() = 0;
    virtual HRESULT STDMETHODCALLTYPE GetCueList(IMFTimedTextCueList **cues) = 0;
};
__CRT_UUID_DECL(IMFTimedTextTrack,
                0x8822c32d, 0x654e, 0x4233, 0xbf, 0x21, 0xd7, 0xf2, 0xe6, 0x7d, 0x30, 0xd4)

#define __IMFTimedTextTrackList_INTERFACE_DEFINED__
MIDL_INTERFACE("23ff334c-442c-445f-bccc-edc438aa11e2")
IMFTimedTextTrackList : public IUnknown
{
public:
    virtual DWORD STDMETHODCALLTYPE GetLength() = 0;
    virtual HRESULT STDMETHODCALLTYPE GetTrack(DWORD index, IMFTimedTextTrack **track) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetTrackById(DWORD trackId, IMFTimedTextTrack **track) = 0;
};
__CRT_UUID_DECL(IMFTimedTextTrackList,
                0x23ff334c, 0x442c, 0x445f, 0xbc, 0xcc, 0xed, 0xc4, 0x38, 0xaa, 0x11, 0xe2)

#define __IMFTimedTextCue_INTERFACE_DEFINED__
MIDL_INTERFACE("1e560447-9a2b-43e1-a94c-b0aaabfbfbc9")
IMFTimedTextCue : public IUnknown
{
public:
    virtual DWORD STDMETHODCALLTYPE GetId() = 0;
    virtual HRESULT STDMETHODCALLTYPE GetOriginalId(LPWSTR *originalId) = 0;
    virtual MF_TIMED_TEXT_TRACK_KIND STDMETHODCALLTYPE GetCueKind() = 0;
    virtual double STDMETHODCALLTYPE GetStartTime() = 0;
    virtual double STDMETHODCALLTYPE GetDuration() = 0;
    virtual DWORD STDMETHODCALLTYPE GetTrackId() = 0;
    virtual HRESULT STDMETHODCALLTYPE GetData(IMFTimedTextBinary **data) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetRegion(IMFTimedTextRegion **region) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetStyle(IMFTimedTextStyle **style) = 0;
    virtual DWORD STDMETHODCALLTYPE GetLineCount() = 0;
    virtual HRESULT STDMETHODCALLTYPE GetLine(DWORD index, IMFTimedTextFormattedText **line) = 0;
};
__CRT_UUID_DECL(IMFTimedTextCue,
                0x1e560447, 0x9a2b, 0x43e1, 0xa9, 0x4c, 0xb0, 0xaa, 0xab, 0xfb, 0xfb, 0xc9)

#define __IMFTimedTextFormattedText_INTERFACE_DEFINED__
MIDL_INTERFACE("e13af3c1-4d47-4354-b1f5-e83ae0ecae60")
IMFTimedTextFormattedText : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE GetText(LPWSTR *text) = 0;
    virtual DWORD STDMETHODCALLTYPE GetSubformattingCount() = 0;
    virtual HRESULT STDMETHODCALLTYPE GetSubformatting(DWORD index, DWORD *firstChar,
                                                      DWORD *charLength,
                                                      IMFTimedTextStyle **style) = 0;
};
__CRT_UUID_DECL(IMFTimedTextFormattedText,
                0xe13af3c1, 0x4d47, 0x4354, 0xb1, 0xf5, 0xe8, 0x3a, 0xe0, 0xec, 0xae, 0x60)

#endif // __IMFTimedText_INTERFACE_DEFINED__

#endif // QWINDOWSMFDEFS_P_H
