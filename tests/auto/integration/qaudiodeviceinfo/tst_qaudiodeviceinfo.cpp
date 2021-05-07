/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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
#include <QtCore/qlocale.h>
#include <qaudiodeviceinfo.h>

#include <QStringList>
#include <QList>
#include <QMediaDevices>

//TESTED_COMPONENT=src/multimedia

class tst_QAudioDeviceInfo : public QObject
{
    Q_OBJECT
public:
    tst_QAudioDeviceInfo(QObject* parent=nullptr) : QObject(parent) {}

private slots:
    void initTestCase();
    void cleanupTestCase();
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
    QAudioDeviceInfo* device;
};

void tst_QAudioDeviceInfo::initTestCase()
{
    // Only perform tests if audio output device exists!
    QList<QAudioDeviceInfo> devices = QMediaDevices::audioOutputs();
    if (devices.size() == 0) {
        QSKIP("NOTE: no audio output device found, no tests will be performed");
    } else {
        device = new QAudioDeviceInfo(devices.at(0));
    }
}

void tst_QAudioDeviceInfo::cleanupTestCase()
{
    delete device;
}

void tst_QAudioDeviceInfo::checkAvailableDefaultInput()
{
    // Only perform tests if audio input device exists!
    QList<QAudioDeviceInfo> devices = QMediaDevices::audioInputs();
    if (devices.size() > 0) {
        QVERIFY(!QMediaDevices::defaultAudioInput().isNull());
    }
}

void tst_QAudioDeviceInfo::checkAvailableDefaultOutput()
{
    // Only perform tests if audio input device exists!
    QList<QAudioDeviceInfo> devices = QMediaDevices::audioOutputs();
    if (devices.size() > 0) {
        QVERIFY(!QMediaDevices::defaultAudioOutput().isNull());
    }
}

void tst_QAudioDeviceInfo::channels()
{
    QVERIFY(device->minimumChannelCount() > 0);
    QVERIFY(device->maximumChannelCount() > device->minimumChannelCount());
}

void tst_QAudioDeviceInfo::sampleFormat()
{
    QList<QAudioFormat::SampleFormat> avail = device->supportedSampleFormats();
    QVERIFY(avail.size() > 0);
}

void tst_QAudioDeviceInfo::sampleRates()
{
    QVERIFY(device->minimumSampleRate() > 0);
    QVERIFY(device->maximumSampleRate() > device->minimumSampleRate());
}

void tst_QAudioDeviceInfo::isFormatSupported()
{
    QAudioFormat format;
    format.setSampleRate(44100);
    format.setChannelCount(2);
    format.setSampleFormat(QAudioFormat::Int16);

    // Should always be true for these format
    QVERIFY(device->isFormatSupported(format));
}

void tst_QAudioDeviceInfo::preferred()
{
    QAudioFormat format = device->preferredFormat();
    QVERIFY(format.isValid());
    QVERIFY(device->isFormatSupported(format));
}

// QAudioDeviceInfo's assignOperator method
void tst_QAudioDeviceInfo::assignOperator()
{
    QAudioDeviceInfo dev;
    QVERIFY(dev.id().isNull());
    QVERIFY(dev.isNull() == true);

    QList<QAudioDeviceInfo> devices = QMediaDevices::audioOutputs();
    QVERIFY(devices.size() > 0);
    QAudioDeviceInfo dev1(devices.at(0));
    dev = dev1;
    QVERIFY(dev.isNull() == false);
    QVERIFY(dev.id() == dev1.id());
}

void tst_QAudioDeviceInfo::id()
{
    QVERIFY(!device->id().isNull());
    QVERIFY(device->id() == QMediaDevices::audioOutputs().at(0).id());
}

// QAudioDeviceInfo's defaultConstructor method
void tst_QAudioDeviceInfo::defaultConstructor()
{
    QAudioDeviceInfo dev;
    QVERIFY(dev.isNull() == true);
    QVERIFY(dev.id().isNull());
}

void tst_QAudioDeviceInfo::equalityOperator()
{
    // Get some default device infos
    QAudioDeviceInfo dev1;
    QAudioDeviceInfo dev2;

    QVERIFY(dev1 == dev2);
    QVERIFY(!(dev1 != dev2));

    // Make sure each available device is not equal to null
    const auto infos = QMediaDevices::audioOutputs();
    for (const QAudioDeviceInfo &info : infos) {
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

QTEST_MAIN(tst_QAudioDeviceInfo)

#include "tst_qaudiodeviceinfo.moc"
