/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

//TESTED_COMPONENT=src/multimedia

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
    QSampleCache cache;

    QSample* sample = cache.requestSample(QUrl::fromLocalFile(QFINDTESTDATA("testdata/test.wav")));
    QVERIFY(sample);
    QVERIFY(cache.isLoading());
    QTRY_VERIFY(!cache.isLoading());
    int sampleSize = sample->data().size();
    sample->release();
    cache.setCapacity(sampleSize / 2); // unloads all samples

    QVERIFY(!cache.isCached(QUrl::fromLocalFile(QFINDTESTDATA("testdata/test.wav"))));

    sample = cache.requestSample(QUrl::fromLocalFile(QFINDTESTDATA("testdata/test.wav")));
    QVERIFY(sample);
    QVERIFY(cache.isLoading());
    QTRY_VERIFY(!cache.isLoading());
    sample->release();

    QVERIFY(cache.isCached(QUrl::fromLocalFile(QFINDTESTDATA("testdata/test.wav"))));

    // load another sample to force sample cache to destroy first sample
    QSample* sampleOther = cache.requestSample(QUrl::fromLocalFile(QFINDTESTDATA("testdata/test2.wav")));
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

QTEST_MAIN(tst_QSampleCache)

#include "tst_qsamplecache.moc"
