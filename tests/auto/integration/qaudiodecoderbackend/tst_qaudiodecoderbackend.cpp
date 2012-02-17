/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
#include <QDebug>
#include "qaudiodecoder_p.h"

#define TEST_FILE_NAME "testdata/test.wav"

QT_USE_NAMESPACE

/*
 This is the backend conformance test.

 Since it relies on platform media framework
 it may be less stable.
*/

class tst_QAudioDecoderBackend : public QObject
{
    Q_OBJECT
public slots:
    void init();
    void cleanup();
    void initTestCase();

private slots:
    void fileTest();
    void deviceTest();
};

void tst_QAudioDecoderBackend::init()
{
}

void tst_QAudioDecoderBackend::initTestCase()
{
}

void tst_QAudioDecoderBackend::cleanup()
{
}

void tst_QAudioDecoderBackend::fileTest()
{
    QAudioDecoder d;
    bool ok;
    QAudioBuffer buffer;
    quint64 duration = 0;
    int byteCount = 0;
    int sampleCount = 0;

    QVERIFY(d.state() == QAudioDecoder::StoppedState);
    QVERIFY(d.bufferAvailable() == false);
    QCOMPARE(d.sourceFilename(), QString(""));
    QVERIFY(d.audioFormat() == QAudioFormat());

    // Test local file
    QFileInfo fileInfo(QFINDTESTDATA(TEST_FILE_NAME));
    d.setSourceFilename(fileInfo.absoluteFilePath());
    QVERIFY(d.state() == QAudioDecoder::StoppedState);
    QVERIFY(!d.bufferAvailable());
    QCOMPARE(d.sourceFilename(), fileInfo.absoluteFilePath());

    QSignalSpy readySpy(&d, SIGNAL(bufferReady()));
    QSignalSpy bufferChangedSpy(&d, SIGNAL(bufferAvailableChanged(bool)));
    QSignalSpy errorSpy(&d, SIGNAL(error(QAudioDecoder::Error)));
    QSignalSpy stateSpy(&d, SIGNAL(stateChanged(QAudioDecoder::State)));

    d.start();
    QTRY_VERIFY(d.state() == QAudioDecoder::DecodingState);
    QTRY_VERIFY(!stateSpy.isEmpty());
    QTRY_VERIFY(!readySpy.isEmpty());
    QTRY_VERIFY(!bufferChangedSpy.isEmpty());
    QVERIFY(d.bufferAvailable());

    buffer = d.read(&ok);
    QVERIFY(ok);
    QVERIFY(buffer.isValid());

    // Test file is 44.1K 16bit mono, 44094 samples
    QCOMPARE(buffer.format().channelCount(), 1);
    QCOMPARE(buffer.format().sampleRate(), 44100);
    QCOMPARE(buffer.format().sampleSize(), 16);
    QCOMPARE(buffer.format().sampleType(), QAudioFormat::SignedInt);
    QCOMPARE(buffer.format().codec(), QString("audio/pcm"));
    QCOMPARE(buffer.byteCount(), buffer.sampleCount() * 2); // 16bit mono

    // The decoder should still have no format set
    QVERIFY(d.audioFormat() == QAudioFormat());

    QVERIFY(errorSpy.isEmpty());

    duration += buffer.duration();
    sampleCount += buffer.sampleCount();
    byteCount += buffer.byteCount();

    // Now drain the decoder
    if (sampleCount < 44094) {
        QTRY_COMPARE(d.bufferAvailable(), true);
    }

    while (d.bufferAvailable()) {
        buffer = d.read(&ok);
        QVERIFY(ok);
        QVERIFY(buffer.isValid());
        duration += buffer.duration();
        sampleCount += buffer.sampleCount();
        byteCount += buffer.byteCount();

        if (sampleCount < 44094) {
            QTRY_COMPARE(d.bufferAvailable(), true);
        }
    }

    // Make sure the duration is roughly correct (+/- 20ms)
    QCOMPARE(sampleCount, 44094);
    QCOMPARE(byteCount, 44094 * 2);
    QVERIFY(qAbs(qint64(duration) - 1000000) < 20000);

    d.stop();
    QTRY_COMPARE(d.state(), QAudioDecoder::StoppedState);
    QVERIFY(!d.bufferAvailable());
    readySpy.clear();
    bufferChangedSpy.clear();
    stateSpy.clear();

    // change output audio format
    QAudioFormat format;
    format.setChannels(2);
    format.setSampleSize(8);
    format.setFrequency(11050);
    format.setCodec("audio/pcm");
    format.setSampleType(QAudioFormat::SignedInt);

    d.setAudioFormat(format);

    // We expect 1 second still, at 11050 * 2 samples == 22k samples.
    // (at 1 byte/sample -> 22kb)

    // Make sure it stuck
    QVERIFY(d.audioFormat() == format);

    duration = 0;
    sampleCount = 0;
    byteCount = 0;

    d.start();
    QTRY_VERIFY(d.state() == QAudioDecoder::DecodingState);
    QTRY_VERIFY(!stateSpy.isEmpty());
    QTRY_VERIFY(!readySpy.isEmpty());
    QTRY_VERIFY(!bufferChangedSpy.isEmpty());
    QVERIFY(d.bufferAvailable());

    buffer = d.read(&ok);
    QVERIFY(ok);
    QVERIFY(buffer.isValid());
    // See if we got the right format
    QVERIFY(buffer.format() == format);

    // The decoder should still have the same format
    QVERIFY(d.audioFormat() == format);

    QVERIFY(errorSpy.isEmpty());

    duration += buffer.duration();
    sampleCount += buffer.sampleCount();
    byteCount += buffer.byteCount();

    // Now drain the decoder
    if (duration < 998000) {
        QTRY_COMPARE(d.bufferAvailable(), true);
    }

    while (d.bufferAvailable()) {
        buffer = d.read(&ok);
        QVERIFY(ok);
        QVERIFY(buffer.isValid());
        duration += buffer.duration();
        sampleCount += buffer.sampleCount();
        byteCount += buffer.byteCount();

        if (duration < 998000) {
            QTRY_COMPARE(d.bufferAvailable(), true);
        }
    }

    // Resampling might end up with fewer or more samples
    // so be a bit sloppy
    QVERIFY(qAbs(sampleCount - 22047) < 100);
    QVERIFY(qAbs(byteCount - 22047) < 100);
    QVERIFY(qAbs(qint64(duration) - 1000000) < 20000);

    d.stop();
    QTRY_COMPARE(d.state(), QAudioDecoder::StoppedState);
    QVERIFY(!d.bufferAvailable());
}

