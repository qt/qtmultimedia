// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsamplecache_p.h"
#include "qaudiohelpers_p.h"

#include <QtCore/qapplicationstatic.h>
#include <QtCore/qbuffer.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qdebug.h>
#include <QtCore/qeventloop.h>
#include <QtCore/qfile.h>
#include <QtCore/qfuturewatcher.h>
#include <QtCore/qloggingcategory.h>
#include <QtMultimedia/qaudiobuffer.h>
#include <QtMultimedia/qaudiodecoder.h>
#include <QtMultimedia/private/qmultimediautils_p.h>
#include <QtConcurrent/qtconcurrentrun.h>

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

QSample::QSample(QUrl url, QSampleCache *parent) : m_parent(parent), m_url(std::move(url)) { }

QSample::QSample(QUrl url, QSampleCache *parent, std::optional<SampleRate> targetSampleRate)
    : m_parent(parent), m_url(std::move(url)), m_targetSampleRate(targetSampleRate)
{
}

QSample::~QSample()
{
    // Remove ourselves from our parent
    if (m_parent)
        m_parent->removeUnreferencedSample(m_url, m_targetSampleRate);

    qCDebug(qLcSampleCache) << "~QSample" << this << ": deleted [" << m_url << "]" << QThread::currentThread();
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

void QSample::clearParent()
{
    m_parent = nullptr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

Q_APPLICATION_STATIC(QSampleCache, sampleCache)

QSampleCache *QSampleCache::instance()
{
    return sampleCache();
}

#if QT_CONFIG(thread)
QThreadPool *QSampleCache::threadPool()
{
#ifdef Q_OS_WASM
    return QThreadPool::globalInstance();
#else
    return &m_threadPool;
#endif
}
#endif

QSampleCache::QSampleCache(QObject *parent) : QObject(parent)
{
#if QT_CONFIG(thread)
    if (!thread()->isMainThread())
        moveToThread(qApp->thread());

#  if !defined(Q_OS_WASM)
    // we limit the number of loader threads to avoid thread explosion
    static constexpr int loaderThreadLimit = 8;
    m_threadPool.setObjectName("QSampleCachePool");
    m_threadPool.setMaxThreadCount(loaderThreadLimit);
    m_threadPool.setExpiryTimeout(15);
    m_threadPool.setThreadPriority(QThread::LowPriority);
    m_threadPool.setServiceLevel(QThread::QualityOfService::Eco);

    qAddPostRoutine([] {
        // HACK: we need to stop the thread pool before qApp is nulled, otherwise some threads might still try construct
        // some Q_APPLICATION_STATIC instances, causing assertion failures inside QNetworkAccessManager
        Q_ASSERT(qApp && "QApplication is still valid");

        QSampleCache *instance = sampleCache();

        instance->m_threadPool.clear();
        instance->m_threadPool.waitForDone();
    });

#  endif // Q_OS_WASM
#endif // QT_CONFIG(thread)
}

QSampleCache::~QSampleCache()
{
#if QT_CONFIG(thread) && !defined(Q_OS_WASM)
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

QSampleCache::SampleLoadResult QSampleCache::loadSample(QSpan<const char> data)
{
    using namespace QtPrivate;

    drwav wavParser;
    bool success = drwav_init_memory(&wavParser, data.data(), data.size(), nullptr);
    if (!success)
        return q23::unexpected(QSampleLoadError::FormatError);

    auto cleanup = qScopeGuard([&] {
        drwav_uninit(&wavParser);
    });

    // using float as internal format. one could argue to use int16 and save half the ram at the
    // cost of potential run-time conversions
    QAudioFormat audioFormat;
    audioFormat.setChannelCount(wavParser.channels);
    audioFormat.setSampleFormat(QAudioFormat::Float);
    audioFormat.setSampleRate(wavParser.sampleRate);
    audioFormat.setChannelConfig(
            QAudioFormat::defaultChannelConfigForChannelCount(wavParser.channels));

    QByteArray sampleData;
    sampleData.resizeForOverwrite(qsizetype(sizeof(float) * wavParser.channels
                                   * wavParser.totalPCMFrameCount));
    uint64_t framesRead = drwav_read_pcm_frames_f32(&wavParser, wavParser.totalPCMFrameCount,
                                                    reinterpret_cast<float *>(sampleData.data()));

    if (framesRead != wavParser.totalPCMFrameCount)
        return q23::unexpected(QSampleLoadError::FormatError);

    return std::pair{
        std::move(sampleData),
        audioFormat,
    };
}

namespace {

QByteArray convertToFloat32(const QByteArray &data, const QAudioFormat &fmt)
{
    if (fmt.sampleFormat() == QAudioFormat::Float)
        return data;

    int totalSamples = fmt.framesForBytes(data.size()) * fmt.channelCount();

    QByteArray result{
        totalSamples * int(sizeof(float)),
        Qt::Initialization::Uninitialized,
    };

    using namespace QAudioHelperInternal;
    convertSampleFormat(as_bytes(QSpan{ data }), toNativeSampleFormat(fmt.sampleFormat()),
                        as_writable_bytes(QSpan{ result }), NativeSampleFormat::float32_t);

    return result;
}

QSampleCache::SampleLoadResult runDecoderLoop(QAudioDecoder &decoder, QEventLoop &loop)
{
    using SampleLoadResult = QSampleCache::SampleLoadResult;

    QByteArray accumulated;
    QAudioFormat fmt;
    SampleLoadResult result = q23::unexpected(QSampleLoadError::DecoderError);

    QObject::connect(&decoder, &QAudioDecoder::bufferReady, &loop, [&] {
        QAudioBuffer buf = decoder.read();
        if (!buf.isValid())
            return;
        if (!fmt.isValid())
            fmt = buf.format();
        accumulated.append(buf.constData<char>(), buf.byteCount());
    });

    QObject::connect(&decoder, &QAudioDecoder::finished, &loop, [&] {
        QByteArray floatData = convertToFloat32(accumulated, fmt);
        QAudioFormat floatFmt = fmt;
        floatFmt.setSampleFormat(QAudioFormat::Float);
        result = std::pair{ std::move(floatData), floatFmt };
        loop.quit();
    });

    QObject::connect(&decoder, qOverload<QAudioDecoder::Error>(&QAudioDecoder::error),
                     &loop, [&](QAudioDecoder::Error) {
        result = q23::unexpected(QSampleLoadError::DecoderError);
        loop.quit();
    });

    decoder.start();
    if (decoder.error() != QAudioDecoder::NoError)
        return q23::unexpected(QSampleLoadError::DecoderError);

    loop.exec(QEventLoop::ExcludeUserInputEvents);

    return result;
}

} // unnamed namespace

QSampleCache::SampleLoadResult
QSampleCache::loadSampleViaDecoder(std::variant<QUrl, QByteArray> arg)
{
    // caveat: we run our own event loop, so this function should ideally not be run from the main thread.
    using namespace QtMultimediaPrivate;

    QAudioDecoder decoder;
    if (!decoder.isSupported())
        return q23::unexpected(QSampleLoadError::NotSupported);

    QEventLoop loop;
    // clang-format off
    return std::visit(qOverloadedVisitor([&](QUrl url) -> SampleLoadResult {
        decoder.setSource(url);
        return runDecoderLoop(decoder, loop);
    }, [&](QByteArray data) -> SampleLoadResult {
        QBuffer buffer;
        buffer.setData(data);
        if (!buffer.open(QIODevice::ReadOnly))
            return q23::unexpected(QSampleLoadError::IoError);
        decoder.setSourceDevice(&buffer);

        return runDecoderLoop(decoder, loop);
    }), std::move(arg));
    // clang-format on
}

#if QT_CONFIG(network)

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

    SampleSourceType realSourceType =
            forceSourceType.value_or(url.scheme() == u"qrc"_s || url.scheme() == u"file"_s
                                             ? SampleSourceType::File
                                             : SampleSourceType::NetworkManager);

    if (realSourceType == SampleSourceType::AudioDecoder)
        return loadSampleViaDecoder(url);

    if (url.scheme().isEmpty())
        // exit early, to avoid QNetworkAccessManager trying to construct a default ssl
        // configuration, which tends to cause timeouts on CI on macos.
        // catch this case and exit early.
        return q23::unexpected(QSampleLoadError::IoError);

    bool errorOccurred = false;

    std::unique_ptr<QIODevice> decoderInput;
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
        return q23::unexpected(QSampleLoadError::IoError);
#endif
    }

    if (!decoderInput->isOpen())
        return q23::unexpected(QSampleLoadError::IoError);

    QByteArray data = decoderInput->readAll();
    if (data.isEmpty() || errorOccurred)
        return q23::unexpected(QSampleLoadError::IoError);

    SampleLoadResult result = loadSample(data);
    if (!result && result.error() == QSampleLoadError::FormatError && !forceSourceType) {
        qCDebug(qLcSampleCache) << "drwav failed, retrying via QAudioDecoder";
        result = loadSampleViaDecoder(data);
        if (!result) {
            qCDebug(qLcSampleCache) << "device-based decode failed, retrying via URL";
            result = loadSampleViaDecoder(url);
        }
    }
    return result;
}

#endif

QFuture<QSampleCache::SampleLoadResult>
QSampleCache::loadSampleAsync(const QUrl &url, std::optional<SampleSourceType> forceSourceType)
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
            fulfilPromise(q23::unexpected(QSampleLoadError::IoError));
            return future;
        }

        QByteArray data = file.readAll();
        if (data.isEmpty()) {
            fulfilPromise(q23::unexpected(QSampleLoadError::IoError));
            return future;
        }

        if (forceSourceType == SampleSourceType::AudioDecoder)
            return loadSampleAsyncViaDecoder(data);

        auto drwavResult = loadSample(data);
        if (drwavResult || drwavResult.error() != QSampleLoadError::FormatError) {
            fulfilPromise(drwavResult);
            return future;
        }
        return loadSampleAsyncViaDecoder(data);
    }

