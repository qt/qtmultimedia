/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include <QtTest/QtTest>
#include <QDebug>

#include <qaudioformat.h>

#include <qaudiocapturesource.h>
#include <qaudioencodercontrol.h>
#include <qmediarecordercontrol.h>
#include <qaudioendpointselector.h>
#include <qaudiodeviceinfo.h>
#include <qaudioinput.h>
#include <qmediaobject.h>

//TESTED_COMPONENT=src/multimedia

#include "mockmediaserviceprovider.h"
#include "mockmediarecorderservice.h"

QT_USE_NAMESPACE

class tst_QAudioCaptureSource: public QObject
{
    Q_OBJECT

public slots:
    void initTestCase();
    void cleanupTestCase();

private slots:
    //void testNullService();
    //void testNullControl();
    void testAudioSource();
    void testOptions();
    void testDevices();
    void testAvailability();
    void testAvailableAudioInputChangedSignal();

private:
    QAudioCaptureSource *audiosource;
    MockMediaRecorderService  *mockMediaRecorderService;
    MockMediaServiceProvider *mockProvider;
};

void tst_QAudioCaptureSource::initTestCase()
{
    mockMediaRecorderService = new MockMediaRecorderService;
    mockProvider = new MockMediaServiceProvider(mockMediaRecorderService);
}

void tst_QAudioCaptureSource::cleanupTestCase()
{
    delete audiosource;
    delete mockProvider;
    audiosource = 0;
}
/*
void tst_QAudioCaptureSource::testNullService()
{
    MockProvider provider(0);
    QAudioCaptureSource source(0, &provider);

    QCOMPARE(source.audioInputs().size(), 0);
    QCOMPARE(source.defaultAudioInput(), QString());
    QCOMPARE(source.activeAudioInput(), QString());
}
*/
/*
void tst_QAudioCaptureSource::testNullControl()
{
    MockRecorderService service;
    service.hasAudioDeviceControl = false;
    MockProvider provider(&service);
    QAudioCaptureSource source(0, &provider);

    QCOMPARE(source.audioInputs().size(), 0);
    QCOMPARE(source.defaultAudioInput(), QString());
    QCOMPARE(source.activeAudioInput(), QString());

    QCOMPARE(source.audioDescription("blah"), QString());

    QSignalSpy deviceNameSpy(&source, SIGNAL(activeAudioInputChanged(QString)));

    source.setAudioInput("blah");
    QCOMPARE(deviceNameSpy.count(), 0);
}
*/
void tst_QAudioCaptureSource::testAudioSource()
{
    audiosource = new QAudioCaptureSource(0, mockProvider);

    QCOMPARE(audiosource->service(),(QMediaService *) mockMediaRecorderService);
}

void tst_QAudioCaptureSource::testOptions()
{
    const QString codec(QLatin1String("audio/mpeg"));

    QStringList options = mockMediaRecorderService->mockAudioEncoderControl->supportedEncodingOptions(codec);
    QCOMPARE(options.count(), 4);
    mockMediaRecorderService->mockAudioEncoderControl->setEncodingOption(codec, options.first(),8000);
    QVERIFY(mockMediaRecorderService->mockAudioEncoderControl->encodingOption(codec, options.first()).toInt() == 8000);
}

void tst_QAudioCaptureSource::testDevices()
{
    audiosource = new QAudioCaptureSource(0,mockProvider);
    QList<QString> devices = audiosource->audioInputs();
    QVERIFY(devices.size() > 0);
    QVERIFY(devices.at(0).compare("device1") == 0);
    QVERIFY(audiosource->audioDescription("device1").compare("dev1 comment") == 0);
    QVERIFY(audiosource->defaultAudioInput() == "device1");
    QVERIFY(audiosource->isAvailable() == true);

    QSignalSpy checkSignal(audiosource, SIGNAL(activeAudioInputChanged(QString)));
    audiosource->setAudioInput("device2");
    QVERIFY(audiosource->activeAudioInput().compare("device2") == 0);
    QVERIFY(checkSignal.count() == 1);
    QVERIFY(audiosource->isAvailable() == true);
}

void tst_QAudioCaptureSource::testAvailability()
{
    MockMediaRecorderService service;
    service.hasControls = false;
    MockMediaServiceProvider provider(&service);
    QAudioCaptureSource source(0, &provider);

    QVERIFY(source.isAvailable() == false);
    QVERIFY(source.availabilityError() == QtMultimedia::ServiceMissingError);

    service.hasControls = true;
    MockMediaServiceProvider provider2(&service);
    QAudioCaptureSource source2(0, &provider2);

    QVERIFY(source2.isAvailable() == true);
    QVERIFY(source2.availabilityError() == QtMultimedia::NoError);
}

void tst_QAudioCaptureSource::testAvailableAudioInputChangedSignal()
{
    // The availabilityChangedSignal is implemented in QAudioCaptureSource. SO using it to test the signal.
    audiosource = new QAudioCaptureSource(0, mockProvider);

    /* Spy the signal availableEndpointChanged and audioInputchanged */
    QSignalSpy changed(mockMediaRecorderService->mockAudioEndpointSelector, SIGNAL(availableEndpointsChanged()));
    QSignalSpy audioInputchange(audiosource, SIGNAL(availableAudioInputsChanged()));

    /* Add the end points and verify if the available end point changed signal is emitted. */
    QMetaObject::invokeMethod(mockMediaRecorderService->mockAudioEndpointSelector, "addEndpoints");
    QVERIFY(changed.count() == 1);
    QVERIFY(audioInputchange.count() == 1);

    /* Now try removes */
    changed.clear();
    audioInputchange.clear();
    QMetaObject::invokeMethod(mockMediaRecorderService->mockAudioEndpointSelector, "removeEndpoints");
    QVERIFY(changed.count() == 1);
    QVERIFY(audioInputchange.count() == 1);
}

QTEST_MAIN(tst_QAudioCaptureSource)

#include "tst_qaudiocapturesource.moc"
