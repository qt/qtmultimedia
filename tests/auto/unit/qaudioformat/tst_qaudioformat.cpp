/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
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


#include <QtTest/QtTest>
#include <QtCore/qlocale.h>
#include <qaudioformat.h>

#include <QStringList>
#include <QList>

//TESTED_COMPONENT=src/multimedia

class tst_QAudioFormat : public QObject
{
    Q_OBJECT

public:
    tst_QAudioFormat(QObject* parent=0) : QObject(parent) {}

private slots:
    void checkNull();
    void checkFrequency();
    void checkSampleSize();
    void checkCodec();
    void checkByteOrder();
    void checkSampleType();
    void checkEquality();
    void checkAssignment();
    void checkSampleRate();
    void checkChannelCount();

    void debugOperator();
    void debugOperator_data();
};

void tst_QAudioFormat::checkNull()
{
    // Default constructed QAudioFormat is invalid.
    QAudioFormat audioFormat0;
    QVERIFY(!audioFormat0.isValid());

    // validity is transferred
    QAudioFormat audioFormat1(audioFormat0);
    QVERIFY(!audioFormat1.isValid());

    audioFormat0.setFrequency(44100);
    audioFormat0.setChannels(2);
    audioFormat0.setSampleSize(16);
    audioFormat0.setCodec("audio/pcm");
    audioFormat0.setSampleType(QAudioFormat::SignedInt);
    QVERIFY(audioFormat0.isValid());
}

void tst_QAudioFormat::checkFrequency()
{
    QAudioFormat audioFormat;
    audioFormat.setFrequency(44100);
    QVERIFY(audioFormat.frequency() == 44100);
}

void tst_QAudioFormat::checkSampleSize()
{
    QAudioFormat audioFormat;
    audioFormat.setSampleSize(16);
    QVERIFY(audioFormat.sampleSize() == 16);
}

void tst_QAudioFormat::checkCodec()
{
    QAudioFormat audioFormat;
    audioFormat.setCodec(QString::fromLatin1("audio/pcm"));
    QVERIFY(audioFormat.codec() == QString::fromLatin1("audio/pcm"));
}

void tst_QAudioFormat::checkByteOrder()
{
    QAudioFormat audioFormat;
    audioFormat.setByteOrder(QAudioFormat::LittleEndian);
    QVERIFY(audioFormat.byteOrder() == QAudioFormat::LittleEndian);

    QTest::ignoreMessage(QtDebugMsg, "LittleEndian");
    qDebug() << QAudioFormat::LittleEndian;

    audioFormat.setByteOrder(QAudioFormat::BigEndian);
    QVERIFY(audioFormat.byteOrder() == QAudioFormat::BigEndian);

    QTest::ignoreMessage(QtDebugMsg, "BigEndian");
    qDebug() << QAudioFormat::BigEndian;
}

void tst_QAudioFormat::checkSampleType()
{
    QAudioFormat audioFormat;
    audioFormat.setSampleType(QAudioFormat::SignedInt);
    QVERIFY(audioFormat.sampleType() == QAudioFormat::SignedInt);
    QTest::ignoreMessage(QtDebugMsg, "SignedInt");
    qDebug() << QAudioFormat::SignedInt;

    audioFormat.setSampleType(QAudioFormat::Unknown);
    QVERIFY(audioFormat.sampleType() == QAudioFormat::Unknown);
    QTest::ignoreMessage(QtDebugMsg, "Unknown");
    qDebug() << QAudioFormat::Unknown;

    audioFormat.setSampleType(QAudioFormat::UnSignedInt);
    QVERIFY(audioFormat.sampleType() == QAudioFormat::UnSignedInt);
    QTest::ignoreMessage(QtDebugMsg, "UnSignedInt");
    qDebug() << QAudioFormat::UnSignedInt;

    audioFormat.setSampleType(QAudioFormat::Float);
    QVERIFY(audioFormat.sampleType() == QAudioFormat::Float);
    QTest::ignoreMessage(QtDebugMsg, "Float");
    qDebug() << QAudioFormat::Float;
}