#if QT_CONFIG(network)

    QNetworkReply *reply = threadLocalNetworkAccessManager().get(QNetworkRequest(url));

    if (reply->error() != QNetworkReply::NoError) {
        fulfilPromise(q23::unexpected(QSampleLoadError::IoError));
        reply->deleteLater();
        return future;
    }

    connect(reply, &QNetworkReply::errorOccurred, reply,
            [reply, promise]([[maybe_unused]] QNetworkReply::NetworkError errorCode) {
        promise->start();
        promise->addResult(q23::unexpected(QSampleLoadError::IoError));
        promise->finish();
        reply->deleteLater(); // we cannot delete immediately
    });

    connect(reply, &QNetworkReply::finished, reply,
            [promise, reply, fulfilPromise = std::move(fulfilPromise), forceSourceType]() mutable {
        QByteArray data = reply->readAll();
        if (data.isEmpty()) {
            promise->start();
            promise->addResult(q23::unexpected(QSampleLoadError::IoError));
            promise->finish();
        } else if (forceSourceType == SampleSourceType::AudioDecoder) {
            auto decoderFuture = loadSampleAsyncViaDecoder(data);
            decoderFuture.then([promise](SampleLoadResult result) {
                promise->start();
                promise->addResult(std::move(result));
                promise->finish();
            });
        } else {
            auto drwavResult = loadSample(data);
            if (drwavResult || drwavResult.error() != QSampleLoadError::FormatError) {
                fulfilPromise(drwavResult);
            } else {
                auto decoderFuture = loadSampleAsyncViaDecoder(data);
                decoderFuture.then([promise](SampleLoadResult result) {
                    promise->start();
                    promise->addResult(std::move(result));
                    promise->finish();
                });
            }
        }
        reply->deleteLater(); // we cannot delete immediately
    });
