/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Spatial Audio module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL-NOGPL2$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QSPATIALAUDIOENGINE_P_H
#define QSPATIALAUDIOENGINE_P_H

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

#include <qspatialaudioengine.h>
#include <qaudiodevice.h>
#include <qthread.h>
#include <qmutex.h>
#include <qurl.h>
#include <qaudiobuffer.h>

namespace vraudio {
class ResonanceAudioApi;
}

QT_BEGIN_NAMESPACE

class QSpatialAudioSoundSource;
class QSpatialAudioStereoSource;
class QAudioSink;
class QAudioOutputStream;
class QAmbisonicDecoder;
class QAudioDecoder;

class QSpatialAudioEnginePrivate
{
public:
    static QSpatialAudioEnginePrivate *get(QSpatialAudioEngine *engine) { return engine ? engine->d : nullptr; }

    static constexpr int bufferSize = 128;

    QSpatialAudioEnginePrivate();
    ~QSpatialAudioEnginePrivate();
    vraudio::ResonanceAudioApi *api = nullptr;
    int sampleRate = 44100;
    float masterVolume = 1.;
    QSpatialAudioEngine::OutputMode outputMode = QSpatialAudioEngine::Normal;
    bool roomEffectsEnabled = true;

    QMutex mutex;
    QAudioFormat format;
    QAudioDevice device;
    QAtomicInteger<bool> paused = false;

    QThread audioThread;
    std::unique_ptr<QAudioOutputStream> outputStream;
    std::unique_ptr<QAmbisonicDecoder> ambisonicDecoder;

    QList<QSpatialAudioSoundSource *> sources;
    QList<QSpatialAudioStereoSource *> stereoSources;

    void addSpatialSound(QSpatialAudioSoundSource *sound);
    void removeSpatialSound(QSpatialAudioSoundSource *sound);
    void addStereoSound(QSpatialAudioStereoSource *sound);
    void removeStereoSound(QSpatialAudioStereoSource *sound);
};

class QSpatialAudioSound : public QObject
{
public:
    QSpatialAudioSound(QObject *parent, int nchannels = 2)
        : QObject(parent)
        , nchannels(nchannels)
    {}

    template<typename T>
    static QSpatialAudioSound *get(T *soundSource) { return soundSource ? soundSource->d : nullptr; }


    QUrl url;
    float volume = 1.;
    int nchannels = 2;
    std::unique_ptr<QAudioDecoder> decoder;
    QSpatialAudioEngine *engine = nullptr;

    QMutex mutex;
    int currentBuffer = 0;
    int bufPos = 0;
    int m_currentLoop = 0;
    QList<QAudioBuffer> buffers;
    int sourceId = -1; // kInvalidSourceId

    QAtomicInteger<bool> m_autoPlay = true;
    QAtomicInteger<bool> m_playing = false;
    QAtomicInt m_loops = 1;

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
