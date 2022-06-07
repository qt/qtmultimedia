// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtCore/QString>
#include <QtTest/QtTest>

#include <qaudiobuffer.h>

class tst_QAudioBuffer : public QObject
{
    Q_OBJECT

public:
    tst_QAudioBuffer();
    ~tst_QAudioBuffer() override;

private Q_SLOTS:
    void ctors();
    void assign();
    void constData() const;
    void data_const() const;
    void data();
    void durations();
    void durations_data();
    void stereoSample();

private:
    QAudioFormat mFormat;
    QAudioBuffer *mNull;
    QAudioBuffer *mEmpty;
    QAudioBuffer *mFromArray;
};

tst_QAudioBuffer::tst_QAudioBuffer()
{
    // Initialize some common buffers
    mFormat.setChannelCount(2);
    mFormat.setSampleFormat(QAudioFormat::Int16);
    mFormat.setSampleRate(10000);

    QByteArray b(4000, char(0x80));
    mNull = new QAudioBuffer;
    mEmpty = new QAudioBuffer(500, mFormat); // 500 stereo frames of 16 bits -> 2KB
    mFromArray = new QAudioBuffer(b, mFormat);
}


tst_QAudioBuffer::~tst_QAudioBuffer()
{
    delete mNull;
    delete mEmpty;
    delete mFromArray;
}

void tst_QAudioBuffer::ctors()
{
    // Null buffer
    QVERIFY(!mNull->isValid());
    QVERIFY(mNull->constData<char>() == nullptr);
    QVERIFY(mNull->data<char>() == nullptr);
    QVERIFY(((const QAudioBuffer*)mNull)->data<char>() == nullptr);
    QCOMPARE(mNull->duration(), 0LL);
    QCOMPARE(mNull->byteCount(), 0);
    QCOMPARE(mNull->sampleCount(), 0);
    QCOMPARE(mNull->frameCount(), 0);
    QCOMPARE(mNull->startTime(), -1LL);

    // Empty buffer
    QVERIFY(mEmpty->isValid());
    QVERIFY(mEmpty->constData<char>() != nullptr);
    QVERIFY(mEmpty->data<char>() != nullptr);
    QVERIFY(((const QAudioBuffer*)mEmpty)->data<char>() != nullptr);
    QCOMPARE(mEmpty->sampleCount(), 1000);
    QCOMPARE(mEmpty->frameCount(), 500);
    QCOMPARE(mEmpty->duration(), 50000LL);
    QCOMPARE(mEmpty->byteCount(), 2000);
    QCOMPARE(mEmpty->startTime(), -1LL);

    // bytearray buffer
    QVERIFY(mFromArray->isValid());
    QVERIFY(mFromArray->constData<char>() != nullptr);
    QVERIFY(mFromArray->data<char>() != nullptr);
    QVERIFY(((const QAudioBuffer*)mFromArray)->data<char>() != nullptr);
    /// 4000 bytes at 10KHz, 2ch, 16bit = 40kBps -> 0.1s
    QCOMPARE(mFromArray->duration(), 100000LL);
    QCOMPARE(mFromArray->byteCount(), 4000);
    QCOMPARE(mFromArray->sampleCount(), 2000);
    QCOMPARE(mFromArray->frameCount(), 1000);
    QCOMPARE(mFromArray->startTime(), -1LL);


    // Now some invalid buffers
    QAudioBuffer badFormat(1000, QAudioFormat());
    QVERIFY(!badFormat.isValid());
    QVERIFY(badFormat.constData<char>() == nullptr);
    QVERIFY(badFormat.data<char>() == nullptr);
    QVERIFY(((const QAudioBuffer*)&badFormat)->data<char>() == nullptr);
    QCOMPARE(badFormat.duration(), 0LL);
    QCOMPARE(badFormat.byteCount(), 0);
    QCOMPARE(badFormat.sampleCount(), 0);
    QCOMPARE(badFormat.frameCount(), 0);
    QCOMPARE(badFormat.startTime(), -1LL);

    QAudioBuffer badArray(QByteArray(), mFormat);
    QVERIFY(!badArray.isValid());
    QVERIFY(badArray.constData<char>() == nullptr);
    QVERIFY(badArray.data<char>() == nullptr);
    QVERIFY(((const QAudioBuffer*)&badArray)->data<char>() == nullptr);
    QCOMPARE(badArray.duration(), 0LL);
    QCOMPARE(badArray.byteCount(), 0);
    QCOMPARE(badArray.sampleCount(), 0);
    QCOMPARE(badArray.frameCount(), 0);
    QCOMPARE(badArray.startTime(), -1LL);

    QAudioBuffer badBoth = QAudioBuffer(QByteArray(), QAudioFormat());
    QVERIFY(!badBoth.isValid());
    QVERIFY(badBoth.constData<char>() == nullptr);
    QVERIFY(badBoth.data<char>() == nullptr);
    QVERIFY(((const QAudioBuffer*)&badBoth)->data<char>() == nullptr);
    QCOMPARE(badBoth.duration(), 0LL);
    QCOMPARE(badBoth.byteCount(), 0);
    QCOMPARE(badBoth.sampleCount(), 0);
    QCOMPARE(badBoth.frameCount(), 0);
    QCOMPARE(badBoth.startTime(), -1LL);
}

