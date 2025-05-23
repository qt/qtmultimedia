// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qffmpegencoderthread_p.h"

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

EncoderThread::EncoderThread(RecordingEngine &recordingEngine) : m_recordingEngine(recordingEngine)
{
}

void EncoderThread::setPaused(bool b)
{
    m_paused.storeRelease(b);
}

} // namespace QFFmpeg

QT_END_NAMESPACE
