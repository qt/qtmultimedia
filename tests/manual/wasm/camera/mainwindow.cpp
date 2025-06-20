// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QAudioInput>
#include <QCamera>
#include <QCameraDevice>
#include <QGraphicsScene>
#include <QGraphicsVideoItem>
#include <QImageCapture>
#include <QMediaCaptureSession>
#include <QMediaDevices>
#include <QMediaFormat>
#include <QApplication>
#include <QTimer>
#include <QVideoWidget>
#include <QLabel>
#include <QFileDialog>
#include <QScreen>
#include <QMediaPlayer>

#if QT_CONFIG(permissions)
  #include <QPermission>
#endif

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow),
      m_recorder(nullptr)
{
    ui->setupUi(this);
    init();
}

MainWindow::~MainWindow()
{
    if (m_recorder)
        delete m_recorder;
    delete ui;
}

void MainWindow::init()
{
#if QT_CONFIG(permissions)
    // camera
    QCameraPermission cameraPermission;
    switch (qApp->checkPermission(cameraPermission)) {
    case Qt::PermissionStatus::Undetermined:
        qApp->requestPermission(cameraPermission, this, &MainWindow::init);
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
        qApp->requestPermission(microphonePermission, this, &MainWindow::init);
        return;
    case Qt::PermissionStatus::Denied:
        qWarning("Microphone permission is not granted!");
        return;
    case Qt::PermissionStatus::Granted:
        break;
    }
#endif
    m_captureSession = new QMediaCaptureSession(this);

    m_mediaDevices = new QMediaDevices(this);
    // wait until devices list is ready
    connect(m_mediaDevices, &QMediaDevices::videoInputsChanged,
            [this]() { doCamera(); });
}

void MainWindow::doCamera()
{
    m_audioInput.reset(new QAudioInput);
    m_captureSession->setAudioInput(m_audioInput.get());
    const QList<QCameraDevice> cameras = QMediaDevices::videoInputs();

    ui->camerasComboBox->clear();
    ui->camerasComboBox->setPlaceholderText("select camera");

    for (const QCameraDevice &cameraDevice : cameras) {
        if (ui->camerasComboBox->findText(cameraDevice.description()) == -1)
            ui->camerasComboBox->addItem(cameraDevice.description(), cameraDevice.id());
    }

    if (cameras.count() == 0) {
        qWarning() << "No camera found";
    } else {

        QGraphicsVideoItem *videoItem = new QGraphicsVideoItem();
        QGraphicsScene *scene = new QGraphicsScene(this);
        m_captureSession->setVideoOutput(videoItem); // sets videoSink

        ui->graphicsView->setScene(scene);
        ui->graphicsView->scene()->addItem(videoItem);
        ui->graphicsView->show();
    }
}

void MainWindow::on_startButton_clicked()
{
    m_camera->start();
}

void MainWindow::on_stopButton_clicked()
{
    m_camera->stop();
}

void MainWindow::on_captureButton_clicked()
{
    connect(m_camera.get(), &QCamera::errorOccurred,
            [](QCamera::Error error, const QString &errorString) {
        qWarning() << "Error occurred" << error << errorString;
    });

    QImageCapture *m_imageCapture = new QImageCapture(this);
    connect(m_imageCapture, &QImageCapture::readyForCaptureChanged, [] (bool ready) {

        qWarning() << "MainWindow::readyForCaptureChanged" << ready;
    });

    connect(m_imageCapture,
            &QImageCapture::imageCaptured,
            [] (int id, const QImage &preview) {
        qWarning() << "MainWindow::imageCaptured" << id << preview;

    });

    connect(m_imageCapture, &QImageCapture::imageSaved,
            [this] (int id, const QString &fileName) {
                Q_UNUSED(id)
        QFileInfo fi(fileName);
        if (!fi.exists()) {
            qWarning() << fileName << "Does not exist";
        } else {
            QDialog *dlg = new QDialog(this);
            dlg->setWindowTitle(fi.fileName());
            QHBoxLayout *l = new QHBoxLayout(dlg);
            QLabel *label = new QLabel(this);
            l->addWidget(label);
            label->setPixmap(QPixmap(fileName));
            dlg->show();
        }

    });
    connect(m_imageCapture,
            &QImageCapture::errorOccurred, []
            (int id, QImageCapture::Error error, const QString &errorString) {
        qWarning() << "MainWindow::errorOccurred" << id << error << errorString;
    });

    m_captureSession->setImageCapture(m_imageCapture);

    // take photo
    if (m_imageCapture->isReadyForCapture())
        m_imageCapture->captureToFile(QStringLiteral("/home/web_user/image.png"));
}

