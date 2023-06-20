// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <QtCore/QString>
#include <QtTest/QtTest>

#include "qaudiodecoder.h"
#include "qmockaudiodecoder.h"
#include "qmockintegration.h"

class tst_QAudioDecoder : public QObject
{
    Q_OBJECT

public:
    tst_QAudioDecoder();

private Q_SLOTS:
    void ctors();
    void read();
    void stop();
    void format();
    void source();
    void readAll();
    void nullControl();

private:
    QMockIntegrationFactory mockIntegrationFactory;
};

tst_QAudioDecoder::tst_QAudioDecoder()
{
}

void tst_QAudioDecoder::ctors()
{
    QAudioDecoder d;
    QVERIFY(!d.isDecoding());
    QVERIFY(d.bufferAvailable() == false);
    QCOMPARE(d.source(), QString(""));

    d.setSource(QUrl());
    QVERIFY(!d.isDecoding());
    QVERIFY(d.bufferAvailable() == false);
    QCOMPARE(d.source(), QUrl());
}

void tst_QAudioDecoder::read()
{
    QAudioDecoder d;
    QVERIFY(!d.isDecoding());
    QVERIFY(d.bufferAvailable() == false);

    QSignalSpy readySpy(&d, SIGNAL(bufferReady()));
    QSignalSpy bufferChangedSpy(&d, SIGNAL(bufferAvailableChanged(bool)));
    QSignalSpy errorSpy(&d, SIGNAL(error(QAudioDecoder::Error)));

    // Starting with empty source == error
    d.start();

    QVERIFY(!d.isDecoding());
    QVERIFY(d.bufferAvailable() == false);

    QCOMPARE(readySpy.size(), 0);
    QCOMPARE(bufferChangedSpy.size(), 0);
    QCOMPARE(errorSpy.size(), 1);

    // Set the source to something
    d.setSource(QUrl::fromLocalFile("Blah"));
    QCOMPARE(d.source(), QUrl::fromLocalFile("Blah"));

    readySpy.clear();
    errorSpy.clear();
    bufferChangedSpy.clear();

    d.start();
    QVERIFY(d.isDecoding());
    QCOMPARE(d.bufferAvailable(), false); // not yet

    // Try to read
    QAudioBuffer b = d.read();
    QVERIFY(!b.isValid());

    // Read again with no parameter
    b = d.read();
    QVERIFY(!b.isValid());

    // Wait a while
    QTRY_VERIFY(d.bufferAvailable());

    QVERIFY(d.bufferAvailable());

    b = d.read();
    QVERIFY(b.format().isValid());
    QVERIFY(b.isValid());
    QVERIFY(b.format().channelCount() == 1);
    QVERIFY(b.sampleCount() == 4);

    QVERIFY(readySpy.size() >= 1);
    QVERIFY(errorSpy.size() == 0);

    if (d.bufferAvailable()) {
        QVERIFY(bufferChangedSpy.size() == 1);
    } else {
        QVERIFY(bufferChangedSpy.size() == 2);
    }
}

void tst_QAudioDecoder::stop()
{
    QAudioDecoder d;
    QVERIFY(!d.isDecoding());
    QVERIFY(d.bufferAvailable() == false);

    QSignalSpy readySpy(&d, SIGNAL(bufferReady()));
    QSignalSpy bufferChangedSpy(&d, SIGNAL(bufferAvailableChanged(bool)));
    QSignalSpy errorSpy(&d, SIGNAL(error(QAudioDecoder::Error)));

    // Starting with empty source == error
    d.start();

    QVERIFY(!d.isDecoding());
    QVERIFY(d.bufferAvailable() == false);

    QCOMPARE(readySpy.size(), 0);
    QCOMPARE(bufferChangedSpy.size(), 0);
    QCOMPARE(errorSpy.size(), 1);

    // Set the source to something
    d.setSource(QUrl::fromLocalFile("Blah"));
    QCOMPARE(d.source(), QUrl::fromLocalFile("Blah"));

    readySpy.clear();
    errorSpy.clear();
    bufferChangedSpy.clear();

    d.start();
    QVERIFY(d.isDecoding());
    QCOMPARE(d.bufferAvailable(), false); // not yet

    // Try to read
    QAudioBuffer b = d.read();
    QVERIFY(!b.isValid());

    // Read again with no parameter
    b = d.read();
    QVERIFY(!b.isValid());

    // Wait a while
    QTRY_VERIFY(d.bufferAvailable());

    QVERIFY(d.bufferAvailable());

    // Now stop
    d.stop();

    QVERIFY(!d.isDecoding());
    QVERIFY(d.bufferAvailable() == false);
}