void tst_QAudioDecoderBackend::deviceTest()
{
    QAudioDecoder d;
    bool ok;
    QAudioBuffer buffer;
    quint64 duration = 0;
    int sampleCount = 0;

    QSignalSpy readySpy(&d, SIGNAL(bufferReady()));
    QSignalSpy bufferChangedSpy(&d, SIGNAL(bufferAvailableChanged(bool)));
    QSignalSpy errorSpy(&d, SIGNAL(error(QAudioDecoder::Error)));
    QSignalSpy stateSpy(&d, SIGNAL(stateChanged(QAudioDecoder::State)));

    QVERIFY(d.state() == QAudioDecoder::StoppedState);
    QVERIFY(d.bufferAvailable() == false);
    QCOMPARE(d.sourceFilename(), QString(""));
    QVERIFY(d.audioFormat() == QAudioFormat());

    QFileInfo fileInfo(QFINDTESTDATA(TEST_FILE_NAME));
    QFile file(fileInfo.absoluteFilePath());
    QVERIFY(file.open(QIODevice::ReadOnly));
    d.setSourceDevice(&file);

    QVERIFY(d.sourceDevice() == &file);
    QVERIFY(d.sourceFilename().isEmpty());

    // We haven't set the format yet
    QVERIFY(d.audioFormat() == QAudioFormat());

    d.start();
    QTRY_VERIFY(d.state() == QAudioDecoder::DecodingState);
    QTRY_VERIFY(!stateSpy.isEmpty());
    QTRY_VERIFY(!readySpy.isEmpty());
    QTRY_VERIFY(!bufferChangedSpy.isEmpty());
    QVERIFY(d.bufferAvailable());

    buffer = d.read(&ok);
    QVERIFY(ok);
    QVERIFY(buffer.isValid());

    // Test file is 44.1K 16bit mono
    QCOMPARE(buffer.format().channelCount(), 1);
    QCOMPARE(buffer.format().sampleRate(), 44100);
    QCOMPARE(buffer.format().sampleSize(), 16);
    QCOMPARE(buffer.format().sampleType(), QAudioFormat::SignedInt);
    QCOMPARE(buffer.format().codec(), QString("audio/pcm"));

    QVERIFY(errorSpy.isEmpty());

    duration += buffer.duration();
    sampleCount += buffer.sampleCount();

    // Now drain the decoder
    if (sampleCount < 44094) {
        QTRY_COMPARE(d.bufferAvailable(), true);
    }

    while (d.bufferAvailable()) {
        buffer = d.read(&ok);
        QVERIFY(ok);
        QVERIFY(buffer.isValid());
        duration += buffer.duration();
        sampleCount += buffer.sampleCount();
        if (sampleCount < 44094) {
            QTRY_COMPARE(d.bufferAvailable(), true);
        }
    }

    // Make sure the duration is roughly correct (+/- 20ms)
    QCOMPARE(sampleCount, 44094);
    QVERIFY(qAbs(qint64(duration) - 1000000) < 20000);

    d.stop();
    QTRY_COMPARE(d.state(), QAudioDecoder::StoppedState);
    QVERIFY(!d.bufferAvailable());
    readySpy.clear();
    bufferChangedSpy.clear();
    stateSpy.clear();

    // Now try changing formats
    QAudioFormat format;
    format.setChannels(2);
    format.setSampleSize(8);
    format.setFrequency(8000);
    format.setCodec("audio/pcm");
    format.setSampleType(QAudioFormat::SignedInt);

    d.setAudioFormat(format);

    // Make sure it stuck
    QVERIFY(d.audioFormat() == format);

    d.start();
    QTRY_VERIFY(d.state() == QAudioDecoder::DecodingState);
    QTRY_VERIFY(!stateSpy.isEmpty());
    QTRY_VERIFY(!readySpy.isEmpty());
    QTRY_VERIFY(!bufferChangedSpy.isEmpty());
    QVERIFY(d.bufferAvailable());

    buffer = d.read(&ok);
    QVERIFY(ok);
    QVERIFY(buffer.isValid());
    // See if we got the right format
    QVERIFY(buffer.format() == format);

    // The decoder should still have the same format
    QVERIFY(d.audioFormat() == format);

    QVERIFY(errorSpy.isEmpty());

    d.stop();
    QTRY_COMPARE(d.state(), QAudioDecoder::StoppedState);
    QVERIFY(!d.bufferAvailable());

}

QTEST_MAIN(tst_QAudioDecoderBackend)

#include "tst_qaudiodecoderbackend.moc"
