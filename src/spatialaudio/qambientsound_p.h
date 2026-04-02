// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-3.0-only

#ifndef QAMBIENTSOUND_P_H
#define QAMBIENTSOUND_P_H

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

#include <QtSpatialAudio/qambientsound.h>
#include <QtSpatialAudio/private/qtspatialaudioglobal_p.h>
#include <QtCore/qmutex.h>
#include <QtCore/qurl.h>
#include <QtCore/qfuture.h>
#include <QtCore/private/qobject_p.h>
#include <QtCore/private/qexpected_p.h>
#include <QtMultimedia/qaudiobuffer.h>
#include <QtMultimedia/qaudiodecoder.h>
#include <QtMultimedia/qaudioformat.h>
#include <QtMultimedia/private/qmultimedia_source_resolver_p.h>

#include <atomic>
#include <memory>

namespace vraudio {
class ResonanceAudioApi;
}

QT_BEGIN_NAMESPACE

class QAudioEngine;
class QQuick3DSpatialSound;
class QQuick3DAmbientSound;

class QAmbientSoundPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QAmbientSound)

public:
    explicit QAmbientSoundPrivate(QAudioEngine *engine);
    QAmbientSoundPrivate(QAudioEngine *engine, int nchannels, int sourceId);
    ~QAmbientSoundPrivate();

    template <typename T>
    static QAmbientSoundPrivate *get(T *soundSource)
    {
        return soundSource ? soundSource->d_func() : nullptr;
    }

    QUrl url() const { return m_url; }
    void loadUrl(const QUrl &url);

    void setVolume(float volume);
    float volume() const { return m_volume; }

    virtual void updateRoomEffects() { }

protected:
    template <typename Functor>
    auto withResonanceApi(Functor &&f)
    {
        auto *api = getAPI();
        if (api)
            f(api);
        else {
            using result = std::invoke_result_t<Functor, vraudio::ResonanceAudioApi *>;
            if constexpr (std::is_void_v<result>)
                return;
            else
                return result{ };
        }
    }

    virtual void applyVolume();

public:
    const int nchannels = 2;
    QAudioEngine *const engine;
    const int sourceId;

    std::atomic_bool m_autoPlay = true;
    std::atomic_bool m_playing = false;
    std::atomic_int m_loops = 1;

    void play() { m_playing = true; }
    void pause() { m_playing = false; }
    void stop()
    {
        QMutexLocker locker(&mutex);
        m_playing = false;
        m_currentSample = 0;
        m_currentLoop = 0;
    }
    void getBuffer(QSpan<float> output, int channels);

private:
    float m_volume = 1.f;

    std::unique_ptr<QAudioDecoder> m_decoder;

    QMutex mutex;
    int m_currentSample = 0;
    int m_currentLoop = 0;

    std::optional<QAudioBuffer> m_buffer;
    QFuture<void> m_loadFuture;

    using LoadResult = q23::expected<QList<QAudioBuffer>, QAudioDecoder::Error>;
    QFuture<LoadResult> load(QUrl resolvedUrl, QAudioFormat format);

    QUrl m_url; // unresolved URL
    using AbstractSourceResolver = QMultimediaPrivate::AbstractSourceResolver;
    using TrivialSourceResolver = QMultimediaPrivate::TrivialSourceResolver;

    friend class QQuick3DSpatialSound;
    friend class QQuick3DAmbientSound;
    std::unique_ptr<const AbstractSourceResolver> m_sourceResolver =
            std::make_unique<TrivialSourceResolver>();

    vraudio::ResonanceAudioApi *getAPI();
};

QT_END_NAMESPACE

#endif // QAMBIENTSOUND_P_H
