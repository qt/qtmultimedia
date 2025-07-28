// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QtTest/qtest.h>

#include <QtCore/qstringlist.h>
#include <QtCore/qlist.h>
#include <QtMultimedia/qmediadevices.h>
#include <QtMultimedia/qaudiodevice.h>

#include <private/mediabackendutils_p.h>
#include <private/osdetection_p.h>

class tst_QAudioDeviceBackend : public QObject
{
    Q_OBJECT
public:
    tst_QAudioDeviceBackend(QObject* parent=nullptr) : QObject(parent) {}

private slots:
    void initTestCase();
    void checkAvailableDefaultInput();
    void checkAvailableDefaultOutput();
    void channels();
    void sampleFormat();
    void sampleRates();
    void isFormatSupported();
    void preferred();
    void assignOperator();
    void id();
    void defaultConstructor();
    void equalityOperator();

private:
    std::unique_ptr<QAudioDevice> device;
};

void tst_QAudioDeviceBackend::initTestCase()
{
    // Only perform tests if audio output device exists!
    QList<QAudioDevice> devices = QMediaDevices::audioOutputs();
    if (devices.size() == 0) {
        QSKIP("NOTE: no audio output device found, no tests will be performed");
    } else {
        device = std::make_unique<QAudioDevice>(devices.at(0));
    }
}

void tst_QAudioDeviceBackend::checkAvailableDefaultInput()
{
    // Only perform tests if audio input device exists!
    QList<QAudioDevice> devices = QMediaDevices::audioInputs();
    if (devices.size() > 0) {
        auto defaultInput = QMediaDevices::defaultAudioInput();
        QVERIFY(!defaultInput.isNull());
        QCOMPARE(std::count(devices.begin(), devices.end(), defaultInput), 1);
    }
}

void tst_QAudioDeviceBackend::checkAvailableDefaultOutput()
{
    // Only perform tests if audio input device exists!
    QList<QAudioDevice> devices = QMediaDevices::audioOutputs();
    if (devices.size() > 0) {
        auto defaultOutput = QMediaDevices::defaultAudioOutput();
        QVERIFY(!defaultOutput.isNull());
        QCOMPARE(std::count(devices.begin(), devices.end(), defaultOutput), 1);
    }
}

void tst_QAudioDeviceBackend::channels()
{
    QVERIFY(device->minimumChannelCount() > 0);
    QVERIFY(device->maximumChannelCount() >= device->minimumChannelCount());
}

void tst_QAudioDeviceBackend::sampleFormat()
{
    QList<QAudioFormat::SampleFormat> avail = device->supportedSampleFormats();
    QVERIFY(avail.size() > 0);
}

void tst_QAudioDeviceBackend::sampleRates()
{
    QVERIFY(device->minimumSampleRate() > 0);
    QVERIFY(device->maximumSampleRate() >= device->minimumSampleRate());
}

void tst_QAudioDeviceBackend::isFormatSupported()
{
    if (isCI() && isMacOS)
        QSKIP("MacOS vms don't seem to support 44100hz on CI?");

    QAudioFormat format;
    format.setSampleRate(44100);
    format.setChannelCount(2);
    format.setSampleFormat(QAudioFormat::Int16);

    // Should always be true for these format
    QVERIFY(device->isFormatSupported(format));
}

void tst_QAudioDeviceBackend::preferred()
{
    auto isPreferredSupported = [](const QAudioDevice &dev) {
        QAudioFormat format = dev.preferredFormat();
        QVERIFY(format.isValid());
        QVERIFY(dev.isFormatSupported(format));
    };

    for (const auto &dev: QMediaDevices::audioOutputs())
        isPreferredSupported(dev);

    for (const auto &dev: QMediaDevices::audioInputs())
        isPreferredSupported(dev);
}

// QAudioDevice's assignOperator method
void tst_QAudioDeviceBackend::assignOperator()
{
    QAudioDevice dev;
    QVERIFY(dev.id().isNull());
    QVERIFY(dev.isNull() == true);

    QList<QAudioDevice> devices = QMediaDevices::audioOutputs();
    QVERIFY(devices.size() > 0);
    QAudioDevice dev1(devices.at(0));
    dev = dev1;
    QVERIFY(dev.isNull() == false);
    QVERIFY(dev.id() == dev1.id());
}

void tst_QAudioDeviceBackend::id()
{
    QVERIFY(!device->id().isNull());
    QVERIFY(device->id() == QMediaDevices::audioOutputs().at(0).id());
}

// QAudioDevice's defaultConstructor method
void tst_QAudioDeviceBackend::defaultConstructor()
{
    QAudioDevice dev;
    QVERIFY(dev.isNull() == true);
    QVERIFY(dev.id().isNull());
}

void tst_QAudioDeviceBackend::equalityOperator()
{
    // Get some default device infos
    QAudioDevice dev1;
    QAudioDevice dev2;

    QVERIFY(dev1 == dev2);
    QVERIFY(!(dev1 != dev2));

    // Make sure each available device is not equal to null
    const auto infos = QMediaDevices::audioOutputs();
    for (const QAudioDevice &info : infos) {
        QVERIFY(dev1 != info);
        QVERIFY(!(dev1 == info));

        dev2 = info;

        QVERIFY(dev2 == info);
        QVERIFY(!(dev2 != info));

        QVERIFY(dev1 != dev2);
        QVERIFY(!(dev1 == dev2));
    }

    // XXX Perhaps each available device should not be equal to any other
}

QTEST_MAIN(tst_QAudioDeviceBackend)

#include "tst_qaudiodevicebackend.moc"
