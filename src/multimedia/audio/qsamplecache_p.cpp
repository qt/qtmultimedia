// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsamplecache_p.h"

#include <QtConcurrent/qtconcurrentrun.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qdebug.h>
#include <QtCore/qeventloop.h>
#include <QtCore/qfile.h>
#include <QtCore/qfuturewatcher.h>
#include <QtCore/qloggingcategory.h>

#if QT_CONFIG(network)
#  include <QtNetwork/qnetworkaccessmanager.h>
#  include <QtNetwork/qnetworkreply.h>
#  include <QtNetwork/qnetworkrequest.h>
#endif

#include "dr_wav.h"

#include <utility>

Q_STATIC_LOGGING_CATEGORY(qLcSampleCache, "qt.multimedia.samplecache")

#if !QT_CONFIG(thread)
#  define thread_local
#endif

QT_BEGIN_NAMESPACE

QSampleCache::QSampleCache(QObject *parent)
    : QObject(parent)
{
#if QT_CONFIG(thread)
    // we limit the number of loader threads to avoid thread explosion
    static constexpr int loaderThreadLimit = 8;
    m_threadPool.setMaxThreadCount(loaderThreadLimit);
    m_threadPool.setExpiryTimeout(15);
    m_threadPool.setThreadPriority(QThread::LowPriority);
    m_threadPool.setServiceLevel(QThread::QualityOfService::Eco);

    if (!thread()->isMainThread()) {
        this->moveToThread(qApp->thread());
        m_threadPool.moveToThread(qApp->thread());
    }
#endif
}

QSampleCache::~QSampleCache()
{
#if QT_CONFIG(thread)
    m_threadPool.clear();
    m_threadPool.waitForDone();
#endif

    for (auto &entry : m_loadedSamples) {
        auto samplePtr = entry.second.lock();
        if (samplePtr)
            samplePtr->clearParent();
    }

    for (auto &entry : m_pendingSamples) {
        auto samplePtr = entry.second.first;
        if (samplePtr)
            samplePtr->clearParent();
    }
}

QSampleCache::SampleLoadResult QSampleCache::loadSample(QByteArray data)
{
    using namespace QtPrivate;

    drwav wavParser;
    bool success = drwav_init_memory(&wavParser, data.constData(), data.size(), nullptr);
    if (!success)
        return std::nullopt;

    // using float as internal format. one could argue to use int16 and save half the ram at the
    // cost of potential run-time conversions
    QAudioFormat audioFormat;
    audioFormat.setChannelCount(wavParser.channels);
    audioFormat.setSampleFormat(QAudioFormat::Float);
    audioFormat.setSampleRate(wavParser.sampleRate);
    audioFormat.setChannelConfig(
            QAudioFormat::defaultChannelConfigForChannelCount(wavParser.channels));

    QByteArray sampleData;
    sampleData.resizeForOverwrite(sizeof(float) * wavParser.channels
                                  * wavParser.totalPCMFrameCount);
    uint64_t framesRead = drwav_read_pcm_frames_f32(&wavParser, wavParser.totalPCMFrameCount,
                                                    reinterpret_cast<float *>(sampleData.data()));

    if (framesRead != wavParser.totalPCMFrameCount)
        return std::nullopt;

    return std::pair{
        std::move(sampleData),
        audioFormat,
    };
}

#if QT_CONFIG(thread) && QT_CONFIG(network)

namespace {

Q_CONSTINIT thread_local std::optional<QNetworkAccessManager> g_networkAccessManager;
QNetworkAccessManager &threadLocalNetworkAccessManager()
{
    if (!g_networkAccessManager.has_value()) {
        g_networkAccessManager.emplace();

        if (QThread::isMainThread()) {
            // poor man's Q_APPLICATION_STATIC
            qAddPostRoutine([] {
                g_networkAccessManager.reset();
            });
        }
    }

    return *g_networkAccessManager;
}

} // namespace

#endif

#if QT_CONFIG(thread)

QSampleCache::SampleLoadResult
QSampleCache::loadSample(const QUrl &url, std::optional<SampleSourceType> forceSourceType)
{
    using namespace Qt::Literals;

    bool errorOccurred = false;

    if (url.scheme().isEmpty())
        // exit early, to avoid QNetworkAccessManager trying to construct a default ssl
        // configuration, which tends to cause timeouts on CI on macos.
        // catch this case and exit early.
        return std::nullopt;

    std::unique_ptr<QIODevice> decoderInput;
    SampleSourceType realSourceType =
            forceSourceType.value_or(url.scheme() == u"qrc"_s || url.scheme() == u"file"_s
                                             ? SampleSourceType::File
                                             : SampleSourceType::NetworkManager);
    if (realSourceType == SampleSourceType::File) {
        QString locationString =
                url.isLocalFile() ? url.toLocalFile() : u":" + url.toString(QUrl::RemoveScheme);

        auto *file = new QFile(locationString);
        bool opened = file->open(QFile::ReadOnly);
        if (!opened)
            errorOccurred = true;
        decoderInput.reset(file);
    } else {
#if QT_CONFIG(network)
        QNetworkReply *reply = threadLocalNetworkAccessManager().get(QNetworkRequest(url));

        if (reply->error() != QNetworkReply::NoError)
            errorOccurred = true;

        connect(reply, &QNetworkReply::errorOccurred, reply,
                [&]([[maybe_unused]] QNetworkReply::NetworkError errorCode) {
            errorOccurred = true;
        });

        decoderInput.reset(reply);
#else
        return std::nullopt;
#endif
    }

    if (!decoderInput->isOpen())
        return std::nullopt;

    QByteArray data = decoderInput->readAll();
    if (data.isEmpty() || errorOccurred)
        return std::nullopt;

    return loadSample(std::move(data));
}

