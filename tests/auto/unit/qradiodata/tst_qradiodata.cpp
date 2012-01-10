/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

//TESTED_COMPONENT=src/multimedia

#include <QtTest/QtTest>
#include <QDebug>
#include <QTimer>

#include <qmediaobject.h>
#include <qmediacontrol.h>
#include <qmediaservice.h>
#include <qradiodatacontrol.h>
#include <qradiodata.h>

#include "mockmediaserviceprovider.h"
#include "mockmediaservice.h"
#include "mockradiodatacontrol.h"

QT_USE_NAMESPACE

class tst_QRadioData: public QObject
{
    Q_OBJECT

public slots:
    void initTestCase();
    void cleanupTestCase();

private slots:
    void testNullService();
    void testNullControl();
    void testAlternativeFrequencies();
    void testRadioDataUpdates();

private:
    MockRadioDataControl     *mock;
    MockMediaService     *service;
    MockMediaServiceProvider    *provider;
    QRadioData    *radio;
};

void tst_QRadioData::initTestCase()
{
    qRegisterMetaType<QRadioData::ProgramType>("QRadioData::ProgramType");

    mock = new MockRadioDataControl(this);
    service = new MockMediaService(this, mock);
    provider = new MockMediaServiceProvider(service);
    radio = new QRadioData(0,provider);
    QVERIFY(radio->service() != 0);
    QVERIFY(radio->isAvailable());
    QVERIFY(radio->availabilityError() == QtMultimedia::NoError);
}

void tst_QRadioData::cleanupTestCase()
{
    QVERIFY(radio->error() == QRadioData::NoError);
    QVERIFY(radio->errorString().isEmpty());

    delete radio;
    delete service;
    delete provider;
}

void tst_QRadioData::testNullService()
{
    const QPair<int, int> nullRange(0, 0);

    MockMediaServiceProvider provider(0);
    QRadioData radio(0, &provider);
    QVERIFY(!radio.isAvailable());
    QCOMPARE(radio.error(), QRadioData::ResourceError);
    QCOMPARE(radio.errorString(), QString());
    QCOMPARE(radio.stationId(), QString());
    QCOMPARE(radio.programType(), QRadioData::Undefined);
    QCOMPARE(radio.programTypeName(), QString());
    QCOMPARE(radio.stationName(), QString());
    QCOMPARE(radio.radioText(), QString());
    QCOMPARE(radio.isAlternativeFrequenciesEnabled(), false);

}

void tst_QRadioData::testNullControl()
{
    const QPair<int, int> nullRange(0, 0);

    MockMediaService service(0, 0);
    MockMediaServiceProvider provider(&service);
    QRadioData radio(0, &provider);
    QVERIFY(!radio.isAvailable());
    QCOMPARE(radio.error(), QRadioData::ResourceError);
    QCOMPARE(radio.errorString(), QString());

    QCOMPARE(radio.stationId(), QString());
    QCOMPARE(radio.programType(), QRadioData::Undefined);
    QCOMPARE(radio.programTypeName(), QString());
    QCOMPARE(radio.stationName(), QString());
    QCOMPARE(radio.radioText(), QString());
    QCOMPARE(radio.isAlternativeFrequenciesEnabled(), false);
    {
        QSignalSpy spy(&radio, SIGNAL(alternativeFrequenciesEnabledChanged(bool)));

        radio.setAlternativeFrequenciesEnabled(true);
        QCOMPARE(radio.isAlternativeFrequenciesEnabled(), false);
        QCOMPARE(spy.count(), 0);
    }
}

void tst_QRadioData::testAlternativeFrequencies()
{
    QSignalSpy readSignal(radio, SIGNAL(alternativeFrequenciesEnabledChanged(bool)));
    radio->setAlternativeFrequenciesEnabled(true);
    QTestEventLoop::instance().enterLoop(1);
    QVERIFY(radio->isAlternativeFrequenciesEnabled() == true);
    QVERIFY(readSignal.count() == 1);
}

void tst_QRadioData::testRadioDataUpdates()
{
    QSignalSpy rtSpy(radio, SIGNAL(radioTextChanged(QString)));
    QSignalSpy ptyPTYSpy(radio, SIGNAL(programTypeChanged(QRadioData::ProgramType)));
    QSignalSpy ptynSpy(radio, SIGNAL(programTypeNameChanged(QString)));
    QSignalSpy piSpy(radio, SIGNAL(stationIdChanged(QString)));
    QSignalSpy psSpy(radio, SIGNAL(stationNameChanged(QString)));
    mock->forceRT("Mock Radio Text");
    mock->forceProgramType(static_cast<int>(QRadioData::Sport));
    mock->forcePTYN("Mock Programme Type Name");
    mock->forcePI("Mock Programme Identification");
    mock->forcePS("Mock Programme Service");
    QTestEventLoop::instance().enterLoop(1);
    QVERIFY(rtSpy.count() == 1);
    QVERIFY(ptyPTYSpy.count() == 1);
    QVERIFY(ptynSpy.count() == 1);
    QVERIFY(piSpy.count() == 1);
    QVERIFY(psSpy.count() == 1);
    qDebug()<<radio->radioText();
    QCOMPARE(radio->radioText(), QString("Mock Radio Text"));
    QCOMPARE(radio->programType(), QRadioData::Sport);
    QCOMPARE(radio->programTypeName(), QString("Mock Programme Type Name"));
    QCOMPARE(radio->stationId(), QString("Mock Programme Identification"));
    QCOMPARE(radio->stationName(), QString("Mock Programme Service"));
}

QTEST_GUILESS_MAIN(tst_QRadioData)
#include "tst_qradiodata.moc"
