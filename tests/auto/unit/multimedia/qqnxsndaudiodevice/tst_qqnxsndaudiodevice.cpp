// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>

#include <QtMultimedia/qaudiodevice.h>
#include <QtMultimedia/qaudioformat.h>
#include <QtMultimedia/private/qaudiodevice_p.h>
#include <QtMultimedia/private/qqnxsndaudiodevice_p.h>

QT_USE_NAMESPACE

// NOLINTBEGIN(readability-convert-member-functions-to-static)

class tst_QQnxSndAudioDevice : public QObject
{
    Q_OBJECT

private:
    static QAudioDevice createDevice(const QByteArray &id, const QString &desc,
                                     QAudioDevice::Mode mode);

private slots:
    void outputPreferredFormat();
    void inputPreferredFormat();
    void supportedSampleRateRange();
    void supportedChannelCountRange();
    void supportedSampleFormats();
    void deviceIdAndDescription();
    void isNotDefault();
    void mode_data();
    void mode();
};

QAudioDevice tst_QQnxSndAudioDevice::createDevice(const QByteArray &id, const QString &desc,
                                                   QAudioDevice::Mode mode)
{
    return QAudioDevicePrivate::createQAudioDevice(
            std::make_unique<QQnxSndAudioDeviceInfo>(id, desc, mode));
}

// QQnxSndAudioDeviceInfo probes the real device (opening the PCM and querying its
// hw_params) to report its capabilities, so the reported format is target-specific.
// To keep these unit tests hermetic and portable, they construct devices with a
// non-openable id, which deterministically exercises the conservative fallback
// format. End-to-end probing against real hardware is covered by tst_qaudiosink.
static constexpr auto kOfflineDeviceId = "hw:offline_unit_test";

void tst_QQnxSndAudioDevice::outputPreferredFormat()
{
    QAudioDevice dev = createDevice(kOfflineDeviceId, "Test Output", QAudioDevice::Output);
    QAudioFormat fmt = dev.preferredFormat();

    QCOMPARE(fmt.channelCount(), 2);
    QCOMPARE(fmt.sampleRate(), 48000);
    QCOMPARE(fmt.sampleFormat(), QAudioFormat::Float);
}

void tst_QQnxSndAudioDevice::inputPreferredFormat()
{
    QAudioDevice dev = createDevice(kOfflineDeviceId, "Test Input", QAudioDevice::Input);
    QAudioFormat fmt = dev.preferredFormat();

    QCOMPARE(fmt.channelCount(), 1);
    QCOMPARE(fmt.sampleRate(), 48000);
    QCOMPARE(fmt.sampleFormat(), QAudioFormat::Float);
}

void tst_QQnxSndAudioDevice::supportedSampleRateRange()
{
    QAudioDevice dev = createDevice(kOfflineDeviceId, "Test", QAudioDevice::Output);

    QCOMPARE(dev.minimumSampleRate(), 8000);
    QCOMPARE(dev.maximumSampleRate(), 48000);
}

void tst_QQnxSndAudioDevice::supportedChannelCountRange()
{
    QAudioDevice dev = createDevice(kOfflineDeviceId, "Test", QAudioDevice::Output);

    QCOMPARE(dev.minimumChannelCount(), 1);
    QCOMPARE(dev.maximumChannelCount(), 2);
}

void tst_QQnxSndAudioDevice::supportedSampleFormats()
{
    QAudioDevice dev = createDevice(kOfflineDeviceId, "Test", QAudioDevice::Output);
    QList<QAudioFormat::SampleFormat> formats = dev.supportedSampleFormats();

    QCOMPARE(formats.size(), 4);
    QVERIFY(formats.contains(QAudioFormat::UInt8));
    QVERIFY(formats.contains(QAudioFormat::Int16));
    QVERIFY(formats.contains(QAudioFormat::Int32));
    QVERIFY(formats.contains(QAudioFormat::Float));
}

void tst_QQnxSndAudioDevice::deviceIdAndDescription()
{
    QAudioDevice dev = createDevice("hw:pcmPreferred", "Preferred PCM Device",
                                    QAudioDevice::Output);

    QCOMPARE(dev.id(), QByteArray("hw:pcmPreferred"));
    QCOMPARE(dev.description(), QStringLiteral("Preferred PCM Device"));
}

void tst_QQnxSndAudioDevice::isNotDefault()
{
    QAudioDevice dev = createDevice(kOfflineDeviceId, "Test", QAudioDevice::Output);

    QVERIFY(!dev.isDefault());
}

void tst_QQnxSndAudioDevice::mode_data()
{
    QTest::addColumn<QAudioDevice::Mode>("mode");
    QTest::addColumn<int>("expectedChannels");

    QTest::newRow("input") << QAudioDevice::Input << 1;
    QTest::newRow("output") << QAudioDevice::Output << 2;
}

void tst_QQnxSndAudioDevice::mode()
{
    QFETCH(QAudioDevice::Mode, mode);
    QFETCH(int, expectedChannels);

    QAudioDevice dev = createDevice(kOfflineDeviceId, "Test", mode);

    QCOMPARE(dev.mode(), mode);
    QCOMPARE(dev.preferredFormat().channelCount(), expectedChannels);

    // All other format properties are identical regardless of mode
    QCOMPARE(dev.preferredFormat().sampleRate(), 48000);
    QCOMPARE(dev.preferredFormat().sampleFormat(), QAudioFormat::Float);
    QCOMPARE(dev.minimumSampleRate(), 8000);
    QCOMPARE(dev.maximumSampleRate(), 48000);
    QCOMPARE(dev.minimumChannelCount(), 1);
    QCOMPARE(dev.maximumChannelCount(), 2);
}

// NOLINTEND(readability-convert-member-functions-to-static)

QTEST_MAIN(tst_QQnxSndAudioDevice)

#include "tst_qqnxsndaudiodevice.moc"
