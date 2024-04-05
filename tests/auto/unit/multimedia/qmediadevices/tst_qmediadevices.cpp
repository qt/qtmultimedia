// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QtTest>
#include <QDebug>

#include <qmediadevices.h>

#include "qmockintegration.h"

QT_USE_NAMESPACE

Q_ENABLE_MOCK_MULTIMEDIA_PLUGIN

class tst_QMediaDevices : public QObject
{
    Q_OBJECT

private slots:
    void videoInputsChangedEmitted_whenCamerasChanged();
    void onlyVideoInputsChangedEmitted_when2MediaDevicesCreated_andCamerasChanged();
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

QTEST_MAIN(tst_QMediaDevices)

#include "tst_qmediadevices.moc"
