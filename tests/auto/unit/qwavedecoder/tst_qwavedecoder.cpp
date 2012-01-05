/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

//TESTED_COMPONENT=src/multimedia

#include <QtTest/QtTest>
#include <private/qwavedecoder_p.h>

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

#ifndef QTRY_COMPARE
#define QTRY_COMPARE(__expr, __expected) \
    do { \
        const int __step = 50; \
        const int __timeout = 1000; \
        if (!(__expr)) { \
            QTest::qWait(0); \
        } \
        for (int __i = 0; __i < __timeout && !((__expr) == (__expected)); __i+=__step) { \
            QTest::qWait(__step); \
        } \
        QCOMPARE(__expr, __expected); \
    } while (0)
#endif


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

void tst_QWaveDecoder::file_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<tst_QWaveDecoder::Corruption>("corruption");
    QTest::addColumn<int>("channels");
    QTest::addColumn<int>("samplesize");
    QTest::addColumn<int>("samplerate");
    QTest::addColumn<QAudioFormat::Endian>("byteorder");

    QTest::newRow("File is empty")  << QString("empty.wav") << tst_QWaveDecoder::NotAWav << -1 << -1 << -1 << QAudioFormat::LittleEndian;
    QTest::newRow("File is one byte")  << QString("onebyte.wav") << tst_QWaveDecoder::NotAWav << -1 << -1 << -1 << QAudioFormat::LittleEndian;
    QTest::newRow("File is not a wav(text)")  << QString("notawav.wav") << tst_QWaveDecoder::NotAWav << -1 << -1 << -1 << QAudioFormat::LittleEndian;
    QTest::newRow("Wav file has no sample data")  << QString("nosampledata.wav") << tst_QWaveDecoder::NoSampleData << -1 << -1 << -1 << QAudioFormat::LittleEndian;
    QTest::newRow("corrupt fmt chunk descriptor")  << QString("corrupt_fmtdesc_1_16_8000.le.wav") << tst_QWaveDecoder::FormatDescriptor << -1 << -1 << -1 << QAudioFormat::LittleEndian;
    QTest::newRow("corrupt fmt string")  << QString("corrupt_fmtstring_1_16_8000.le.wav") << tst_QWaveDecoder::FormatString << -1 << -1 << -1 << QAudioFormat::LittleEndian;
    QTest::newRow("corrupt data chunk descriptor")  << QString("corrupt_datadesc_1_16_8000.le.wav") << tst_QWaveDecoder::DataDescriptor << -1 << -1 << -1 << QAudioFormat::LittleEndian;

    QTest::newRow("File isawav_1_8_8000.wav") << QString("isawav_1_8_8000.wav")  << tst_QWaveDecoder::None << 1 << 8 << 8000 << QAudioFormat::LittleEndian;
    QTest::newRow("File isawav_1_8_44100.wav") << QString("isawav_1_8_44100.wav")  << tst_QWaveDecoder::None << 1 << 8 << 44100 << QAudioFormat::LittleEndian;
    QTest::newRow("File isawav_2_8_8000.wav") << QString("isawav_2_8_8000.wav")  << tst_QWaveDecoder::None << 2 << 8 << 8000 << QAudioFormat::LittleEndian;
    QTest::newRow("File isawav_2_8_44100.wav") << QString("isawav_2_8_44100.wav")  << tst_QWaveDecoder::None << 2 << 8 << 44100 << QAudioFormat::LittleEndian;

    QTest::newRow("File isawav_1_16_8000_le.wav") << QString("isawav_1_16_8000_le.wav")  << tst_QWaveDecoder::None << 1 << 16 << 8000 << QAudioFormat::LittleEndian;
    QTest::newRow("File isawav_1_16_44100_le.wav") << QString("isawav_1_16_44100_le.wav")  << tst_QWaveDecoder::None << 1 << 16 << 44100 << QAudioFormat::LittleEndian;
    QTest::newRow("File isawav_2_16_8000_be.wav") << QString("isawav_2_16_8000_be.wav")  << tst_QWaveDecoder::None << 2 << 16 << 8000 << QAudioFormat::BigEndian;
    QTest::newRow("File isawav_2_16_44100_be.wav") << QString("isawav_2_16_44100_be.wav")  << tst_QWaveDecoder::None << 2 << 16 << 44100 << QAudioFormat::BigEndian;

    // 32 bit waves are not supported
    QTest::newRow("File isawav_1_32_8000_le.wav") << QString("isawav_1_32_8000_le.wav")  << tst_QWaveDecoder::FormatDescriptor << 1 << 32 << 8000 << QAudioFormat::LittleEndian;
    QTest::newRow("File isawav_1_32_44100_le.wav") << QString("isawav_1_32_44100_le.wav")  << tst_QWaveDecoder::FormatDescriptor << 1 << 32 << 44100 << QAudioFormat::LittleEndian;
    QTest::newRow("File isawav_2_32_8000_be.wav") << QString("isawav_2_32_8000_be.wav")  << tst_QWaveDecoder::FormatDescriptor << 2 << 32 << 8000 << QAudioFormat::BigEndian;
    QTest::newRow("File isawav_2_32_44100_be.wav") << QString("isawav_2_32_44100_be.wav")  << tst_QWaveDecoder::FormatDescriptor << 2 << 32 << 44100 << QAudioFormat::BigEndian;
}

