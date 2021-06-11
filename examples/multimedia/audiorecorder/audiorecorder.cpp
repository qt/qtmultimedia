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

#include "audiorecorder.h"
#include "audiolevel.h"

#include "ui_audiorecorder.h"

#include <QMediaEncoder>
#include <QDir>
#include <QFileDialog>
#include <QMediaEncoder>
#include <QStandardPaths>
#include <qmediadevices.h>
#include <qaudiodevice.h>
#include <qaudiobuffer.h>
#include <qaudioinput.h>
#include <qcameraimagecapture.h>

static QList<qreal> getBufferLevels(const QAudioBuffer &buffer);

AudioRecorder::AudioRecorder()
    : ui(new Ui::AudioRecorder)
{
    ui->setupUi(this);

    m_audioEncoder = new QMediaEncoder(this);
    m_captureSession.setEncoder(m_audioEncoder);
    m_captureSession.setAudioInput(new QAudioInput(this));
    // ### replace with a monitoring output once we have it.
//    m_probe = new QAudioProbe(this);
//    connect(m_probe, &QAudioProbe::audioBufferProbed,
//            this, &AudioRecorder::processBuffer);
//    m_probe->setSource(m_audioRecorder);

    //audio devices
    ui->audioDeviceBox->addItem(tr("Default"), QVariant(QString()));
    for (auto device: QMediaDevices::audioInputs()) {
        auto name = device.description();
        ui->audioDeviceBox->addItem(name, QVariant::fromValue(device));
    }

    //audio codecs
    ui->audioCodecBox->addItem(tr("Default"), QVariant(QString()));
    for (auto &codec : QMediaFormat().supportedAudioCodecs(QMediaFormat::Encode))
        ui->audioCodecBox->addItem(QMediaFormat::audioCodecDescription(codec), QVariant::fromValue(codec));

    //containers
    ui->containerBox->addItem(tr("Default"), QVariant(QString()));
    for (auto &container : QMediaFormat().supportedFileFormats(QMediaFormat::Encode)) {
        if (container < QMediaFormat::AAC) // ### Somewhat hacky, skip video formats
            continue;
        ui->containerBox->addItem(QMediaFormat::fileFormatDescription(container), QVariant::fromValue(container));
    }

    //sample rate
    ui->sampleRateBox->setRange(m_captureSession.audioInput()->device().minimumSampleRate(),
                                m_captureSession.audioInput()->device().maximumSampleRate());
    ui->sampleRateBox->setValue(qBound(m_captureSession.audioInput()->device().minimumSampleRate(), 44100,
                                       m_captureSession.audioInput()->device().maximumSampleRate()));

    //channels
    ui->channelsBox->addItem(tr("Default"), QVariant(-1));
    ui->channelsBox->addItem(QStringLiteral("1"), QVariant(1));
    ui->channelsBox->addItem(QStringLiteral("2"), QVariant(2));
    ui->channelsBox->addItem(QStringLiteral("4"), QVariant(4));

    //quality
    ui->qualitySlider->setRange(0, int(QCameraImageCapture::VeryHighQuality));
    ui->qualitySlider->setValue(int(QCameraImageCapture::NormalQuality));

    //bitrates:
    ui->bitrateBox->addItem(tr("Default"), QVariant(0));
    ui->bitrateBox->addItem(QStringLiteral("32000"), QVariant(32000));
    ui->bitrateBox->addItem(QStringLiteral("64000"), QVariant(64000));
    ui->bitrateBox->addItem(QStringLiteral("96000"), QVariant(96000));
    ui->bitrateBox->addItem(QStringLiteral("128000"), QVariant(128000));

    connect(m_audioEncoder, &QMediaEncoder::durationChanged, this, &AudioRecorder::updateProgress);
    connect(m_audioEncoder, &QMediaEncoder::statusChanged, this, &AudioRecorder::updateStatus);
    connect(m_audioEncoder, &QMediaEncoder::stateChanged, this, &AudioRecorder::onStateChanged);
    connect(m_audioEncoder, &QMediaEncoder::errorChanged, this, &AudioRecorder::displayErrorMessage);
}

void AudioRecorder::updateProgress(qint64 duration)
{
    if (m_audioEncoder->error() != QMediaEncoder::NoError || duration < 2000)
        return;

    ui->statusbar->showMessage(tr("Recorded %1 sec").arg(duration / 1000));
}

void AudioRecorder::updateStatus(QMediaEncoder::Status status)
{
    QString statusMessage;

    switch (status) {
    case QMediaEncoder::RecordingStatus:
        statusMessage = tr("Recording to %1").arg(m_audioEncoder->actualLocation().toString());
        break;
    case QMediaEncoder::PausedStatus:
        clearAudioLevels();
        statusMessage = tr("Paused");
        break;
    case QMediaEncoder::StoppedStatus:
        clearAudioLevels();
        statusMessage = tr("Stopped");
    default:
        break;
    }

    if (m_audioEncoder->error() == QMediaEncoder::NoError)
        ui->statusbar->showMessage(statusMessage);
}