void tst_QAudioDecoder::format()
{
    QAudioDecoder d;
    QVERIFY(!d.isDecoding());
    QVERIFY(d.bufferAvailable() == false);

    QSignalSpy readySpy(&d, SIGNAL(bufferReady()));
    QSignalSpy bufferChangedSpy(&d, SIGNAL(bufferAvailableChanged(bool)));
    QSignalSpy errorSpy(&d, SIGNAL(error(QAudioDecoder::Error)));

    // Set the source to something
    d.setSource(QUrl::fromLocalFile("Blah"));
    QCOMPARE(d.source(), QUrl::fromLocalFile("Blah"));

    readySpy.clear();
    errorSpy.clear();
    bufferChangedSpy.clear();

    d.start();
    QVERIFY(d.isDecoding());
    QCOMPARE(d.bufferAvailable(), false); // not yet

    // Try to read
    QAudioBuffer b = d.read();
    QVERIFY(!b.isValid());

    // Read again with no parameter
    b = d.read();
    QVERIFY(!b.isValid());

    // Wait a while
    QTRY_VERIFY(d.bufferAvailable());

    b = d.read();
    QVERIFY(b.format().isValid());
    QVERIFY(d.audioFormat() == b.format());

    // Setting format while decoding is forbidden
    QAudioFormat f(d.audioFormat());
    f.setChannelCount(2);

    d.setAudioFormat(f);
    QVERIFY(d.audioFormat() != f);
    QVERIFY(d.audioFormat() == b.format());

    // Now stop, and set something specific
    d.stop();
    d.setAudioFormat(f);
    QVERIFY(d.audioFormat() == f);

    // Decode again
    d.start();
    QTRY_VERIFY(d.bufferAvailable());

    b = d.read();
    QVERIFY(d.audioFormat() == f);
    QVERIFY(b.format() == f);
}

void tst_QAudioDecoder::source()
{
    QAudioDecoder d;

    QVERIFY(d.source().isEmpty());
    QVERIFY(d.sourceDevice() == nullptr);

    QFile f;
    d.setSourceDevice(&f);
    QVERIFY(d.source().isEmpty());
    QVERIFY(d.sourceDevice() == &f);

    d.setSource(QUrl::fromLocalFile("Foo"));
    QVERIFY(d.source() == QUrl::fromLocalFile("Foo"));
    QVERIFY(d.sourceDevice() == nullptr);

    d.setSourceDevice(nullptr);
    QVERIFY(d.source().isEmpty());
    QVERIFY(d.sourceDevice() == nullptr);

    d.setSource(QUrl::fromLocalFile("Foo"));
    QVERIFY(d.source() == QUrl::fromLocalFile("Foo"));
    QVERIFY(d.sourceDevice() == nullptr);

    d.setSource(QString());
    QVERIFY(d.source() == QString());
    QVERIFY(d.sourceDevice() == nullptr);
}

void tst_QAudioDecoder::readAll()
{
    QAudioDecoder d;
    d.setSource(QUrl::fromLocalFile("Foo"));
    QVERIFY(!d.isDecoding());

    QSignalSpy durationSpy(&d, SIGNAL(durationChanged(qint64)));
    QSignalSpy positionSpy(&d, SIGNAL(positionChanged(qint64)));
    QSignalSpy isDecodingSpy(&d, SIGNAL(isDecodingChanged(bool)));
    QSignalSpy finishedSpy(&d, SIGNAL(finished()));
    QSignalSpy bufferAvailableSpy(&d, SIGNAL(bufferAvailableChanged(bool)));
    d.start();
    int i = 0;
    forever {
        QVERIFY(d.isDecoding());
        QCOMPARE(isDecodingSpy.size(), 1);
        QCOMPARE(durationSpy.size(), 1);
        QVERIFY(finishedSpy.isEmpty());
        QTRY_VERIFY(bufferAvailableSpy.size() >= 1);
        if (d.bufferAvailable()) {
            QAudioBuffer b = d.read();
            QVERIFY(b.isValid());
            QCOMPARE(b.startTime() / 1000, d.position());
            QVERIFY(!positionSpy.isEmpty());
            QList<QVariant> arguments = positionSpy.takeLast();
            QCOMPARE(arguments.at(0).toLongLong(), b.startTime() / 1000);

            i++;
            if (i == MOCK_DECODER_MAX_BUFFERS) {
                QCOMPARE(finishedSpy.size(), 1);
                QCOMPARE(isDecodingSpy.size(), 2);
                QVERIFY(!d.isDecoding());
                QList<QVariant> arguments = isDecodingSpy.takeLast();
                QVERIFY(arguments.at(0).toBool() == false);
                QVERIFY(!d.bufferAvailable());
                QVERIFY(!bufferAvailableSpy.isEmpty());
                arguments = bufferAvailableSpy.takeLast();
                QVERIFY(arguments.at(0).toBool() == false);
                break;
            }
        } else
            QTest::qWait(30);
    }
}

void tst_QAudioDecoder::nullControl()
{
    QMockIntegration::instance()->setFlags(QMockIntegration::NoAudioDecoderInterface);
    QAudioDecoder d;

    QVERIFY(d.error() == QAudioDecoder::NotSupportedError);
    QVERIFY(!d.errorString().isEmpty());

    QVERIFY(!d.isDecoding());

    QVERIFY(d.source().isEmpty());
    d.setSource(QUrl::fromLocalFile("test"));
    QVERIFY(d.source().isEmpty());

    QFile f;
    QVERIFY(d.sourceDevice() == nullptr);
    d.setSourceDevice(&f);
    QVERIFY(d.sourceDevice() == nullptr);

    QAudioFormat format;
    format.setChannelCount(2);
    QVERIFY(!d.audioFormat().isValid());
    d.setAudioFormat(format);
    QVERIFY(!d.audioFormat().isValid());

    QVERIFY(!d.read().isValid());
    QVERIFY(!d.bufferAvailable());

    QVERIFY(d.position() == -1);
    QVERIFY(d.duration() == -1);

    d.start();
    QVERIFY(d.error() == QAudioDecoder::NotSupportedError);
    QVERIFY(!d.errorString().isEmpty());
    QVERIFY(!d.isDecoding());
    d.stop();
}

QTEST_MAIN(tst_QAudioDecoder)

#include "tst_qaudiodecoder.moc"
