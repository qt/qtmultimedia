// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qffmpegencoderthread_p.h"

#include <QtCore/qmetaobject.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpegrecordingengine_p.h>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

EncoderThread::EncoderThread(RecordingEngine &recordingEngine) : m_recordingEngine(recordingEngine)
{
    m_resolvedPromise.start();
}

void EncoderThread::setPaused(bool paused)
{
    auto guard = lockLoopData();
    m_paused = paused;
}

void EncoderThread::setAutoStop(bool autoStop)
{
    auto guard = lockLoopData();
    m_autoStop = autoStop;
}

void EncoderThread::setEndOfSourceStream()
{
    {
        auto guard = lockLoopData();
        m_endOfSourceStream = true;
    }

    emit endOfSourceStream();
}

bool EncoderThread::init()
{
    {
        // Guard to allow checkIfCanPushFrame to check init status
        auto guard = lockLoopData();
        m_initialized = true;
    }

    markResolved(true);

    // Following will wait until recording engine confirms all encoders are resolved
    m_encodingStarted = m_recordingEngine.waitForEncodingStart();

    return true;
}

void EncoderThread::markResolved(bool succeeded)
{
    m_resolvedPromise.addResult(succeeded);
    m_resolvedPromise.finish();
}

} // namespace QFFmpeg

QT_END_NAMESPACE

#include "moc_qffmpegencoderthread_p.cpp"
