// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QtTest>
#include <private/qsamplecache_p.h>

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

    void testEnoughCapacity_data() { generateTestData(); }
    void testEnoughCapacity();

    void testNotEnoughCapacity_data() { generateTestData(); }
    void testNotEnoughCapacity();

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
};

void tst_QSampleCache::testCachedSample()
{
    QFETCH(const QSampleCache::SampleSourceType, sampleSourceType);

    QSampleCache cache;
    cache.setSampleSourceType(sampleSourceType);

    QSample* sample = cache.requestSample(QUrl::fromLocalFile(QFINDTESTDATA("testdata/test.wav")));
    QVERIFY(sample);
    QTRY_VERIFY(!cache.isLoading());

    QSample* sampleCached = cache.requestSample(QUrl::fromLocalFile(QFINDTESTDATA("testdata/test.wav")));
    QCOMPARE(sample, sampleCached); // sample is cached
    QVERIFY(cache.isCached(QUrl::fromLocalFile(QFINDTESTDATA("testdata/test.wav"))));
    // loading thread still starts, but does nothing in this case
    QTRY_VERIFY(!cache.isLoading());

    sample->release();
    sampleCached->release();
}

void tst_QSampleCache::testNotCachedSample()
{
    QFETCH(const QSampleCache::SampleSourceType, sampleSourceType);

    QSampleCache cache;
    cache.setSampleSourceType(sampleSourceType);

    QSample* sample = cache.requestSample(QUrl::fromLocalFile(QFINDTESTDATA("testdata/test.wav")));
    QVERIFY(sample);
    QVERIFY(cache.isLoading());
    QTRY_VERIFY(!cache.isLoading());
    sample->release();

    QVERIFY(!cache.isCached(QUrl::fromLocalFile(QFINDTESTDATA("testdata/test.wav"))));
}

void tst_QSampleCache::testEnoughCapacity()
{
    QFETCH(const QSampleCache::SampleSourceType, sampleSourceType);

    QSampleCache cache;
    cache.setSampleSourceType(sampleSourceType);

    QSample* sample = cache.requestSample(QUrl::fromLocalFile(QFINDTESTDATA("testdata/test.wav")));
    QVERIFY(sample);
    QVERIFY(cache.isLoading());
    QTRY_VERIFY(!cache.isLoading());
    int sampleSize = sample->data().size();
    sample->release();
    cache.setCapacity(sampleSize * 2);

    QVERIFY(!cache.isCached(QUrl::fromLocalFile(QFINDTESTDATA("testdata/test.wav"))));

    sample = cache.requestSample(QUrl::fromLocalFile(QFINDTESTDATA("testdata/test.wav")));
    QVERIFY(sample);
    QVERIFY(cache.isLoading());
    QTRY_VERIFY(!cache.isLoading());
    sample->release();

    QVERIFY(cache.isCached(QUrl::fromLocalFile(QFINDTESTDATA("testdata/test.wav"))));

    // load another sample and make sure first sample is not destroyed
    QSample* sampleOther = cache.requestSample(QUrl::fromLocalFile(QFINDTESTDATA("testdata/test2.wav")));
    QVERIFY(sampleOther);
    QVERIFY(cache.isLoading());
    QTRY_VERIFY(!cache.isLoading());
    sampleOther->release();

    QVERIFY(cache.isCached(QUrl::fromLocalFile(QFINDTESTDATA("testdata/test.wav"))));
    QVERIFY(cache.isCached(QUrl::fromLocalFile(QFINDTESTDATA("testdata/test2.wav"))));

    QSample* sampleCached = cache.requestSample(QUrl::fromLocalFile(QFINDTESTDATA("testdata/test.wav")));
    QCOMPARE(sample, sampleCached); // sample is cached
    QVERIFY(cache.isCached(QUrl::fromLocalFile(QFINDTESTDATA("testdata/test.wav"))));
    QVERIFY(cache.isCached(QUrl::fromLocalFile(QFINDTESTDATA("testdata/test2.wav"))));
    QVERIFY(!cache.isLoading());

    sampleCached->release();
}

void tst_QSampleCache::testNotEnoughCapacity()
{
    QFETCH(const QSampleCache::SampleSourceType, sampleSourceType);

    QSampleCache cache;
    cache.setSampleSourceType(sampleSourceType);

    using namespace Qt::Literals;

    QSample* sample = cache.requestSample(QUrl::fromLocalFile(QFINDTESTDATA("testdata/test.wav")));
    QVERIFY(sample);
    QVERIFY(cache.isLoading());
    QTRY_VERIFY(!cache.isLoading());
    int sampleSize = sample->data().size();
    sample->release();
    cache.setCapacity(sampleSize / 2); // unloads all samples

    QVERIFY(!cache.isCached(QUrl::fromLocalFile(QFINDTESTDATA("testdata/test.wav"))));

    QTest::ignoreMessage(QtMsgType::QtWarningMsg,
                         QRegularExpression("QSampleCache: usage .* out of limit .*"));
    sample = cache.requestSample(QUrl::fromLocalFile(QFINDTESTDATA("testdata/test.wav")));
    QVERIFY(sample);
    QVERIFY(cache.isLoading());
    QTRY_VERIFY(!cache.isLoading());
    sample->release();

    QVERIFY(cache.isCached(QUrl::fromLocalFile(QFINDTESTDATA("testdata/test.wav"))));

    // load another sample to force sample cache to destroy first sample
    QTest::ignoreMessage(QtMsgType::QtWarningMsg,
                         QRegularExpression("QSampleCache: usage .* out of limit .*"));
    QSample *sampleOther =
            cache.requestSample(QUrl::fromLocalFile(QFINDTESTDATA("testdata/test2.wav")));
    QVERIFY(sampleOther);
    QVERIFY(cache.isLoading());
    QTRY_VERIFY(!cache.isLoading());
    sampleOther->release();

    QVERIFY(!cache.isCached(QUrl::fromLocalFile(QFINDTESTDATA("testdata/test.wav"))));
}

void tst_QSampleCache::testInvalidFile()
{
    QFETCH(const QSampleCache::SampleSourceType, sampleSourceType);

    QSampleCache cache;
    cache.setSampleSourceType(sampleSourceType);

    QSample* sample = cache.requestSample(QUrl::fromLocalFile("invalid"));
    QVERIFY(sample);
    QTRY_COMPARE(sample->state(), QSample::Error);
    QVERIFY(!cache.isLoading());
    sample->release();

    QVERIFY(!cache.isCached(QUrl::fromLocalFile("invalid")));
}

void tst_QSampleCache::testIncompatibleFile()
{
    QFETCH(const QSampleCache::SampleSourceType, sampleSourceType);

    QSampleCache cache;
    cache.setSampleSourceType(sampleSourceType);
    cache.setCapacity(10024);

    // Load a sample that is known to fail and verify that
    // it remains in the cache with an error status.
    const QUrl corruptedWavUrl = QUrl::fromLocalFile(QFINDTESTDATA("testdata/corrupted.wav"));
    for (int i = 0; i < 3; ++i) {
        QSample* sample = cache.requestSample(corruptedWavUrl);
        QVERIFY(sample);
        QTRY_VERIFY(!cache.isLoading());
        QCOMPARE(sample->state(), QSample::Error);
        sample->release();

        QVERIFY(cache.isCached(corruptedWavUrl));
    }
}

QTEST_GUILESS_MAIN(tst_QSampleCache)

#include "tst_qsamplecache.moc"
