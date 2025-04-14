// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWASMJS_P_H
#define QWASMJS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QObject>

#include <emscripten.h>
#include <emscripten/val.h>
#include <emscripten/bind.h>
#include <private/qstdweb_p.h>


class JsMediaInputStream : public QObject
{
    Q_OBJECT
public:
    explicit JsMediaInputStream(QObject *parent = nullptr);

    bool isActive() { return m_active; }

    void setUseAudio(bool useAudio) { m_needsAudio = useAudio; }
    void setUseVideo(bool useVideo) { m_needsVideo = useVideo; }
    void setStreamDevice(const std::string &id);
    emscripten::val getMediaStream() { return m_mediaStream; }

signals:
    void mediaStreamReady();
    void activated(bool active);

private:
    void setupMediaStream(emscripten::val mStream);
    emscripten::val m_mediaStream = emscripten::val::undefined();
    bool m_needsAudio = false;
    bool m_needsVideo = false;
    bool m_active = false;

    QScopedPointer<qstdweb::EventCallback> m_activeStreamEvent;
    QScopedPointer<qstdweb::EventCallback> m_inactiveStreamEvent;

};

#endif // QWASMJS_P_H
