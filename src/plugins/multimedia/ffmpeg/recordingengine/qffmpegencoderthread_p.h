// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QFFMPEGENCODERTHREAD_P_H
#define QFFMPEGENCODERTHREAD_P_H

#include "qffmpegthread_p.h"

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

class RecordingEngine;

class EncoderThread : public ConsumerThread
{
public:
    EncoderThread(RecordingEngine &recordingEngine);
    virtual void setPaused(bool b);

protected:
    QAtomicInteger<bool> m_paused = false;
    RecordingEngine &m_recordingEngine;
};

} // namespace QFFmpeg

QT_END_NAMESPACE

#endif
