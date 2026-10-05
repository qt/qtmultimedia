// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

#ifndef QWASMVIDEOFRAMEGRABBER_H
#define QWASMVIDEOFRAMEGRABBER_H

#include <QtMultimedia/qvideoframeformat.h>

#include <emscripten/html5_webgl.h>

#include <string_view>

QT_BEGIN_NAMESPACE

class QWasmVideoOutput;

class QWasmVideoFrameGrabber
{
public:
    explicit QWasmVideoFrameGrabber(QWasmVideoOutput *videoOutput);
    ~QWasmVideoFrameGrabber();

    void startFrameLoop();

    static QVideoFrameFormat::PixelFormat fromJsPixelFormat(std::string_view videoFormat);

private:
    void detectWebGLContext();
    void processVideoFrame();
    void processWebGLVideoFrame();

    QWasmVideoOutput *m_videoOutput = nullptr;
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE m_glContextHandle = 0;
    // Pending requestAnimationFrame id, 0 while no loop is scheduled.
    int m_animationFrameRequestId = 0;
    bool m_hasWebGLContext = false;
    bool m_webGLContextChecked = false;
};

QT_END_NAMESPACE
#endif // QWASMVIDEOFRAMEGRABBER_H