void tst_QAudioBuffer::assign()
{
    // TODO Needs strong behaviour definition
}

void tst_QAudioBuffer::constData() const
{
    const void *data = mEmpty->constData<void *>();
    QVERIFY(data != nullptr);

    const unsigned int *idata = reinterpret_cast<const unsigned int*>(data);
    QCOMPARE(*idata, 0U);

    const QAudioBuffer::U8S *sdata = mEmpty->constData<QAudioBuffer::U8S>();
    QVERIFY(sdata);
    QCOMPARE(sdata->value(QAudioFormat::FrontLeft), (unsigned char)0);
    QCOMPARE(sdata->value(QAudioFormat::FrontRight), (unsigned char)0);

    // The bytearray one should be 0x80
    data = mFromArray->constData<void *>();
    QVERIFY(data != nullptr);

    idata = reinterpret_cast<const unsigned int *>(data);
    QCOMPARE(*idata, 0x80808080);

    sdata = mFromArray->constData<QAudioBuffer::U8S>();
    QCOMPARE(sdata->value(QAudioFormat::FrontLeft), (unsigned char)0x80);
    QCOMPARE(sdata->value(QAudioFormat::FrontRight), (unsigned char)0x80);
}

void tst_QAudioBuffer::data_const() const
{
    const void *data = ((const QAudioBuffer*)mEmpty)->data<void *>();
    QVERIFY(data != nullptr);

    const unsigned int *idata = reinterpret_cast<const unsigned int*>(data);
    QCOMPARE(*idata, 0U);

    const QAudioBuffer::U8S *sdata = ((const QAudioBuffer*)mEmpty)->constData<QAudioBuffer::U8S>();
    QVERIFY(sdata);
    QCOMPARE(sdata->value(QAudioFormat::FrontLeft), (unsigned char)0);
    QCOMPARE(sdata->value(QAudioFormat::FrontRight), (unsigned char)0);

    // The bytearray one should be 0x80
    data = ((const QAudioBuffer*)mFromArray)->data<void *>();
    QVERIFY(data != nullptr);

    idata = reinterpret_cast<const unsigned int *>(data);
    QCOMPARE(*idata, 0x80808080);

    sdata = ((const QAudioBuffer*)mFromArray)->constData<QAudioBuffer::U8S>();
    QCOMPARE(sdata->value(QAudioFormat::FrontLeft), (unsigned char)0x80);
    QCOMPARE(sdata->value(QAudioFormat::FrontRight), (unsigned char)0x80);
}

void tst_QAudioBuffer::data()
{
    void *data = mEmpty->data<void *>();
    QVERIFY(data != nullptr);

    unsigned int *idata = reinterpret_cast<unsigned int*>(data);
    QCOMPARE(*idata, 0U);

    QAudioBuffer::U8S *sdata = mEmpty->data<QAudioBuffer::U8S>();
    QVERIFY(sdata);
    QCOMPARE(sdata->value(QAudioFormat::FrontLeft), (unsigned char)0);
    QCOMPARE(sdata->value(QAudioFormat::FrontRight), (unsigned char)0);

    // The bytearray one should be 0x80
    data = mFromArray->data<void *>();
    QVERIFY(data != nullptr);

    idata = reinterpret_cast<unsigned int *>(data);
    QCOMPARE(*idata, 0x80808080);

    sdata = mFromArray->data<QAudioBuffer::U8S>();
    QCOMPARE(sdata->value(QAudioFormat::FrontLeft), (unsigned char)0x80);
    QCOMPARE(sdata->value(QAudioFormat::FrontRight), (unsigned char)0x80);
}

void tst_QAudioBuffer::durations()
{
    QFETCH(int, channelCount);
    QFETCH(int, frameCount);
    int sampleCount = frameCount * channelCount;
    QFETCH(QAudioFormat::SampleFormat, sampleFormat);
    QFETCH(int, sampleRate);
    QFETCH(qint64, duration);
    QFETCH(int, byteCount);

    QAudioFormat f;
    f.setChannelCount(channelCount);
    f.setSampleFormat(sampleFormat);
    f.setSampleRate(sampleRate);

    QAudioBuffer b(frameCount, f);

    QCOMPARE(b.frameCount(), frameCount);
    QCOMPARE(b.sampleCount(), sampleCount);
    QCOMPARE(b.duration(), duration);
    QCOMPARE(b.byteCount(), byteCount);
}

