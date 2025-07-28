// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QtTest/qsignalspy.h>
#include <QDebug>

#include <qmediadevices.h>

#include "qmockaudiodevices.h"
#include "qmockvideodevices.h"
#include "qmockintegration.h"

QT_USE_NAMESPACE

Q_ENABLE_MOCK_MULTIMEDIA_PLUGIN

class tst_QMediaDevices : public QObject
{
    Q_OBJECT

public slots:
    void cleanup() { QMockIntegration::instance()->resetInstance(); }

private slots:
    void videoInputsChangedEmitted_whenCamerasChanged();
    void onlyVideoInputsChangedEmitted_when2MediaDevicesCreated_andCamerasChanged();

    void audioInputs_invokesFindAudioInputsOnceAfterUpdate();
    void audioOutputs_invokesFindAudioInputsOnceAfterUpdate();
    void videoInputs_invokesFindVideoInputsOnceAfterUpdate();

    void connectToAudioInputsChanged_initializesOnlyAudioDevices();
    void connectToAudioOutputsChanged_initializesOnlyAudioDevices();
    void connectToVideoInputsChanged_initializesOnlyVideoDevices();
};

void tst_QMediaDevices::videoInputsChangedEmitted_whenCamerasChanged()
{
    QMediaDevices mediaDevices;
    QSignalSpy videoInputsSpy(&mediaDevices, &QMediaDevices::videoInputsChanged);

    QCOMPARE(videoInputsSpy.size(), 0);

    QMockIntegration::instance()->addNewCamera();
    QTRY_COMPARE(videoInputsSpy.size(), 1);

    QMockIntegration::instance()->addNewCamera();
    QCOMPARE(videoInputsSpy.size(), 2);
}

void tst_QMediaDevices::onlyVideoInputsChangedEmitted_when2MediaDevicesCreated_andCamerasChanged()
{
    QMediaDevices mediaDevicesA;
    QMediaDevices mediaDevicesB;

    QSignalSpy videoInputsSpyA(&mediaDevicesA, &QMediaDevices::videoInputsChanged);
    QSignalSpy videoInputsSpyB(&mediaDevicesB, &QMediaDevices::videoInputsChanged);
    QSignalSpy audioInputsSpy(&mediaDevicesA, &QMediaDevices::audioInputsChanged);
    QSignalSpy audioOutputsSpy(&mediaDevicesA, &QMediaDevices::audioOutputsChanged);

    QMockIntegration::instance()->addNewCamera();
    QCOMPARE(videoInputsSpyA.size(), 1);
    QCOMPARE(videoInputsSpyB.size(), 1);

    QCOMPARE(audioInputsSpy.size(), 0);
    QCOMPARE(audioOutputsSpy.size(), 0);
}

void tst_QMediaDevices::audioInputs_invokesFindAudioInputsOnceAfterUpdate()
{
    QMediaDevices mediaDevices;
    QMockAudioDevices* audioDevices = QMockIntegration::instance()->audioDevices();

    QCOMPARE(audioDevices->getFindAudioInputsInvokeCount(), 0);

    for (int i = 0; i < 3; ++i) {
        mediaDevices.audioInputs();
        QCOMPARE(audioDevices->getFindAudioInputsInvokeCount(), 1);
    }

    audioDevices->addAudioInput();

    for (int i = 0; i < 3; ++i) {
        mediaDevices.audioInputs();
        QCOMPARE(audioDevices->getFindAudioInputsInvokeCount(), 2);
    }

    QCOMPARE(audioDevices->getFindAudioOutputsInvokeCount(), 0);
    QCOMPARE(QMockIntegration::instance()->createAudioDevicesInvokeCount(), 1);
    QCOMPARE(QMockIntegration::instance()->createVideoDevicesInvokeCount(), 0);
}

