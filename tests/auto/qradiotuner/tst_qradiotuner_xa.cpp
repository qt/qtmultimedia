/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Mobility Components.
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

#include "tst_qradiotuner_xa.h"

QT_USE_NAMESPACE

#define QTEST_MAIN_S60(TestObject) \
    int main(int argc, char *argv[]) { \
        char *new_argv[3]; \
        QApplication app(argc, argv); \
        \
        QString str = "C:\\data\\" + QFileInfo(QCoreApplication::applicationFilePath()).baseName() + ".log"; \
        QByteArray   bytes  = str.toAscii(); \
        \
        char arg1[] = "-o"; \
        \
        new_argv[0] = argv[0]; \
        new_argv[1] = arg1; \
        new_argv[2] = bytes.data(); \
        \
        TestObject tc; \
        return QTest::qExec(&tc, 3, new_argv); \
    }

#define QTRY_COMPARE(a,e)                       \
    for (int _i = 0; _i < 5000; _i += 100) {    \
        if ((a) == (e)) break;                  \
        QTest::qWait(100);                      \
    }                                           \
//    QCOMPARE(a, e)

void tst_QXARadio_xa::initTestCase()
{
    qRegisterMetaType<QRadioTuner::State>("QRadioTuner::State");
    radio = new QRadioTuner(0);
//    QVERIFY(radio->service() != 0);
//    QSignalSpy stateSpy(radio, SIGNAL(stateChanged(QRadioTuner::State)));
    radio->start();
//    QTRY_COMPARE(stateSpy.count(), 1); // wait for callbacks to complete in symbian API
//    QCOMPARE(radio->state(), QRadioTuner::ActiveState);

}

void tst_QXARadio_xa::cleanupTestCase()
{
    QVERIFY(radio->service() != 0);
}

void tst_QXARadio_xa::testBand()
{
    qRegisterMetaType<QRadioTuner::Band>("QRadioTuner::Band");

    QVERIFY(radio->isBandSupported(QRadioTuner::FM));
    QVERIFY(!radio->isBandSupported(QRadioTuner::SW));
    radio->setBand(QRadioTuner::FM);
    QVERIFY(radio->band() == QRadioTuner::FM);
    if(radio->isBandSupported(QRadioTuner::AM)) {
        QSignalSpy readSignal(radio, SIGNAL(bandChanged(QRadioTuner::Band)));
        radio->setBand(QRadioTuner::AM);
        QTRY_COMPARE(readSignal.count(), 1); // wait for callbacks to complete in symbian API
        QVERIFY(radio->band() == QRadioTuner::AM);
    }
}

void tst_QXARadio_xa::testFrequency()
{
    QSignalSpy readSignal(radio, SIGNAL(frequencyChanged(int)));
    radio->setFrequency(90900000);
    QTRY_COMPARE(readSignal.count(), 1); // wait for callbacks to complete in symbian API
    QVERIFY(radio->frequency() == 90900000);
    // frequencyStep for FM radio is 100kHz (100000Hz)
    QVERIFY(radio->frequencyStep(QRadioTuner::FM) == 100000);
    QPair<int,int> test = radio->frequencyRange(QRadioTuner::FM);
    // frequency range for FM radio is 87,5MHz - 108MHz
    QVERIFY(test.first == 87500000);
    QVERIFY(test.second == 108000000);
}

void tst_QXARadio_xa::testMute()
{
    QSignalSpy readSignal(radio, SIGNAL(mutedChanged(bool)));
    radio->setMuted(true);
    QTRY_COMPARE(readSignal.count(), 1); // wait for callbacks to complete in symbian API
    QVERIFY(radio->isMuted());
    QVERIFY(readSignal.count() == 1);
}

