/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
#include <QDebug>

#include <qaudioformat.h>

#include <qmediarecorder.h>
#include <qaudioencodersettingscontrol.h>
#include <qmediarecordercontrol.h>
#include <qaudiodeviceinfo.h>
#include <qaudioinput.h>
#include <qmediasource.h>

//TESTED_COMPONENT=src/multimedia

#include "mockmediaserviceprovider.h"
#include "mockmediarecorderservice.h"

QT_USE_NAMESPACE

class tst_QAudioRecorder: public QObject
{
    Q_OBJECT

public slots:
    void init();
    void cleanup();

private slots:
    void testNullService();
    void testNullControl();
    void testAudioSource();
    void testDevices();
    void testAvailability();

private:
    QMediaRecorder *audiosource;
    MockMediaRecorderService  *mockMediaRecorderService;
    MockMediaServiceProvider *mockProvider;
};

void tst_QAudioRecorder::init()
{
    mockMediaRecorderService = new MockMediaRecorderService(this, new MockMediaRecorderControl(this));
    mockProvider = new MockMediaServiceProvider(mockMediaRecorderService);
    audiosource = nullptr;

    QMediaServiceProvider::setDefaultServiceProvider(mockProvider);
}

void tst_QAudioRecorder::cleanup()
{
    delete mockMediaRecorderService;
    delete mockProvider;
    delete audiosource;
    mockMediaRecorderService = nullptr;
    mockProvider = nullptr;
    audiosource = nullptr;
}

void tst_QAudioRecorder::testNullService()
{
    mockProvider->service = nullptr;
    QMediaRecorder source;

    QVERIFY(!source.isAvailable());
    QCOMPARE(source.availability(), QMultimedia::ServiceMissing);

    QCOMPARE(source.audioInput(), QAudioDeviceInfo());
}


void tst_QAudioRecorder::testNullControl()
{
    mockMediaRecorderService->hasControls = false;
    QMediaRecorder source;

    QVERIFY(!source.isAvailable());
    QCOMPARE(source.availability(), QMultimedia::ServiceMissing);

    QCOMPARE(source.audioInput(), QAudioDeviceInfo());

    QSignalSpy deviceNameSpy(&source, SIGNAL(audioInputChanged(QString)));

    source.setAudioInput(QAudioDeviceInfo());
    QCOMPARE(deviceNameSpy.count(), 0);
}

void tst_QAudioRecorder::testAudioSource()
{
    audiosource = new QMediaRecorder;

    QCOMPARE(audiosource->mediaSource()->service(),(QMediaService *) mockMediaRecorderService);
}

void tst_QAudioRecorder::testDevices()
{
//    audiosource = new QMediaRecorder;
//    QList<QString> devices = audiosource->audioInputs();
//    QVERIFY(devices.size() > 0);
//    QVERIFY(devices.at(0).compare("device1") == 0);
//    QVERIFY(audiosource->audioInputDescription("device1").compare("dev1 comment") == 0);
//    QVERIFY(audiosource->defaultAudioInput() == "device1");
//    QVERIFY(audiosource->isAvailable() == true);

//    QSignalSpy checkSignal(audiosource, SIGNAL(audioInputChanged(QString)));
//    audiosource->setAudioInput("device2");
//    QVERIFY(audiosource->audioInput().compare("device2") == 0);
//    QVERIFY(checkSignal.count() == 1);
//    QVERIFY(audiosource->isAvailable() == true);
}

void tst_QAudioRecorder::testAvailability()
{
    QMediaRecorder source;

    QVERIFY(source.isAvailable());
    QCOMPARE(source.availability(), QMultimedia::Available);
}

QTEST_GUILESS_MAIN(tst_QAudioRecorder)

#include "tst_qaudiorecorder.moc"
