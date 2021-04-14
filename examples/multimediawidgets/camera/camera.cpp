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

#include "camera.h"
#include "ui_camera.h"
#include "videosettings.h"
#include "imagesettings.h"
#include "metadatadialog.h"

#include <QMediaEncoder>
#include <QVideoWidget>
#include <QCameraInfo>
#include <QMediaMetaData>

#include <QMessageBox>
#include <QPalette>
#include <QImage>

#include <QtWidgets>
#include <QMediaDeviceManager>
#include <QMediaFormat>

Camera::Camera()
    : ui(new Ui::Camera)
{
    ui->setupUi(this);

    //Camera devices:

    videoDevicesGroup = new QActionGroup(this);
    videoDevicesGroup->setExclusive(true);
    updateCameras();
    connect(&m_manager, &QMediaDeviceManager::videoInputsChanged, this, &Camera::updateCameras);

    connect(videoDevicesGroup, &QActionGroup::triggered, this, &Camera::updateCameraDevice);
    connect(ui->captureWidget, &QTabWidget::currentChanged, this, &Camera::updateCaptureMode);

    connect(ui->metaDataButton, &QPushButton::clicked, this, &Camera::showMetaDataDialog);

    setCamera(QMediaDeviceManager::defaultVideoInput());

    qDebug() << "Supported Containers:";
    auto containers = QMediaFormat().supportedFileFormats(QMediaFormat::Encode);
    for (const auto c : containers)
        qDebug() << "    " << QMediaFormat::fileFormatName(c);

    qDebug() << "Supported Audio Codecs:";
    auto audio = QMediaFormat().supportedAudioCodecs(QMediaFormat::Encode);
    for (const auto c : audio)
        qDebug() << "    " << QMediaFormat::audioCodecName(c);

    qDebug() << "Supported Video Codecs:";
    auto video = QMediaFormat().supportedVideoCodecs(QMediaFormat::Encode);
    for (const auto c : video)
        qDebug() << "    " << QMediaFormat::videoCodecName(c);
}

void Camera::setCamera(const QCameraInfo &cameraInfo)
{
    m_camera.reset(new QCamera(cameraInfo));
    m_captureSession.setCamera(m_camera.data());

    connect(m_camera.data(), &QCamera::activeChanged, this, &Camera::updateCameraActive);
    connect(m_camera.data(), &QCamera::errorOccurred, this, &Camera::displayCameraError);

    m_mediaEncoder.reset(new QMediaEncoder);
    m_captureSession.setEncoder(m_mediaEncoder.data());
    connect(m_mediaEncoder.data(), &QMediaEncoder::stateChanged, this, &Camera::updateRecorderState);

    m_imageCapture = new QCameraImageCapture;
    m_captureSession.setImageCapture(m_imageCapture);

    connect(m_mediaEncoder.data(), &QMediaEncoder::durationChanged, this, &Camera::updateRecordTime);
    connect(m_mediaEncoder.data(), QOverload<QMediaEncoder::Error>::of(&QMediaEncoder::error),
            this, &Camera::displayRecorderError);

    connect(ui->exposureCompensation, &QAbstractSlider::valueChanged, this, &Camera::setExposureCompensation);

    m_captureSession.setVideoOutput(ui->viewfinder);

    updateCameraActive(m_camera->isActive());
    updateRecorderState(m_mediaEncoder->state());

    connect(m_imageCapture, &QCameraImageCapture::readyForCaptureChanged, this, &Camera::readyForCapture);
    connect(m_imageCapture, &QCameraImageCapture::imageCaptured, this, &Camera::processCapturedImage);
    connect(m_imageCapture, &QCameraImageCapture::imageSaved, this, &Camera::imageSaved);
    connect(m_imageCapture, QOverload<int, QCameraImageCapture::Error, const QString &>::of(&QCameraImageCapture::error),
            this, &Camera::displayCaptureError);
    readyForCapture(m_imageCapture->isReadyForCapture());

    QCameraImageProcessing *imageProcessing = m_camera->imageProcessing();
    if (!imageProcessing) {
        ui->brightnessSlider->setEnabled(false);
        ui->contrastSlider->setEnabled(false);
        ui->saturationSlider->setEnabled(false);
        ui->hueSlider->setEnabled(false);
    } else {
        connect(ui->brightnessSlider, &QSlider::valueChanged, [imageProcessing](int value) {
            imageProcessing->setBrightness(value/100.);
        });
        connect(ui->contrastSlider, &QSlider::valueChanged, [imageProcessing](int value) {
            imageProcessing->setContrast(value/100.);
        });
        connect(ui->saturationSlider, &QSlider::valueChanged, [imageProcessing](int value) {
            imageProcessing->setSaturation(value/100.);
        });
        connect(ui->hueSlider, &QSlider::valueChanged, [imageProcessing](int value) {
            imageProcessing->setHue(value/100.);
        });
    }


    updateCaptureMode();
    m_camera->start();
}