#else
    fulfilPromise(q23::unexpected(QSampleLoadError::IoError));
#endif
    return future;
}

namespace {

// keeps the only reference to the QAudioDecoder instances alive until they finish decoding.
// We cannot keep the QAudioDecoder instances in the the decoder connections, as they may not
// fire if the application is shutting down before the decoder terminates
struct QStaticDecoderSingleton
{
    void addDecoder(std::shared_ptr<QAudioDecoder> decoder)
    {
        std::lock_guard guard(mutex);
        decoders.insert(std::move(decoder));
    }

    void removeDecoder(std::shared_ptr<QAudioDecoder> decoder)
    {
        std::lock_guard guard(mutex);
        decoders.erase(decoder);
    }

    std::set<std::shared_ptr<QAudioDecoder>> decoders;
    QBasicMutex mutex;
};

Q_APPLICATION_STATIC(QStaticDecoderSingleton, decoders);

} // namespace

QFuture<QSampleCache::SampleLoadResult>
QSampleCache::loadSampleAsyncViaDecoder(QByteArray data)
{
    // NB: we heap-allocate the QAudioDecoder and keep it alive until decoding finishes or an error
    // occurs. however we cannot keep the QAudioDecoder alive in the lambda captures of the decoder
    // signals, as they might not fire if the application is shutting down before the decoder
    // terminates, causing a potential leak. Instead, we keep the decoders in a singleton set until
    // they finish decoding, and capture weak pointers to them in the lambda captures of the
    // finished/error signals.

    auto promise = std::make_shared<QPromise<SampleLoadResult>>();
    auto future = promise->future();

    auto decoder = QtMultimediaPrivate::makeSharedDeleteLater<QAudioDecoder>();
    if (!decoder->isSupported()) {
        promise->start();
        promise->addResult(q23::unexpected(QSampleLoadError::NotSupported));
        promise->finish();
        return future;
    }

    auto *buffer = new QBuffer(decoder.get());
    buffer->setData(data);
    buffer->open(QIODevice::ReadOnly);

    auto accum = std::make_shared<QByteArray>();
    auto fmt = std::make_shared<QAudioFormat>();

    decoders->addDecoder(decoder);

    QObject::connect(decoder.get(), &QAudioDecoder::bufferReady, decoder.get(),
                     [weakDecoder = std::weak_ptr<QAudioDecoder>(decoder), accum, fmt] {
        auto decoder = weakDecoder.lock();
        if (!decoder)
            return;

        QAudioBuffer buf = decoder->read();
        if (!buf.isValid())
            return;
        if (!fmt->isValid())
            *fmt = buf.format();
        accum->append(buf.constData<char>(), buf.byteCount());
    });

    QObject::connect(decoder.get(), &QAudioDecoder::finished, decoder.get(),
                     [promise, accum, fmt, weakDecoder = std::weak_ptr<QAudioDecoder>(decoder)] {
        auto decoder = weakDecoder.lock();
        if (!decoder)
            return;

        QByteArray floatData = convertToFloat32(*accum, *fmt);
        QAudioFormat floatFmt = *fmt;
        floatFmt.setSampleFormat(QAudioFormat::Float);
        promise->start();
        promise->addResult(std::pair{ std::move(floatData), floatFmt });
        promise->finish();

        decoders->removeDecoder(decoder);
    });

    QObject::connect(
            decoder.get(), qOverload<QAudioDecoder::Error>(&QAudioDecoder::error), decoder.get(),
            [promise, weakDecoder = std::weak_ptr<QAudioDecoder>(decoder)](QAudioDecoder::Error) {
        auto decoder = weakDecoder.lock();
        if (!decoder)
            return;

        promise->start();
        promise->addResult(q23::unexpected(QSampleLoadError::DecoderError));
        promise->finish();
        decoders->removeDecoder(decoder);
    });

    decoder->setSourceDevice(buffer);
    decoder->start();
    return future;
}