void tst_QMediaDevices::audioOutputs_invokesFindAudioInputsOnceAfterUpdate() {
    QMediaDevices mediaDevices;
    QMockAudioDevices* audioDevices = QMockIntegration::instance()->audioDevices();

    QCOMPARE(audioDevices->getFindAudioOutputsInvokeCount(), 0);

    for (int i = 0; i < 3; ++i) {
        mediaDevices.audioOutputs();
        QCOMPARE(audioDevices->getFindAudioOutputsInvokeCount(), 1);
    }

    audioDevices->addAudioOutput();

    for (int i = 0; i < 3; ++i) {
        mediaDevices.audioOutputs();
        QCOMPARE(audioDevices->getFindAudioOutputsInvokeCount(), 2);
    }

    QCOMPARE(audioDevices->getFindAudioInputsInvokeCount(), 0);
    QCOMPARE(QMockIntegration::instance()->createAudioDevicesInvokeCount(), 1);
    QCOMPARE(QMockIntegration::instance()->createVideoDevicesInvokeCount(), 0);
}

void tst_QMediaDevices::videoInputs_invokesFindVideoInputsOnceAfterUpdate()
{
    QMediaDevices mediaDevices;
    QMockVideoDevices* videoDevices = QMockIntegration::instance()->videoDevices();

    QCOMPARE(videoDevices->getFindVideoInputsInvokeCount(), 0);

    for (int i = 0; i < 3; ++i) {
        mediaDevices.videoInputs();
        QCOMPARE(videoDevices->getFindVideoInputsInvokeCount(), 1);
    }

    QMockIntegration::instance()->addNewCamera();

    for (int i = 0; i < 3; ++i) {
        mediaDevices.videoInputs();
        QCOMPARE(videoDevices->getFindVideoInputsInvokeCount(), 2);
    }

    QCOMPARE(QMockIntegration::instance()->createAudioDevicesInvokeCount(), 0);
    QCOMPARE(QMockIntegration::instance()->createVideoDevicesInvokeCount(), 1);
}

void tst_QMediaDevices::connectToAudioInputsChanged_initializesOnlyAudioDevices()
{
    QMediaDevices mediaDevices;

    QCOMPARE(QMockIntegration::instance()->createAudioDevicesInvokeCount(), 0);

    QSignalSpy spy(&mediaDevices, &QMediaDevices::audioInputsChanged);

    QCOMPARE(QMockIntegration::instance()->createAudioDevicesInvokeCount(), 1);
    QCOMPARE(QMockIntegration::instance()->createVideoDevicesInvokeCount(), 0);

    QMockAudioDevices *audioDevices = QMockIntegration::instance()->audioDevices();

    audioDevices->addAudioInput();
    QCOMPARE(spy.size(), 1);
}

void tst_QMediaDevices::connectToAudioOutputsChanged_initializesOnlyAudioDevices()
{
    QMediaDevices mediaDevices;

    QCOMPARE(QMockIntegration::instance()->createAudioDevicesInvokeCount(), 0);

    QSignalSpy spy(&mediaDevices, &QMediaDevices::audioOutputsChanged);

    QCOMPARE(QMockIntegration::instance()->createAudioDevicesInvokeCount(), 1);
    QCOMPARE(QMockIntegration::instance()->createVideoDevicesInvokeCount(), 0);

    QMockAudioDevices *audioDevices = QMockIntegration::instance()->audioDevices();

    audioDevices->addAudioOutput();
    QCOMPARE(spy.size(), 1);
}

void tst_QMediaDevices::connectToVideoInputsChanged_initializesOnlyVideoDevices()
{
    QMediaDevices mediaDevices;

    QSignalSpy spy(&mediaDevices, &QMediaDevices::videoInputsChanged);

    QCOMPARE(QMockIntegration::instance()->createAudioDevicesInvokeCount(), 0);
    QCOMPARE(QMockIntegration::instance()->createVideoDevicesInvokeCount(), 1);

    QMockIntegration::instance()->addNewCamera();
    QCOMPARE(spy.size(), 1);
}

QTEST_MAIN(tst_QMediaDevices)

#include "tst_qmediadevices.moc"