void AudioRecorder::onStateChanged(QMediaEncoder::State state)
{
    switch (state) {
    case QMediaEncoder::RecordingState:
        ui->recordButton->setText(tr("Stop"));
        ui->pauseButton->setText(tr("Pause"));
        break;
    case QMediaEncoder::PausedState:
        ui->recordButton->setText(tr("Stop"));
        ui->pauseButton->setText(tr("Resume"));
        break;
    case QMediaEncoder::StoppedState:
        ui->recordButton->setText(tr("Record"));
        ui->pauseButton->setText(tr("Pause"));
        break;
    }

    ui->pauseButton->setEnabled(m_audioEncoder->state() != QMediaEncoder::StoppedState);
}

static QVariant boxValue(const QComboBox *box)
{
    int idx = box->currentIndex();
    if (idx == -1)
        return QVariant();

    return box->itemData(idx);
}

void AudioRecorder::toggleRecord()
{
    if (m_audioEncoder->state() == QMediaEncoder::StoppedState) {
        m_captureSession.audioInput()->setDevice(boxValue(ui->audioDeviceBox).value<QAudioDevice>());

        QMediaEncoderSettings settings;
        settings.setFileFormat(boxValue(ui->containerBox).value<QMediaFormat::FileFormat>());
        settings.setAudioCodec(boxValue(ui->audioCodecBox).value<QMediaFormat::AudioCodec>());
        settings.setAudioSampleRate(ui->sampleRateBox->value());
        settings.setAudioBitRate(boxValue(ui->bitrateBox).toInt());
        settings.setAudioChannelCount(boxValue(ui->channelsBox).toInt());
        settings.setQuality(QMediaEncoderSettings::Quality(ui->qualitySlider->value()));
        settings.setEncodingMode(ui->constantQualityRadioButton->isChecked() ?
                                 QMediaEncoderSettings::ConstantQualityEncoding :
                                 QMediaEncoderSettings::ConstantBitRateEncoding);

        m_audioEncoder->setEncoderSettings(settings);
        m_audioEncoder->record();
    }
    else {
        m_audioEncoder->stop();
    }
}

void AudioRecorder::togglePause()
{
    if (m_audioEncoder->state() != QMediaEncoder::PausedState)
        m_audioEncoder->pause();
    else
        m_audioEncoder->record();
}

void AudioRecorder::setOutputLocation()
{
#ifdef Q_OS_WINRT
    // UWP does not allow to store outside the sandbox
    const QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (!QDir().mkpath(cacheDir)) {
        qWarning() << "Failed to create cache directory";
        return;
    }
    QString fileName = cacheDir + QLatin1String("/output.wav");
#else
    QString fileName = QFileDialog::getSaveFileName();
#endif
    m_audioEncoder->setOutputLocation(QUrl::fromLocalFile(fileName));
    m_outputLocationSet = true;
}

void AudioRecorder::displayErrorMessage()
{
    ui->statusbar->showMessage(m_audioEncoder->errorString());
}

void AudioRecorder::clearAudioLevels()
{
    for (auto m_audioLevel : qAsConst(m_audioLevels))
        m_audioLevel->setLevel(0);
}

// returns the audio level for each channel
QList<qreal> getBufferLevels(const QAudioBuffer &buffer)
{
    QList<qreal> values;

    auto format = buffer.format();
    if (!format.isValid())
        return values;

    int channels = buffer.format().channelCount();
    values.fill(0, channels);

    int bytesPerSample = format.bytesPerSample();
    QList<qreal> max_values;
    max_values.fill(0, channels);

    const char *data = buffer.constData<char>();
    for (int i = 0; i < buffer.frameCount(); ++i) {
        for (int j = 0; j < channels; ++j) {
            qreal value = qAbs(format.normalizedSampleValue(data));
            if (value > max_values.at(j))
                max_values[j] = value;
            data += bytesPerSample;
        }
    }

    return max_values;
}

void AudioRecorder::processBuffer(const QAudioBuffer& buffer)
{
    if (m_audioLevels.count() != buffer.format().channelCount()) {
        qDeleteAll(m_audioLevels);
        m_audioLevels.clear();
        for (int i = 0; i < buffer.format().channelCount(); ++i) {
            AudioLevel *level = new AudioLevel(ui->centralwidget);
            m_audioLevels.append(level);
            ui->levelsLayout->addWidget(level);
        }
    }

    QList<qreal> levels = getBufferLevels(buffer);
    for (int i = 0; i < levels.count(); ++i)
        m_audioLevels.at(i)->setLevel(levels.at(i));
}
