// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsamplecache_p.h"
#include "qwavedecoder.h"

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

#include <QtCore/QDebug>
#include <QtCore/qloggingcategory.h>

static Q_LOGGING_CATEGORY(qLcSampleCache, "qt.multimedia.samplecache")

#include <mutex>

QT_BEGIN_NAMESPACE


/*!
    \class QSampleCache
    \internal

    When you want to get a sound sample data, you need to request the QSample reference from QSampleCache.


    \code
        QSample *m_sample;     // class member.

      private Q_SLOTS:
        void decoderError();
        void sampleReady();
    \endcode

    \code
      Q_GLOBAL_STATIC(QSampleCache, sampleCache) //declare a singleton manager
    \endcode

    \code
        m_sample = sampleCache()->requestSample(url);
        switch(m_sample->state()) {
        case QSample::Ready:
            sampleReady();
            break;
        case QSample::Error:
            decoderError();
            break;
        default:
            connect(m_sample, SIGNAL(error()), this, SLOT(decoderError()));
            connect(m_sample, SIGNAL(ready()), this, SLOT(sampleReady()));
            break;
        }
    \endcode

    When you no longer need the sound sample data, you need to release it:

    \code
       if (m_sample) {
           m_sample->release();
           m_sample = 0;
       }
    \endcode
*/

QSampleCache::QSampleCache(QObject *parent)
    : QObject(parent)
    , m_networkAccessManager(nullptr)
    , m_capacity(0)
    , m_usage(0)
    , m_loadingRefCount(0)
{
    m_loadingThread.setObjectName(QLatin1String("QSampleCache::LoadingThread"));
}

QNetworkAccessManager& QSampleCache::networkAccessManager()
{
    if (!m_networkAccessManager)
        m_networkAccessManager = new QNetworkAccessManager();
    return *m_networkAccessManager;
}

QSampleCache::~QSampleCache()
{
    const std::lock_guard<QRecursiveMutex> locker(m_mutex);

    m_loadingThread.quit();
    m_loadingThread.wait();

    // Killing the loading thread means that no samples can be
    // deleted using deleteLater.  And some samples that had deleteLater
    // already called won't have been processed (m_staleSamples)
    for (auto it = m_samples.cbegin(), end = m_samples.cend(); it != end; ++it)
        delete it.value();

    const auto copyStaleSamples = m_staleSamples; //deleting a sample does affect the m_staleSamples list, but we create a copy
    for (QSample* sample : copyStaleSamples)
        delete sample;

    delete m_networkAccessManager;
}

void QSampleCache::loadingRelease()
{
    QMutexLocker locker(&m_loadingMutex);
    m_loadingRefCount--;
    if (m_loadingRefCount == 0) {
        if (m_loadingThread.isRunning()) {
            if (m_networkAccessManager) {
                m_networkAccessManager->deleteLater();
                m_networkAccessManager = nullptr;
            }
            m_loadingThread.exit();
        }
    }
}

bool QSampleCache::isLoading() const
{
    return m_loadingThread.isRunning();
}

bool QSampleCache::isCached(const QUrl &url) const
{
    const std::lock_guard<QRecursiveMutex> locker(m_mutex);
    return m_samples.contains(url);
}

QSample* QSampleCache::requestSample(const QUrl& url)
{
    //lock and add first to make sure live loadingThread will not be killed during this function call
    m_loadingMutex.lock();
    const bool needsThreadStart = m_loadingRefCount == 0;
    m_loadingRefCount++;
    m_loadingMutex.unlock();

    qCDebug(qLcSampleCache) << "QSampleCache: request sample [" << url << "]";
    std::unique_lock<QRecursiveMutex> locker(m_mutex);
    QMap<QUrl, QSample*>::iterator it = m_samples.find(url);
    QSample* sample;
    if (it == m_samples.end()) {
        if (needsThreadStart) {
            // Previous thread might be finishing, need to wait for it. If not, this is a no-op.
            m_loadingThread.wait();
            m_loadingThread.start();
        }
        sample = new QSample(url, this);
        m_samples.insert(url, sample);
#if QT_CONFIG(thread)
        sample->moveToThread(&m_loadingThread);
#endif
    } else {
        sample = *it;
    }

    sample->addRef();
    locker.unlock();

    sample->loadIfNecessary();
    return sample;
}