void Camera::keyPressEvent(QKeyEvent * event)
{
    if (event->isAutoRepeat())
        return;

    switch (event->key()) {
    case Qt::Key_CameraFocus:
        displayViewfinder();
        event->accept();
        break;
    case Qt::Key_Camera:
        if (m_doImageCapture) {
            takeImage();
        } else {
            if (m_mediaEncoder->state() == QMediaEncoder::RecordingState)
                stop();
            else
                record();
        }
        event->accept();
        break;
    default:
        QMainWindow::keyPressEvent(event);
    }
}

void Camera::keyReleaseEvent(QKeyEvent *event)
{
    QMainWindow::keyReleaseEvent(event);
}

void Camera::updateRecordTime()
{
    QString str = QString("Recorded %1 sec").arg(m_mediaEncoder->duration()/1000);
    ui->statusbar->showMessage(str);
}

void Camera::processCapturedImage(int requestId, const QImage& img)
{
    Q_UNUSED(requestId);
    QImage scaledImage = img.scaled(ui->viewfinder->size(),
                                    Qt::KeepAspectRatio,
                                    Qt::SmoothTransformation);

    ui->lastImagePreviewLabel->setPixmap(QPixmap::fromImage(scaledImage));

    // Display captured image for 4 seconds.
    displayCapturedImage();
    QTimer::singleShot(4000, this, &Camera::displayViewfinder);
}

void Camera::configureCaptureSettings()
{
    if (m_doImageCapture)
        configureImageSettings();
    else
        configureVideoSettings();
}

void Camera::configureVideoSettings()
{
    VideoSettings settingsDialog(m_mediaEncoder.data());
    settingsDialog.setWindowFlags(settingsDialog.windowFlags() & ~Qt::WindowContextHelpButtonHint);

    settingsDialog.setEncoderSettings(m_encoderSettings);

    if (settingsDialog.exec()) {
        m_encoderSettings = settingsDialog.encoderSettings();

        m_mediaEncoder->setEncoderSettings(m_encoderSettings);
    }
}

void Camera::configureImageSettings()
{
    ImageSettings settingsDialog(m_imageCapture);
    settingsDialog.setWindowFlags(settingsDialog.windowFlags() & ~Qt::WindowContextHelpButtonHint);

    settingsDialog.setImageSettings(m_imageSettings);

    if (settingsDialog.exec()) {
        m_imageSettings = settingsDialog.imageSettings();
        m_imageCapture->setEncodingSettings(m_imageSettings);
    }
}

void Camera::record()
{
    m_mediaEncoder->record();
    updateRecordTime();
}

void Camera::pause()
{
    m_mediaEncoder->pause();
}

void Camera::stop()
{
    m_mediaEncoder->stop();
}

void Camera::setMuted(bool muted)
{
    m_captureSession.setMuted(muted);
}

void Camera::takeImage()
{
    m_isCapturingImage = true;
    m_imageCapture->capture();
}

void Camera::displayCaptureError(int id, const QCameraImageCapture::Error error, const QString &errorString)
{
    Q_UNUSED(id);
    Q_UNUSED(error);
    QMessageBox::warning(this, tr("Image Capture Error"), errorString);
    m_isCapturingImage = false;
}

void Camera::startCamera()
{
    m_camera->start();
}

void Camera::stopCamera()
{
    m_camera->stop();
}

void Camera::updateCaptureMode()
{
    int tabIndex = ui->captureWidget->currentIndex();
    m_doImageCapture = (tabIndex == 0);
}

