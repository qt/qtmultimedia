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

#include <qaudioprobe.h>
#include <qmediarecorder.h>

//TESTED_COMPONENT=src/multimedia

#include "mockmediaserviceprovider.h"
#include "mockmediarecorderservice.h"
#include "mockmediasource.h"

QT_USE_NAMESPACE

class tst_QAudioProbe: public QObject
{
    Q_OBJECT

public slots:
    void init();
    void cleanup();

private slots:
    void testNullService();
    void testNullControl();
    void testRecorder();
    void testRecorderDeleteRecorder();
    void testRecorderDeleteProbe();
    void testMediaSource();

private:
    QMediaRecorder *recorder;
    MockMediaRecorderControl *mockMediaRecorderControl;
    MockMediaRecorderService  *mockMediaRecorderService;
    MockMediaServiceProvider *mockProvider;
};

void tst_QAudioProbe::init()
{
    mockMediaRecorderControl = new MockMediaRecorderControl(this);
    mockMediaRecorderService = new MockMediaRecorderService(this, mockMediaRecorderControl);
    mockProvider = new MockMediaServiceProvider(mockMediaRecorderService);
    mockProvider->deleteServiceOnRelease = true;
    recorder = nullptr;

    QMediaServiceProvider::setDefaultServiceProvider(mockProvider);
}

void tst_QAudioProbe::cleanup()
{
    delete recorder;
    delete mockProvider;
    delete mockMediaRecorderControl;
    mockMediaRecorderControl = nullptr;
    mockMediaRecorderService = nullptr;
    mockProvider = nullptr;
    recorder = nullptr;
}

void tst_QAudioProbe::testNullService()
{
    mockProvider->service = nullptr;
    recorder = new QMediaRecorder;

    QVERIFY(!recorder->isAvailable());
    QCOMPARE(recorder->availability(), QMultimedia::ServiceMissing);

    QAudioProbe probe;
    QVERIFY(!probe.isActive());
    QVERIFY(!probe.setSource(recorder));
    QVERIFY(!probe.isActive());
    delete recorder;
    recorder = nullptr;
    QVERIFY(!probe.isActive());
}


void tst_QAudioProbe::testNullControl()
{
    mockMediaRecorderService->hasControls = false;
    recorder = new QMediaRecorder;

    QVERIFY(!recorder->isAvailable());
    QCOMPARE(recorder->availability(), QMultimedia::ServiceMissing);

    QAudioProbe probe;
    QVERIFY(!probe.isActive());
    QVERIFY(!probe.setSource(recorder));
    QVERIFY(!probe.isActive());
    delete recorder;
    recorder = nullptr;
    QVERIFY(!probe.isActive());
}

void tst_QAudioProbe::testRecorder()
{
    recorder = new QMediaRecorder;
    QVERIFY(recorder->isAvailable());

    QAudioProbe probe;
    QVERIFY(!probe.isActive());
    QVERIFY(probe.setSource(recorder));
    QVERIFY(probe.isActive());
    probe.setSource((QMediaRecorder*)nullptr);
    QVERIFY(!probe.isActive());
}

void tst_QAudioProbe::testRecorderDeleteRecorder()
{
    recorder = new QMediaRecorder;
    QVERIFY(recorder->isAvailable());

    QAudioProbe probe;
    QVERIFY(!probe.isActive());
    QVERIFY(probe.setSource(recorder));
    QVERIFY(probe.isActive());

    delete recorder;
    recorder = nullptr;
    QVERIFY(!probe.isActive());
    probe.setSource((QMediaRecorder*)nullptr);
    QVERIFY(!probe.isActive());
}

void tst_QAudioProbe::testRecorderDeleteProbe()
{
    recorder = new QMediaRecorder;
    QVERIFY(recorder->isAvailable());

    QAudioProbe *probe = new QAudioProbe;
    QVERIFY(!probe->isActive());
    QVERIFY(probe->setSource(recorder));
    QVERIFY(probe->isActive());

    delete probe;
    QVERIFY(recorder->isAvailable());
}

void tst_QAudioProbe::testMediaSource()
{
    QMediaSource *object = new MockMediaSource(nullptr, mockMediaRecorderService);
    QVERIFY(object->isAvailable());

    QAudioProbe probe;
    QVERIFY(!probe.isActive());
    QVERIFY(probe.setSource(object));
    QVERIFY(probe.isActive());
    probe.setSource((QMediaSource*)nullptr);
    QVERIFY(!probe.isActive());
    delete object;
}

QTEST_GUILESS_MAIN(tst_QAudioProbe)

#include "tst_qaudioprobe.moc"
