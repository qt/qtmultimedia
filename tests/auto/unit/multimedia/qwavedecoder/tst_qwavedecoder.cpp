// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QtTest/qsignalspy.h>
#include <qwavedecoder.h>
#include <private/qsinewavevalidator_p.h>

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

class tst_QWaveDecoder : public QObject
{
    Q_OBJECT
public:
    enum Corruption {
        None = 1,
        NotAWav = 2,
        NoSampleData = 4,
        FormatDescriptor = 8,
        FormatString = 16,
        DataDescriptor = 32
    };

public slots:

    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

private slots:

    void file_data();
    void file();

    void http_data() {file_data();}
    void http();

    void readAllAtOnce();
    void readPerByte();
};

void tst_QWaveDecoder::init()
{
}

void tst_QWaveDecoder::cleanup()
{
}

void tst_QWaveDecoder::initTestCase()
{
}

void tst_QWaveDecoder::cleanupTestCase()
{
}

static QString testFilePath(const char *filename)
{
    QString path = QStringLiteral("data/%1").arg(filename);
    return QFINDTESTDATA(path);
}

void tst_QWaveDecoder::file_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<tst_QWaveDecoder::Corruption>("corruption");
    QTest::addColumn<int>("channels");
    QTest::addColumn<int>("samplesize");
    QTest::addColumn<int>("samplerate");
    QTest::addColumn<bool>("validateContent");

    // clang-format off
    QTest::newRow("File is empty")  << testFilePath("empty.wav") << tst_QWaveDecoder::NotAWav << -1 << -1 << -1 << true;
    QTest::newRow("File is one byte")  << testFilePath("onebyte.wav") << tst_QWaveDecoder::NotAWav << -1 << -1 << -1 << true;
    QTest::newRow("File is not a wav(text)")  << testFilePath("notawav.wav") << tst_QWaveDecoder::NotAWav << -1 << -1 << -1 << true;
    QTest::newRow("Wav file has no sample data")  << testFilePath("nosampledata.wav") << tst_QWaveDecoder::NoSampleData << -1 << -1 << -1 << true;
    QTest::newRow("corrupt fmt chunk descriptor")  << testFilePath("corrupt_fmtdesc_1_16_8000.le.wav") << tst_QWaveDecoder::FormatDescriptor << -1 << -1 << -1 << true;
    QTest::newRow("corrupt fmt string")  << testFilePath("corrupt_fmtstring_1_16_8000.le.wav") << tst_QWaveDecoder::FormatString << -1 << -1 << -1 << true;
    QTest::newRow("corrupt data chunk descriptor")  << testFilePath("corrupt_datadesc_1_16_8000.le.wav") << tst_QWaveDecoder::DataDescriptor << -1 << -1 << -1 << true;

    QTest::newRow("File isawav_1_8_8000.wav") << testFilePath("isawav_1_8_8000.wav")  << tst_QWaveDecoder::None << 1 << 8 << 8000 << true;
    QTest::newRow("File isawav_1_8_44100.wav") << testFilePath("isawav_1_8_44100.wav")  << tst_QWaveDecoder::None << 1 << 8 << 44100 << true;
    QTest::newRow("File isawav_2_8_8000.wav") << testFilePath("isawav_2_8_8000.wav")  << tst_QWaveDecoder::None << 2 << 8 << 8000 << true;
    QTest::newRow("File isawav_2_8_44100.wav") << testFilePath("isawav_2_8_44100.wav")  << tst_QWaveDecoder::None << 2 << 8 << 44100 << true;

    QTest::newRow("File isawav_1_16_8000_le.wav") << testFilePath("isawav_1_16_8000_le.wav")  << tst_QWaveDecoder::None << 1 << 16 << 8000 << true;
    QTest::newRow("File isawav_1_16_44100_le.wav") << testFilePath("isawav_1_16_44100_le.wav")  << tst_QWaveDecoder::None << 1 << 16 << 44100 << true;
    QTest::newRow("File isawav_2_16_8000_be.wav") << testFilePath("isawav_2_16_8000_be.wav")  << tst_QWaveDecoder::None << 2 << 16 << 8000 << true;
    QTest::newRow("File isawav_2_16_44100_be.wav") << testFilePath("isawav_2_16_44100_be.wav")  << tst_QWaveDecoder::None << 2 << 16 << 44100 << true;
    // The next file has extra data in the wave header.
    QTest::newRow("File isawav_1_16_44100_le_2.wav") << testFilePath("isawav_1_16_44100_le_2.wav")  << tst_QWaveDecoder::None << 1 << 16 << 44100 << false;
    // The next file has embedded bext chunk with odd payload (QTBUG-122193)
    QTest::newRow("File isawav_1_8_8000_odd_bext.wav") << testFilePath("isawav_1_8_8000_odd_bext.wav")  << tst_QWaveDecoder::None << 1 << 8 << 8000 << false;
    // The next file has embedded bext chunk with even payload
    QTest::newRow("File isawav_1_8_8000_even_bext.wav") << testFilePath("isawav_1_8_8000_even_bext.wav")  << tst_QWaveDecoder::None << 1 << 8 << 8000 << false;

    // 24bit wav are not supported
    QTest::newRow("File isawav_1_24_8000_le.wav")  << testFilePath("isawav_1_24_8000_le.wav")  << tst_QWaveDecoder::FormatDescriptor << 1 << 24 << 8000 << true;
    QTest::newRow("File isawav_1_24_44100_le.wav") << testFilePath("isawav_1_24_44100_le.wav") << tst_QWaveDecoder::FormatDescriptor << 1 << 24 << 44100 << true;
    QTest::newRow("File isawav_2_24_8000_be.wav")  << testFilePath("isawav_2_24_8000_be.wav")  << tst_QWaveDecoder::FormatDescriptor << 2 << 24 << 8000 << true;
    QTest::newRow("File isawav_2_24_44100_be.wav") << testFilePath("isawav_2_24_44100_be.wav") << tst_QWaveDecoder::FormatDescriptor << 2 << 24 << 44100 << true;

    // 32 bit waves are not supported
    QTest::newRow("File isawav_1_32_8000_le.wav") << testFilePath("isawav_1_32_8000_le.wav")  << tst_QWaveDecoder::FormatDescriptor << 1 << 32 << 8000 << true;
    QTest::newRow("File isawav_1_32_44100_le.wav") << testFilePath("isawav_1_32_44100_le.wav")  << tst_QWaveDecoder::FormatDescriptor << 1 << 32 << 44100 << true;
    QTest::newRow("File isawav_2_32_8000_be.wav") << testFilePath("isawav_2_32_8000_be.wav")  << tst_QWaveDecoder::FormatDescriptor << 2 << 32 << 8000 << true;
    QTest::newRow("File isawav_2_32_44100_be.wav") << testFilePath("isawav_2_32_44100_be.wav")  << tst_QWaveDecoder::FormatDescriptor << 2 << 32 << 44100 << true;

    // f32 waves are not supported
    QTest::newRow("File isawav_1_f32_8000_le.wav") << testFilePath("isawav_1_f32_8000_le.wav")  << tst_QWaveDecoder::FormatDescriptor << 1 << 32 << 8000 << true;
    QTest::newRow("File isawav_1_f32_44100_le.wav") << testFilePath("isawav_1_f32_44100_le.wav")  << tst_QWaveDecoder::FormatDescriptor << 1 << 32 << 44100 << true;
    QTest::newRow("File isawav_2_f32_8000_be.wav") << testFilePath("isawav_2_f32_8000_be.wav")  << tst_QWaveDecoder::FormatDescriptor << 2 << 32 << 8000 << true;
    QTest::newRow("File isawav_2_f32_44100_be.wav") << testFilePath("isawav_2_f32_44100_be.wav")  << tst_QWaveDecoder::FormatDescriptor << 2 << 32 << 44100 << true;

    // clang-format on
}

