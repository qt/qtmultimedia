// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
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

    auto privA = std::make_unique<QAudioDevicePrivate>(idA, modeA, u""_s);
    const QAudioDevice a = QAudioDevicePrivate::createQAudioDevice(std::move(privA));

    auto privB = std::make_unique<QAudioDevicePrivate>(idB, modeB, u""_s);
    const QAudioDevice b = QAudioDevicePrivate::createQAudioDevice(std::move(privB));

    QCOMPARE(a == b, expected);
}

void tst_QAudioDevice::compare_returnsTrue_whenIsDefaultDiffers() {
    const QByteArray id = "ABC"_ba;
    const QAudioDevice::Mode mode = QAudioDevice::Mode::Input;

    auto privA = std::make_unique<QAudioDevicePrivate>(id, mode, u""_s);
    privA->isDefault = true;
    const QAudioDevice a = QAudioDevicePrivate::createQAudioDevice(std::move(privA));

    auto privB = std::make_unique<QAudioDevicePrivate>(id, mode, u""_s);
    privB->isDefault = false;
    const QAudioDevice b = QAudioDevicePrivate::createQAudioDevice(std::move(privB));

    QVERIFY(a == b);
}

QTEST_MAIN(tst_QAudioDevice)

#include "tst_qaudiodevice.moc"
