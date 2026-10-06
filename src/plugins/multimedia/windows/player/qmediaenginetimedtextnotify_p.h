// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QMEDIAENGINETIMEDTEXTNOTIFY_P_H
#define QMEDIAENGINETIMEDTEXTNOTIFY_P_H

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

#include <qwindowsmfdefs_p.h>

#include <QtCore/qobject.h>

#include <QtCore/private/qcomobject_p.h>

QT_BEGIN_NAMESPACE

class QMediaEngineTimedTextNotify : public QObject, public QComObject<IMFTimedTextNotify>
{
    Q_OBJECT
public:
    // IMFTimedTextNotify
    void STDMETHODCALLTYPE TrackAdded(DWORD trackId) override;
    void STDMETHODCALLTYPE TrackRemoved(DWORD trackId) override;
    void STDMETHODCALLTYPE TrackSelected(DWORD trackId, BOOL selected) override;
    void STDMETHODCALLTYPE TrackReadyStateChanged(DWORD trackId) override;
    void STDMETHODCALLTYPE Error(MF_TIMED_TEXT_ERROR_CODE errorCode, HRESULT extendedErrorCode,
                                 DWORD sourceTrackId) override;
    void STDMETHODCALLTYPE Cue(MF_TIMED_TEXT_CUE_EVENT cueEvent, double currentTime,
                               IMFTimedTextCue *cue) override;
    void STDMETHODCALLTYPE Reset() override;

Q_SIGNALS:
    void trackAdded(DWORD trackId);
    void subtitleTextChanged(const QString &text);
};

QT_END_NAMESPACE

#endif // QMEDIAENGINETIMEDTEXTNOTIFY_P_H