void tst_QWaveDecoder::file()
{
    QFETCH(QString, file);
    QFETCH(tst_QWaveDecoder::Corruption, corruption);
    QFETCH(int, channels);
    QFETCH(int, samplesize);
    QFETCH(int, samplerate);
    QFETCH(bool, validateContent);

    QFile stream;
    stream.setFileName(file);
    QVERIFY(stream.open(QIODevice::ReadOnly));

    QVERIFY(stream.isOpen());

    QWaveDecoder waveDecoder(&stream);
    QSignalSpy validFormatSpy(&waveDecoder, &QWaveDecoder::formatKnown);
    QSignalSpy parsingErrorSpy(&waveDecoder, &QWaveDecoder::parsingError);

    QVERIFY(waveDecoder.open(QIODeviceBase::ReadOnly));

    if (corruption == NotAWav) {
        QSKIP("Not all failures detected correctly yet");
        QTRY_COMPARE(parsingErrorSpy.count(), 1);
        QCOMPARE(validFormatSpy.count(), 0);
    } else if (corruption == NoSampleData) {
        QTRY_COMPARE(validFormatSpy.count(), 1);
        QCOMPARE(parsingErrorSpy.count(), 0);
        QVERIFY(waveDecoder.audioFormat().isValid());
        QVERIFY(waveDecoder.size() == 0);
        QVERIFY(waveDecoder.duration() == 0);
    } else if (corruption == FormatDescriptor) {
        QTRY_COMPARE(parsingErrorSpy.count(), 1);
        QCOMPARE(validFormatSpy.count(), 0);
    } else if (corruption == FormatString) {
        QTRY_COMPARE(parsingErrorSpy.count(), 1);
        QCOMPARE(validFormatSpy.count(), 0);
        QVERIFY(!waveDecoder.audioFormat().isValid());
    } else if (corruption == DataDescriptor) {
        QTRY_COMPARE(parsingErrorSpy.count(), 1);
        QCOMPARE(validFormatSpy.count(), 0);
        QVERIFY(waveDecoder.size() == 0);
    } else if (corruption == None) {
        QTRY_COMPARE(validFormatSpy.count(), 1);
        QCOMPARE(parsingErrorSpy.count(), 0);
        QVERIFY(waveDecoder.audioFormat().isValid());
        QVERIFY(waveDecoder.size() > 0);
        QVERIFY(waveDecoder.duration() == 250);
        QAudioFormat format = waveDecoder.audioFormat();
        QVERIFY(format.isValid());
        QVERIFY(format.channelCount() == channels);
        QCOMPARE(format.bytesPerSample() * 8, samplesize);
        QVERIFY(format.sampleRate() == samplerate);

        std::array<QSineWaveValidator, 2> validators{
            QSineWaveValidator(220, format.sampleRate()),
            QSineWaveValidator(220, format.sampleRate()),
        };

        auto expectedNumberOfFrames = format.sampleRate() / 4; // 250ms
        for (int frame = 0; frame != expectedNumberOfFrames; ++frame) {
            for (int channel = 0; channel != format.channelCount(); ++channel) {
                QByteArray array = waveDecoder.read(format.bytesPerSample());
                float sample = format.normalizedSampleValue(array.constData());
                validators[channel].feedSample(sample);
            }
        }

        if (validateContent) {
            QCOMPARE_GT(validators[0].peak(), 0.05);
            QCOMPARE_LE(validators[0].peak(), 1);
            QCOMPARE_LT(validators[0].notchPeak(), 0.05);
            if (format.channelCount() == 2) {
                QCOMPARE_GT(validators[1].peak(), 0.05);
                QCOMPARE_LE(validators[1].peak(), 1);
                QCOMPARE_LT(validators[1].notchPeak(), 0.05);
            }
        }
    }

    stream.close();
}