#endif

QFuture<QSampleCache::SampleLoadResult> QSampleCache::loadSampleAsync(const QUrl &url)
{
    auto promise = std::make_shared<QPromise<QSampleCache::SampleLoadResult>>();
    auto future = promise->future();

    auto fulfilPromise = [&](auto &&result) mutable {
        promise->start();
        promise->addResult(result);
        promise->finish();
    };

    using namespace Qt::Literals;

    SampleSourceType realSourceType = (url.scheme() == u"qrc"_s || url.scheme() == u"file"_s)
            ? SampleSourceType::File
            : SampleSourceType::NetworkManager;
    if (realSourceType == SampleSourceType::File) {
        QString locationString = url.toString(QUrl::RemoveScheme);
        if (url.scheme() == u"qrc"_s)
            locationString = u":" + locationString;
        QFile file{ locationString };
        bool opened = file.open(QFile::ReadOnly);
        if (!opened) {
            fulfilPromise(std::nullopt);
            return future;
        }

        QByteArray data = file.readAll();
        if (data.isEmpty()) {
            fulfilPromise(std::nullopt);
            return future;
        }

        fulfilPromise(loadSample(std::move(data)));
        return future;
    }
#if QT_CONFIG(network)
    QNetworkReply *reply = threadLocalNetworkAccessManager().get(QNetworkRequest(url));

    if (reply->error() != QNetworkReply::NoError) {
        fulfilPromise(std::nullopt);
        delete reply;
        return future;
    }

    connect(reply, &QNetworkReply::errorOccurred, reply,
            [reply, promise]([[maybe_unused]] QNetworkReply::NetworkError errorCode) {
        promise->start();
        promise->addResult(std::nullopt);
        promise->finish();
        reply->deleteLater(); // we cannot delete immediately
    });

    connect(reply, &QNetworkReply::finished, reply, [promise, reply] {
        promise->start();
        QByteArray data = reply->readAll();
        if (data.isEmpty())
            promise->addResult(std::nullopt);
        else
            promise->addResult(loadSample(std::move(data)));
        promise->finish();
        reply->deleteLater(); // we cannot delete immediately
    });
#else
    fulfilPromise(std::nullopt);
#endif
    return future;
}

bool QSampleCache::isCached(const QUrl &url) const
{
    std::lock_guard guard(m_mutex);

    return m_loadedSamples.find(url) != m_loadedSamples.end()
            || m_pendingSamples.find(url) != m_pendingSamples.end();
}

QFuture<SharedSamplePtr> QSampleCache::requestSampleFuture(const QUrl &url)
{
    std::lock_guard guard(m_mutex);

    auto promise = std::make_shared<QPromise<SharedSamplePtr>>();
    auto future = promise->future();

    // found and ready
    auto found = m_loadedSamples.find(url);
    if (found != m_loadedSamples.end()) {
        SharedSamplePtr foundSample = found->second.lock();
        Q_ASSERT(foundSample);
        Q_ASSERT(foundSample->state() == QSample::Ready);
        promise->start();
        promise->addResult(std::move(foundSample));
        promise->finish();
        return future;
    }

    // already in the process of being loaded
    auto pending = m_pendingSamples.find(url);
    if (pending != m_pendingSamples.end()) {
        pending->second.second.append(promise);
        return future;
    }

    // we need to start a new load process
    SharedSamplePtr sample = std::make_shared<QSample>(url, this);
    m_pendingSamples.emplace(url, std::pair{ sample, QList<SharedSamplePromise>{ promise } });

#if QT_CONFIG(thread)
    QFuture<SampleLoadResult> futureResult =
            QtConcurrent::run(&m_threadPool, [url, type = m_sampleSourceType] {
        return loadSample(url, type);
    });
#else
    QFuture<SampleLoadResult> futureResult = loadSampleAsync(url);
#endif

    futureResult.then(this,
                      [this, url, sample = std::move(sample)](SampleLoadResult loadResult) mutable {
        if (loadResult)
            sample->setData(loadResult->first, loadResult->second);
        else
            sample->setError();

        std::lock_guard guard(m_mutex);

        auto pending = m_pendingSamples.find(url);
        if (pending != m_pendingSamples.end()) {
            for (auto &promise : pending->second.second) {
                promise->start();
                promise->addResult(loadResult ? sample : nullptr);
                promise->finish();
            }
        }

        if (loadResult)
            m_loadedSamples.emplace(url, sample);

        if (pending != m_pendingSamples.end())
            m_pendingSamples.erase(pending);
        sample = {};
    });

    return future;
}

QSample::~QSample()
{
    // Remove ourselves from our parent
    if (m_parent)
        m_parent->removeUnreferencedSample(m_url);

    qCDebug(qLcSampleCache) << "~QSample" << this << ": deleted [" << m_url << "]" << QThread::currentThread();
}

void QSampleCache::removeUnreferencedSample(const QUrl &url)
{
    std::lock_guard guard(m_mutex);
    m_loadedSamples.erase(url);
}

void QSample::setError()
{
    m_state = State::Error;
}

void QSample::setData(QByteArray data, QAudioFormat format)
{
    m_state = State::Ready;
    m_soundData = std::move(data);
    m_audioFormat = format;
}

QSample::State QSample::state() const
{
    return m_state;
}

QSample::QSample(QUrl url, QSampleCache *parent) : m_parent(parent), m_url(std::move(url)) { }

void QSample::clearParent()
{
    m_parent = nullptr;
}

QT_END_NAMESPACE

#if !QT_CONFIG(thread)
#  undef thread_local
#endif