void tst_QWaveDecoder::file()
{
    QFETCH(QString, file);
    QFETCH(tst_QWaveDecoder::Corruption, corruption);
    QFETCH(int, channels);
    QFETCH(int, samplesize);
    QFETCH(int, samplerate);
    QFETCH(QAudioFormat::Endian, byteorder);

    QFile stream;
    stream.setFileName(QString("data/") + file);
    stream.open(QIODevice::ReadOnly);

    QVERIFY(stream.isOpen());

    QWaveDecoder waveDecoder(&stream);
    QSignalSpy validFormatSpy(&waveDecoder, SIGNAL(formatKnown()));
    QSignalSpy parsingErrorSpy(&waveDecoder, SIGNAL(parsingError()));

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
        QVERIFY(format.channels() == channels);
        QVERIFY(format.sampleSize() == samplesize);
        QVERIFY(format.sampleRate() == samplerate);
        if (format.sampleSize() != 8) {
            QVERIFY(format.byteOrder() == byteorder);
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
    QFETCH(QAudioFormat::Endian, byteorder);

    QFile stream;
    stream.setFileName(QString("data/") + file);
    stream.open(QIODevice::ReadOnly);

    QVERIFY(stream.isOpen());

    QNetworkAccessManager nam;

    QNetworkReply *reply = nam.get(QNetworkRequest(QUrl::fromLocalFile(QString::fromLatin1("data/") + file)));

    QWaveDecoder waveDecoder(reply);
    QSignalSpy validFormatSpy(&waveDecoder, SIGNAL(formatKnown()));
    QSignalSpy parsingErrorSpy(&waveDecoder, SIGNAL(parsingError()));

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
        QVERIFY(format.channels() == channels);
        QVERIFY(format.sampleSize() == samplesize);
        QVERIFY(format.sampleRate() == samplerate);
        if (format.sampleSize() != 8) {
            QVERIFY(format.byteOrder() == byteorder);
        }
    }

    delete reply;
}

void tst_QWaveDecoder::readAllAtOnce()
{
    QFile stream;
    stream.setFileName(QString("data/isawav_2_8_44100.wav"));
    stream.open(QIODevice::ReadOnly);

    QVERIFY(stream.isOpen());

    QWaveDecoder waveDecoder(&stream);
    QSignalSpy validFormatSpy(&waveDecoder, SIGNAL(formatKnown()));

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
    stream.setFileName(QString("data/isawav_2_8_44100.wav"));
    stream.open(QIODevice::ReadOnly);

    QVERIFY(stream.isOpen());

    QWaveDecoder waveDecoder(&stream);
    QSignalSpy validFormatSpy(&waveDecoder, SIGNAL(formatKnown()));

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

Q_DECLARE_METATYPE(tst_QWaveDecoder::Corruption)

QTEST_MAIN(tst_QWaveDecoder)

#include "tst_qwavedecoder.moc"

