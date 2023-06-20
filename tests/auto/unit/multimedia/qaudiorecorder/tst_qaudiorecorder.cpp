// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/QtTest>
#include <QDebug>

#include <qaudioformat.h>

#include <qmediarecorder.h>
#include <private/qplatformmediarecorder_p.h>
#include <qaudiodevice.h>
#include <qaudiosource.h>
#include <qmediacapturesession.h>

//TESTED_COMPONENT=src/multimedia

#include "qmockmediacapturesession.h"
#include "qmockintegration.h"

QT_USE_NAMESPACE

class tst_QAudioRecorder: public QObject
{
    Q_OBJECT

public slots:
    void init();
    void cleanup();

private slots:
    void testAudioSource();
    void testDevices();
    void testAvailability();

private:
    QMediaRecorder *encoder = nullptr;
    QMockIntegrationFactory mockIntegrationFactory;
};

void tst_QAudioRecorder::init()
{
    encoder = nullptr;
}

void tst_QAudioRecorder::cleanup()
{
    delete encoder;
    encoder = nullptr;
}

void tst_QAudioRecorder::testAudioSource()
{
    QMediaCaptureSession session;
    encoder = new QMediaRecorder;
    session.setRecorder(encoder);

    QCOMPARE(session.camera(), nullptr);
}

void tst_QAudioRecorder::testDevices()
{
//    audiosource = new QMediaRecorder;
//    QList<QAudioDevice> devices = mockIntegration->audioInputs();
//    QVERIFY(devices.size() > 0);
//    QVERIFY(devices.at(0).id() == "device1");
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
    QMediaCaptureSession session;
    QMediaRecorder source;
    session.setRecorder(&source);

    QVERIFY(source.isAvailable());
}

QTEST_GUILESS_MAIN(tst_QAudioRecorder)

#include "tst_qaudiorecorder.moc"
