/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "audiodevices.h"
#include <qmediadevicemanager.h>

// Utility functions for converting QAudioFormat fields into text

static QString toString(QAudioFormat::SampleFormat sampleFormat)
{
    switch (sampleFormat) {
    case QAudioFormat::Unknown:
    case QAudioFormat::NSampleFormats:
        return "Unknown";
    case QAudioFormat::UInt8:
        return "Unsigned 8 bit";
    case QAudioFormat::Int16:
        return "Signed 16 bit";
    case QAudioFormat::Int32:
        return "Signed 32 bit";
    case QAudioFormat::Float:
        return "Float";
    }
}

AudioDevicesBase::AudioDevicesBase(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi(this);
}

AudioDevicesBase::~AudioDevicesBase() = default;


AudioTest::AudioTest(QWidget *parent)
    : AudioDevicesBase(parent)
{
    connect(testButton, &QPushButton::clicked, this, &AudioTest::test);
    connect(modeBox, QOverload<int>::of(&QComboBox::activated), this, &AudioTest::modeChanged);
    connect(deviceBox, QOverload<int>::of(&QComboBox::activated), this, &AudioTest::deviceChanged);
    connect(sampleRateSpinBox, &QSpinBox::valueChanged, this, &AudioTest::sampleRateChanged);
    connect(channelsSpinBox, &QSpinBox::valueChanged, this, &AudioTest::channelChanged);
    connect(sampleFormatBox, QOverload<int>::of(&QComboBox::activated), this, &AudioTest::sampleFormatChanged);
    connect(populateTableButton, &QPushButton::clicked, this, &AudioTest::populateTable);
    connect(QMediaDeviceManager::instance(), &QMediaDeviceManager::audioInputsChanged, this, &AudioTest::updateAudioDevices);
    connect(QMediaDeviceManager::instance(), &QMediaDeviceManager::audioOutputsChanged, this, &AudioTest::updateAudioDevices);

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
    }
    else
        testResult->setText(tr("No Device"));
}

void AudioTest::updateAudioDevices()
{
    deviceBox->clear();
    auto *deviceManager = QMediaDeviceManager::instance();
    const auto devices = m_mode == QAudio::AudioInput ? deviceManager->audioInputs() : deviceManager->audioOutputs();
    for (auto &deviceInfo: devices)
        deviceBox->addItem(deviceInfo.description(), QVariant::fromValue(deviceInfo));
}


void AudioTest::modeChanged(int idx)
{
    testResult->clear();
    m_mode = idx == 0 ? QAudio::AudioInput : QAudio::AudioOutput;
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
    m_deviceInfo = deviceBox->itemData(idx).value<QAudioDeviceInfo>();

    sampleRateSpinBox->clear();
    auto sampleRates = m_deviceInfo.supportedSampleRates();
    sampleRateSpinBox->setMinimum(sampleRates.minimum);
    sampleRateSpinBox->setMaximum(sampleRates.maximum);
    int sampleValue = qBound(sampleRates.minimum, 48000, sampleRates.maximum);
    sampleRateSpinBox->setValue(sampleValue);
    m_settings.setSampleRate(sampleValue);

    channelsSpinBox->clear();
    auto channelRates = m_deviceInfo.supportedChannelCounts();
    channelsSpinBox->setMinimum(channelRates.minimum);
    channelsSpinBox->setMaximum(channelRates.maximum);
    int channelValue = qBound(channelRates.minimum, 2, channelRates.maximum);
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

        auto sampleRates = m_deviceInfo.supportedSampleRates();
        QTableWidgetItem *sampleRateItem = new QTableWidgetItem(QString("%1 - %2").arg(sampleRates.minimum).arg(sampleRates.maximum));
        allFormatsTable->setItem(row, 0, sampleRateItem);

        auto channels = m_deviceInfo.supportedChannelCounts();
        QTableWidgetItem *channelsItem = new QTableWidgetItem(QString("%1 - %2").arg(channels.minimum).arg(channels.maximum));
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