void tst_QAudioBuffer::durations_data()
{
    QTest::addColumn<int>("channelCount");
    QTest::addColumn<int>("frameCount");
    QTest::addColumn<QAudioFormat::SampleFormat>("sampleFormat");
    QTest::addColumn<int>("sampleRate");
    QTest::addColumn<qint64>("duration");
    QTest::addColumn<int>("byteCount");
    QTest::newRow("M8_1000_8K") << 1 << 1000 << QAudioFormat::UInt8 << 8000 << 125000LL << 1000;
    QTest::newRow("M8_2000_8K") << 1 << 2000 << QAudioFormat::UInt8 << 8000 << 250000LL << 2000;
    QTest::newRow("M8_1000_4K") << 1 << 1000 << QAudioFormat::UInt8 << 4000 << 250000LL << 1000;

    QTest::newRow("S8_1000_8K") << 2 << 500 << QAudioFormat::UInt8 << 8000 << 62500LL << 1000;

    QTest::newRow("SF_1000_8K") << 2 << 500 << QAudioFormat::Float << 8000 << 62500LL << 4000;

    QTest::newRow("S32_1000_16K") << 4 << 250 << QAudioFormat::Int32 << 16000 << 15625LL << 4000;
}

void tst_QAudioBuffer::stereoSample()
{
    // Uninitialized (should default to zero level for type)
    QAudioBuffer::U8S u8s;
    QAudioBuffer::S16S s16s;
    QAudioBuffer::F32S f32s;
    u8s.clear();
    s16s.clear();
    f32s.clear();

    QCOMPARE(u8s[QAudioFormat::FrontLeft], (unsigned char) 0x80);
    QCOMPARE(u8s[QAudioFormat::FrontRight], (unsigned char) 0x80);

    QCOMPARE(s16s[QAudioFormat::FrontLeft], (signed short) 0x0);
    QCOMPARE(s16s[QAudioFormat::FrontRight], (signed short) 0x0);

    QCOMPARE(f32s[QAudioFormat::FrontLeft], 0.0f);
    QCOMPARE(f32s[QAudioFormat::FrontRight], 0.0f);

    // Initialized
    QAudioBuffer::U8S u8s2{34, 145};
    QAudioBuffer::S16S s16s2{-10000, 346};
    QAudioBuffer::F32S f32s2{500.7f, -123.1f};

    QCOMPARE(u8s2[QAudioFormat::FrontLeft], (unsigned char) 34);
    QCOMPARE(u8s2[QAudioFormat::FrontRight], (unsigned char) 145);

    QCOMPARE(s16s2[QAudioFormat::FrontLeft], (signed short) -10000);
    QCOMPARE(s16s2[QAudioFormat::FrontRight], (signed short) 346);

    QCOMPARE(f32s2[QAudioFormat::FrontLeft], 500.7f);
    QCOMPARE(f32s2[QAudioFormat::FrontRight], -123.1f);

    // Assigned
    u8s = u8s2;
    s16s = s16s2;
    f32s = f32s2;

    QCOMPARE(u8s[QAudioFormat::FrontLeft], (unsigned char) 34);
    QCOMPARE(u8s[QAudioFormat::FrontRight], (unsigned char) 145);

    QCOMPARE(s16s[QAudioFormat::FrontLeft], (signed short) -10000);
    QCOMPARE(s16s[QAudioFormat::FrontRight], (signed short) 346);

    QCOMPARE(f32s[QAudioFormat::FrontLeft], 500.7f);
    QCOMPARE(f32s[QAudioFormat::FrontRight], -123.1f);

    // Cleared
    u8s.clear();
    s16s.clear();
    f32s.clear();

    QCOMPARE(u8s[QAudioFormat::FrontLeft], (unsigned char) 0x80);
    QCOMPARE(u8s[QAudioFormat::FrontRight], (unsigned char) 0x80);

    QCOMPARE(s16s[QAudioFormat::FrontLeft], (signed short) 0x0);
    QCOMPARE(s16s[QAudioFormat::FrontRight], (signed short) 0x0);

    QCOMPARE(f32s[QAudioFormat::FrontLeft], 0.0f);
    QCOMPARE(f32s[QAudioFormat::FrontRight], 0.0f);
}


QTEST_APPLESS_MAIN(tst_QAudioBuffer);

#include "tst_qaudiobuffer.moc"
