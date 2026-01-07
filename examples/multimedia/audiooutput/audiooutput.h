// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef AUDIOOUTPUT_H
#define AUDIOOUTPUT_H

#include <QAudioSink>
#include <QByteArray>
#include <QComboBox>
#include <QIODevice>
#include <QLabel>
#include <QMainWindow>
#include <QMediaDevices>
#include <QMessageBox>
#include <QObject>
#include <QPushButton>
#include <QSlider>
#include <QTimer>

class Generator : public QIODevice
{
    Q_OBJECT

public:
    Generator(const QAudioFormat &format, qint64 durationUs, int frequency);

    void start();
    void stop();

    qint64 readData(char *data, qint64 maxlen) override;
    qint64 writeData(const char *data, qint64 len) override;
    qint64 bytesAvailable() const override;
    qint64 size() const override { return m_buffer.size(); }

private:
    void generateData(const QAudioFormat &format, qint64 durationUs, int frequency);

private:
    qint64 m_pos = 0;
    QByteArray m_buffer;
};

enum class AudioTestMode {
    Pull,
    Push,
    Callback,
};

class AudioTest : public QMainWindow
{
    Q_OBJECT

public:
    AudioTest();
    ~AudioTest();

private:
    void initializeWindow();
    void startAudioSink(const QAudioDevice &, const QAudioFormat &);
    void cleanupAudioSink();

private:
    QMediaDevices *m_devices = nullptr;
    QTimer *m_pushTimer = nullptr;

    // Owned by layout
    QComboBox *m_channelsBox = nullptr;
    QComboBox *m_rateBox = nullptr;
    QComboBox *m_formatBox = nullptr;
    QComboBox *m_modeBox = nullptr;
    QPushButton *m_suspendResumeButton = nullptr;
    QComboBox *m_deviceBox = nullptr;
    QLabel *m_volumeLabel = nullptr;
    QSlider *m_volumeSlider = nullptr;

    std::unique_ptr<Generator> m_generator;
    std::unique_ptr<QAudioSink> m_audioSink;

    AudioTestMode m_mode = AudioTestMode::Pull;
    QAudioDevice m_currentDevice;
    void restartAudioStream();

private slots:
    void formatChanged(QComboBox *box);
    void toggleSuspendResume();
    void deviceChanged(int index);
    void volumeChanged(int);
    void updateAudioDevices();
};

#endif // AUDIOOUTPUT_H
