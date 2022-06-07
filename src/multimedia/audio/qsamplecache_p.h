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

#include <QtCore/qobject.h>
#include <QtCore/qthread.h>
#include <QtCore/qurl.h>
#include <QtCore/qmutex.h>
#include <QtCore/qmap.h>
#include <QtCore/qset.h>
#include <qaudioformat.h>
#include <qnetworkreply.h>
#include <private/qglobal_p.h>

QT_BEGIN_NAMESPACE

class QIODevice;
class QNetworkAccessManager;
class QSampleCache;
class QWaveDecoder;

// Lives in application thread
class Q_MULTIMEDIA_EXPORT QSample : public QObject
{
    Q_OBJECT
public:
    friend class QSampleCache;
    enum State
    {
        Creating,
        Loading,
        Error,
        Ready,
    };

    State state() const;
    // These are not (currently) locked because they are only meant to be called after these
    // variables are updated to their final states
    const QByteArray& data() const { Q_ASSERT(state() == Ready); return m_soundData; }
    const QAudioFormat& format() const { Q_ASSERT(state() == Ready); return m_audioFormat; }
    void release();

Q_SIGNALS:
    void error();
    void ready();

protected:
    QSample(const QUrl& url, QSampleCache *parent);

private Q_SLOTS:
    void load();
    void loadingError(QNetworkReply::NetworkError);
    void decoderError();
    void readSample();
    void decoderReady();

private:
    void onReady();
    void cleanup();
    void addRef();
    void loadIfNecessary();
    QSample();
    ~QSample();

    mutable QMutex m_mutex;
    QSampleCache *m_parent;
    QByteArray   m_soundData;
    QAudioFormat m_audioFormat;
    QIODevice    *m_stream;
    QWaveDecoder *m_waveDecoder;
    QUrl         m_url;
    qint64       m_sampleReadLength;
    State        m_state;
    int          m_ref;
};

class Q_MULTIMEDIA_EXPORT QSampleCache : public QObject
{
    Q_OBJECT
public:
    friend class QSample;

    QSampleCache(QObject *parent = nullptr);
    ~QSampleCache();

    QSample* requestSample(const QUrl& url);
    void setCapacity(qint64 capacity);

    bool isLoading() const;
    bool isCached(const QUrl& url) const;

private:
    QMap<QUrl, QSample*> m_samples;
    QSet<QSample*> m_staleSamples;
    QNetworkAccessManager *m_networkAccessManager;
    mutable QRecursiveMutex m_mutex;
    qint64 m_capacity;
    qint64 m_usage;
    QThread m_loadingThread;

    QNetworkAccessManager& networkAccessManager();
    void refresh(qint64 usageChange);
    bool notifyUnreferencedSample(QSample* sample);
    void removeUnreferencedSample(QSample* sample);
    void unloadSample(QSample* sample);

    void loadingRelease();
    int m_loadingRefCount;
    QMutex m_loadingMutex;
};

QT_END_NAMESPACE

#endif // QSAMPLECACHE_P_H