void QSampleCache::setCapacity(qint64 capacity)
{
    const std::lock_guard<QRecursiveMutex> locker(m_mutex);
    if (m_capacity == capacity)
        return;
    qCDebug(qLcSampleCache) << "QSampleCache: capacity changes from " << m_capacity << "to " << capacity;
    if (m_capacity > 0 && capacity <= 0) { //memory management strategy changed
        for (QMap<QUrl, QSample*>::iterator it = m_samples.begin(); it != m_samples.end();) {
            QSample* sample = *it;
            if (sample->m_ref == 0) {
                unloadSample(sample);
                it = m_samples.erase(it);
            } else {
                ++it;
            }
        }
    }

    m_capacity = capacity;
    refresh(0);
}

// Called locked
void QSampleCache::unloadSample(QSample *sample)
{
    m_usage -= sample->m_soundData.size();
    m_staleSamples.insert(sample);
    sample->deleteLater();
}

// Called in both threads
void QSampleCache::refresh(qint64 usageChange)
{
    const std::lock_guard<QRecursiveMutex> locker(m_mutex);
    m_usage += usageChange;
    if (m_capacity <= 0 || m_usage <= m_capacity)
        return;

    qint64 recoveredSize = 0;

    //free unused samples to keep usage under capacity limit.
    for (QMap<QUrl, QSample*>::iterator it = m_samples.begin(); it != m_samples.end();) {
        QSample* sample = *it;
        if (sample->m_ref > 0) {
            ++it;
            continue;
        }
        recoveredSize += sample->m_soundData.size();
        unloadSample(sample);
        it = m_samples.erase(it);
        if (m_usage <= m_capacity)
            return;
    }

    qCDebug(qLcSampleCache) << "QSampleCache: refresh(" << usageChange
             << ") recovered size =" << recoveredSize
             << "new usage =" << m_usage;

    if (m_usage > m_capacity)
        qWarning() << "QSampleCache: usage[" << m_usage << " out of limit[" << m_capacity << "]";
}

// Called in both threads
void QSampleCache::removeUnreferencedSample(QSample *sample)
{
    const std::lock_guard<QRecursiveMutex> locker(m_mutex);
    m_staleSamples.remove(sample);
}

// Called in loader thread (since this lives in that thread)
// Also called from application thread after loader thread dies.
QSample::~QSample()
{
    // Remove ourselves from our parent
    m_parent->removeUnreferencedSample(this);

    QMutexLocker locker(&m_mutex);
    qCDebug(qLcSampleCache) << "~QSample" << this << ": deleted [" << m_url << "]" << QThread::currentThread();
    cleanup();
}

// Called in application thread
void QSample::loadIfNecessary()
{
    QMutexLocker locker(&m_mutex);
    if (m_state == QSample::Error || m_state == QSample::Creating) {
        m_state = QSample::Loading;
        QMetaObject::invokeMethod(this, "load", Qt::QueuedConnection);
    } else {
        qobject_cast<QSampleCache*>(m_parent)->loadingRelease();
    }
}

// Called in application thread
bool QSampleCache::notifyUnreferencedSample(QSample* sample)
{
    if (m_loadingThread.isRunning())
        m_loadingThread.wait();

    const std::lock_guard<QRecursiveMutex> locker(m_mutex);

    if (m_capacity > 0)
        return false;
    m_samples.remove(sample->m_url);
    unloadSample(sample);
    return true;
}

// Called in application thread
void QSample::release()
{
    QMutexLocker locker(&m_mutex);
    qCDebug(qLcSampleCache) << "Sample:: release" << this << QThread::currentThread() << m_ref;
    if (--m_ref == 0) {
        locker.unlock();
        m_parent->notifyUnreferencedSample(this);
    }
}

// Called in dtor and when stream is loaded
// must be called locked.
void QSample::cleanup()
{
    qCDebug(qLcSampleCache) << "QSample: cleanup";
    if (m_waveDecoder) {
        m_waveDecoder->disconnect(this);
        m_waveDecoder->deleteLater();
    }
    if (m_stream) {
        m_stream->disconnect(this);
        m_stream->deleteLater();
    }

    m_waveDecoder = nullptr;
    m_stream = nullptr;
}

// Called in application thread
void QSample::addRef()
{
    m_ref++;
}

