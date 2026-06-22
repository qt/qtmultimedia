// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-3.0-only

#ifndef QAUDIOENGINE_THREADED_P_H
#define QAUDIOENGINE_THREADED_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtMultimedia/qaudiodevice.h>
#include <QtSpatialAudio/private/qtspatialaudioglobal_p.h>
#include <QtSpatialAudio/private/qaudioengine_p.h>
#include <QtCore/qmutex.h>
#include <QtCore/qthread.h>

#include <map>

namespace vraudio {
class ResonanceAudio;
} // namespace vraudio

QT_BEGIN_NAMESPACE

class QAudioOutputStream;
class QAudioRoom;
class QAudioListener;

namespace QSpatialAudioPrivate {
class QSpatialAudioPlaybackState;
} // namespace QSpatialAudioPrivate

class QAudioEngineThreaded final : public QAudioEnginePrivate
{
public:
    explicit QAudioEngineThreaded(int sampleRate);
    Q_DISABLE_COPY_MOVE(QAudioEngineThreaded)
    ~QAudioEngineThreaded() override;

    void start() override;
    void stop() override;
    void setPaused(bool paused) override;
    bool isPaused() const override;
    void setOutputDevice(const QAudioDevice &device) override;
    QAudioDevice outputDevice() const override { return m_device; }

    void addSound(QSpatialAudioSoundPrivate *) override;
    void removeSound(QSpatialAudioSoundPrivate *) override;

    using SharedPlaybackState = std::shared_ptr<QSpatialAudioPrivate::QSpatialAudioPlaybackState>;
    void setSoundPlaybackData(QSpatialAudioSoundPrivate *, SharedPlaybackState) override;

    void updateRoomEffects() override;

private:
    friend class QAudioOutputStream;

    QMutex mutex;
    QAudioDevice m_device;
    QAtomicInteger<bool> m_paused = false;

    QThread audioThread;
    std::unique_ptr<QAudioOutputStream> outputStream;

    std::map<QSpatialAudioSoundPrivate *, SharedPlaybackState> playbackStates;
};

QT_END_NAMESPACE

#endif
