// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-3.0-only

#ifndef QAUDIOENGINE_WITHPLAYER_P_H
#define QAUDIOENGINE_WITHPLAYER_P_H

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
#include <QtMultimedia/qaudiodevice.h>

namespace vraudio {
class ResonanceAudio;
} // namespace vraudio

QT_BEGIN_NAMESPACE

namespace QtMultimediaPrivate {
class QRtAudioEngine;
class QRtAudioEngineVoice;
} // namespace QtMultimediaPrivate

class QAudioEngineWithPlayer final : public QAudioEnginePrivate
{
public:
    explicit QAudioEngineWithPlayer(int sampleRate);
    Q_DISABLE_COPY_MOVE(QAudioEngineWithPlayer)
    ~QAudioEngineWithPlayer() override;

    void start() override;
    void stop() override;
    void setPaused(bool paused) override;
    bool isPaused() const override;
    void setOutputDevice(const QAudioDevice &device) override;
    QAudioDevice outputDevice() const override;

    void addSound(QSpatialAudioSoundPrivate *sound) override;
    void removeSound(QSpatialAudioSoundPrivate *sound) override;
    void setSoundPlaybackData(QSpatialAudioSoundPrivate *, SharedPlaybackState) override;

    void setOutputMode(QAudioEngine::OutputMode) override;
    void updateRoomEffects() override;

    QAudioFormat audioFormat() const { return m_format; }

private:
    using QRtAudioEngine = QtMultimediaPrivate::QRtAudioEngine;
    using QRtAudioEngineVoice = QtMultimediaPrivate::QRtAudioEngineVoice;

    friend class QResonanceAudioPlayer;

    QAudioDevice m_device;
    QAudioFormat m_format;
    std::shared_ptr<QRtAudioEngine> m_playbackEngine;
    std::shared_ptr<QRtAudioEngineVoice> m_engineVoice;
    bool m_paused = false;

    std::map<QSpatialAudioSoundPrivate *, SharedPlaybackState> playbackStates;
};

QT_END_NAMESPACE

#endif
