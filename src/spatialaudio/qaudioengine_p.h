// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-3.0-only

#ifndef QAUDIOENGINE_P_H
#define QAUDIOENGINE_P_H

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

#include <QtGui/qvectornd.h>
#include <QtCore/private/qobject_p.h>
#include <QtGui/qvectornd.h>
#include <QtSpatialAudio/qaudioengine.h>

#include <optional>

namespace vraudio {
class ResonanceAudio;
} // namespace vraudio

QT_BEGIN_NAMESPACE

class QSpatialSound;
class QAmbientSound;
class QAudioRoom;
class QAudioListener;

class QAudioEnginePrivate : public QObjectPrivate
{
public:
    Q_DECLARE_PUBLIC(QAudioEngine)

    static QAudioEnginePrivate *get(QAudioEngine *engine)
    {
        return engine ? static_cast<QAudioEnginePrivate *>(engine->d_func()) : nullptr;
    }

    explicit QAudioEnginePrivate(int sampleRate);
    ~QAudioEnginePrivate() override;

    int sampleRate() const { return m_sampleRate; }
    static constexpr int framesPerBuffer = 128;

    void setDistanceScale(float scale);
    float distanceScale() const;

    // resonanceAudio access
    void setMasterVolume(float);
    float masterVolume() const;
    float m_masterVolume = 1.f;

    // Listener position
    // if set to nullopt, no QAudioListener is registered
    virtual void setListenerPosition(std::optional<QVector3D>);
    std::optional<QVector3D> listenerPosition() const { return m_position; }
    void setListenerRotation(const QQuaternion &);

    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void setPaused(bool) = 0;
    virtual bool isPaused() const = 0;
    virtual void setOutputDevice(const QAudioDevice &) = 0;
    virtual QAudioDevice outputDevice() const = 0;
    virtual void setOutputMode(QAudioEngine::OutputMode) = 0;
    virtual QAudioEngine::OutputMode outputMode() const = 0;
    virtual void setRoomEffectsEnabled(bool) = 0;
    virtual bool roomEffectsEnabled() const = 0;

    virtual void addSpatialSound(QSpatialSound *) = 0;
    virtual void removeSpatialSound(QSpatialSound *) = 0;
    virtual void addStereoSound(QAmbientSound *) = 0;
    virtual void removeStereoSound(QAmbientSound *) = 0;

    virtual void addRoom(QAudioRoom *) = 0;
    virtual void removeRoom(QAudioRoom *) = 0;
    virtual QAudioRoom *currentRoom() const = 0;

protected:
    struct SmallestRoomForListenerResult
    {
        QAudioRoom *room;
        float volume;
    };

    SmallestRoomForListenerResult findSmallestRoomForListener(QSpan<QAudioRoom *> rooms) const;

private:
    const int m_sampleRate = 44100;

    // Resonance Audio uses meters internally, while Qt Quick 3D and our API uses cm by
    // default. To make things independent from the scale setting, we store all distances in
    // meters internally and convert in the setters and getters.
    float m_distanceScale = 0.01f;

    std::optional<QVector3D> m_position;

public:
    const std::unique_ptr<vraudio::ResonanceAudio> resonanceAudio;
};

QT_END_NAMESPACE

#endif