bool QSampleCache::isCached(const QUrl &url, std::optional<int> targetSampleRate) const
{
    std::lock_guard guard(m_mutex);
    using namespace QtMultimediaPrivate;

    const SampleKey key{ url, asSampleRate(targetSampleRate) };
    return m_loadedSamples.find(key) != m_loadedSamples.end()
            || m_pendingSamples.find(key) != m_pendingSamples.end();
}

QFuture<SharedSamplePtr>
QSampleCache::requestSampleFuture(const QUrl &url, std::optional<int> targetSampleRate,
                                  std::optional<SampleSourceType> forceSourceType)
{
    std::lock_guard guard(m_mutex);
    using namespace QtMultimediaPrivate;

    auto targetRate = asSampleRate(targetSampleRate);
    const SampleKey key{ url, targetRate };

    auto promise = std::make_shared<QPromise<SharedSamplePtr>>();
    auto future = promise->future();

    // found and ready
    auto found = m_loadedSamples.find(key);
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
    auto pending = m_pendingSamples.find(key);
    if (pending != m_pendingSamples.end()) {
        pending->second.second.append(promise);
        return future;
    }

    // we need to start a new load process
    SharedSamplePtr sample = std::make_shared<QSample>(url, this, targetRate);
    m_pendingSamples.emplace(key, std::pair{ sample, QList<SharedSamplePromise>{ promise } });

    QFuture<SampleLoadResult> futureResult = [&] {
#if QT_CONFIG(thread)
        if (threadPool()->maxThreadCount() > 0)
            return QtConcurrent::run(threadPool(), [url, type = forceSourceType] {
                return loadSample(url, type);
            });
#endif
        return loadSampleAsync(url, forceSourceType);
    }();

    futureResult.then(this,
                      [this, key, targetRate,
                       sample = std::move(sample)](SampleLoadResult loadResult) mutable {
        if (loadResult) {
            QByteArray sampleData = loadResult->first;
            QAudioFormat sampleFormat = loadResult->second;

            if (targetRate && sampleFormat.sampleRate() != qToUnderlying(*targetRate)) {
                const int rate = qToUnderlying(*targetRate);
                const qsizetype totalFloats = sampleData.size() / qsizetype(sizeof(float));
                QSpan<const float> inputSpan{
                    reinterpret_cast<const float *>(sampleData.constData()),
                    totalFloats
                };
                sampleData = QAudioHelperInternal::resampleAudioCatmullRom(
                        inputSpan, sampleFormat.channelCount(),
                        sampleFormat.sampleRate(), rate);
                sampleFormat.setSampleRate(rate);
            }

            sample->setData(std::move(sampleData), sampleFormat);
        } else {
            sample->setError();
        }

        std::lock_guard guard(m_mutex);

        auto pending = m_pendingSamples.find(key);
        if (pending != m_pendingSamples.end()) {
            for (auto &promise : pending->second.second) {
                promise->start();
                promise->addResult(loadResult ? sample : nullptr);
                promise->finish();
            }
        }

        if (loadResult)
            m_loadedSamples.emplace(key, sample);

        if (pending != m_pendingSamples.end())
            m_pendingSamples.erase(pending);
        sample = {};
    });

    return future;
}

void QSampleCache::removeUnreferencedSample(const QUrl &url,
                                            std::optional<SampleRate> targetSampleRate)
{
    std::lock_guard guard(m_mutex);
    m_loadedSamples.erase(SampleKey{ url, targetSampleRate });
}

QT_END_NAMESPACE

#if !QT_CONFIG(thread)
#  undef thread_local
#endif