void tst_QAudioFormat::checkEquality()
{
    QAudioFormat audioFormat0;
    QAudioFormat audioFormat1;

    // Null formats are equivalent
    QVERIFY(audioFormat0 == audioFormat1);
    QVERIFY(!(audioFormat0 != audioFormat1));

    // on filled formats
    audioFormat0.setFrequency(8000);
    audioFormat0.setChannels(1);
    audioFormat0.setSampleSize(8);
    audioFormat0.setCodec("audio/pcm");
    audioFormat0.setByteOrder(QAudioFormat::LittleEndian);
    audioFormat0.setSampleType(QAudioFormat::UnSignedInt);

    audioFormat1.setFrequency(8000);
    audioFormat1.setChannels(1);
    audioFormat1.setSampleSize(8);
    audioFormat1.setCodec("audio/pcm");
    audioFormat1.setByteOrder(QAudioFormat::LittleEndian);
    audioFormat1.setSampleType(QAudioFormat::UnSignedInt);

    QVERIFY(audioFormat0 == audioFormat1);
    QVERIFY(!(audioFormat0 != audioFormat1));

    audioFormat0.setFrequency(44100);
    QVERIFY(audioFormat0 != audioFormat1);
    QVERIFY(!(audioFormat0 == audioFormat1));
}

void tst_QAudioFormat::checkAssignment()
{
    QAudioFormat audioFormat0;
    QAudioFormat audioFormat1;

    audioFormat0.setFrequency(8000);
    audioFormat0.setChannels(1);
    audioFormat0.setSampleSize(8);
    audioFormat0.setCodec("audio/pcm");
    audioFormat0.setByteOrder(QAudioFormat::LittleEndian);
    audioFormat0.setSampleType(QAudioFormat::UnSignedInt);

    audioFormat1 = audioFormat0;
    QVERIFY(audioFormat1 == audioFormat0);

    QAudioFormat audioFormat2(audioFormat0);
    QVERIFY(audioFormat2 == audioFormat0);
}

/* sampleRate() API property test. */
void tst_QAudioFormat::checkSampleRate()
{
    QAudioFormat audioFormat;
    QVERIFY(audioFormat.sampleRate() == -1);

    audioFormat.setSampleRate(123);
    QVERIFY(audioFormat.sampleRate() == 123);
}

/* channelCount() API property test. */
void tst_QAudioFormat::checkChannelCount()
{
    // channels is the old name for channelCount, so
    // they should always be equal
    QAudioFormat audioFormat;
    QVERIFY(audioFormat.channelCount() == -1);
    QVERIFY(audioFormat.channels() == -1);

    audioFormat.setChannelCount(123);
    QVERIFY(audioFormat.channelCount() == 123);
    QVERIFY(audioFormat.channels() == 123);

    audioFormat.setChannels(5);
    QVERIFY(audioFormat.channelCount() == 5);
    QVERIFY(audioFormat.channels() == 5);
}

void tst_QAudioFormat::debugOperator_data()
{
    QTest::addColumn<QAudioFormat>("format");
    QTest::addColumn<QString>("stringized");

    // A small sampling
    QAudioFormat f;
    QTest::newRow("plain") << f << QString::fromLatin1("QAudioFormat(-1Hz, -1bit, channelCount=-1, sampleType=Unknown, byteOrder=LittleEndian, codec=\"\") ");

    f.setSampleRate(22050);
    f.setByteOrder(QAudioFormat::LittleEndian);
    f.setChannelCount(4);
    f.setCodec("audio/pcm");
    f.setSampleType(QAudioFormat::Float);

    QTest::newRow("float") << f << QString::fromLatin1("QAudioFormat(22050Hz, -1bit, channelCount=4, sampleType=Float, byteOrder=LittleEndian, codec=\"audio/pcm\") ");

    f.setSampleType(QAudioFormat::UnSignedInt);
    QTest::newRow("unsigned") << f << QString::fromLatin1("QAudioFormat(22050Hz, -1bit, channelCount=4, sampleType=UnSignedInt, byteOrder=LittleEndian, codec=\"audio/pcm\") ");

    f.setSampleRate(44100);
    QTest::newRow("44.1 unsigned") << f << QString::fromLatin1("QAudioFormat(44100Hz, -1bit, channelCount=4, sampleType=UnSignedInt, byteOrder=LittleEndian, codec=\"audio/pcm\") ");

    f.setByteOrder(QAudioFormat::BigEndian);
    QTest::newRow("44.1 big unsigned") << f << QString::fromLatin1("QAudioFormat(44100Hz, -1bit, channelCount=4, sampleType=UnSignedInt, byteOrder=BigEndian, codec=\"audio/pcm\") ");
}

void tst_QAudioFormat::debugOperator()
{
    QFETCH(QAudioFormat, format);
    QFETCH(QString, stringized);

    QTest::ignoreMessage(QtDebugMsg, stringized.toLatin1().constData());
    qDebug() << format;
}

QTEST_MAIN(tst_QAudioFormat)

#include "tst_qaudioformat.moc"
