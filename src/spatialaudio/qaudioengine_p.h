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

#include <qtspatialaudioglobal_p.h>
#include <qaudioengine.h>
#include <qaudiodevice.h>
#include <qaudiodecoder.h>
#include <qthread.h>
#include <qmutex.h>
#include <qurl.h>
#include <qaudiobuffer.h>
#include <qvector3d.h>
#include <qfile.h>

namespace vraudio {
class ResonanceAudio;
}

QT_BEGIN_NAMESPACE

class QSpatialSound;
class QAmbientSound;
class QAudioSink;
class QAudioOutputStream;
class QAmbisonicDecoder;
class QAudioDecoder;
class QAudioRoom;
class QAudioListener;

class QAudioEnginePrivate
{
public:
    static QAudioEnginePrivate *get(QAudioEngine *engine) { return engine ? engine->d : nullptr; }

    static constexpr int bufferSize = 128;

    QAudioEnginePrivate();
    ~QAudioEnginePrivate();
    vraudio::ResonanceAudio *resonanceAudio = nullptr;
    int sampleRate = 44100;
    float masterVolume = 1.;
    QAudioEngine::OutputMode outputMode = QAudioEngine::Surround;
    bool roomEffectsEnabled = true;

    // Resonance Audio uses meters internally, while Qt Quick 3D and our API uses cm by default.
    // To make things independent from the scale setting, we store all distances in meters internally
    // and convert in the setters and getters.
    float distanceScale = 0.01f;

    QMutex mutex;
    QAudioDevice device;
    QAtomicInteger<bool> paused = false;

    QThread audioThread;
    std::unique_ptr<QAudioOutputStream> outputStream;

    QAudioListener *listener = nullptr;
    QList<QSpatialSound *> sources;
    QList<QAmbientSound *> stereoSources;
    QList<QAudioRoom *> rooms;
    mutable bool listenerPositionDirty = true;
    QAudioRoom *currentRoom = nullptr;

    void addSpatialSound(QSpatialSound *sound);
    void removeSpatialSound(QSpatialSound *sound);
    void addStereoSound(QAmbientSound *sound);
    void removeStereoSound(QAmbientSound *sound);

    void addRoom(QAudioRoom *room);
    void removeRoom(QAudioRoom *room);
    void updateRooms();

    QVector3D listenerPosition() const;
};

QT_END_NAMESPACE

#endif
