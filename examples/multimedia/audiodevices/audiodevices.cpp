// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "audiodevices.h"

#include <QMediaDevices>
#include <QMediaFormat>

#if QT_CONFIG(permissions)
  #include <QPermission>
#endif

#include <bitset>

using namespace Qt::Literals;

// Utility functions for converting QAudioFormat fields into text

static QString toString(QAudioFormat::SampleFormat sampleFormat)
{
    switch (sampleFormat) {
    case QAudioFormat::UInt8:
        return u"Unsigned 8 bit"_s;
    case QAudioFormat::Int16:
        return u"Signed 16 bit"_s;
    case QAudioFormat::Int32:
        return u"Signed 32 bit"_s;
    case QAudioFormat::Float:
        return u"Float"_s;
    default:
        return u"Unknown"_s;
    }
}

static QString toString(QAudioFormat::ChannelConfig channelConfig)
{
    switch (channelConfig) {
    case QAudioFormat::ChannelConfigMono:
        return u"Mono"_s;
    case QAudioFormat::ChannelConfigStereo:
        return u"Stereo"_s;
    case QAudioFormat::ChannelConfig2Dot1:
        return u"2.1"_s;
    case QAudioFormat::ChannelConfig3Dot0:
        return u"3.0"_s;
    case QAudioFormat::ChannelConfigSurround5Dot0:
        return u"5.0 Surround"_s;
    case QAudioFormat::ChannelConfigSurround5Dot1:
        return u"5.1 Surround"_s;
    case QAudioFormat::ChannelConfigSurround7Dot0:
        return u"7.0 Surround"_s;
    case QAudioFormat::ChannelConfigSurround7Dot1:
        return u"7.1 Surround"_s;
    default:
        break;
    }

    static auto channelLabels = std::array{
        u"UnknownPosition",
        u"FrontLeft",
        u"FrontRight",
        u"FrontCenter",
        u"LFE",
        u"BackLeft",
        u"BackRight",
        u"FrontLeftOfCenter",
        u"FrontRightOfCenter",
        u"BackCenter",
        u"SideLeft",
        u"SideRight",
        u"TopCenter",
        u"TopFrontLeft",
        u"TopFrontCenter",
        u"TopFrontRight",
        u"TopBackLeft",
        u"TopBackCenter",
        u"TopBackRight",
        u"LFE2",
        u"TopSideLeft",
        u"TopSideRight",
        u"BottomFrontCenter",
        u"BottomFrontLeft",
        u"BottomFrontRight",
    };

    QStringList labels;
    std::bitset<QAudioFormat::NChannelPositions> configBits(channelConfig);
    for (size_t i = 0; i < configBits.size(); ++i) {
        if (configBits.test(i))
            labels.emplace_back(channelLabels[i]);
    }
    return labels.join(", ");
}

AudioDevicesBase::AudioDevicesBase(QWidget *parent) : QMainWindow(parent)
{
    setupUi(this);
}

AudioDevicesBase::~AudioDevicesBase() = default;

AudioDevices::AudioDevices(QWidget *parent)
    : AudioDevicesBase(parent), m_devices(new QMediaDevices(this))
{
    init();
}

void AudioDevices::updateDevicePropertes()
{
    QStringList sampleFormats;
    for (auto format : m_deviceInfo.supportedSampleFormats())
        sampleFormats << toString(format);
    sampleFormatField->setText(sampleFormats.join(", "));

    if (m_deviceInfo.minimumChannelCount() != m_deviceInfo.maximumChannelCount()) {
        QString channelRange = u"%1 - %2"_s.arg(m_deviceInfo.minimumChannelCount())
                                       .arg(m_deviceInfo.maximumChannelCount());
        channelNumberField->setText(channelRange);
    } else {
        channelNumberField->setText(QString::number(m_deviceInfo.minimumChannelCount()));
    }

    QString sampleRateRange = u"%1 - %2 Hz"_s.arg(m_deviceInfo.minimumSampleRate())
                                      .arg(m_deviceInfo.maximumSampleRate());
    samplingRatesField->setText(sampleRateRange);

    QString channelConfig = toString(m_deviceInfo.channelConfiguration());
    channelLayoutField->setText(channelConfig);

    QString preferredFormat = u"%1, %2 Hz, %3 channels (%4)"_s.arg(
            toString(m_deviceInfo.preferredFormat().sampleFormat()),
            QString::number(m_deviceInfo.preferredFormat().sampleRate()),
            QString::number(m_deviceInfo.preferredFormat().channelCount()),
            toString(m_deviceInfo.preferredFormat().channelConfig()));
    preferredFormatField->setText(preferredFormat);

    for (QLineEdit *field : { samplingRatesField, channelNumberField, sampleFormatField,
                              channelLayoutField, preferredFormatField })
        field->setCursorPosition(0);
}

void AudioDevices::init()
{
#if QT_CONFIG(permissions)
    // microphone
    QMicrophonePermission microphonePermission;
    switch (qApp->checkPermission(microphonePermission)) {
    case Qt::PermissionStatus::Undetermined:
        qApp->requestPermission(microphonePermission, this, &AudioDevices::init);
        return;
    case Qt::PermissionStatus::Denied:
        qWarning("Microphone permission is not granted!");
        return;
    case Qt::PermissionStatus::Granted:
        break;
    }
#endif
    connect(modeBox, QOverload<int>::of(&QComboBox::activated), this, &AudioDevices::modeChanged);
    connect(deviceBox, QOverload<int>::of(&QComboBox::activated), this,
            &AudioDevices::deviceChanged);
    connect(m_devices, &QMediaDevices::audioInputsChanged, this, &AudioDevices::updateAudioDevices);
    connect(m_devices, &QMediaDevices::audioOutputsChanged, this,
            &AudioDevices::updateAudioDevices);

    modeBox->setCurrentIndex(1);
    updateAudioDevices();
    updateDevicePropertes();
}

void AudioDevices::updateAudioDevices()
{
    QSignalBlocker blockUpdates(deviceBox);
    const auto devices =
            m_mode == QAudioDevice::Input ? m_devices->audioInputs() : m_devices->audioOutputs();
    {
        deviceBox->clear();
        for (auto &deviceInfo : devices) {
            QString description = deviceInfo.description();
            description.replace(u"\n"_s, u" - "_s);
            if (deviceInfo.isDefault())
                description += " (default)";

            deviceBox->addItem(description, QVariant::fromValue(deviceInfo));
        }
    }

    // select previously selected
    for (int index = 0; index != devices.size(); ++index) {
        if (devices[index].id() == m_deviceInfo.id()) {
            deviceBox->setCurrentIndex(index);
            deviceChanged(index);
            return;
        }
    }

    // select default
    for (int index = 0; index != devices.size(); ++index) {
        if (devices[index].isDefault()) {
            deviceBox->setCurrentIndex(index);
            deviceChanged(index);
            m_deviceInfo = devices[index];
            return;
        }
    }
}

void AudioDevices::modeChanged(int idx)
{
    m_mode = idx == 0 ? QAudioDevice::Input : QAudioDevice::Output;
    updateAudioDevices();
    deviceBox->setCurrentIndex(0);
    deviceChanged(0);
}

void AudioDevices::deviceChanged(int idx)
{
    if (deviceBox->count() == 0) {
        channelNumberField->clear();
        sampleFormatField->clear();
        samplingRatesField->clear();
        channelLayoutField->clear();
        return;
    }

    // device has changed
    m_deviceInfo = deviceBox->itemData(idx).value<QAudioDevice>();

    updateDevicePropertes();
}

#include "moc_audiodevices.cpp"
