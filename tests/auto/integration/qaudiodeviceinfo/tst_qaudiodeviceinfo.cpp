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
#include <QMediaDeviceManager>

//TESTED_COMPONENT=src/multimedia

class tst_QAudioDeviceInfo : public QObject
{
    Q_OBJECT
public:
    tst_QAudioDeviceInfo(QObject* parent=nullptr) : QObject(parent) {}

private slots:
    void initTestCase();
    void checkAvailableDefaultInput();
    void checkAvailableDefaultOutput();
    void channels();
    void sampleSizes();
    void byteOrders();
    void sampleTypes();
    void sampleRates();
    void isFormatSupported();
    void preferred();
    void nearest();
    void supportedChannelCounts();
    void supportedSampleRates();
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
    QList<QAudioDeviceInfo> devices = QMediaDeviceManager::audioOutputs();
    if (devices.size() == 0) {
        QSKIP("NOTE: no audio output device found, no tests will be performed");
    } else {
        device = new QAudioDeviceInfo(devices.at(0));
    }
}

void tst_QAudioDeviceInfo::checkAvailableDefaultInput()
{
    QMediaDeviceManager *manager = QMediaDeviceManager::instance();
    // Only perform tests if audio input device exists!
    QList<QAudioDeviceInfo> devices = manager->audioInputs();
    if (devices.size() > 0) {
        QVERIFY(!manager->defaultAudioInput().isNull());
    }
}

void tst_QAudioDeviceInfo::checkAvailableDefaultOutput()
{
    QMediaDeviceManager *manager = QMediaDeviceManager::instance();
    // Only perform tests if audio input device exists!
    QList<QAudioDeviceInfo> devices = manager->audioOutputs();
    if (devices.size() > 0) {
        QVERIFY(!manager->defaultAudioOutput().isNull());
    }
}

void tst_QAudioDeviceInfo::channels()
{
    QList<int> avail = device->supportedChannelCounts();
    QVERIFY(avail.size() > 0);
}

void tst_QAudioDeviceInfo::sampleSizes()
{
    QList<int> avail = device->supportedSampleSizes();
    QVERIFY(avail.size() > 0);
}

void tst_QAudioDeviceInfo::byteOrders()
{
    QList<QAudioFormat::Endian> avail = device->supportedByteOrders();
    QVERIFY(avail.size() > 0);
}

void tst_QAudioDeviceInfo::sampleTypes()
{
    QList<QAudioFormat::SampleType> avail = device->supportedSampleTypes();
    QVERIFY(avail.size() > 0);
}

void tst_QAudioDeviceInfo::sampleRates()
{
    QList<int> avail = device->supportedSampleRates();
    QVERIFY(avail.size() > 0);
}

void tst_QAudioDeviceInfo::isFormatSupported()
{
    QAudioFormat format;
    format.setSampleRate(44100);
    format.setChannelCount(2);
    format.setSampleType(QAudioFormat::SignedInt);
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleSize(16);

    // Should always be true for these format
    QVERIFY(device->isFormatSupported(format));
}

void tst_QAudioDeviceInfo::preferred()
{
    QAudioFormat format = device->preferredFormat();
    QVERIFY(format.isValid());
    QVERIFY(device->isFormatSupported(format));
    QVERIFY(device->nearestFormat(format) == format);
}

// Returns closest QAudioFormat to settings that system audio supports.
void tst_QAudioDeviceInfo::nearest()
{
    /*
    QAudioFormat format1, format2;
    format1.setSampleRate(8000);
    format2 = device->nearestFormat(format1);
    QVERIFY(format2.sampleRate() == 44100);
    */
    QAudioFormat format;
    format.setSampleRate(44100);
    format.setChannelCount(2);
    format.setSampleType(QAudioFormat::SignedInt);
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleSize(16);

    QAudioFormat format2 = device->nearestFormat(format);

    // This is definitely dependent on platform support (but isFormatSupported tests that above)
    QVERIFY(format2.sampleRate() == 44100);
}

// Returns a list of supported channel counts.
void tst_QAudioDeviceInfo::supportedChannelCounts()
{
    QList<int> avail = device->supportedChannelCounts();
    QVERIFY(avail.size() > 0);
}

// Returns a list of supported sample rates.
void tst_QAudioDeviceInfo::supportedSampleRates()
{
    QList<int> avail = device->supportedSampleRates();
    QVERIFY(avail.size() > 0);
}

// QAudioDeviceInfo's assignOperator method
void tst_QAudioDeviceInfo::assignOperator()
{
    QAudioDeviceInfo dev;
    QVERIFY(dev.id().isNull());
    QVERIFY(dev.isNull() == true);

    QList<QAudioDeviceInfo> devices = QMediaDeviceManager::audioOutputs();
    QVERIFY(devices.size() > 0);
    QAudioDeviceInfo dev1(devices.at(0));
    dev = dev1;
    QVERIFY(dev.isNull() == false);
    QVERIFY(dev.id() == dev1.id());
}

void tst_QAudioDeviceInfo::id()
{
    QVERIFY(!device->id().isNull());
    QVERIFY(device->id() == QMediaDeviceManager::audioOutputs().at(0).id());
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
    const auto infos = QMediaDeviceManager::audioOutputs();
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
