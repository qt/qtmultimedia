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
    std::unique_ptr<QAmbisonicDecoder> ambisonicDecoder;

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

class QAmbientSoundPrivate : public QObject
{
public:
    QAmbientSoundPrivate(QObject *parent, int nchannels = 2)
        : QObject(parent)
        , nchannels(nchannels)
    {}

    template<typename T>
    static QAmbientSoundPrivate *get(T *soundSource) { return soundSource ? soundSource->d : nullptr; }

    QUrl url;
    float volume = 1.;
    int nchannels = 2;
    std::unique_ptr<QAudioDecoder> decoder;
    std::unique_ptr<QFile> sourceDeviceFile;
    QAudioEngine *engine = nullptr;

    QMutex mutex;
    int currentBuffer = 0;
    int bufPos = 0;
    int m_currentLoop = 0;
    QList<QAudioBuffer> buffers;
    int sourceId = -1; // kInvalidSourceId

    QAtomicInteger<bool> m_autoPlay = true;
    QAtomicInteger<bool> m_playing = false;
    QAtomicInt m_loops = 1;
    bool m_loading = false;

    void play() {
        m_playing = true;
    }
    void pause() {
        m_playing = false;
    }
    void stop() {
        QMutexLocker locker(&mutex);
        m_playing = false;
        currentBuffer = 0;
        bufPos = 0;
        m_currentLoop = 0;
    }

    void load();
    void getBuffer(float *buf, int frames, int channels);

private Q_SLOTS:
    void bufferReady();
    void finished();

};

QT_END_NAMESPACE

#endif
