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

#include "videosettings.h"
#include "ui_videosettings.h"

#include <QComboBox>
#include <QSpinBox>
#include <QDebug>
#include <QMediaRecorder>
#include <QMediaFormat>
#include <QAudioDevice>
#include <QMediaCaptureSession>
#include <QCameraDevice>
#include <QCamera>
#include <QAudioInput>

VideoSettings::VideoSettings(QMediaRecorder *mediaRecorder, QWidget *parent)
    : QDialog(parent),
      ui(new Ui::VideoSettingsUi),
      mediaRecorder(mediaRecorder)
{
    ui->setupUi(this);

    //audio codecs
    ui->audioCodecBox->addItem(tr("Default audio codec"), QVariant::fromValue(QMediaFormat::AudioCodec::Unspecified));
    const auto supportedAudioCodecs = QMediaFormat().supportedAudioCodecs(QMediaFormat::Encode);
    for (const auto &codec : supportedAudioCodecs) {
        QString description = QMediaFormat::audioCodecDescription(codec);
        ui->audioCodecBox->addItem(QMediaFormat::audioCodecName(codec) + ": " + description, QVariant::fromValue(codec));
    }

    //sample rate:
    auto audioDevice = mediaRecorder->captureSession()->audioInput()->device();
    ui->audioSampleRateBox->setRange(audioDevice.minimumSampleRate(),
                                     audioDevice.maximumSampleRate());

    //video codecs
    ui->videoCodecBox->addItem(tr("Default video codec"), QVariant::fromValue(QMediaFormat::VideoCodec::Unspecified));
    const auto supportedVideoCodecs = QMediaFormat().supportedVideoCodecs(QMediaFormat::Encode);
    for (const auto &codec : supportedVideoCodecs) {
        QString description = QMediaFormat::videoCodecDescription(codec);
        ui->videoCodecBox->addItem(QMediaFormat::videoCodecName(codec) + ": " + description, QVariant::fromValue(codec));
    }


    ui->videoResolutionBox->addItem(tr("Default"));
    auto supportedResolutions = mediaRecorder->captureSession()->camera()->cameraDevice().photoResolutions(); // ### Should use resolutions from video formats
    for (const QSize &resolution : supportedResolutions) {
        ui->videoResolutionBox->addItem(QString("%1x%2").arg(resolution.width()).arg(resolution.height()),
                                        QVariant(resolution));
    }

    ui->videoFramerateBox->addItem(tr("Default"));
//    const QList<qreal> supportedFrameRates = mediaRecorder->supportedFrameRates();
//    for (qreal rate : supportedFrameRates) {
//        QString rateString = QString("%1").arg(rate, 0, 'f', 2);
//        ui->videoFramerateBox->addItem(rateString, QVariant(rate));
//    }

    //containers
    ui->containerFormatBox->addItem(tr("Default container"), QVariant::fromValue(QMediaFormat::UnspecifiedFormat));
    const auto formats = QMediaFormat().supportedFileFormats(QMediaFormat::Encode);
    for (auto format : formats) {
        ui->containerFormatBox->addItem(QMediaFormat::fileFormatName(format) + ": " + QMediaFormat::fileFormatDescription(format),
                                        QVariant::fromValue(format));
    }

    ui->qualitySlider->setRange(0, int(QMediaRecorder::VeryHighQuality));

    QMediaFormat format = mediaRecorder->mediaFormat();
    selectComboBoxItem(ui->containerFormatBox, QVariant::fromValue(format.fileFormat()));
    selectComboBoxItem(ui->audioCodecBox, QVariant::fromValue(format.audioCodec()));
    selectComboBoxItem(ui->videoCodecBox, QVariant::fromValue(format.videoCodec()));

    ui->qualitySlider->setValue(mediaRecorder->quality());
    ui->audioSampleRateBox->setValue(mediaRecorder->audioSampleRate());
    selectComboBoxItem(ui->videoResolutionBox, QVariant(mediaRecorder->videoResolution()));

    //special case for frame rate
    for (int i = 0; i < ui->videoFramerateBox->count(); ++i) {
        qreal itemRate = ui->videoFramerateBox->itemData(i).value<qreal>();
        if (qFuzzyCompare(itemRate, mediaRecorder->videoFrameRate())) {
            ui->videoFramerateBox->setCurrentIndex(i);
            break;
        }
    }

}

VideoSettings::~VideoSettings()
{
    delete ui;
}

void VideoSettings::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void VideoSettings::applySettings()
{
    QMediaFormat format;
    format.setFileFormat(boxValue(ui->containerFormatBox).value<QMediaFormat::FileFormat>());
    format.setAudioCodec(boxValue(ui->audioCodecBox).value<QMediaFormat::AudioCodec>());
    format.setVideoCodec(boxValue(ui->videoCodecBox).value<QMediaFormat::VideoCodec>());

    mediaRecorder->setQuality(QMediaRecorder::Quality(ui->qualitySlider->value()));
    mediaRecorder->setAudioSampleRate(ui->audioSampleRateBox->value());
    mediaRecorder->setVideoResolution(boxValue(ui->videoResolutionBox).toSize());
    mediaRecorder->setVideoFrameRate(boxValue(ui->videoFramerateBox).value<qreal>());
}

QVariant VideoSettings::boxValue(const QComboBox *box) const
{
    int idx = box->currentIndex();
    if (idx == -1)
        return QVariant();

    return box->itemData(idx);
}

void VideoSettings::selectComboBoxItem(QComboBox *box, const QVariant &value)
{
    for (int i = 0; i < box->count(); ++i) {
        if (box->itemData(i) == value) {
            box->setCurrentIndex(i);
            break;
        }
    }
}
