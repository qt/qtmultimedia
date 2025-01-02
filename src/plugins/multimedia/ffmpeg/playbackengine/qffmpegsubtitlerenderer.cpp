// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "playbackengine/qffmpegsubtitlerenderer_p.h"

#include "qvideosink.h"
#include "qdebug.h"

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

SubtitleRenderer::SubtitleRenderer(const TimeController &tc, QVideoSink *sink)
    : Renderer(tc), m_sink(sink)
{
}

void SubtitleRenderer::setOutput(QVideoSink *sink, bool cleanPrevSink)
{
    setOutputInternal(m_sink, sink, [cleanPrevSink](QVideoSink *prev) {
        if (prev && cleanPrevSink)
            prev->setSubtitleText({});
    });
}

SubtitleRenderer::~SubtitleRenderer()
{
    if (m_sink)
        m_sink->setSubtitleText({});
}

Renderer::RenderingResult SubtitleRenderer::renderInternal(Frame frame)
{
    if (m_sink)
        m_sink->setSubtitleText(frame.isValid() ? frame.text() : QString());

    return {};
}

} // namespace QFFmpeg

QT_END_NAMESPACE

#include "moc_qffmpegsubtitlerenderer_p.cpp"
