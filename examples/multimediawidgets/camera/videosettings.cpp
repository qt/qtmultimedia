/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
#include "ui_videosettings_mobile.h"
#else
#include "ui_videosettings.h"
#endif

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

QString toFormattedString(const QCameraFormat &cameraFormat)
{
    QString string;
    const auto &separator = QStringLiteral(" ");

    string.append(QVideoFrameFormat::pixelFormatToString(cameraFormat.pixelFormat()));
    string.append(separator);

    string.append(QString::number(cameraFormat.resolution().width()));
    string.append(QStringLiteral("x"));
    string.append(QString::number(cameraFormat.resolution().height()));
    string.append(separator);

    string.append(QString::number(cameraFormat.minFrameRate()));
    string.append(QStringLiteral("-"));
    string.append(QString::number(cameraFormat.maxFrameRate()));
    string.append(QStringLiteral("FPS"));

    return string;
}

VideoSettings::VideoSettings(QMediaRecorder *mediaRecorder, QWidget *parent)
    : QDialog(parent),
      ui(new Ui::VideoSettingsUi),
      mediaRecorder(mediaRecorder)
{
    ui->setupUi(this);

    //sample rate:
    auto audioDevice = mediaRecorder->captureSession()->audioInput()->device();
    ui->audioSampleRateBox->setRange(audioDevice.minimumSampleRate(),
                                     audioDevice.maximumSampleRate());

    // camera format
    ui->videoFormatBox->addItem(tr("Default camera format"));

    const QList<QCameraFormat> videoFormats =
            mediaRecorder->captureSession()->camera()->cameraDevice().videoFormats();

    for (const QCameraFormat &format : videoFormats) {
        ui->videoFormatBox->addItem(toFormattedString(format), QVariant::fromValue(format));
    }

    connect(ui->videoFormatBox, &QComboBox::currentIndexChanged, [this](int /*index*/) {
        const auto &cameraFormat = boxValue(ui->videoFormatBox).value<QCameraFormat>();
        ui->fpsSlider->setRange(cameraFormat.minFrameRate(), cameraFormat.maxFrameRate());
        ui->fpsSpinBox->setRange(cameraFormat.minFrameRate(), cameraFormat.maxFrameRate());
    });

    auto currentCameraFormat = mediaRecorder->captureSession()->camera()->cameraFormat();
    ui->fpsSlider->setRange(currentCameraFormat.minFrameRate(), currentCameraFormat.maxFrameRate());
    ui->fpsSpinBox->setRange(currentCameraFormat.minFrameRate(),
                             currentCameraFormat.maxFrameRate());

    connect(ui->fpsSlider, &QSlider::valueChanged, ui->fpsSpinBox, &QSpinBox::setValue);
    connect(ui->fpsSpinBox, &QSpinBox::valueChanged, ui->fpsSlider, &QSlider::setValue);

    updateFormatsAndCodecs();
    connect(ui->audioCodecBox, &QComboBox::currentIndexChanged, this, &VideoSettings::updateFormatsAndCodecs);
    connect(ui->videoCodecBox, &QComboBox::currentIndexChanged, this, &VideoSettings::updateFormatsAndCodecs);
    connect(ui->containerFormatBox, &QComboBox::currentIndexChanged, this, &VideoSettings::updateFormatsAndCodecs);

    ui->qualitySlider->setRange(0, int(QMediaRecorder::VeryHighQuality));

    QMediaFormat format = mediaRecorder->mediaFormat();
    selectComboBoxItem(ui->containerFormatBox, QVariant::fromValue(format.fileFormat()));
    selectComboBoxItem(ui->audioCodecBox, QVariant::fromValue(format.audioCodec()));
    selectComboBoxItem(ui->videoCodecBox, QVariant::fromValue(format.videoCodec()));

    ui->qualitySlider->setValue(mediaRecorder->quality());
    ui->audioSampleRateBox->setValue(mediaRecorder->audioSampleRate());
    selectComboBoxItem(
            ui->videoFormatBox,
            QVariant::fromValue(mediaRecorder->captureSession()->camera()->cameraFormat()));

    ui->fpsSlider->setValue(mediaRecorder->videoFrameRate());
    ui->fpsSpinBox->setValue(mediaRecorder->videoFrameRate());
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

    mediaRecorder->setMediaFormat(format);
    mediaRecorder->setQuality(QMediaRecorder::Quality(ui->qualitySlider->value()));
    mediaRecorder->setAudioSampleRate(ui->audioSampleRateBox->value());

    const auto &cameraFormat = boxValue(ui->videoFormatBox).value<QCameraFormat>();
    mediaRecorder->setVideoResolution(cameraFormat.resolution());
    mediaRecorder->setVideoFrameRate(ui->fpsSlider->value());

    mediaRecorder->captureSession()->camera()->setCameraFormat(cameraFormat);
}

void VideoSettings::updateFormatsAndCodecs()
{
    if (m_updatingFormats)
        return;
    m_updatingFormats = true;

    QMediaFormat format;
    if (ui->containerFormatBox->count())
        format.setFileFormat(boxValue(ui->containerFormatBox).value<QMediaFormat::FileFormat>());
    if (ui->audioCodecBox->count())
        format.setAudioCodec(boxValue(ui->audioCodecBox).value<QMediaFormat::AudioCodec>());
    if (ui->videoCodecBox->count())
        format.setVideoCodec(boxValue(ui->videoCodecBox).value<QMediaFormat::VideoCodec>());

    int currentIndex = 0;
    ui->audioCodecBox->clear();
    ui->audioCodecBox->addItem(tr("Default audio codec"), QVariant::fromValue(QMediaFormat::AudioCodec::Unspecified));
    for (auto codec : format.supportedAudioCodecs(QMediaFormat::Encode)) {
        if (codec == format.audioCodec())
            currentIndex = ui->audioCodecBox->count();
        ui->audioCodecBox->addItem(QMediaFormat::audioCodecDescription(codec), QVariant::fromValue(codec));
    }
    ui->audioCodecBox->setCurrentIndex(currentIndex);

    currentIndex = 0;
    ui->videoCodecBox->clear();
    ui->videoCodecBox->addItem(tr("Default video codec"), QVariant::fromValue(QMediaFormat::VideoCodec::Unspecified));
    for (auto codec : format.supportedVideoCodecs(QMediaFormat::Encode)) {
        if (codec == format.videoCodec())
            currentIndex = ui->videoCodecBox->count();
        ui->videoCodecBox->addItem(QMediaFormat::videoCodecDescription(codec), QVariant::fromValue(codec));
    }
    ui->videoCodecBox->setCurrentIndex(currentIndex);

    currentIndex = 0;
    ui->containerFormatBox->clear();
    ui->containerFormatBox->addItem(tr("Default file format"), QVariant::fromValue(QMediaFormat::UnspecifiedFormat));
    for (auto container : format.supportedFileFormats(QMediaFormat::Encode)) {
        if (container == format.fileFormat())
            currentIndex = ui->containerFormatBox->count();
        ui->containerFormatBox->addItem(QMediaFormat::fileFormatDescription(container), QVariant::fromValue(container));
    }
    ui->containerFormatBox->setCurrentIndex(currentIndex);

    m_updatingFormats = false;

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
