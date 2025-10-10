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

#include <QtCore/qatomic.h>
#include <QtCore/qthread.h>
#include <QtMultimedia/qmediaplayer.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpegplaybackenginedefs_p.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpegplaybackutils_p.h>

#include <chrono>

QT_BEGIN_NAMESPACE

class QTimer;

namespace QFFmpeg {

class PlaybackEngineObject : public QObject
{
    Q_OBJECT
public:
    using TimePoint = std::chrono::steady_clock::time_point;
    using TimePointOpt = std::optional<TimePoint>;

    explicit PlaybackEngineObject(const PlaybackEngineObjectID &id);

    ~PlaybackEngineObject() override;

    bool isPaused() const;

    bool isAtEnd() const;

    void kill();

    void setPaused(bool isPaused);

    quint64 objectID() const { return m_id.objectID; }

signals:
    void atEnd(PlaybackEngineObjectID id);

    void error(QMediaPlayer::Error, const QString &errorString);

protected:
    bool checkSessionID(quint64 sessionID) const { return sessionID == m_id.sessionID; }

    bool checkID(const PlaybackEngineObjectID &id) const
    {
        return checkSessionID(id.sessionID) && id.objectID == objectID();
    }

    const PlaybackEngineObjectID &id() const
    {
        Q_ASSERT(thread()->isCurrentThread());
        return m_id;
    }

    QTimer &timer();

    void scheduleNextStep(bool allowDoImmediatelly = true);

    virtual void onPauseChanged();

    virtual bool canDoNextStep() const;

    virtual std::chrono::milliseconds timerInterval() const;

    void setAtEnd(bool isAtEnd);

    virtual void doNextStep() { }

private slots:
    void onTimeout();

private:
    std::unique_ptr<QTimer> m_timer;

    QAtomicInteger<bool> m_paused = true;
    QAtomicInteger<bool> m_atEnd = false;
    QAtomicInteger<bool> m_deleting = false;
    PlaybackEngineObjectID m_id;
};
} // namespace QFFmpeg

QT_END_NAMESPACE

#endif // QFFMPEGPLAYBACKENGINEOBJECT_P_H
