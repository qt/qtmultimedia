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
#include <QObject>
#include <QPushButton>
#include <QScopedPointer>
#include <QSlider>
#include <QTimer>

#include <math.h>

class Generator : public QIODevice
{
    Q_OBJECT

public:
    Generator(const QAudioFormat &format, qint64 durationUs, int sampleRate);

    void start();
    void stop();

    qint64 readData(char *data, qint64 maxlen) override;
    qint64 writeData(const char *data, qint64 len) override;
    qint64 bytesAvailable() const override;
    qint64 size() const override { return m_buffer.size(); }

private:
    void generateData(const QAudioFormat &format, qint64 durationUs, int sampleRate);

private:
    qint64 m_pos = 0;
    QByteArray m_buffer;
};

class AudioTest : public QMainWindow
{
    Q_OBJECT

public:
    AudioTest();
    ~AudioTest();

private:
    void initializeWindow();
    void initializeAudio(const QAudioDevice &deviceInfo);

private:
    QMediaDevices *m_devices = nullptr;
    QTimer *m_pushTimer = nullptr;

    // Owned by layout
    QPushButton *m_modeButton = nullptr;
    QPushButton *m_suspendResumeButton = nullptr;
    QComboBox *m_deviceBox = nullptr;
    QLabel *m_volumeLabel = nullptr;
    QSlider *m_volumeSlider = nullptr;

    QScopedPointer<Generator> m_generator;
    QScopedPointer<QAudioSink> m_audioOutput;

    bool m_pullMode = true;

private slots:
    void toggleMode();
    void toggleSuspendResume();
    void deviceChanged(int index);
    void volumeChanged(int);
    void updateAudioDevices();
};

#endif // AUDIOOUTPUT_H
