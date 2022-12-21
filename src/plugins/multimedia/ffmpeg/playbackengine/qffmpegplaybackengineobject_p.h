// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QFFMPEGPLAYBACKENGINEOBJECT_P_H
#define QFFMPEGPLAYBACKENGINEOBJECT_P_H

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

#include "playbackengine/qffmpegplaybackenginedefs_p.h"
#include "qthread.h"
#include "qtimer.h"

#include <atomic>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

class PlaybackEngineObject : public QObject
{
    Q_OBJECT
public:
    using TimePoint = std::chrono::steady_clock::time_point;
    using TimePointOpt = std::optional<TimePoint>;

    bool isPaused() const;

    bool isAtEnd() const;

    void kill();

    void setPaused(bool isPaused);

signals:
    void atEnd();

    void error(int code, const QString &errorString);

protected:
    QTimer &timer();

    void scheduleNextStep(bool allowDoImmediatelly = true);

    virtual void onPauseChanged();

    virtual bool canDoNextStep() const;

    virtual int timerInterval() const;

    void setAtEnd(bool isAtEnd);

    virtual void doNextStep() { }

private:
    QTimer *m_timer = nullptr;

    std::atomic_bool m_paused = true;
    std::atomic_bool m_atEnd = false;
    std::atomic_bool m_deleting = false;
};
} // namespace QFFmpeg

QT_END_NAMESPACE

#endif // QFFMPEGPLAYBACKENGINEOBJECT_P_H
