// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSAMPLECACHE_P_H
#define QSAMPLECACHE_P_H

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

#include <QtMultimedia/qaudioformat.h>
#include <QtCore/private/qexpected_p.h>
#include <QtCore/private/qglobal_p.h>
#include <QtCore/qfuture.h>
#include <QtCore/qmutex.h>
#include <QtCore/qobject.h>
#include <QtCore/qpointer.h>
#include <QtCore/qset.h>
#include <QtCore/qspan.h>
#include <QtCore/qthread.h>
#include <QtCore/qurl.h>

#include <memory>
#include <optional>

QT_BEGIN_NAMESPACE

enum class QSampleLoadError : uint8_t
{
    IoError,
    FormatError,
    DecoderError,
    NotSupported,
};

namespace QtMultimediaPrivate {

enum class SampleRate : int;
inline std::optional<SampleRate> asSampleRate(std::optional<int> arg)
{
    if (!arg)
        return std::nullopt;
    return SampleRate(*arg);
}

} // namespace QtMultimediaPrivate

class QSampleCache;

class Q_MULTIMEDIA_EXPORT QSample
{
public:
    friend class QSampleCache;
    using SampleRate = QtMultimediaPrivate::SampleRate;

    enum State : uint8_t {
        Creating,
        Error,
        Ready,
    };
    using SharedSamplePromise = QSharedPointer<QPromise<q23::expected<QSample *, QSample::State>>>;
    ~QSample();

    State state() const;
    const QByteArray& data() const { Q_ASSERT(state() == Ready); return m_soundData; }
    QSpan<const float> dataAsFloatSpan() const
    {
        Q_ASSERT(state() == Ready);
        Q_ASSERT(m_audioFormat.sampleFormat() == QAudioFormat::Float);
        return QSpan<const float>(reinterpret_cast<const float *>(m_soundData.constData()),
                                  m_audioFormat.framesForBytes(m_soundData.size())
                                          * m_audioFormat.channelCount());
    }

    const QAudioFormat &format() const
    {
        Q_ASSERT(state() == Ready);
        return m_audioFormat;
    }
    qsizetype frameCount() const
    {
        Q_ASSERT(state() == Ready);
        return m_audioFormat.framesForBytes(m_soundData.size());
    }

    void setError();
    void setData(QByteArray, QAudioFormat);

    QSample(QUrl url, QSampleCache *parent);
    QSample(QUrl url, QSampleCache *parent, std::optional<SampleRate> targetSampleRate);

    // For testing only
    QSample(QByteArray data, QAudioFormat format)
        : m_parent(nullptr), m_soundData(std::move(data)), m_audioFormat(format), m_url(), m_state(Ready) {}

private:
    QSample();

    QSampleCache *m_parent;
    QByteArray   m_soundData;
    QAudioFormat m_audioFormat;
    const QUrl   m_url;
    const std::optional<SampleRate> m_targetSampleRate;
    State        m_state = State::Creating;

    friend class QSampleCache;
    void clearParent();
};

using SharedSamplePtr = std::shared_ptr<QSample>;
using WeakSamplePtr = std::weak_ptr<QSample>;

class Q_MULTIMEDIA_EXPORT QSampleCache : public QObject
{
public:
    friend class QSample;

    enum class SampleSourceType
    {
        File,
        NetworkManager,
        AudioDecoder,
    };

    static QSampleCache *instance();

    explicit QSampleCache(QObject *parent = nullptr);
    ~QSampleCache() override;

    QFuture<SharedSamplePtr>
    requestSampleFuture(const QUrl &, std::optional<int> targetSampleRate = std::nullopt);
    bool isCached(const QUrl &url, std::optional<int> targetSampleRate = std::nullopt) const;

    // For tests only
    void setSampleSourceType(SampleSourceType sampleSourceType)
    {
        m_sampleSourceType = sampleSourceType;
    }

    using SampleLoadResult = q23::expected<std::pair<QByteArray, QAudioFormat>, QSampleLoadError>;

    static SampleLoadResult loadSample(QSpan<const char>);
    static SampleLoadResult loadSampleViaDecoder(std::variant<QUrl, QByteArray>);
    static QFuture<SampleLoadResult> loadSampleAsyncViaDecoder(QByteArray);

private:
    std::unique_ptr<QIODevice> createStreamForSample(QSample &sample);
#if QT_CONFIG(thread)
    QThreadPool *threadPool();
#endif

private:
    using SharedSamplePromise = std::shared_ptr<QPromise<SharedSamplePtr>>;

    mutable QRecursiveMutex m_mutex;
    using SampleRate = QtMultimediaPrivate::SampleRate;

    using SampleKey = std::pair<QUrl, std::optional<SampleRate>>;
    std::map<SampleKey, WeakSamplePtr> m_loadedSamples;
    std::map<SampleKey, std::pair<SharedSamplePtr, QList<SharedSamplePromise>>> m_pendingSamples;

    void removeUnreferencedSample(const QUrl &url, std::optional<SampleRate> targetSampleRate);

#if QT_CONFIG(thread)
    static SampleLoadResult
    loadSample(const QUrl &, std::optional<SampleSourceType> forceSourceType = std::nullopt);
#  ifndef Q_OS_WASM
    QThreadPool m_threadPool{ this };
#  endif
#endif
    QFuture<SampleLoadResult> loadSampleAsync(const QUrl &);

    std::optional<SampleSourceType> m_sampleSourceType;
};

QT_END_NAMESPACE

#endif // QSAMPLECACHE_P_H
