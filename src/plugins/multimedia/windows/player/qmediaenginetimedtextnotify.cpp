// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qmediaenginetimedtextnotify_p.h"

#include <QtMultimedia/private/qcomtaskresource_p.h>
#include <QtCore/qstring.h>
#include <QtCore/private/qcomptr_p.h>

QT_BEGIN_NAMESPACE

void STDMETHODCALLTYPE QMediaEngineTimedTextNotify::TrackAdded(DWORD trackId)
{
    emit trackAdded(trackId);
}

void STDMETHODCALLTYPE QMediaEngineTimedTextNotify::TrackRemoved(DWORD) { }

void STDMETHODCALLTYPE QMediaEngineTimedTextNotify::TrackSelected(DWORD, BOOL) { }

void STDMETHODCALLTYPE QMediaEngineTimedTextNotify::TrackReadyStateChanged(DWORD) { }

void STDMETHODCALLTYPE QMediaEngineTimedTextNotify::Error(MF_TIMED_TEXT_ERROR_CODE, HRESULT, DWORD)
{
}

void STDMETHODCALLTYPE QMediaEngineTimedTextNotify::Cue(MF_TIMED_TEXT_CUE_EVENT cueEvent,
                                                        double /*currentTime*/,
                                                        IMFTimedTextCue *cue)
{
    // Extract text on the MF thread while the cue is still valid.
    QString text;
    if (cue && cueEvent == MF_TIMED_TEXT_CUE_EVENT_ACTIVE) {
        const DWORD lineCount = cue->GetLineCount();
        for (DWORD l = 0; l < lineCount; ++l) {
            ComPtr<IMFTimedTextFormattedText> line;
            if (SUCCEEDED(cue->GetLine(l, &line)) && line) {
                QComTaskResource<WCHAR> str;
                if (SUCCEEDED(line->GetText(str.address())) && str) {
                    if (!text.isEmpty())
                        text += u'\n';
                    text += QString::fromWCharArray(str.get());
                }
            }
        }
    }
    // cueEvent == INACTIVE → empty text (clear subtitle)
    emit subtitleTextChanged(text);
}

void STDMETHODCALLTYPE QMediaEngineTimedTextNotify::Reset() { }

QT_END_NAMESPACE
