// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QImageCapture>
#include <QMediaCaptureSession>
#include <QGraphicsVideoItem>
#include <QCamera>
#include <QMediaDevices>
#include <QMediaRecorder>

#include <memory>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }

class QMediaCaptureSession;
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void init();

private slots:
    void doCamera();
    void on_startButton_clicked();
    void on_stopButton_clicked();
    void on_captureButton_clicked();
    void on_openButton_clicked();
    void on_recordButton_clicked();
    void on_camerasComboBox_textActivated(const QString &arg1);

private:
    Ui::MainWindow *ui;

    void enableButtons(bool ok);
    void showFile(const QString &filename);

    std::unique_ptr<QCamera> m_camera;
    QMediaCaptureSession *m_captureSession;
    std::unique_ptr<QAudioInput> m_audioInput;
    QMediaDevices *m_mediaDevices;
    QMediaRecorder *m_recorder;
    bool isRecording = false;
};

QT_END_NAMESPACE
#endif // MAINWINDOW_H
