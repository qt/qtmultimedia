// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QtTest>
#include <qaudiodevice.h>
#include "private/qaudiodevice_p.h"

using namespace Qt::StringLiterals;

class tst_QAudioDevice : public QObject
{
    Q_OBJECT
public:
    tst_QAudioDevice(QObject* parent=nullptr) : QObject(parent) {}

private slots:
    void basicComparison_data();
    void basicComparison();

    void compare_returnsTrue_whenIsDefaultDiffers();
};

void tst_QAudioDevice::basicComparison_data()
{
    QTest::addColumn<QByteArray>("idA");
    QTest::addColumn<QAudioDevice::Mode>("modeA");
    QTest::addColumn<QByteArray>("idB");
    QTest::addColumn<QAudioDevice::Mode>("modeB");
    QTest::addColumn<bool>("expected");

    const QByteArray idA = "ABC"_ba;
    const QByteArray idB = "DEF"_ba;

    QTest::newRow("Equal ID, both input mode")
        << idA << QAudioDevice::Mode::Input
        << idA << QAudioDevice::Mode::Input
        << true;

    QTest::newRow("Equal ID, both output mode")
        << idA << QAudioDevice::Mode::Input
        << idA << QAudioDevice::Mode::Input
        << true;

    QTest::newRow("Equal ID, inequal mode")
        << idA << QAudioDevice::Mode::Input
        << idA << QAudioDevice::Mode::Output
        << false;

    QTest::newRow("Inequal ID, both input mode")
        << idA << QAudioDevice::Mode::Input
        << idB << QAudioDevice::Mode::Input
        << false;

    QTest::newRow("Inequal ID, inequal mode")
        << idA << QAudioDevice::Mode::Output
        << idB << QAudioDevice::Mode::Input
        << false;

    QTest::newRow("Both null IDs, equal mode")
        << QByteArray() << QAudioDevice::Mode::Input
        << QByteArray() << QAudioDevice::Mode::Input
        << true;

    QTest::newRow("Both null IDs, inequal mode")
        << QByteArray() << QAudioDevice::Mode::Input
        << QByteArray() << QAudioDevice::Mode::Output
        << false;

    QTest::newRow("One null ID, equal mode")
        << QByteArray() << QAudioDevice::Mode::Input
        << idA << QAudioDevice::Mode::Input
        << false;
}

void tst_QAudioDevice::basicComparison()
{
    QFETCH(QByteArray, idA);
    QFETCH(QAudioDevice::Mode, modeA);
    QFETCH(QByteArray, idB);
    QFETCH(QAudioDevice::Mode, modeB);
    QFETCH(bool, expected);

    QAudioDevicePrivate *privA = new QAudioDevicePrivate(idA, modeA, u""_s);
    const QAudioDevice a = privA->create();

    QAudioDevicePrivate *privB = new QAudioDevicePrivate(idB, modeB, u""_s);
    const QAudioDevice b = privB->create();

    QCOMPARE(a == b, expected);
}

void tst_QAudioDevice::compare_returnsTrue_whenIsDefaultDiffers() {
    const QByteArray id = "ABC"_ba;
    const QAudioDevice::Mode mode = QAudioDevice::Mode::Input;

    QAudioDevicePrivate *privA = new QAudioDevicePrivate(id, mode, u""_s);
    privA->isDefault = true;
    const QAudioDevice a = privA->create();

    QAudioDevicePrivate *privB = new QAudioDevicePrivate(id, mode, u""_s);
    privB->isDefault = false;
    const QAudioDevice b = privB->create();

    QVERIFY(a == b);
}

QTEST_MAIN(tst_QAudioDevice)

#include "tst_qaudiodevice.moc"
