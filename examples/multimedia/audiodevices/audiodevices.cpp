// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "audiodevices.h"

#include <QCameraDevice>
#include <QMediaDevices>
#include <QMediaFormat>

#if QT_CONFIG(permissions)
  #include <QPermission>
#endif

// Utility functions for converting QAudioFormat fields into text

static QString toString(QAudioFormat::SampleFormat sampleFormat)
{
    switch (sampleFormat) {
    case QAudioFormat::UInt8:
        return "Unsigned 8 bit";
    case QAudioFormat::Int16:
        return "Signed 16 bit";
    case QAudioFormat::Int32:
        return "Signed 32 bit";
    case QAudioFormat::Float:
        return "Float";
    default:
        return "Unknown";
    }
}

AudioDevicesBase::AudioDevicesBase(QWidget *parent) : QMainWindow(parent)
{
    setupUi(this);
}

AudioDevicesBase::~AudioDevicesBase() = default;

AudioTest::AudioTest(QWidget *parent) : AudioDevicesBase(parent), m_devices(new QMediaDevices(this))
{
    init();
}

void AudioTest::init()
{
#if QT_CONFIG(permissions)
    // camera
    QCameraPermission cameraPermission;
    switch (qApp->checkPermission(cameraPermission)) {
    case Qt::PermissionStatus::Undetermined:
        qApp->requestPermission(cameraPermission, this, &AudioTest::init);
        return;
    case Qt::PermissionStatus::Denied:
        qWarning("Camera permission is not granted!");
        return;
    case Qt::PermissionStatus::Granted:
        break;
    }
    // microphone
    QMicrophonePermission microphonePermission;
    switch (qApp->checkPermission(microphonePermission)) {
    case Qt::PermissionStatus::Undetermined:
        qApp->requestPermission(microphonePermission, this, &AudioTest::init);
        return;
    case Qt::PermissionStatus::Denied:
        qWarning("Microphone permission is not granted!");
        return;
    case Qt::PermissionStatus::Granted:
        break;
    }
#endif
    m_devices->videoInputs();
    qDebug() << "<<<<<<<<<<<<<<<<<<";
    QMediaFormat().supportedFileFormats(QMediaFormat::Encode);
    connect(testButton, &QPushButton::clicked, this, &AudioTest::test);
    connect(modeBox, QOverload<int>::of(&QComboBox::activated), this, &AudioTest::modeChanged);
    connect(deviceBox, QOverload<int>::of(&QComboBox::activated), this, &AudioTest::deviceChanged);
    connect(sampleRateSpinBox, &QSpinBox::valueChanged, this, &AudioTest::sampleRateChanged);
    connect(channelsSpinBox, &QSpinBox::valueChanged, this, &AudioTest::channelChanged);
    connect(sampleFormatBox, QOverload<int>::of(&QComboBox::activated), this,
            &AudioTest::sampleFormatChanged);
    connect(populateTableButton, &QPushButton::clicked, this, &AudioTest::populateTable);
    connect(m_devices, &QMediaDevices::audioInputsChanged, this, &AudioTest::updateAudioDevices);
    connect(m_devices, &QMediaDevices::audioOutputsChanged, this, &AudioTest::updateAudioDevices);

    modeBox->setCurrentIndex(0);
    modeChanged(0);
    deviceBox->setCurrentIndex(0);
    deviceChanged(0);
}

void AudioTest::test()
{
    // tries to set all the settings picked.
    testResult->clear();

    if (!m_deviceInfo.isNull()) {
        if (m_deviceInfo.isFormatSupported(m_settings)) {
            testResult->setText(tr("Success"));
            nearestSampleRate->setText("");
            nearestChannel->setText("");
            nearestSampleFormat->setText("");
        } else {
            QAudioFormat nearest = m_deviceInfo.preferredFormat();
            testResult->setText(tr("Failed"));
            nearestSampleRate->setText(QString("%1").arg(nearest.sampleRate()));
            nearestChannel->setText(QString("%1").arg(nearest.channelCount()));
            nearestSampleFormat->setText(toString(nearest.sampleFormat()));
        }
    } else
        testResult->setText(tr("No Device"));
}

