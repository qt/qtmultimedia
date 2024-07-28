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
    void testCachedSample();
    void testNotCachedSample();
    void testEnoughCapacity();
    void testNotEnoughCapacity();
    void testInvalidFile();
    void testIncompatibleFile();

private:

};

void tst_QSampleCache::testCachedSample()
{
    QSampleCache cache;

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
    QSampleCache cache;

    QSample* sample = cache.requestSample(QUrl::fromLocalFile(QFINDTESTDATA("testdata/test.wav")));
    QVERIFY(sample);
    QVERIFY(cache.isLoading());
    QTRY_VERIFY(!cache.isLoading());
    sample->release();

    QVERIFY(!cache.isCached(QUrl::fromLocalFile(QFINDTESTDATA("testdata/test.wav"))));
}

void tst_QSampleCache::testEnoughCapacity()
{
    QSampleCache cache;

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
    using namespace Qt::Literals;
    QSampleCache cache;

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
    QSampleCache cache;

    QSample* sample = cache.requestSample(QUrl::fromLocalFile("invalid"));
    QVERIFY(sample);
    QTRY_COMPARE(sample->state(), QSample::Error);
    QVERIFY(!cache.isLoading());
    sample->release();

    QVERIFY(!cache.isCached(QUrl::fromLocalFile("invalid")));
}

void tst_QSampleCache::testIncompatibleFile()
{
    QSampleCache cache;
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

QTEST_MAIN(tst_QSampleCache)

#include "tst_qsamplecache.moc"