void tst_QWaveDecoder::http()
{
    QFETCH(QString, file);
    QFETCH(tst_QWaveDecoder::Corruption, corruption);
    QFETCH(int, channels);
    QFETCH(int, samplesize);
    QFETCH(int, samplerate);

    QFile stream;
    stream.setFileName(file);
    QVERIFY(stream.open(QIODevice::ReadOnly));

    QVERIFY(stream.isOpen());

    QNetworkAccessManager nam;

    QNetworkReply *reply = nam.get(QNetworkRequest(QUrl::fromLocalFile(file)));

    QWaveDecoder waveDecoder(reply);
    QSignalSpy validFormatSpy(&waveDecoder, &QWaveDecoder::formatKnown);
    QSignalSpy parsingErrorSpy(&waveDecoder, &QWaveDecoder::parsingError);

    QVERIFY(waveDecoder.open(QIODeviceBase::ReadOnly));

    if (corruption == NotAWav) {
        QSKIP("Not all failures detected correctly yet");
        QTRY_COMPARE(parsingErrorSpy.count(), 1);
        QCOMPARE(validFormatSpy.count(), 0);
    } else if (corruption == NoSampleData) {
        QTRY_COMPARE(validFormatSpy.count(), 1);
        QCOMPARE(parsingErrorSpy.count(), 0);
        QVERIFY(waveDecoder.audioFormat().isValid());
        QVERIFY(waveDecoder.size() == 0);
        QVERIFY(waveDecoder.duration() == 0);
    } else if (corruption == FormatDescriptor) {
        QTRY_COMPARE(parsingErrorSpy.count(), 1);
        QCOMPARE(validFormatSpy.count(), 0);
    } else if (corruption == FormatString) {
        QTRY_COMPARE(parsingErrorSpy.count(), 1);
        QCOMPARE(validFormatSpy.count(), 0);
        QVERIFY(!waveDecoder.audioFormat().isValid());
    } else if (corruption == DataDescriptor) {
        QTRY_COMPARE(parsingErrorSpy.count(), 1);
        QCOMPARE(validFormatSpy.count(), 0);
        QVERIFY(waveDecoder.size() == 0);
    } else if (corruption == None) {
        QTRY_COMPARE(validFormatSpy.count(), 1);
        QCOMPARE(parsingErrorSpy.count(), 0);
        QVERIFY(waveDecoder.audioFormat().isValid());
        QVERIFY(waveDecoder.size() > 0);
        QVERIFY(waveDecoder.duration() == 250);
        QAudioFormat format = waveDecoder.audioFormat();
        QVERIFY(format.isValid());
        QVERIFY(format.channelCount() == channels);
        QCOMPARE(format.bytesPerSample() * 8, samplesize);
        QVERIFY(format.sampleRate() == samplerate);
    }

    delete reply;
}