void AudioTest::updateAudioDevices()
{
    deviceBox->clear();
    const auto devices =
            m_mode == QAudioDevice::Input ? m_devices->audioInputs() : m_devices->audioOutputs();
    for (auto &deviceInfo : devices)
        deviceBox->addItem(deviceInfo.description(), QVariant::fromValue(deviceInfo));
}

void AudioTest::modeChanged(int idx)
{
    testResult->clear();
    m_mode = idx == 0 ? QAudioDevice::Input : QAudioDevice::Output;
    updateAudioDevices();
    deviceBox->setCurrentIndex(0);
    deviceChanged(0);
}

void AudioTest::deviceChanged(int idx)
{
    testResult->clear();

    if (deviceBox->count() == 0)
        return;

    // device has changed
    m_deviceInfo = deviceBox->itemData(idx).value<QAudioDevice>();

    sampleRateSpinBox->clear();
    sampleRateSpinBox->setMinimum(m_deviceInfo.minimumSampleRate());
    sampleRateSpinBox->setMaximum(m_deviceInfo.maximumSampleRate());
    int sampleValue =
            qBound(m_deviceInfo.minimumSampleRate(), 48000, m_deviceInfo.maximumSampleRate());
    sampleRateSpinBox->setValue(sampleValue);
    m_settings.setSampleRate(sampleValue);

    channelsSpinBox->clear();
    channelsSpinBox->setMinimum(m_deviceInfo.minimumChannelCount());
    channelsSpinBox->setMaximum(m_deviceInfo.maximumChannelCount());
    int channelValue =
            qBound(m_deviceInfo.minimumChannelCount(), 2, m_deviceInfo.maximumChannelCount());
    channelsSpinBox->setValue(channelValue);

    sampleFormatBox->clear();
    const QList<QAudioFormat::SampleFormat> sampleFormats = m_deviceInfo.supportedSampleFormats();
    for (auto sampleFormat : sampleFormats)
        sampleFormatBox->addItem(toString(sampleFormat));
    if (sampleFormats.size())
        m_settings.setSampleFormat(sampleFormats.at(0));

    allFormatsTable->clearContents();
}

void AudioTest::populateTable()
{
    int row = 0;

    for (auto sampleFormat : m_deviceInfo.supportedSampleFormats()) {
        allFormatsTable->setRowCount(row + 1);

        QTableWidgetItem *sampleTypeItem = new QTableWidgetItem(toString(sampleFormat));
        allFormatsTable->setItem(row, 2, sampleTypeItem);

        QTableWidgetItem *sampleRateItem =
                new QTableWidgetItem(QString("%1 - %2")
                                             .arg(m_deviceInfo.minimumSampleRate())
                                             .arg(m_deviceInfo.maximumSampleRate()));
        allFormatsTable->setItem(row, 0, sampleRateItem);

        QTableWidgetItem *channelsItem =
                new QTableWidgetItem(QString("%1 - %2")
                                             .arg(m_deviceInfo.minimumChannelCount())
                                             .arg(m_deviceInfo.maximumChannelCount()));
        allFormatsTable->setItem(row, 1, channelsItem);

        ++row;
    }
}

void AudioTest::sampleRateChanged(int value)
{
    // sample rate has changed
    m_settings.setSampleRate(value);
}

void AudioTest::channelChanged(int channels)
{
    m_settings.setChannelCount(channels);
}

void AudioTest::sampleFormatChanged(int idx)
{
    auto formats = m_deviceInfo.supportedSampleFormats();
    m_settings.setSampleFormat(formats.at(idx));
}

#include "moc_audiodevices.cpp"
