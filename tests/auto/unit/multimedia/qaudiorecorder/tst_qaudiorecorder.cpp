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
#include <private/qplatformmediarecorder_p.h>
#include <qaudiodevice.h>
#include <qaudiosource.h>
#include <qmediacapturesession.h>

//TESTED_COMPONENT=src/multimedia

#include "qmockmediacapturesession.h"
#include "qmockintegration_p.h"

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
    QMockIntegration *mockIntegration;
};

void tst_QAudioRecorder::init()
{
    mockIntegration = new QMockIntegration;
    encoder = nullptr;
}

void tst_QAudioRecorder::cleanup()
{
    delete encoder;
    delete mockIntegration;
    mockIntegration = nullptr;
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
