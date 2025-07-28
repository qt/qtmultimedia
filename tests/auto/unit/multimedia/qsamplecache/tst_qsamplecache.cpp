// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <private/qsamplecache_p.h>
#include <QtCore/qfuturewatcher.h>

class tst_QSampleCache : public QObject
{
    Q_OBJECT
public:

public slots:

private slots:
    void testCachedSample_data() { generateTestData(); }
    void testCachedSample();

    void testNotCachedSample_data() { generateTestData(); }
    void testNotCachedSample();

    void testInvalidFile_data() { generateTestData(); }
    void testInvalidFile();

    void testIncompatibleFile_data() { generateTestData(); }
    void testIncompatibleFile();

private:
    void generateTestData()
    {
        QTest::addColumn<QSampleCache::SampleSourceType>("sampleSourceType");
#ifdef QT_FEATURE_network
        QTest::newRow("NetworkManager") << QSampleCache::SampleSourceType::NetworkManager;
#endif
        QTest::newRow("File") << QSampleCache::SampleSourceType::File;
    }

    SharedSamplePtr requestSample(QSampleCache &cache, const QUrl &url)
    {
        auto future = cache.requestSampleFuture(url);

        QFutureWatcher<SharedSamplePtr> watcher;
        watcher.setFuture(future);

        QEventLoop loop;
        connect(&watcher, &QFutureWatcher<SharedSamplePtr>::finished, &loop, [&] {
            loop.exit(0);
        });
        loop.exec(QEventLoop::EventLoopExec);
        return future.result();
    }
};

void tst_QSampleCache::testCachedSample()
{
    QFETCH(const QSampleCache::SampleSourceType, sampleSourceType);

    QSampleCache cache;
    cache.setSampleSourceType(sampleSourceType);

    SharedSamplePtr sample =
            requestSample(cache, QUrl::fromLocalFile(QFINDTESTDATA("testdata/test.wav")));
    QVERIFY(sample);

    SharedSamplePtr sampleCached =
            requestSample(cache, QUrl::fromLocalFile(QFINDTESTDATA("testdata/test.wav")));
    QCOMPARE(sample, sampleCached); // sample is cached
    QVERIFY(cache.isCached(QUrl::fromLocalFile(QFINDTESTDATA("testdata/test.wav"))));
}

void tst_QSampleCache::testNotCachedSample()
{
    QFETCH(const QSampleCache::SampleSourceType, sampleSourceType);

    QSampleCache cache;
    cache.setSampleSourceType(sampleSourceType);

    SharedSamplePtr sample =
            requestSample(cache, QUrl::fromLocalFile(QFINDTESTDATA("testdata/test.wav")));
    QVERIFY(sample);
    sample = {};

    QVERIFY(!cache.isCached(QUrl::fromLocalFile(QFINDTESTDATA("testdata/test.wav"))));
}

void tst_QSampleCache::testInvalidFile()
{
    QFETCH(const QSampleCache::SampleSourceType, sampleSourceType);

    QSampleCache cache;
    cache.setSampleSourceType(sampleSourceType);

    SharedSamplePtr sample = requestSample(cache, QUrl::fromLocalFile("invalid"));
    QVERIFY(!sample);
    sample = {};

    QVERIFY(!cache.isCached(QUrl::fromLocalFile("invalid")));
}

void tst_QSampleCache::testIncompatibleFile()
{
    QFETCH(const QSampleCache::SampleSourceType, sampleSourceType);

    QSampleCache cache;
    cache.setSampleSourceType(sampleSourceType);

    const QUrl corruptedWavUrl = QUrl::fromLocalFile(QFINDTESTDATA("testdata/corrupted.wav"));
    SharedSamplePtr sample = requestSample(cache, corruptedWavUrl);
    QVERIFY(!sample);
}

QTEST_GUILESS_MAIN(tst_QSampleCache)

#include "tst_qsamplecache.moc"