void MainWindow::on_openButton_clicked()
{
    // open
    QFileDialog *dialog = new QFileDialog(this);
    dialog->setNameFilter(tr("All Files (*.*)"));
    connect(dialog, &QFileDialog::fileSelected,
            [this](const QString &file) {
                qWarning() << "open this file" << file;
                showFile(file);
            });

    dialog->show();
}

void MainWindow::on_camerasComboBox_textActivated(const QString &arg1)
{
    const QList<QCameraDevice> cameras = QMediaDevices::videoInputs();
    for (const QCameraDevice &cameraDevice :cameras) {
        if (arg1 == cameraDevice.description()) {
            m_camera.reset(new QCamera(cameraDevice));
            connect(m_captureSession, &QMediaCaptureSession::cameraChanged,
                    [this]() {
                        enableButtons(true);
                    });
            m_captureSession->setCamera(m_camera.get());
            break;
        }
    }
}

void MainWindow::on_recordButton_clicked()
{
    if (!isRecording) {
        if (m_recorder)
            delete m_recorder;
        m_recorder = new QMediaRecorder();
        connect(m_recorder, &QMediaRecorder::durationChanged,
                [](qint64 duration) {
            qWarning() << "MainWindow::durationChanged"
                       << duration;
        });
        m_captureSession->setRecorder(m_recorder);

        m_recorder->setQuality(QMediaRecorder::HighQuality);
        m_recorder->setOutputLocation(QUrl::fromLocalFile("test.mp4"));
        m_recorder->record();
        isRecording = true;
        ui->recordButton->setText(QStringLiteral("Stop"));
    } else {
        m_recorder->stop();
        isRecording = false;
        m_recorder->deleteLater();
        ui->recordButton->setText(QStringLiteral("Record"));
    }
}

void MainWindow::enableButtons(bool ok)
{
    ui->captureButton->setEnabled(ok);
    ui->startButton->setEnabled(ok);
    ui->stopButton->setEnabled(ok);
    ui->openButton->setEnabled(ok);
    ui->recordButton->setEnabled(ok);
}

void MainWindow::showFile(const QString &fileName)
{

    const QSize screenGeometry = screen()->availableSize();
    QFileInfo fi(fileName);
    QDialog *dlg = new QDialog(this);
    dlg->setWindowTitle(fi.fileName());
    QHBoxLayout *layout = new QHBoxLayout(dlg);
    QMediaPlayer *player = new QMediaPlayer(dlg);
    connect(player, &QMediaPlayer::errorOccurred, [=] (QMediaPlayer::Error error, const QString &errorString) {
            qWarning() << "MediaPlayer erro!:" << error << errorString;
    });

    QGraphicsVideoItem *m_videoItem = new QGraphicsVideoItem();
    QSizeF dialogSize(screenGeometry.width() / 2, screenGeometry.height() / 2);
    m_videoItem->setSize(dialogSize);
    player->setVideoOutput(m_videoItem);
    dlg->setGeometry(20, 20, dialogSize.width() + 40, dialogSize.height() + 40);

    QGraphicsScene *scene = new QGraphicsScene(dlg);
    QGraphicsView *graphicsView = new QGraphicsView(scene);
    scene->addItem(m_videoItem);
    layout->addWidget(graphicsView);

    player->setSource(QUrl(fileName));

    dlg->show();
    player->play();
}