// Called in loading thread
void QSample::readSample()
{
#if QT_CONFIG(thread)
    Q_ASSERT(QThread::currentThread()->objectName() == QLatin1String("QSampleCache::LoadingThread"));
#endif
    QMutexLocker m(&m_mutex);
    qint64 read = m_waveDecoder->read(m_soundData.data() + m_sampleReadLength,
                      qMin(m_waveDecoder->bytesAvailable(),
                           qint64(m_waveDecoder->size() - m_sampleReadLength)));
    qCDebug(qLcSampleCache) << "QSample: readSample" << read;
    if (read > 0)
        m_sampleReadLength += read;
    if (m_sampleReadLength < m_waveDecoder->size())
        return;
    Q_ASSERT(m_sampleReadLength == qint64(m_soundData.size()));
    onReady();
}

// Called in loading thread
void QSample::decoderReady()
{
#if QT_CONFIG(thread)
    Q_ASSERT(QThread::currentThread()->objectName() == QLatin1String("QSampleCache::LoadingThread"));
#endif
    QMutexLocker m(&m_mutex);
    qCDebug(qLcSampleCache) << "QSample: decoder ready";
    m_parent->refresh(m_waveDecoder->size());

    m_soundData.resize(m_waveDecoder->size());
    m_sampleReadLength = 0;
    qint64 read = m_waveDecoder->read(m_soundData.data(), m_waveDecoder->size());
    qCDebug(qLcSampleCache) << "    bytes read" << read;
    if (read > 0)
        m_sampleReadLength += read;
    if (m_sampleReadLength >= m_waveDecoder->size())
        onReady();
}

// Called in all threads
QSample::State QSample::state() const
{
    QMutexLocker m(&m_mutex);
    return m_state;
}

// Called in loading thread
// Essentially a second ctor, doesn't need locks (?)
void QSample::load()
{
#if QT_CONFIG(thread)
    Q_ASSERT(QThread::currentThread()->objectName() == QLatin1String("QSampleCache::LoadingThread"));
#endif
    qCDebug(qLcSampleCache) << "QSample: load [" << m_url << "]";
    m_stream = m_parent->networkAccessManager().get(QNetworkRequest(m_url));
    connect(m_stream, SIGNAL(errorOccurred(QNetworkReply::NetworkError)), SLOT(loadingError(QNetworkReply::NetworkError)));
    m_waveDecoder = new QWaveDecoder(m_stream);
    connect(m_waveDecoder, SIGNAL(formatKnown()), SLOT(decoderReady()));
    connect(m_waveDecoder, SIGNAL(parsingError()), SLOT(decoderError()));
    connect(m_waveDecoder, SIGNAL(readyRead()), SLOT(readSample()));

    m_waveDecoder->open(QIODevice::ReadOnly);
}

void QSample::loadingError(QNetworkReply::NetworkError errorCode)
{
#if QT_CONFIG(thread)
    Q_ASSERT(QThread::currentThread()->objectName() == QLatin1String("QSampleCache::LoadingThread"));
#endif
    QMutexLocker m(&m_mutex);
    qCDebug(qLcSampleCache) << "QSample: loading error" << errorCode;
    cleanup();
    m_state = QSample::Error;
    qobject_cast<QSampleCache*>(m_parent)->loadingRelease();
    emit error();
}

// Called in loading thread
void QSample::decoderError()
{
#if QT_CONFIG(thread)
    Q_ASSERT(QThread::currentThread()->objectName() == QLatin1String("QSampleCache::LoadingThread"));
#endif
    QMutexLocker m(&m_mutex);
    qCDebug(qLcSampleCache) << "QSample: decoder error";
    cleanup();
    m_state = QSample::Error;
    qobject_cast<QSampleCache*>(m_parent)->loadingRelease();
    emit error();
}

// Called in loading thread from decoder when sample is done. Locked already.
void QSample::onReady()
{
#if QT_CONFIG(thread)
    Q_ASSERT(QThread::currentThread()->objectName() == QLatin1String("QSampleCache::LoadingThread"));
#endif
    m_audioFormat = m_waveDecoder->audioFormat();
    qCDebug(qLcSampleCache) << "QSample: load ready format:" << m_audioFormat;
    cleanup();
    m_state = QSample::Ready;
    qobject_cast<QSampleCache*>(m_parent)->loadingRelease();
    emit ready();
}

// Called in application thread, then moved to loader thread
QSample::QSample(const QUrl& url, QSampleCache *parent)
    : m_parent(parent)
    , m_stream(nullptr)
    , m_waveDecoder(nullptr)
    , m_url(url)
    , m_sampleReadLength(0)
    , m_state(Creating)
    , m_ref(0)
{
}

QT_END_NAMESPACE

#include "moc_qsamplecache_p.cpp"