void tst_QXARadio_xa::testSearch()
{
    QSignalSpy readSignal(radio, SIGNAL(searchingChanged(bool)));
    QVERIFY(!radio->isSearching());

    radio->searchForward();
    // Note: DON'T wait for callback to complete in symbian API
    QVERIFY(radio->isSearching());

    radio->cancelSearch();
    QTRY_COMPARE(readSignal.count(), 1); // wait for callbacks to complete in symbian API
    QVERIFY(!radio->isSearching());

    radio->searchBackward();
    // Note: DON'T wait for callbacks to complete in symbian API
    QVERIFY(radio->isSearching());

    radio->cancelSearch();
    QTRY_COMPARE(readSignal.count(), 1); // wait for callbacks to complete in symbian API
    QVERIFY(!radio->isSearching());
}

void tst_QXARadio_xa::testVolume()
{
    QSignalSpy readSignal(radio, SIGNAL(volumeChanged(int)));
    radio->setVolume(50);
    QTRY_COMPARE(readSignal.count(), 1); // wait for callbacks to complete in symbian API
    QVERIFY(radio->volume() == 50);
}

void tst_QXARadio_xa::testSignal()
{
    QVERIFY(radio->signalStrength() != 0);
    // There is no set of this only a get, do nothing else.
}

void tst_QXARadio_xa::testStereo()
{
    // Default = Auto. Testing transition from auto to mono:
    QVERIFY(radio->isStereo());
    QSignalSpy readSignal(radio, SIGNAL(stereoStatusChanged(bool)));
    radio->setStereoMode(QRadioTuner::ForceMono);
    QTRY_COMPARE(readSignal.count(), 1); // wait for callbacks to complete in symbian API
    QVERIFY(radio->stereoMode() == QRadioTuner::ForceMono);
//    QVERIFY(readSignal.count() == 1);

    // testing transition from mono to stereo:
    radio->setStereoMode(QRadioTuner::ForceStereo);
    QTRY_COMPARE(readSignal.count(), 1); // wait for callbacks to complete in symbian API
    QVERIFY(radio->stereoMode() == QRadioTuner::ForceStereo);
//    QVERIFY(readSignal.count() == 1);

    // testing transition from stereo to auto:
    radio->setStereoMode(QRadioTuner::Auto);
    QTRY_COMPARE(readSignal.count(), 1); // wait for callbacks to complete in symbian API
    QVERIFY(radio->stereoMode() == QRadioTuner::Auto);
 //   QVERIFY(readSignal.count() == 1);

    // testing transition from auto to stereo:
    radio->setStereoMode(QRadioTuner::ForceStereo);
    QTRY_COMPARE(readSignal.count(), 1); // wait for callbacks to complete in symbian API
    QVERIFY(radio->stereoMode() == QRadioTuner::ForceStereo);
 //   QVERIFY(readSignal.count() == 1);

    // testing transition from stereo to mono:
    radio->setStereoMode(QRadioTuner::ForceMono);
    QTRY_COMPARE(readSignal.count(), 1); // wait for callbacks to complete in symbian API
    QVERIFY(radio->stereoMode() == QRadioTuner::ForceMono);
 //   QVERIFY(readSignal.count() == 1);

    // testing transition from mono to auto:
    radio->setStereoMode(QRadioTuner::Auto);
    QTRY_COMPARE(readSignal.count(), 1); // wait for callbacks to complete in symbian API
    QVERIFY(radio->stereoMode() == QRadioTuner::Auto);
//    QVERIFY(readSignal.count() == 1);

}

void tst_QXARadio_xa::testAvailability()
{
    QVERIFY(radio->isAvailable());
    QVERIFY(radio->availabilityError() == QtMultimediaKit::NoError);
}

void tst_QXARadio_xa::testStopRadio()
{
    QVERIFY(radio->service() != 0);
    QVERIFY(radio->error() == QRadioTuner::NoError);
    QVERIFY(radio->errorString().isEmpty());

    QSignalSpy stateSpy(radio, SIGNAL(stateChanged(QRadioTuner::State)));

    radio->stop();
    QTRY_COMPARE(stateSpy.count(), 1); // wait for callbacks to complete in symbian API
    QCOMPARE(radio->state(), QRadioTuner::StoppedState);

//   delete radio;
}
