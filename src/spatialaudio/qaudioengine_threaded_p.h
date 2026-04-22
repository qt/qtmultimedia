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

#include <QtSpatialAudio/private/qtspatialaudioglobal_p.h>
#include <QtSpatialAudio/private/qaudioengine_p.h>
#include <QtCore/qmutex.h>
#include <QtCore/qthread.h>
#include <QtMultimedia/qaudiodevice.h>

namespace vraudio {
class ResonanceAudio;
} // namespace vraudio

QT_BEGIN_NAMESPACE

class QSpatialSound;
class QAmbientSound;
class QAudioOutputStream;
class QAudioRoom;
class QAudioListener;

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
    void setOutputMode(QAudioEngine::OutputMode) override;
    QAudioEngine::OutputMode outputMode() const override { return m_outputMode; };

    void setRoomEffectsEnabled(bool) override;
    bool roomEffectsEnabled() const override;
    void setListenerPosition(std::optional<QVector3D> pos) override;

    void addSpatialSound(QSpatialSound *sound) override;
    void removeSpatialSound(QSpatialSound *sound) override;
    void addStereoSound(QAmbientSound *sound) override;
    void removeStereoSound(QAmbientSound *sound) override;

    void addRoom(QAudioRoom *room) override;
    void removeRoom(QAudioRoom *room) override;
    QAudioRoom *currentRoom() const override { return m_currentRoom; }
    void updateRooms();

private:
    friend class QAudioOutputStream;
    QAudioEngine::OutputMode m_outputMode = QAudioEngine::Surround;
    bool m_roomEffectsEnabled = true;

    QMutex mutex;
    QAudioDevice m_device;
    QAtomicInteger<bool> m_paused = false;

    QThread audioThread;
    std::unique_ptr<QAudioOutputStream> outputStream;

    QList<QSpatialSound *> sources;
    QList<QAmbientSound *> stereoSources;
    QList<QAudioRoom *> rooms;
    mutable bool listenerPositionDirty = true;
    QAudioRoom *m_currentRoom = nullptr;
};

QT_END_NAMESPACE

#endif
