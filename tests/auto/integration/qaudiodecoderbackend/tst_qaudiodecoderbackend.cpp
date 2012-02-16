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

 Since it relies on platform media framework and sound hardware
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
    void decoderTest();
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

void tst_QAudioDecoderBackend::decoderTest()
{
    QAudioDecoder d;
    QVERIFY(d.state() == QAudioDecoder::StoppedState);
    QVERIFY(d.bufferAvailable() == false);
    QCOMPARE(d.sourceFilename(), QString(""));

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
    QTRY_VERIFY(!stateSpy.isEmpty());
    QTRY_VERIFY(!readySpy.isEmpty());
    QTRY_VERIFY(!bufferChangedSpy.isEmpty());
    QVERIFY(d.bufferAvailable());

    bool ok;
    QAudioBuffer buffer = d.read(&ok);
    QVERIFY(ok);
    QVERIFY(buffer.isValid());
    QCOMPARE(buffer.format(), d.audioFormat());

    QVERIFY(errorSpy.isEmpty());

    d.stop();
    QTRY_COMPARE(d.state(), QAudioDecoder::StoppedState);
    QVERIFY(!d.bufferAvailable());
    readySpy.clear();
    bufferChangedSpy.clear();
    stateSpy.clear();

    // Test source device
    QFile file(fileInfo.absoluteFilePath());
    QVERIFY(file.open(QIODevice::ReadOnly));
    d.setSourceDevice(&file);

    // change output audio format
    QAudioFormat format;
    format.setChannels(1);
    format.setSampleSize(8);
    format.setFrequency(8000);
    format.setCodec("audio/pcm");
    format.setSampleType(QAudioFormat::SignedInt);
    d.setAudioFormat(format);

    d.start();
    QTRY_VERIFY(!stateSpy.isEmpty());
    QTRY_VERIFY(!readySpy.isEmpty());
    QTRY_VERIFY(!bufferChangedSpy.isEmpty());
    QVERIFY(d.bufferAvailable());

    buffer = d.read(&ok);
    QVERIFY(ok);
    QVERIFY(buffer.isValid());
    QCOMPARE(buffer.format(), d.audioFormat());

    QVERIFY(errorSpy.isEmpty());
}

QTEST_MAIN(tst_QAudioDecoderBackend)

#include "tst_qaudiodecoderbackend.moc"