void Camera::updateCameraActive(bool active)
{
    if (active) {
        ui->actionStartCamera->setEnabled(false);
        ui->actionStopCamera->setEnabled(true);
        ui->captureWidget->setEnabled(true);
        ui->actionSettings->setEnabled(true);
    } else {
        ui->actionStartCamera->setEnabled(true);
        ui->actionStopCamera->setEnabled(false);
        ui->captureWidget->setEnabled(false);
        ui->actionSettings->setEnabled(false);
    }
}

void Camera::updateRecorderState(QMediaEncoder::State state)
{
    switch (state) {
    case QMediaEncoder::StoppedState:
        ui->recordButton->setEnabled(true);
        ui->pauseButton->setEnabled(true);
        ui->stopButton->setEnabled(false);
        ui->metaDataButton->setEnabled(true);
        break;
    case QMediaEncoder::PausedState:
        ui->recordButton->setEnabled(true);
        ui->pauseButton->setEnabled(false);
        ui->stopButton->setEnabled(true);
        ui->metaDataButton->setEnabled(false);
        break;
    case QMediaEncoder::RecordingState:
        ui->recordButton->setEnabled(false);
        ui->pauseButton->setEnabled(true);
        ui->stopButton->setEnabled(true);
        ui->metaDataButton->setEnabled(false);
        break;
    }
}

void Camera::setExposureCompensation(int index)
{
    m_camera->exposure()->setExposureCompensation(index*0.5);
}

void Camera::displayRecorderError()
{
    QMessageBox::warning(this, tr("Capture Error"), m_mediaEncoder->errorString());
}

void Camera::displayCameraError()
{
    QMessageBox::warning(this, tr("Camera Error"), m_camera->errorString());
}

void Camera::updateCameraDevice(QAction *action)
{
    setCamera(qvariant_cast<QCameraInfo>(action->data()));
}

void Camera::displayViewfinder()
{
    ui->stackedWidget->setCurrentIndex(0);
}

void Camera::displayCapturedImage()
{
    ui->stackedWidget->setCurrentIndex(1);
}

void Camera::readyForCapture(bool ready)
{
    ui->takeImageButton->setEnabled(ready);
}

void Camera::imageSaved(int id, const QString &fileName)
{
    Q_UNUSED(id);
    ui->statusbar->showMessage(tr("Captured \"%1\"").arg(QDir::toNativeSeparators(fileName)));

    m_isCapturingImage = false;
    if (m_applicationExiting)
        close();
}

void Camera::closeEvent(QCloseEvent *event)
{
    if (m_isCapturingImage) {
        setEnabled(false);
        m_applicationExiting = true;
        event->ignore();
    } else {
        event->accept();
    }
}

void Camera::updateCameras()
{
    ui->menuDevices->clear();
    const QList<QCameraInfo> availableCameras = QMediaDeviceManager::videoInputs();
    for (const QCameraInfo &cameraInfo : availableCameras) {
        QAction *videoDeviceAction = new QAction(cameraInfo.description(), videoDevicesGroup);
        videoDeviceAction->setCheckable(true);
        videoDeviceAction->setData(QVariant::fromValue(cameraInfo));
        if (cameraInfo == QMediaDeviceManager::defaultVideoInput())
            videoDeviceAction->setChecked(true);

        ui->menuDevices->addAction(videoDeviceAction);
    }
}

void Camera::showMetaDataDialog()
{
    if (!m_metaDataDialog)
        m_metaDataDialog = new MetaDataDialog(this);
    m_metaDataDialog->setAttribute(Qt::WA_DeleteOnClose, false);
    if (m_metaDataDialog->exec() == QDialog::Accepted)
        saveMetaData();
}

void Camera::saveMetaData()
{
    QMediaMetaData data;
    for (int i = 0; i < QMediaMetaData::NumMetaData; i++) {
        QString val = m_metaDataDialog->m_metaDataFields[i]->text();
        if (!val.isEmpty()) {
            auto key = static_cast<QMediaMetaData::Key>(i);
            if (i == QMediaMetaData::CoverArtImage) {
                QImage coverArt(val);
                data.insert(key, coverArt);
            }
            else if (i == QMediaMetaData::ThumbnailImage) {
                QImage thumbnail(val);
                data.insert(key, thumbnail);
            }
            else if (i == QMediaMetaData::Date) {
                QDateTime date = QDateTime::fromString(val);
                data.insert(key, date);
            }
            else {
                data.insert(key, val);
            }
        }
    }
    m_mediaEncoder->setMetaData(data);
}