void tst_QWaveDecoder::readAllAtOnce()
{
    QFile stream;
    stream.setFileName(testFilePath("isawav_2_8_44100.wav"));
    QVERIFY(stream.open(QIODevice::ReadOnly));

    QVERIFY(stream.isOpen());

    QWaveDecoder waveDecoder(&stream);
    QSignalSpy validFormatSpy(&waveDecoder, &QWaveDecoder::formatKnown);

    QVERIFY(waveDecoder.open(QIODeviceBase::ReadOnly));

    QTRY_COMPARE(validFormatSpy.count(), 1);
    QVERIFY(waveDecoder.size() > 0);

    QByteArray buffer;
    buffer.resize(waveDecoder.size());

    qint64 readSize = waveDecoder.read(buffer.data(), waveDecoder.size());
    QVERIFY(readSize == waveDecoder.size());

    readSize = waveDecoder.read(buffer.data(), 1);
    QVERIFY(readSize == 0);

    stream.close();
}

void tst_QWaveDecoder::readPerByte()
{
    QFile stream;
    stream.setFileName(testFilePath("isawav_2_8_44100.wav"));
    QVERIFY(stream.open(QIODevice::ReadOnly));

    QVERIFY(stream.isOpen());

    QWaveDecoder waveDecoder(&stream);
    QSignalSpy validFormatSpy(&waveDecoder, &QWaveDecoder::formatKnown);

    QVERIFY(waveDecoder.open(QIODeviceBase::ReadOnly));

    QTRY_COMPARE(validFormatSpy.count(), 1);
    QVERIFY(waveDecoder.size() > 0);

    qint64 readSize = 0;
    char buf;
    for (int ii = 0; ii < waveDecoder.size(); ++ii)
        readSize += waveDecoder.read(&buf, 1);
    QVERIFY(readSize == waveDecoder.size());
    QVERIFY(waveDecoder.read(&buf,1) == 0);

    stream.close();
}

QTEST_MAIN(tst_QWaveDecoder)

#include "tst_qwavedecoder.moc"

