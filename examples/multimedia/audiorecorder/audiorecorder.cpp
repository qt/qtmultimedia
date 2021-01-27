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

#include <QMediaRecorder>
#include <QDir>
#include <QFileDialog>
#include <QMediaRecorder>
#include <QStandardPaths>
#include <qmediadevicemanager.h>
#include <qaudiodeviceinfo.h>
#include <qaudiobuffer.h>

static QList<qreal> getBufferLevels(const QAudioBuffer &buffer);

AudioRecorder::AudioRecorder()
    : ui(new Ui::AudioRecorder)
{
    ui->setupUi(this);

    m_audioRecorder = new QMediaRecorder(QMediaRecorder::AudioOnly, this);
    // ### replace with a monitoring output once we have it.
//    m_probe = new QAudioProbe(this);
//    connect(m_probe, &QAudioProbe::audioBufferProbed,
//            this, &AudioRecorder::processBuffer);
//    m_probe->setSource(m_audioRecorder);

    QMediaDeviceManager *manager = QMediaDeviceManager::instance();

    //audio devices
    ui->audioDeviceBox->addItem(tr("Default"), QVariant(QString()));
    for (auto device: manager->audioInputs()) {
        auto name = device.description();
        ui->audioDeviceBox->addItem(name, QVariant::fromValue(device));
    }

    //audio codecs
    ui->audioCodecBox->addItem(tr("Default"), QVariant(QString()));
    for (auto &codecName: m_audioRecorder->supportedAudioCodecs()) {
        ui->audioCodecBox->addItem(codecName, QVariant(codecName));
    }

    //containers
    ui->containerBox->addItem(tr("Default"), QVariant(QString()));
    for (auto &containerName: m_audioRecorder->supportedContainers()) {
        ui->containerBox->addItem(containerName, QVariant(containerName));
    }

    //sample rate
    ui->sampleRateBox->addItem(tr("Default"), QVariant(0));
    for (int sampleRate: m_audioRecorder->supportedAudioSampleRates()) {
        ui->sampleRateBox->addItem(QString::number(sampleRate), QVariant(
                sampleRate));
    }

    //channels
    ui->channelsBox->addItem(tr("Default"), QVariant(-1));
    ui->channelsBox->addItem(QStringLiteral("1"), QVariant(1));
    ui->channelsBox->addItem(QStringLiteral("2"), QVariant(2));
    ui->channelsBox->addItem(QStringLiteral("4"), QVariant(4));

    //quality
    ui->qualitySlider->setRange(0, int(QMultimedia::VeryHighQuality));
    ui->qualitySlider->setValue(int(QMultimedia::NormalQuality));

    //bitrates:
    ui->bitrateBox->addItem(tr("Default"), QVariant(0));
    ui->bitrateBox->addItem(QStringLiteral("32000"), QVariant(32000));
    ui->bitrateBox->addItem(QStringLiteral("64000"), QVariant(64000));
    ui->bitrateBox->addItem(QStringLiteral("96000"), QVariant(96000));
    ui->bitrateBox->addItem(QStringLiteral("128000"), QVariant(128000));

    connect(m_audioRecorder, &QMediaRecorder::durationChanged, this, &AudioRecorder::updateProgress);
    connect(m_audioRecorder, &QMediaRecorder::statusChanged, this, &AudioRecorder::updateStatus);
    connect(m_audioRecorder, &QMediaRecorder::stateChanged, this, &AudioRecorder::onStateChanged);
    connect(m_audioRecorder, QOverload<QMediaRecorder::Error>::of(&QMediaRecorder::error), this,
            &AudioRecorder::displayErrorMessage);
}

void AudioRecorder::updateProgress(qint64 duration)
{
    if (m_audioRecorder->error() != QMediaRecorder::NoError || duration < 2000)
        return;

    ui->statusbar->showMessage(tr("Recorded %1 sec").arg(duration / 1000));
}

void AudioRecorder::updateStatus(QMediaRecorder::Status status)
{
    QString statusMessage;

    switch (status) {
    case QMediaRecorder::RecordingStatus:
        statusMessage = tr("Recording to %1").arg(m_audioRecorder->actualLocation().toString());
        break;
    case QMediaRecorder::PausedStatus:
        clearAudioLevels();
        statusMessage = tr("Paused");
        break;
    case QMediaRecorder::UnloadedStatus:
    case QMediaRecorder::LoadedStatus:
        clearAudioLevels();
        statusMessage = tr("Stopped");
    default:
        break;
    }

    if (m_audioRecorder->error() == QMediaRecorder::NoError)
        ui->statusbar->showMessage(statusMessage);
}

void AudioRecorder::onStateChanged(QMediaRecorder::State state)
{
    switch (state) {
    case QMediaRecorder::RecordingState:
        ui->recordButton->setText(tr("Stop"));
        ui->pauseButton->setText(tr("Pause"));
        break;
    case QMediaRecorder::PausedState:
        ui->recordButton->setText(tr("Stop"));
        ui->pauseButton->setText(tr("Resume"));
        break;
    case QMediaRecorder::StoppedState:
        ui->recordButton->setText(tr("Record"));
        ui->pauseButton->setText(tr("Pause"));
        break;
    }

    ui->pauseButton->setEnabled(m_audioRecorder->state() != QMediaRecorder::StoppedState);
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
    if (m_audioRecorder->state() == QMediaRecorder::StoppedState) {
        m_audioRecorder->setAudioInput(boxValue(ui->audioDeviceBox).value<QAudioDeviceInfo>());

        QAudioEncoderSettings settings;
        settings.setCodec(boxValue(ui->audioCodecBox).toString());
        settings.setSampleRate(boxValue(ui->sampleRateBox).toInt());
        settings.setBitRate(boxValue(ui->bitrateBox).toInt());
        settings.setChannelCount(boxValue(ui->channelsBox).toInt());
        settings.setQuality(QMultimedia::EncodingQuality(ui->qualitySlider->value()));
        settings.setEncodingMode(ui->constantQualityRadioButton->isChecked() ?
                                 QMultimedia::ConstantQualityEncoding :
                                 QMultimedia::ConstantBitRateEncoding);

        QString container = boxValue(ui->containerBox).toString();

        m_audioRecorder->setEncodingSettings(settings, QVideoEncoderSettings(), container);
        m_audioRecorder->record();
    }
    else {
        m_audioRecorder->stop();
    }
}

void AudioRecorder::togglePause()
{
    if (m_audioRecorder->state() != QMediaRecorder::PausedState)
        m_audioRecorder->pause();
    else
        m_audioRecorder->record();
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
    m_audioRecorder->setOutputLocation(QUrl::fromLocalFile(fileName));
    m_outputLocationSet = true;
}

void AudioRecorder::displayErrorMessage()
{
    ui->statusbar->showMessage(m_audioRecorder->errorString());
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
